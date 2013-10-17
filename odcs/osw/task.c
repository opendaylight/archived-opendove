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
#include "inc/osw.h"

task_base *tsbase;

/*      
 ******************************************************************************
 * search_task --                                                         *//**
 *  
 * \brief This routine search a task according to name. If found, return task 
 *        id; else return 0.
 *  
 * \param [in]  name    Task name.
 *
 * \retval non-zero     Success(task id)
 * \retval 0            Failure
 *
 *****************************************************************************/
long search_task(const char *name)
{
    hlist_head *hhead = &tsbase->task_list;
    hlist_node *hnode = NULL;
    task_info *ti = NULL;

    pthread_mutex_lock(&tsbase->lock);

    hlist_for_each_entry(task_info, ti, hnode, hhead, entry)
    {
        if (!strcmp(ti->name, name))
        {
            pthread_mutex_unlock(&tsbase->lock);
            return (long)ti;
        }
    }

    pthread_mutex_unlock(&tsbase->lock);
    return 0;
}

/*      
 ******************************************************************************
 * wrapper --                                                             *//**
 *  
 * \brief This routine is a wrapper function which help to reclaim allocated 
 *        resource after application function exits.
 *  
 * \param [in]  tid     Task Id.
 *
 * \retval None
 *
 *****************************************************************************/
static void *wrapper(void *arg)
{
    task_info *ti = (task_info *)arg;

    ti->fn(ti->arg);

    del_task((long)ti);

    return NULL;
}

/*
 ******************************************************************************
 * create_task --                                                         *//**
 *
 * \brief This routine creates a task and allocates related resource for it.
 *
 * \param [in]  name     Task name.
 * \param [in]  priority Task priority.
 * \param [in]  ssize    Task stack size.
 * \param [in]  fn       Task entry point function.
 * \param [in]  arg      Task argument to above function.
 *
 * \param [out] tid      Task id;
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int create_task(const char *name, unsigned int priority, size_t ssize,
                void (*fn)(void *), void *arg, long *tid)
{
    pthread_attr_t attr;
    struct sched_param param;
    task_info *ti;

    /* check if task already exists. If exists, nothing to do */
    if (search_task(name))
    {
        printf("Task %s already exists!\r\n", name);
        return OSW_ERROR;
    }

    /* allocate resource and initialize it  */
    ti = (task_info *)malloc(sizeof(task_info));
    if (ti == NULL)
    {
        printf("No suffient memory to create a task!\r\n");
        return OSW_ERROR;
    }
    memset(ti, 0, sizeof(task_info));

    *tid = (long)ti;
    hlist_node_init(&ti->entry);
    strncpy(ti->name, name, TASK_NAME_LEN);
    ti->fn = fn;
    ti->arg = arg;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, ssize);
    ti->stack_size = ssize;

    /* so far no need to support schedule policy and priority, set to default */
    (void)priority;
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
    param.sched_priority = 0;
    pthread_attr_setschedparam(&attr, &param);

    /* initialize resource related to event */
    pthread_cond_init(&ti->event_cond, NULL);
    pthread_mutex_init(&ti->event_lock, NULL);

    pthread_mutex_lock(&tsbase->lock);
    hlist_head_add(&tsbase->task_list, &ti->entry);

    /* invoke system call to create real thread */
    if (pthread_create(&ti->id, &attr, wrapper, (void *)ti))
    {
        hlist_node_remove(&ti->entry);
        pthread_mutex_unlock(&tsbase->lock);

        pthread_mutex_destroy(&ti->event_lock);
        pthread_cond_destroy(&ti->event_cond);
        free(ti);
        return OSW_ERROR;
    }

    pthread_mutex_unlock(&tsbase->lock);
    return OSW_OK;
}

/*      
 ******************************************************************************
 * del_task --                                                            *//**
 *  
 * \brief This routine deletes a task according to id.
 *  
 * \param [in]  tid     Task Id.
 *
 * \retval None
 *
 *****************************************************************************/
void del_task(long tid)
{
    task_info *ti = (task_info *)tid;

    /* remove from task list */
    pthread_mutex_lock(&tsbase->lock);
    hlist_node_remove(&ti->entry);
    pthread_mutex_unlock(&tsbase->lock);

    /* release related resource */
    pthread_mutex_destroy(&ti->event_lock);
    pthread_cond_destroy(&ti->event_cond);

    if (pthread_self() == ti->id)
    {
        pthread_detach(ti->id);
        free(ti);
        pthread_exit(NULL);
    }
    else
    {
        pthread_cancel(ti->id);
        pthread_join(ti->id, NULL);
        free(ti);
    }

    return;
}

/*      
 ******************************************************************************
 * send_event --                                                          *//**
 *  
 * \brief This routine sends a event to a task.
 *  
 * \param [in]  tid     Task Id.
 * \param [in]  event   Event to the task.
 *
 * \retval OSW_OK       Success
 *
 *****************************************************************************/
int send_event(long tid, unsigned int event)
{
    task_info *ti = (task_info *)tid;

    pthread_mutex_lock(&ti->event_lock);
    ti->events |= event;
    pthread_cond_signal(&ti->event_cond);
    //pthread_cond_broadcast(&ti->event_cond);
    pthread_mutex_unlock(&ti->event_lock);

    return OSW_OK;
}

/*
 ******************************************************************************
 * recv_event --                                                          *//**
 *
 * \brief This routine receives interested events. If flag set to NO_WAIT, check
 *        and return, or wait until anyone of interested events happens.
 *
 * \param [in]  tid             Task id.
 * \param [in]  interest_event  The interested events. 
 * \param [in]  flag            Flag
 *
 * \param [out] recv_events     The received events;

 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int recv_event(long tid, unsigned int interest_events, unsigned int flag, unsigned int *recv_events)
{
    task_info *ti = (task_info *)tid;
    int ret = OSW_OK;

    do
    {
        pthread_mutex_lock(&ti->event_lock);
        *recv_events = ti->events & interest_events;
        ti->events &= ~(*recv_events);

#if 0
        if ((*recv_events) || (flag == OSW_NO_WAIT))
            break;
#else
        if (*recv_events)
            break;
        if (flag == OSW_NO_WAIT)
        {
            ret = OSW_ERROR;
            break;
        }
#endif

        pthread_cond_wait(&ti->event_cond, &ti->event_lock);
        pthread_mutex_unlock(&ti->event_lock);
    } while(1);

    pthread_mutex_unlock(&ti->event_lock);
    return ret;
}

/*
 ******************************************************************************
 * task_base_init --                                                      *//**
 *
 * \brief This routine allocates related resource for task management.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int task_base_init(void)
{
    /* create task base and initialize it */
    tsbase = (task_base *)malloc(sizeof(task_base));
    if (tsbase == NULL)
    {
        printf("Fail to memory allocation!\r\n");
        return OSW_ERROR;
    }
    memset(tsbase, 0, sizeof(task_base));

    pthread_mutex_init(&tsbase->lock, NULL);
    hlist_head_init(&tsbase->task_list);

    return OSW_OK;
}

