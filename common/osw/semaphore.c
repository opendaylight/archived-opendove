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
#include <semaphore.h>
#include "inc/osw.h"

sem_base *sembase;

/*      
 ******************************************************************************
 * search_sem --                                                         *//**
 *  
 * \brief This routine search a semaphore according to name. If found, return task 
 *        id; else return 0.
 *  
 * \param [in]  name    Task name.
 *
 * \retval non-zero     Success(semaphore id)
 * \retval 0            Failure
 *
 *****************************************************************************/
static long search_sem(const char *name)
{
    hlist_head *hhead = &sembase->sem_list;
    hlist_node *hnode = NULL;
    sem_info *si = NULL;

    pthread_mutex_lock(&sembase->lock);

    hlist_for_each_entry(sem_info, si, hnode, hhead, entry)
    {
        if (!strcmp(si->name, name))
        {
            pthread_mutex_unlock(&sembase->lock);
            return (long)si;
        }
    }

    pthread_mutex_unlock(&sembase->lock);
    return 0;
}

/*
 ******************************************************************************
 * create_sem --                                                          *//**
 *
 * \brief This routine creates a semaphore and allocates related resource for it.
 *
 * \param [in]  name     Semaphore name.
 * \param [in]  init_val Initial value for the semaphore
 * \param [in]  flag     unused.
 *  
 * \param [out] sid      Semaphore id.
        
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *  
 *****************************************************************************/
int create_sem(const char *name, unsigned int init_val, unsigned int flag, long *sid)
{
    sem_info *si;

    /* check if semaphore already exists. If exists, nothing to do */
    if (search_sem(name))
    {
        printf("Semaphore %s already exists!\r\n", name);
        return OSW_ERROR;
    }

    si = (sem_info *)malloc(sizeof(sem_info));
    if (si == NULL)
    {
        printf("No suffient memory to create a semaphore!\r\n");
        return OSW_ERROR;
    }
    memset(si, 0, sizeof(sem_info));

    *sid = (long)si;
    hlist_node_init(&si->entry);
    strncpy(si->name, name, SEM_NAME_LEN);

    pthread_mutex_lock(&sembase->lock);
    hlist_head_add(&sembase->sem_list, &si->entry);

    if (sem_init(&si->id, 0, init_val) < 0)
    {
        hlist_node_remove(&si->entry);
        pthread_mutex_unlock(&sembase->lock);
        return OSW_ERROR;
    }

    pthread_mutex_unlock(&sembase->lock);
    return OSW_OK;
}

/*      
 ******************************************************************************
 * del_sem --                                                             *//**
 *  
 * \brief This routine deletes a semaphore according to id.
 *  
 * \param [in]  sid     Semaphore Id.
 *
 * \retval None
 *
 *****************************************************************************/
void del_sem(long sid)
{
    sem_info *si = (sem_info *)sid;

    pthread_mutex_lock(&sembase->lock);
    hlist_node_remove(&si->entry);
    pthread_mutex_unlock(&sembase->lock);
    sem_destroy(&si->id);
    return;
}

/*      
 ******************************************************************************
 * sem_take --                                                            *//**
 *  
 * \brief This routine acquires a semaphore.
 *  
 * \param [in]  sid     Semaphore Id.
 *
 * \retval OSW_OK       Success
 * \retval OSW_ERROR    Failure
 *
 *****************************************************************************/
int sem_take(long sid)
{
    sem_info *si = (sem_info *)sid;

    if (sem_wait(&si->id) < 0)
        return OSW_ERROR;

    return OSW_OK;
}

/*      
 ******************************************************************************
 * sem_give --                                                            *//**
 *  
 * \brief This routine releases a semaphore.
 *  
 * \param [in]  sid     Semaphore Id.
 *
 * \retval OSW_OK       Success
 * \retval OSW_ERROR    Failure
 *
 *****************************************************************************/
int sem_give(long sid)
{
    sem_info *si = (sem_info *)sid;

    if (sem_post(&si->id) < 0)
        return OSW_ERROR;

    return OSW_OK;
}

/*
 ******************************************************************************
 * sem_base_init --                                                       *//**
 *
 * \brief This routine allocates related resource for semaphore management.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int sem_base_init(void)
{
    /* create semaphore base and initialize it */
    sembase = (sem_base *)malloc(sizeof(sem_base));
    if (sembase == NULL)
    {
        printf("Fail to memory allocation!\r\n");
        return OSW_ERROR;
    }
    memset(sembase, 0, sizeof(sem_base));

    pthread_mutex_init(&sembase->lock, NULL);
    hlist_head_init(&sembase->sem_list);

    return OSW_OK;
}

