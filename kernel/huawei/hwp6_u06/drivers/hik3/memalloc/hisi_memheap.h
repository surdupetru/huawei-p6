/* 
 * memory heap (module header)
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
--  Abstract :
--
------------------------------------------------------------------------------*/
#ifndef __HISI_MEMHEAP
#define __HISI_MEMHEAP

#define INIT_FAILED         -1
#define INIT_SUCCEED        0
#define MEMALLOC_FAILED     0

uint initial_list(uint, ulong);
void destroy_list(void);
ulong memory_alloc(uint);
void memory_free(ulong);
void free_allocated_memory();

#endif
