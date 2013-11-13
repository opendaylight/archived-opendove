/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw_semaphore.h
 *
 *  Abstract:
 *      Describes the Semaphore Infrastructure
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

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <semaphore.h>
#include "osw_list.h"

#define SEM_NAME_LEN    16

typedef struct _sem_info
{
    hlist_node entry;
    sem_t id;
    char name[SEM_NAME_LEN + 1];
} sem_info;

typedef struct _sem_base
{
   pthread_mutex_t lock;

   hlist_head sem_list;
} sem_base;

int create_sem(const char *name, unsigned int init_val, unsigned int flag, long *sid);
void del_sem(long sid);

int sem_take(long sid);
int sem_give(long sid);

int sem_base_init(void);

#endif // _SEMAPHORE_H_
