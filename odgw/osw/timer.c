/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Author:
 *      John He
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "inc/osw.h"

/* time wheel base */
tvec_base *tvbase;

/*
 ******************************************************************************
 * create_timer --                                                        *//**
 *
 * \brief This routine creates a timer.
 *
 * \param [in]  task_name   Task to which a event is sent when a timer expires.
 * \param [in]  event       Event sent out when a timer expires.
 * \param [in]  callback    Function pointer invoked when a timer expires.
 * \param [in]  data        Parameter passed to callback function.
 *
 * \param [out] timer_id    Pointer to timer id;
 *
 * \retval OSW_OK           Success
 * \retval OSW_ERROR        Failure
 *
 *****************************************************************************/
int create_timer(const char *task_name, unsigned int event,
                 void(*callback)(unsigned long), unsigned long data,
                 long *timer_id)
{
    timer_list *timer = NULL;
    long tid = 0;

    /* valid check */
    if ((task_name == NULL) && (callback == NULL))
        return OSW_ERROR;
    if (!callback)
        if ((tid = search_task(task_name)) == 0)
            return OSW_ERROR;

    /* allocate related resource and initialize it */
    timer = (timer_list *)malloc(sizeof(timer_list));
    if (timer == NULL)
        return OSW_ERROR;

    hlist_node_init(&timer->entry);
    timer->tid = tid;
    timer->event = event;
    timer->callback = callback;
    timer->data = data;
    *timer_id = (long)timer;

    return OSW_OK;
}

/*
 ******************************************************************************
 * del_timer --                                                           *//**
 *
 * \brief This routine deletes a timer.
 *
 * \param [in]  timer_id    Timer id.
 *
 * \retval OSW_OK       Success
 * \retval OSW_ERROR    Failure
 *
 *****************************************************************************/
int del_timer(long timer_id)
{
    timer_list *timer = (timer_list *)timer_id;

    if (timer == NULL)
        return OSW_ERROR;

    free(timer);

    return OSW_OK;
}

/*
 ******************************************************************************
 * add_timer --                                                           *//**
 *
 * \brief This routine really adds a timer into timer wheel. This is a internal
 *        function.
 *
 * \param [in]  timer   Pointer to a timer.
 *
 * \retval None
 *
 *****************************************************************************/
static void add_timer(timer_list *timer)
{
    hlist_head *vec;
    unsigned long expires = timer->expires;
    unsigned long offset = timer->expires - tvbase->ticks;
    unsigned long index;

    /* determine which vector and which position */
    if (offset < TV1_OFFSET)
    {
        index = (expires/TV1_INTERVAL) % TV1_SIZE;
        vec = &tvbase->tv1[index];
    }
    else if (offset < TV2_OFFSET)
    {
        index = (expires/TV2_INTERVAL) % TV2_SIZE;
        vec = &tvbase->tv2[index];
    }
    else if (offset < TV3_OFFSET)
    {
        index = (expires/TV3_INTERVAL) % TV3_SIZE;
        vec = &tvbase->tv3[index];
    }
    else if (offset < TV4_OFFSET)
    {
        index = (expires/TV4_INTERVAL) % TV4_SIZE;
        vec = &tvbase->tv4[index];
    }
    else
    {
        index = (expires/TV5_INTERVAL) % TV5_SIZE;
        vec = &tvbase->tv5[index];
    }

    /* insert it into list */
#ifdef TIMER_DEBUG 
    printf("%p>>%p\r\n", timer, vec);
#endif
    hlist_head_add(vec, &timer->entry);

    return;
}

/*
 ******************************************************************************
 * start_timer_internal --                                                *//**
 *
 * \brief This routine internally adds a timer into timer wheel to start it.
 *
 * \param [in]  timer_id    Timer id.
 * \param [in]  expires     Time after what a timer will expire.
 *
 * \retval None
 *
 *****************************************************************************/
void start_timer_internal(long timer_id, unsigned long expires)
{
    timer_list *timer = (timer_list *)timer_id;

    pthread_mutex_lock(&tvbase->lock);

    timer->expires = expires + tvbase->ticks;
    add_timer(timer);

    pthread_mutex_unlock(&tvbase->lock);
    return;
}

/*
 ******************************************************************************
 * start_timer --                                                         *//**
 *
 * \brief This routine adds a timer into timer wheel to start it.
 *
 * \param [in]  timer_id    Timer id.
 * \param [in]  sec         The seconds value of the timer.
 * \param [in]  msec        The milliseconds value of the timer.
 *
 * \retval None
 *
 *****************************************************************************/
void start_timer(long timer_id, unsigned int sec, unsigned int msec)
{
    unsigned long expires;

    expires = (sec*1000 + msec)/MSEC_PER_TICK;

    return start_timer_internal(timer_id, expires);
}

/*
 ******************************************************************************
 * stop_timer --                                                          *//**
 *
 * \brief This routine removes a timer from timer wheel to stop it.
 *
 * \param [in]  timer_id    Timer id.
 *
 * \retval None
 *
 *****************************************************************************/
void stop_timer(long timer_id)
{
    timer_list *timer = (timer_list *)timer_id;

    pthread_mutex_lock(&tvbase->lock);

    if (timer &&
        timer->entry.pprevious &&
        timer->entry.next != LIST_POISON1 &&
        timer->entry.pprevious != LIST_POISON2)
        hlist_node_remove(&timer->entry);

    pthread_mutex_unlock(&tvbase->lock);
    return;
}

/*
 ******************************************************************************
 * process_tvec --                                                        *//**
 *
 * \brief This routine processes all timers, check whether every timer expired
 *        If expired, invokes registered callback function or sends event; 
 *        else re-adds into timer wheel. 
 *
 * \param [in]  hhead   Timer hash list head.
 *
 * \retval None
 *
 *****************************************************************************/
static void process_tvec(hlist_head *hhead)
{
    hlist_node *hnode = hhead->first;
    timer_list *timer = NULL;
    unsigned long expires;

    /* here there is a special case in tv5.
     * for example, tv5 supports up to 1 day,
     * if usr add a 3 day timer, so re-adding it
     * to the same vector will happen when processing 
     * the vector where the timer is at.
     * This case is normal, but we have to deal with
     * it.
     */
    while (hnode)
    {
        timer = hlist_entry(hnode, timer_list, entry);
        hnode = hnode->next;
        hlist_node_remove(&timer->entry);

        expires = timer->expires - tvbase->ticks;
        if (!expires)
        {
            if (timer->callback)
            {
                timer->callback(timer->data);
            }
            else
            {
                send_event(timer->tid, timer->event);
            }
        }
        else
        {
#ifdef TIMER_DEBUG 
            printf(">>\n");
#endif
            add_timer(timer);
        }
    }

    return;
}

/*
 ******************************************************************************
 * process_tick --                                                        *//**
 *
 * \brief This routine scans timer wheel and process expired vectors
 *
 * \retval None
 *
 *****************************************************************************/
static void process_tick(void)
{
    int index;

    pthread_mutex_lock(&tvbase->lock);

    /* process tv1 */
    index = tvbase->ticks % TV1_SIZE;
    process_tvec(&tvbase->tv1[index]);
    if (!index)
    {
        /* If tv1 overflows, process tv2 */
        index = (tvbase->ticks/TV2_INTERVAL) % TV2_SIZE;
        process_tvec(&tvbase->tv2[index]);
        if (!index)
        {
            /* If tv2 overflows, process tv3 */
            index = (tvbase->ticks/TV3_INTERVAL) % TV3_SIZE;
            process_tvec(&tvbase->tv3[index]);
            if (!index)
            {
                /* If tv3 overflows, process tv4 */
                index = (tvbase->ticks/TV4_INTERVAL) % TV4_SIZE;
                process_tvec(&tvbase->tv4[index]);
                if (!index)
                {
                    /* If tv4 overflows, process tv5 */
                    index = (tvbase->ticks/TV5_INTERVAL) % TV5_SIZE;
                    process_tvec(&tvbase->tv5[index]);
                }
            }
        }
    }

    pthread_mutex_unlock(&tvbase->lock);

    ++tvbase->ticks;
}

/*
 ******************************************************************************
 * timer_main --                                                          *//**
 *
 * \brief This routine implements a self-defined heartbeat to periodically scan
 *        timer wheel.
 *
 * \param [in]  arg     Not used.
 *
 * \retval None
 *
 *****************************************************************************/
static void timer_main(void *arg)
{
    struct timespec req;
    struct timeval cur;
    struct timeval last;
    unsigned long elapsed_usec, total_elapsed_usec = 0;
    unsigned long ticks, total_ticks = 0, real_total_ticks = 0;

    /* unused argument */
    (void)arg;

    req.tv_sec = MSEC_PER_TICK/1000;
    req.tv_nsec = MSEC_PER_TICK*1000000;

    last.tv_sec = 0;
    last.tv_usec = 0;

    while (1)
    {
        gettimeofday(&cur, NULL);

        /* compute elapsed usecond during a cycle 
         * including timer processing and a sleep.
         * we have to conside the below case in which 
         * maybe due to some reasons, for example,  
         * callback handling for long time, a cycle exceeds a tick.
         */
        if ((last.tv_sec == 0) && (last.tv_usec == 0))
            last = cur;
        elapsed_usec = COMPUTE_ELAPSED_TIME(cur, last);
        ticks = elapsed_usec/(1000*MSEC_PER_TICK);
#ifdef TIMER_DEBUG 
        printf("\033[36m\r[%d.%d - %d.%d][%d, %d]\033[m\n",
               last.tv_sec, last.tv_usec, cur.tv_sec, cur.tv_usec, elapsed_usec, ticks);
#endif

        /* error alway exists every cycle, so expires of a timer 
         * is longer, accumulation error is bigger, for example,
         * timer is one day, maybe accumulation error is 1 minute.
         * So we have to correct error.
         */
        total_elapsed_usec += elapsed_usec;
        real_total_ticks = total_elapsed_usec/(1000*MSEC_PER_TICK);
        total_ticks += ticks;

        if (real_total_ticks != total_ticks)
        {
#ifdef TIMER_DEBUG
            printf("rts = %lu, ts = %lu\r\n", real_total_ticks, total_ticks);
#endif
            ticks += (real_total_ticks - total_ticks);
            total_elapsed_usec = real_total_ticks = total_ticks = 0;
        }

        while (ticks) 
        {
            process_tick();
            ticks--;
        }

        /* sleep a self-defined tick */
        nanosleep(&req, NULL);
        last = cur;
    }

    return;
}

/*
 ******************************************************************************
 * timer_base_init --                                                     *//**
 *
 * \brief This routine creates timer task and allocates related resource.
 *
 * \retval OSW_OK       Success
 * \retval OSW_ERROR    Failure
 *
 *****************************************************************************/
int timer_base_init(void)
{
    pthread_mutexattr_t lockattr;

    /* create timer vector base and initialize it */
    tvbase = (tvec_base *)malloc(sizeof(tvec_base));
    if (tvbase == NULL)
    {
        printf("Fail to memory allocation!\r\n");
        return OSW_ERROR;
    }
    memset(tvbase, 0, sizeof(tvec_base));

    pthread_mutexattr_init(&lockattr);
    pthread_mutexattr_settype(&lockattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(tvbase->lock), &lockattr);

    /* create timer thread */
    if (create_task("Timer", 0, 0x100000, timer_main, 0, &tvbase->tid))
    {
        printf("Fail to create timer thread!\r\n");
        return OSW_ERROR;
    }

    return OSW_OK;
}

