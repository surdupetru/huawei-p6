/**********************************************************************************
 * Si8240 Linux Driver
 *
 * Copyright (C) 2011-2012 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **********************************************************************************/

typedef struct _QueueHeader_t
{
        uint8_t head;   // queue empty condition head == tail
        uint8_t tail,tailSample;
} QueueHeader_t,*PQueueHeader_t;

#define QUEUE_SIZE(x) (sizeof(x.queue)/sizeof(x.queue[0]))
#define MAX_QUEUE_DEPTH(x) (QUEUE_SIZE(x) -1)
#define QUEUE_DEPTH(x) ((x.header.head <= (x.header.tailSample= x.header.tail))?(x.header.tailSample-x.header.head):(QUEUE_SIZE(x)-x.header.head+x.header.tailSample))
#define QUEUE_FULL(x) (QUEUE_DEPTH(x) >= MAX_QUEUE_DEPTH(x))

#define ADVANCE_QUEUE_HEAD(x) { x.header.head = (x.header.head < MAX_QUEUE_DEPTH(x))?(x.header.head+1):0; }
#define ADVANCE_QUEUE_TAIL(x) { x.header.tail = (x.header.tail < MAX_QUEUE_DEPTH(x))?(x.header.tail+1):0; }

#define RETREAT_QUEUE_HEAD(x) { x.header.head = (x.header.head > 0)?(x.header.head-1):MAX_QUEUE_DEPTH(x); }

