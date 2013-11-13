/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw_task.h
 *
 *  Abstract:
 *      Describes the Task Infrastructure
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

#ifndef _TASK_H_
#define _TASK_H_

#define TASK_NAME_LEN   16

typedef struct _task_info
{
    hlist_node entry;
    pthread_t id;

    unsigned int priority;
    size_t stack_size;

    void (*fn)(void *);
    void *arg;

    pthread_cond_t event_cond;
    pthread_mutex_t event_lock;
    unsigned int events;

    char name[TASK_NAME_LEN + 1];
} task_info;

typedef struct _task_base
{
   pthread_mutex_t lock;

   hlist_head task_list;
} task_base;

int create_task(const char *name, unsigned int priority, size_t ssize, void (*fn)(void *), void *arg, long *tid);
void del_task(long tid);
long search_task(const char *name);

int send_event(long tid, unsigned int event);
int recv_event(long tid, unsigned int interest_events, unsigned int flag, unsigned int *recv_events);

int task_base_init(void);

#endif // _TASK_H_
