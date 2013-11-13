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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mqueue.h>
#include <errno.h>
#include "inc/osw.h"
#include <syslog.h>

queue_base *qbase;

/*      
 ******************************************************************************
 * search_queue --                                                        *//**
 *  
 * \brief This routine search a queue according to name. If found, return queue 
 *        id; else return 0.
 *  
 * \param [in]  name    Queue name.
 *  
 * \retval non-zero     Success(queue id)
 * \retval 0            Failure
 *
 *****************************************************************************/
static long search_queue(const char *name)
{
    hlist_head *hhead = &qbase->queue_list;
    hlist_node *hnode = NULL;
    queue_info *qi = NULL;

    pthread_mutex_lock(&qbase->lock);

    hlist_for_each_entry(queue_info, qi, hnode, hhead, entry)
    {
        if (!strcmp(qi->name, name))
        {
            pthread_mutex_unlock(&qbase->lock);
            return (long)qi;
        }
    }

    pthread_mutex_unlock(&qbase->lock);
    return 0;
}

#ifndef NON_POSIX_MQUEUE
/*
 ******************************************************************************
 * create_queue --                                                        *//**
 *
 * \brief This routine creates a queue and allocates related resource for it.
 *
 * \param [in]  name     Queue name.
 * \param [in]  msg_len  Max length of message.
 * \param [in]  msg_num  Max number of messages.
 *  
 * \param [out] qid      Queue id.
        
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *  
 *****************************************************************************/
int create_queue(const char *name, unsigned int msg_len, unsigned int msg_num, long *qid)
{
    struct mq_attr attr;
    queue_info *qi;
    char temp_name[2*QUEUE_NAME_LEN];

    /* check if queue already exists. If exists, nothing to do */
    if (search_queue(name))
    {
        printf("Queue %s already exists!\r\n", name);
        return OSW_ERROR;
    }

    /* allocate resource and initialize it  */
    qi = (queue_info *)malloc(sizeof(queue_info));
    if (qi == NULL)
    {
        printf("No sufficient memory to create a message queue!\r\n");
        return OSW_ERROR;
    }
    memset(qi, 0, sizeof(queue_info));

    *qid = (long)qi;
    hlist_node_init(&qi->entry);
    strncpy(qi->name, name, QUEUE_NAME_LEN);
    memset(temp_name, 0, sizeof(temp_name));
    sprintf(temp_name, "%s%s", "/", qi->name);
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = msg_num;
    attr.mq_msgsize = msg_len;

    pthread_mutex_lock(&qbase->lock);
    hlist_head_add(&qbase->queue_list, &qi->entry);

    if ((qi->id = mq_open(temp_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &attr)) < 0)
    {
        hlist_node_remove(&qi->entry);
        pthread_mutex_unlock(&qbase->lock);
        return OSW_ERROR;
    }

    pthread_mutex_unlock(&qbase->lock);
    return OSW_OK;
}

/*      
 ******************************************************************************
 * del_queue --                                                           *//**
 *  
 * \brief This routine deletes a queue according to id.
 *  
 * \param [in]  qid     Queue Id.
 *
 * \retval None
 *
 *****************************************************************************/
void del_queue(long qid)
{
    queue_info *qi = (queue_info *)qid;

    pthread_mutex_lock(&qbase->lock);
    hlist_node_remove(&qi->entry);
    pthread_mutex_unlock(&qbase->lock);
    mq_close(qi->id);
    return;
}

/*      
 ******************************************************************************
 * queue_send --                                                          *//**
 *  
 * \brief This routine sends a message to a queue.
 *  
 * \param [in]  qid      Queue Id.
 * \param [in]  msg_buf  Buffer pointer.
 * \param [in]  msg_len  Message length.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int queue_send(long qid, char *msg_buf, unsigned int msg_len)
{
    queue_info *qi = (queue_info *)qid;

    if (mq_send(qi->id, msg_buf, msg_len, 0) < 0)
        return OSW_ERROR;

    return OSW_OK;
}

/*      
 ******************************************************************************
 * queue_receive --                                                       *//**
 *  
 * \brief This routine receives a message from a queue.
 *  
 * \param [in]  qid      Queue Id.
 * \param [in]  msg_buf  Buffer pointer.
 * \param [in]  msg_len  Buffer size.
 * \param [in]  timeout  Time to wait in unit of msec. Now support three options
 *                       (OSW_NO_WAIT, OSW_WAIT, specific expired time).
 *
 * \param [out] msg_buf  Buffer which a received message is copied into.

 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int queue_receive(long qid, char *msg_buf, unsigned int msg_len, int timeout)
{
    queue_info *qi = (queue_info *)qid;
    struct timespec ts;
    struct timeval tp;
    unsigned int sec = 0 , usec = 0;

    gettimeofday(&tp, NULL);

    if (timeout == OSW_NO_WAIT)
    {
        ts.tv_sec = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;

        if (mq_timedreceive(qi->id, msg_buf, msg_len, NULL, &ts) <= 0)
            return OSW_ERROR;
    }
    else if (timeout == (int)OSW_WAIT)
    {
        if (mq_receive(qi->id, msg_buf, msg_len, NULL) < 0)
            return OSW_ERROR;
    }
    else
    {
        usec = tp.tv_usec + timeout * 1000;
        if (usec > 1000000)
        {
            sec = usec/1000000;
            usec = usec%1000000;
        }
        ts.tv_sec  = tp.tv_sec + sec;
        ts.tv_nsec = usec * 1000;

        if (mq_timedreceive(qi->id, msg_buf, msg_len, NULL, &ts) <= 0)
            return OSW_ERROR;
    }

    return OSW_OK;
}
#else
/*
 ******************************************************************************
 * create_queue --                                                        *//**
 *
 * \brief This routine creates a queue and allocates related resource for it.
 *
 * \param [in]  name     Queue name.
 * \param [in]  msg_len  Max length of message.
 * \param [in]  msg_num  Max number of messages.
 *  
 * \param [out] qid      Queue id.
        
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *  
 *****************************************************************************/
int create_queue(const char *name, unsigned int msg_len, unsigned int msg_num, long *qid)
{
    queue_info *qi;

    /* check if queue already exists. If exists, nothing to do */
    if (search_queue(name))
    {
        printf("Queue %s already exists!\r\n", name);
        return OSW_ERROR;
    }

    /* allocate resource and initialize it  */
    qi = (queue_info *)malloc(sizeof(queue_info));
    if (qi == NULL)
    {
        printf("No suffient memory to create a message queue!\r\n");
        return OSW_ERROR;
    }
    memset(qi, 0, sizeof(queue_info));

    *qid = (long)qi;
    hlist_node_init(&qi->entry);
    strncpy(qi->name, name, QUEUE_NAME_LEN);
    qi->msg_len = msg_len;
    qi->msg_num = msg_num;
    if ((qi->buffer = (char *)malloc(msg_len*msg_num)) == NULL)
    {
        printf("No suffient memory to create a message queue!\r\n");
        return OSW_ERROR;
    }

    pthread_cond_init(&qi->cond, NULL);
    pthread_mutex_init(&qi->lock, NULL);

    pthread_mutex_lock(&qbase->lock);
    hlist_head_add(&qbase->queue_list, &qi->entry);
    pthread_mutex_unlock(&qbase->lock);

    return OSW_OK;
}

/*      
 ******************************************************************************
 * del_queue --                                                           *//**
 *  
 * \brief This routine deletes a queue according to id.
 *  
 * \param [in]  qid     Queue Id.
 *
 * \retval None
 *
 *****************************************************************************/
void del_queue(long qid)
{
    queue_info *qi = (queue_info *)qid;

    pthread_mutex_lock(&qbase->lock);
    hlist_node_remove(&qi->entry);
    pthread_mutex_unlock(&qbase->lock);

    pthread_mutex_destroy(&qi->lock);
    pthread_cond_destroy(&qi->cond);

    free(qi->buffer);
    free(qi);
    return;
}

/*      
 ******************************************************************************
 * queue_send --                                                          *//**
 *  
 * \brief This routine sends a message to a queue.
 *  
 * \param [in]  qid      Queue Id.
 * \param [in]  msg_buf  Buffer pointer.
 * \param [in]  msg_len  Message length.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int queue_send(long qid, char *msg_buf, unsigned int msg_len)
{
    queue_info *qi = (queue_info *)qid;
    char *data_ptr;

    if (msg_len > qi->msg_len)
        return OSW_ERROR;
    
    pthread_mutex_lock(&qi->lock);

    //if ((qi->write_ptr == qi->read_ptr) && (qi->avail_num != 0))
    if (qi->avail_num >= qi->msg_num)
    {
        /* message queue is already full, return error */
        pthread_mutex_unlock(&qi->lock);
        return OSW_ERROR;
    }

    data_ptr = qi->buffer + qi->write_index*qi->msg_len;
    memcpy(data_ptr, msg_buf, msg_len);
    qi->write_index = (qi->write_index + 1) %  qi->msg_num;
    qi->avail_num++;
    pthread_cond_signal(&qi->cond);

    pthread_mutex_unlock(&qi->lock);

    return OSW_OK;
}

/*      
 ******************************************************************************
 * queue_receive --                                                       *//**
 *  
 * \brief This routine receives a message from a queue.
 *  
 * \param [in]  qid      Queue Id.
 * \param [in]  msg_buf  Buffer pointer.
 * \param [in]  msg_len  Buffer size.
 * \param [in]  timeout  Time to wait in unit of msec. Now support three options
 *                       (OSW_NO_WAIT, OSW_WAIT, specific expired time).
 *
 * \param [out] msg_buf  Buffer which a received message is copied into.

 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int queue_receive(long qid, char *msg_buf, unsigned int msg_len, int timeout)
{
    queue_info *qi = (queue_info *)qid;
    struct timespec ts;
    struct timeval tp;
    unsigned int sec = 0 , usec = 0;
    char *data_ptr;
    int rc;

    if (msg_len > qi->msg_len)
        msg_len = qi->msg_len;

    pthread_mutex_lock(&qi->lock);

    if (timeout == OSW_NO_WAIT)
    {
        if (qi->avail_num == 0)
        {
            pthread_mutex_unlock(&qi->lock);
            return OSW_ERROR;
        }
    }
    else if (timeout == (int)OSW_WAIT)
    {
        while (qi->avail_num == 0)
            pthread_cond_wait(&qi->cond, &qi->lock);
    }
    else
    {
        if (qi->avail_num == 0)
        {
            gettimeofday(&tp, NULL);

            usec = tp.tv_usec + timeout * 1000;
            if (usec > 1000000)
            {
                sec = usec/1000000;
                usec = usec%1000000;
            }
            ts.tv_sec  = tp.tv_sec + sec;
            ts.tv_nsec = usec * 1000;

            rc = pthread_cond_timedwait(&qi->cond, &qi->lock, &ts);
            if (rc == ETIMEDOUT)
            {
                pthread_mutex_unlock(&qi->lock);
                return OSW_ERROR;
            }
        }
    }

    data_ptr = qi->buffer + qi->read_index*qi->msg_len;
    memcpy(msg_buf, data_ptr, msg_len);
    qi->read_index = (qi->read_index + 1) %  qi->msg_num;
    qi->avail_num--;

    pthread_mutex_unlock(&qi->lock);
    return OSW_OK;
}
#endif

/*
 ******************************************************************************
 * queue_base_init --                                                     *//**
 *
 * \brief This routine allocates related resource for message queue management.
 *
 * \retval OSW_OK        Success
 * \retval OSW_ERROR     Failure
 *
 *****************************************************************************/
int queue_base_init(void)
{
    /* create queue base and initialize it */
    qbase = (queue_base *)malloc(sizeof(queue_base));
    if (qbase == NULL)
    {
        printf("Fail to memory allocation!\r\n");
        return OSW_ERROR;
    }
    memset(qbase, 0, sizeof(queue_base));

    pthread_mutex_init(&qbase->lock, NULL);
    hlist_head_init(&qbase->queue_list);

    return OSW_OK;
}

