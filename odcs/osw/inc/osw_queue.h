/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw_queue.h
 *
 *  Abstract:
 *      Describes the Message Queue Infrastructure
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

#ifndef _QUEUE_H_
#define _QUEUE_H_ 

#define QUEUE_NAME_LEN  16

#ifndef NON_POSIX_MQUEUE

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

typedef struct _queue_info
{
    hlist_node entry;
    mqd_t id;
    char name[QUEUE_NAME_LEN + 1];
} queue_info;
#else
typedef struct _queue_info
{   
    hlist_node entry;

    unsigned int msg_len;
    unsigned int msg_num;
    char *buffer;
    unsigned int write_index;
    unsigned int read_index;
    unsigned int avail_num;

    pthread_cond_t cond;
    pthread_mutex_t lock;

    char name[QUEUE_NAME_LEN + 1];
} queue_info;
#endif

typedef struct _queue_base
{
   pthread_mutex_t lock;

   hlist_head queue_list;
} queue_base;

int create_queue(const char *name, unsigned int msg_len, unsigned int msg_num, long *qid);
void del_queue(long sid);
int queue_send(long qid, char *msg_buf, unsigned int msg_len);
int queue_receive(long qid, char *msg_buf, unsigned int msg_len, int timeout);

int queue_base_init(void);

#endif // _QUEUE_H_
