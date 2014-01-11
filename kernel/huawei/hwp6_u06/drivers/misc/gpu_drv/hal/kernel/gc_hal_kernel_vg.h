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






#ifndef __gc_hal_kernel_vg_h_
#define __gc_hal_kernel_vg_h_

#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_kernel_hardware.h"

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/


/* gckKERNEL object. */
struct _gckVGKERNEL
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to gckOS object. */
    gckOS                       os;

    /* Pointer to gckHARDWARE object. */
    gckVGHARDWARE                   hardware;

    /* Pointer to gckINTERRUPT object. */
    gckVGINTERRUPT              interrupt;

    /* Pointer to gckCOMMAND object. */
    gckVGCOMMAND                    command;

    /* Pointer to context. */
    gctPOINTER                  context;

    /* Pointer to gckMMU object. */
    gckVGMMU                        mmu;

    gckKERNEL                   kernel;
};

/* gckMMU object. */
struct _gckVGMMU
{
    /* The object. */
    gcsOBJECT                   object;

    /* Pointer to gckOS object. */
    gckOS                       os;

    /* Pointer to gckHARDWARE object. */
    gckVGHARDWARE                   hardware;

    /* The page table mutex. */
    gctPOINTER                  mutex;

    /* Page table information. */
    gctSIZE_T                   pageTableSize;
    gctPHYS_ADDR                pageTablePhysical;
    gctPOINTER                  pageTableLogical;

    /* Allocation index. */
    gctUINT32                   entryCount;
    gctUINT32                   entry;
};

#endif /* __gc_hal_kernel_h_ */
