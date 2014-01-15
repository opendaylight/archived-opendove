/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw_timer.h
 *
 *  Abstract:
 *      Describes the Timer Infrastructure
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      Userspace Mode
 *
 *  Revision History:
 *
 */

#ifndef _TIMER_H_
#define _TIMER_H_


#ifndef TEST_DEBUG
#define TV1_BITS        6
#define TV2_BITS        4
#define TV3_BITS        4
#define TV4_BITS        4
#define TV5_BITS        4
#else
#define TV1_BITS        2
#define TV2_BITS        1
#define TV3_BITS        1
#define TV4_BITS        1
#define TV5_BITS        1
#endif

/* time vector size */
#define TV1_SIZE        (1<<TV1_BITS)
#define TV2_SIZE        (1<<TV2_BITS)
#define TV3_SIZE        (1<<TV3_BITS)
#define TV4_SIZE        (1<<TV4_BITS)
#define TV5_SIZE        (1<<TV5_BITS)

/* offset */
#define TV1_OFFSET      (1<<(TV1_BITS))
#define TV2_OFFSET      (1<<(TV1_BITS+TV2_BITS))
#define TV3_OFFSET      (1<<(TV1_BITS+TV2_BITS+TV3_BITS))
#define TV4_OFFSET      (1<<(TV1_BITS+TV2_BITS+TV3_BITS+TV4_BITS))
#define TV5_OFFSET      (1<<(TV1_BITS+TV2_BITS+TV3_BITS+TV4_BITS+TV5_BITS))

/* interval */
#define TV1_INTERVAL    (1)
#define TV2_INTERVAL    (1<<(TV1_BITS))
#define TV3_INTERVAL    (1<<(TV1_BITS+TV2_BITS))
#define TV4_INTERVAL    (1<<(TV1_BITS+TV2_BITS+TV3_BITS))
#define TV5_INTERVAL    (1<<(TV1_BITS+TV2_BITS+TV3_BITS+TV4_BITS))

#define MSEC_PER_TICK   100 /* 100ms */

#define COMPUTE_ELAPSED_TIME(tv1, tv2)                          \
({                                                              \
    long long __delta_sec = (tv1).tv_sec - (tv2).tv_sec;        \
    long long __delta_usec = (tv1).tv_usec - (tv2).tv_usec;     \
    switch (__delta_sec) {                                      \
    default:                                                    \
        __delta_usec = __delta_sec * 1000000 + __delta_usec;    \
        break;                                                  \
    case 2:                                                     \
        __delta_usec += 1000000;                                \
    case 1:                                                     \
        __delta_usec += 1000000;                                \
    case 0: ;                                                   \
    }                                                           \
    __delta_usec;                                               \
})

typedef struct _timer_list
{
    hlist_node entry;
    unsigned long expires;

    long tid;
    unsigned int event;

    void (*callback)(unsigned long);
    unsigned long data;
} timer_list;

typedef struct _tvec_base
{
    pthread_mutex_t lock;
    unsigned long ticks;

    hlist_head tv1[TV1_SIZE];
    hlist_head tv2[TV2_SIZE];
    hlist_head tv3[TV3_SIZE];
    hlist_head tv4[TV4_SIZE];
    hlist_head tv5[TV5_SIZE];

    long tid; 
} tvec_base;

int create_timer(const char *task_name, unsigned int event,
                 void(*callback)(unsigned long), unsigned long data,
                 long *timer_id);
int del_timer(long timer_id);
void start_timer(long timer_id, unsigned int sec, unsigned int msec);
void stop_timer(long timer_id);
int timer_base_init(void);

#endif // _TIMER_H_

