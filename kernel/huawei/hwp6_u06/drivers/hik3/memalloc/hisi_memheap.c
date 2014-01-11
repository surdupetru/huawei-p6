/* 
 * memory heap
 *
 * Copyright (C) 2012 Google Finland Oy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
--------------------------------------------------------------------------------
--
--  Abstract : memory heap
--
------------------------------------------------------------------------------*/

/*** Header Files ***/
#include <linux/kernel.h>
#include <linux/module.h>
/* needed for remap_page_range */
#include <linux/mm.h>
/* obviously, for kmalloc */
#include <linux/slab.h>
/* for struct file_operations, register_chrdev() */
#include <linux/fs.h>
/* standard error codes */
#include <linux/errno.h>
/* this header files wraps some common module-space operations ...
   here we use mem_map_reserve() macro */
#include <linux/kdev_t.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/list.h>
/* our header */
#include "hisi_memheap.h"

typedef struct MemNode {
	struct list_head NodeList;
	uint m_BusAddr;		/*BusAddr */
	uint m_Size;		/*size */
	unchar m_Usage;		/*usage */
} MEMNODE;
typedef MEMNODE *pMEMNODE;

#define MemFree             0
#define MemUsing            1
#define NODEFOUND           1
#define NODEMISS            0
#define PAGESIZE            4096

/*** global various ***/
static struct list_head g_ListHead;	/*List Head */

/* list starting MemNode */
#define LISTSTART         list_entry((g_ListHead.next), MEMNODE, NodeList)
#define PREVNODE(x)       list_entry((x)->NodeList.prev, MEMNODE, NodeList)
#define NEXTNODE(x)       list_entry((x)->NodeList.next, MEMNODE, NodeList)

/*******************************************************
  Function:       aligned_size
  Description:    align mem-size to 4kb, for remap
  Called By:      memory_alloc
  Input:          request buffer size
  Return:         aligned size
********************************************************/
static uint aligned_size(uint size)
{
	ulong temp = size % PAGESIZE;
	if (temp) {		/*non aligned size */
		temp = (size / PAGESIZE + 1) * PAGESIZE;
	} else {		/*aligned size */
		temp = size;
	}
	return temp;
}

/*******************************************************
  Function:       create_node
  Description:    create a mem-node
  Called By:      memory_alloc
  Input:          buffer size, base addr, prev pNode
  Return:         New mem-node
********************************************************/
static pMEMNODE create_node(uint size, uint base, pMEMNODE pNode)
{
	pMEMNODE NewNode = (pMEMNODE) kmalloc(sizeof(MEMNODE), GFP_KERNEL);

	if (NewNode != NULL) {
		NewNode->m_BusAddr = base;
		NewNode->m_Size = size;
		NewNode->m_Usage = MemFree;
		list_add(&NewNode->NodeList, &pNode->NodeList);
	}

	return NewNode;
}

/*******************************************************
  Function:       del_node
  Description:    del a mem-node
  Called By:      memory_free
  Input:          mem-node found by find_node()
  Return:         N/A
********************************************************/
static void del_node(pMEMNODE pNode)
{
	pMEMNODE pPrevNode = PREVNODE(pNode);
	pPrevNode->m_Size += pNode->m_Size;	/*merge pNode size to PrevNode */

	list_del(&pNode->NodeList);
	kfree(pNode);
	return;
}

/*******************************************************
  Function:       find_node
  Description:    find a mem-node by given address
  Called By:      memory_free
  Input:          address of the free node
  Return:         pointer of the free node
********************************************************/
static pMEMNODE find_node(uint addr)
{
	pMEMNODE pNode;
	unchar find = NODEMISS;

	list_for_each_entry(pNode, &g_ListHead, NodeList) {
		if (addr == pNode->m_BusAddr) {
			/*printk(KERN_ERR "MemNode Found,BusAddr: 0x%x\n",addr); */
			find = NODEFOUND;
			break;
		}
	}

	if (NODEMISS == find) {
		pNode = NULL;
		printk(KERN_ERR "MemNode: busaddr 0x%x donot match\n", addr);
	}
	return pNode;
}

/*******************************************************
  Function:       list_dump
  Description:    this is used to dump the node list
  Called By:      N/A
  Input:          N/A
  Return:         N/A
********************************************************/
void list_dump(void)
{
	pMEMNODE pNode;
	uint i = 0;
	list_for_each_entry(pNode, &g_ListHead, NodeList) {
		i++;
		printk(KERN_INFO "Node(%d): usage:%d; size:%dByte; ba:0x%08x\n",
		       i, pNode->m_Usage, pNode->m_Size, pNode->m_BusAddr);
	}
}

/*******************************************************
  Function:       initial_list
  Description:    this is used to initial a MEMNODE list
  Called By:      called by memalloc.c
  Input:          buffer size, base addresss
  Return:         succeed: return 0; failed: return errno
********************************************************/

uint initial_list(uint size, ulong ba)
{
	/*allocate memory from pmem, create list */
	pMEMNODE pStart = NULL;

	if (size <= 0 || ba <= 0)
		return INIT_FAILED;

	INIT_LIST_HEAD(&g_ListHead);	/*initial a list */

	pStart = create_node(size, ba, (pMEMNODE) & g_ListHead);

	if (!pStart) {
		return INIT_FAILED;
	}

	return INIT_SUCCEED;
}

/*******************************************************
  Function:       destroy_list
  Description:    this is used to destroy a MEMNODE list
  Called By:      called by memalloc.c
  Input:          N/A
  Return:         N/A
********************************************************/
void destroy_list(void)
{
	pMEMNODE pLast = LISTSTART;

	while (pLast->NodeList.next != &g_ListHead) {	/*move to tail */
		pLast = NEXTNODE(pLast);
	}

	while (pLast->NodeList.prev != &g_ListHead) {	/*del from tail to head */
		pLast = PREVNODE(pLast);	/*move to prev node,and del "its next" */
		del_node(NEXTNODE(pLast));
	}

	return;
}

/*******************************************************
  Function:       memory_alloc
  Description:    alloc a buffer by the given size,
                    split into 2 parts, in-use & unused spaces
  Called By:      memalloc.c
  Input:          buffer size
  Return:         base address of buffer; failed return 0
********************************************************/
ulong memory_alloc(uint size)
{

	pMEMNODE pNode;
	uint aligsize;
	unchar alloced = NODEMISS;

	if (size <= 0) {
		printk(KERN_ERR "%s(%d): wrong params\n", __FUNCTION__,
		       __LINE__);
		goto out;
	}
	aligsize = aligned_size(size);	/*aligned to 4kb */

	/*printk("%s(%d): size:%d; aligsize:%d\n",
	   __FUNCTION__,__LINE__,size,aligsize); */

	list_for_each_entry(pNode, &g_ListHead, NodeList) {
		if ((MemFree == pNode->m_Usage) && (pNode->m_Size >= aligsize)) {
			/*printk("%s(%d):Alloc %d match\n", __FUNCTION__,__LINE__,aligsize); */
			alloced = NODEFOUND;
			break;
		}
	}

	if ((&g_ListHead) == (&pNode->NodeList)) {	/*loop back to head,no match */
		alloced = NODEMISS;
		printk(KERN_ERR "%s(%d): memalloc failed: request size:%d\n",
		       __FUNCTION__, __LINE__, size);
		list_dump();

		goto out;
	}

	if ((NODEFOUND == alloced) && (pNode->m_Size > aligsize)) {
		pMEMNODE pFreeNode = create_node((pNode->m_Size - aligsize),
						 (aligsize + pNode->m_BusAddr),
						 pNode);

		if (!pFreeNode) {
			pNode = (pMEMNODE) NULL;
			alloced = NODEMISS;

			printk(KERN_ERR "%s(%d):Failed to memalloc free node\n",
			       __FUNCTION__, __LINE__);
		} else {
			/* if free space bigger than request, split into free & inuse parts
			   return with inuse size */
			pNode->m_Size = aligsize;
		}
	}

	/*1. request size just equal free space, just mark m_Usage to 1;
	   2. pNode->m_Size has changed upper, mark m_Usage to 1 */
	if (NODEFOUND == alloced)
		pNode->m_Usage = MemUsing;

out:
	if ((NODEFOUND == alloced) && (pNode->m_BusAddr > 0))
		return pNode->m_BusAddr;
	else
		return MEMALLOC_FAILED;	/*alloc mem failed,return 0 */
}

/*******************************************************
  Function:       memory_free
  Description:    free a buffer by the given address,
                    split into 2 parts, in-use & unused spaces
  Called By:      memalloc.c
  Input:          buffer size
  Return:         base address of buffer
********************************************************/
void memory_free(ulong addr)
{
	pMEMNODE pMerge, pNode;

	if (addr <= 0) {
		printk(KERN_ERR "%s(%d):wrong params\n", __FUNCTION__,
		       __LINE__);
		return;
	}

	pNode = find_node(addr);
	if (pNode) {
		/*printk("%s(%d): size:%dByte free\n",
		   __FUNCTION__,__LINE__,pNode->m_Size); */
		pNode->m_Usage = MemFree;
	} else {
		printk(KERN_ERR "%s(%d): no mem-node found\n", __FUNCTION__,
		       __LINE__);
		return;
	}

	if (&g_ListHead != pNode->NodeList.prev) {	/*try to merge prev Node */
		pMerge = PREVNODE(pNode);

		/* check prevnode & nextnode whether can merge */
		if (pMerge && MemFree == pMerge->m_Usage) {
			/*printk("++++ mem_free(Ln:%d): del node!\n", __LINE__); //xmd test */
			del_node(pNode);
			pNode = pMerge;
		}
	}

	if (&g_ListHead != pNode->NodeList.next) {	/*try to merge next Node */
		pMerge = NEXTNODE(pNode);

		if (pMerge && MemFree == pMerge->m_Usage) {
			/*printk("++++ mem_free(Ln:%d): del node!\n", __LINE__); //xmd test */
			del_node(pMerge);
		}
	}

	return;
}

/*******************************************************
  Function:       free_allocated_memory
  Description:    free a buffer when app do not call the memory_free
  Called By:      memalloc.c
  Input:          NA
  Return:         NA
********************************************************/
void free_allocated_memory()
{
	pMEMNODE pMerge, pNode;
	unchar find = NODEMISS;

	list_for_each_entry(pNode, &g_ListHead, NodeList) {
		if (pNode->m_Usage == MemUsing) {
			printk
			    ("free the used memory when memalloc was closed\n");
			pNode->m_Usage = MemFree;
			find = NODEFOUND;

			if (&g_ListHead != pNode->NodeList.prev)	//try to merge prev Node
			{
				pMerge = PREVNODE(pNode);

				/* check prevnode & nextnode whether can merge */
				if (pMerge && MemFree == pMerge->m_Usage) {
					del_node(pNode);
					pNode = pMerge;
				}
			}

			if (&g_ListHead != pNode->NodeList.next)	//try to merge next Node
			{
				pMerge = NEXTNODE(pNode);

				if (pMerge && MemFree == pMerge->m_Usage) {
					del_node(pMerge);
				}
			}
		}
	}

	return;
}
