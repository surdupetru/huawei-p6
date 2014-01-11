/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#ifndef __gc_hal_kernel_context_h_
#define __gc_hal_kernel_context_h_

#include "gc_hal_kernel_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maps state locations within the context buffer. */
typedef struct _gcsSTATE_MAP * gcsSTATE_MAP_PTR;
typedef struct _gcsSTATE_MAP
{
    /* Index of the state in the context buffer. */
    gctUINT                     index;

    /* State mask. */
    gctUINT32                   mask;
}
gcsSTATE_MAP;

/* Context buffer. */
typedef struct _gcsCONTEXT * gcsCONTEXT_PTR;
typedef struct _gcsCONTEXT
{
    /* For debugging: the number of context buffer in the order of creation. */
#if gcmIS_DEBUG(gcdDEBUG_CODE)
    gctUINT                     num;
#endif

    /* Pointer to gckEVENT object. */
    gckEVENT                    eventObj;

    /* Context busy signal. */
    gctSIGNAL                   signal;

    /* Physical address of the context buffer. */
    gctPHYS_ADDR                physical;

    /* Logical address of the context buffer. */
    gctUINT32_PTR               logical;

    /* Pointer to the LINK commands. */
    gctPOINTER                  link2D;
    gctPOINTER                  link3D;

    /* The number of pending state deltas. */
    gctUINT                     deltaCount;

    /* Pointer to the first delta to be applied. */
    gcsSTATE_DELTA_PTR          delta;

    /* Next context buffer. */
    gcsCONTEXT_PTR              next;
}
gcsCONTEXT;

/* gckCONTEXT structure that hold the current context. */
struct _gckCONTEXT
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to gckOS object. */
    gckOS                       os;

    /* Pointer to gckHARDWARE object. */
    gckHARDWARE                 hardware;

    /* Command buffer alignment. */
    gctSIZE_T                   alignment;
    gctSIZE_T                   reservedHead;
    gctSIZE_T                   reservedTail;

    /* Context buffer metrics. */
    gctSIZE_T                   stateCount;
    gctSIZE_T                   totalSize;
    gctSIZE_T                   bufferSize;
    gctUINT32                   linkIndex2D;
    gctUINT32                   linkIndex3D;
    gctUINT32                   linkIndexXD;
    gctUINT32                   entryOffset3D;
    gctUINT32                   entryOffsetXDFrom2D;
    gctUINT32                   entryOffsetXDFrom3D;

    /* Dirty flags. */
    gctBOOL                     dirty;
    gctBOOL                     dirty2D;
    gctBOOL                     dirty3D;
    gcsCONTEXT_PTR              dirtyBuffer;

    /* State mapping. */
    gcsSTATE_MAP_PTR            map;

    /* List of context buffers. */
    gcsCONTEXT_PTR              buffer;

    /* A copy of the user record array. */
    gctUINT                     recordArraySize;
    gcsSTATE_DELTA_RECORD_PTR   recordArray;

    /* Requested pipe select for context. */
    gcePIPE_SELECT              entryPipe;
    gcePIPE_SELECT              exitPipe;

    /* Variables used for building state buffer. */
    gctUINT32                   lastAddress;
    gctSIZE_T                   lastSize;
    gctUINT32                   lastIndex;
    gctBOOL                     lastFixed;

    /* Hint array. */
#if gcdSECURE_USER
    gctBOOL_PTR                 hint;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_kernel_context_h_ */

