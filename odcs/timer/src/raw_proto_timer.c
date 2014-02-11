/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      raw_proto_timer.c
 *      RAW PKT Retransmission Code
 *
 *  ALERT:
 *      
 *      
 *
 *  Author:
 *      Dove Development Team
 *
 */
#if !defined(DPS_SERVER)
#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#endif
#include <xlocale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "raw_proto_timer.h"

// RAW PROTOCOL TIMER LOGS
#define RAW_PROTO_TIMER_LOGLEVEL_EMERGENCY        0x00000000
#define RAW_PROTO_TIMER_LOGLEVEL_ALERT            0x00000001
#define RAW_PROTO_TIMER_LOGLEVEL_CRITICAL         0x00000002
#define RAW_PROTO_TIMER_LOGLEVEL_ERROR            0x00000003
#define RAW_PROTO_TIMER_LOGLEVEL_WARNING          0x00000004
#define RAW_PROTO_TIMER_LOGLEVEL_NOTICE           0x00000005
#define RAW_PROTO_TIMER_LOGLEVEL_INFO             0x00000006
#define RAW_PROTO_TIMER_LOGLEVEL_DEBUG            0x00000007
#define RAW_PROTO_TIMER_LOGLEVEL_MASK             0x0000000f
#define MAX_BUFFER_LEN     512   // For Log 
#define UINT_MAX_VAL       0xFFFFFFFF // For Query ID
// Globals
#define   WAIT_TIME_SECONDS  1  // Check every 1 sec for retransmission

rpt_callback_ptr gTimerAgentCallbackPtr;

/* Callback functions for all types of packet owners */
rpt_callback_ptr rpt_callbacks[RPT_OWNER_MAXNUM + 1] = {NULL, NULL, NULL, NULL, NULL};

pthread_mutex_t  callbackPtrMutex ;


//  List of PKTS Waiting in the Queue
typedef struct raw_proto_pkt_timer_s {
    int                  pktId;     
    time_t               lastSentTime;
    char                 *rawPkt;
    int                  rawPktLen;
    int                  retransmitInterval;
    int                  maxNumRetransmit;
    int                  transmitCount;
    void *               context;
    rpt_owner_t          owner;
    int                  sockFd;
    struct sockaddr      addr;
    struct raw_proto_pkt_timer_s  *next;       /* next in the list */
} __attribute__((__packed__)) raw_proto_pkt_timer_t;

// This Structure will be used to represent a RAW Packet (Including HDR * TLV)
typedef struct raw_packet_s {
    char             *data;
    int              pktId; 
    int              len;
    int              retransmitInterval;
    int              maxNumRetransmit;
    void *           context;
    rpt_owner_t      owner;
    int              sockFd; 
    struct sockaddr  *addr;
}  __attribute__((__packed__)) raw_packet_t;  


// Mutexes
pthread_mutex_t        pktTimerListMutex ;

int                    start_timer_flag = FALSE; 

int                    stop_timer_flag = FALSE; 

pthread_cond_t         wait_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t        wait_mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_t raw_proto_threads[2];

#if defined(RAW_PROTO_DEBUG)
#define RAW_DEBUG_LEVEL_MIN    0
#define RAW_DEBUG_LEVEL_MEDIUM 1
#define RAW_DEBUG_LEVEL_MAX    2

static int  raw_debug_level = RAW_DEBUG_LEVEL_MIN;
#endif /* defined(RAW_PROTO_DEBUG) */

unsigned int raw_proto_timer_stop_cnt = 0;

// Declare the Static functions.
static int   insert_raw_pkt (raw_packet_t *rawPkt);
static int   delete_raw_pkt (int rawPktId, void **context, rpt_owner_t *owner);

static void*  raw_pkt_retransmision_thread_entry (void *ptr);
static void   raw_pkt_retransmit (void);


// RAW Protocol Log functions.
void print_ht_timer_list(raw_proto_pkt_timer_t  *startNode);

static void raw_timer_log(int level, const char *fmt, va_list ap);

static void _raw_timer_log_info(const char *fmt, ...);

#if 0
static void _raw_timer_log_die(const char *fmt, ...);
static void _raw_timer_log_emergency(const char *fmt, ...);
static void _raw_timer_log_alert(const char *fmt, ...);
static void _raw_timer_log_critical(const char *fmt, ...);
static void _raw_timer_log_error(const char *fmt, ...);
static void _raw_timer_log_warn(const char *fmt, ...);
static void _raw_timer_log_notice(const char *fmt, ...);
static void _raw_timer_log_debug(const char *fmt, ...);
#endif /* 0 */

unsigned int rawTimerLogLevel = RAW_PROTO_TIMER_LOGLEVEL_EMERGENCY; // Variable to Control Log level
unsigned int rawTimerLogFlag  = 1;    //  0: To turn off Log, 1: To turn on Log

#define RAW_PROTO_TIMER_LOG_TEXT "RAW PROTOCOL TIMER: "
#define raw_timer_log_info(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_INFO && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_die(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_ERROR && (rawTimerLogFlag != 0))\
        _raw_timer_log_die(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_emergency(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_EMERGENCY && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_alert(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_ALERT && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_critical(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_CRITICAL && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_error(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_ERROR && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_warn(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_WARNING && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_notice(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_NOTICE && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)

#define raw_timer_log_debug(module_log_level, fmt, args...) \
    if (module_log_level >= RAW_PROTO_TIMER_LOGLEVEL_DEBUG && (rawTimerLogFlag != 0))\
        _raw_timer_log_info(RAW_PROTO_TIMER_LOG_TEXT"%s:%s:%d: " fmt, __FILE__, \
        __FUNCTION__,__LINE__,##args)


#define RAW_DATA_STRUCTURE_HASH

#if defined (RAW_DATA_STRUCTURE_HASH)
#define MAX_HASH_BUCKET 1000   // Bucket Size: Some Large Number
struct HashTable_s
{
    raw_proto_pkt_timer_t *bucketPtr;
};    

typedef struct HashTable_s HashTable_t;

HashTable_t   gHashTab[MAX_HASH_BUCKET];

static int hash(int key)
{
    return key%MAX_HASH_BUCKET;
}

#endif 
/*
 *********************************************************************
 *
 * Static Functions
 *
 *********************************************************************
 */

/* Calculate the Re-Transmission Interval using the Exponential back-off
 * Algorithm: Time Gap for n th Retransmission is:
 *  E(n) =  3 +   {(2 ** n  - 1)/2 }
 *  We will use the round off value( the next integer value) of it because our 
 *  thread wakes up every second..
 */ 

static unsigned int uceil(float x)
{
    unsigned int y = (unsigned int)x;
    if (x == (float)y)
        return y;
    else {
        return y+1;
    }
}
static unsigned int calculate_retry_interval(int n)
{
    unsigned int value = 0; /* Total Value */
    float x;
    x =  RETRY_TIMER_OFFSET + (((1 << n) - 1 )/(2.0));

    value = (int)uceil(x);
    return value;
}


#if defined (RAW_DATA_STRUCTURE_HASH)
/*>>

    static int insert_raw_pkt (raw_packet_t *rawPkt) 

    DESCRIPTION:
    This function will Insert a RAW PKT into the Hash Table of PKTS.
  
    ARGS:
    rawPkt       Pointer to RAW PKT that will be inserted to the Hash Table

    RETURNS:
    0           success
    -1          failure, rawPkt == NULL
    -2          failure, list full, no room for a new entry

    COMMENTS:

    EXAMPLE:

<<*/


static int insert_raw_pkt (raw_packet_t *rawPkt)
{
    raw_proto_pkt_timer_t *timerNode = NULL;
    raw_proto_pkt_timer_t *currTimerNode = NULL;

    int bucketIndex;

    raw_timer_log_info(rawTimerLogLevel,
                       "Entered ..%s:%d\n", __FUNCTION__, __LINE__);
    if (!rawPkt) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "Invalid rawPkt == NULL\n");
        return (-1);
    }

    pthread_mutex_lock(&pktTimerListMutex);
    raw_timer_log_info(rawTimerLogLevel,
                       "Grabbed pktTimerListMutex Grab  >> %s:%d\n", __FUNCTION__, __LINE__);

    bucketIndex = hash(rawPkt->pktId);

    timerNode = (raw_proto_pkt_timer_t *)malloc(sizeof(raw_proto_pkt_timer_t));

    if (!timerNode) {
        raw_timer_log_info(rawTimerLogLevel, "malloc() FAIL: %s:%d\n",__FUNCTION__, __LINE__);
        pthread_mutex_unlock(&pktTimerListMutex);
        return (-3);
    }

    memset(timerNode,0,sizeof(raw_proto_pkt_timer_t));

    raw_timer_log_info(rawTimerLogLevel,
                       "%s:%d >>> About to Add PKT ID: %d\n", __FUNCTION__, __LINE__, rawPkt->pktId);
    timerNode->pktId              = rawPkt->pktId;
    timerNode->transmitCount      = 1;
    timerNode->rawPkt             = rawPkt->data;
    timerNode->lastSentTime       = time(NULL);
    timerNode->rawPktLen          = rawPkt->len;
    timerNode->retransmitInterval = rawPkt->retransmitInterval;
    timerNode->maxNumRetransmit   = rawPkt->maxNumRetransmit;
    timerNode->sockFd             = rawPkt->sockFd;
    timerNode->addr               = *((struct sockaddr *)rawPkt->addr);
    timerNode->context            = rawPkt->context;
    timerNode->owner              = rawPkt->owner;
    timerNode->next               = NULL;

    if ( gHashTab[bucketIndex].bucketPtr == NULL )
    {   
        raw_timer_log_info(rawTimerLogLevel,
                           "%s:%d >>> First Time:.....BUCKET = %d\n", 
                           __FUNCTION__, __LINE__, bucketIndex);
        gHashTab[bucketIndex].bucketPtr = timerNode;
        timerNode->next = NULL;
    }
    else
    {
        raw_timer_log_info(rawTimerLogLevel,
                           "Nodes present adding in BUCKET %d",bucketIndex);
        currTimerNode = gHashTab[bucketIndex].bucketPtr;
        while(currTimerNode->next !=NULL)
        {
            currTimerNode = currTimerNode->next;
        }
        currTimerNode->next = timerNode;
        timerNode->next = NULL;
    }   

    print_ht_timer_list(gHashTab[bucketIndex].bucketPtr);
    raw_timer_log_info(rawTimerLogLevel,
                           "Unlocking pktTimerListMutex\n");
    pthread_mutex_unlock(&pktTimerListMutex);

    return (0);
}
#endif /* defined (RAW_DATA_STRUCTURE_HASH) */


#if defined (RAW_DATA_STRUCTURE_HASH)
/*>>

    static int  delete_raw_pkt (int rawPktId) 

    DESCRIPTION:
    This function will Delete a RAW PKT from the List of PKTS.
  
    ARGS:
    rawPktId       Packet ID of the packet to delete from the list

    PRE-REQUISITES:
    raw_pkt_id_context_mutex is unlocked

    RETURNS:
    0            success
    non-zero     failure

    COMMENTS:

    EXAMPLE:


<<*/
static int delete_raw_pkt (int rawPktId, void **context, rpt_owner_t *owner)
{
    raw_proto_pkt_timer_t  *timerNode        = NULL;
    raw_proto_pkt_timer_t  *prevTimerNode    = NULL;
    raw_proto_pkt_timer_t  *bucketHeadNode   = NULL;

    int                    nodeFound         = FALSE;
    unsigned int           hashval;

    if (rawPktId == 0) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "Invalid rawPktID = 0, Delete RAW PKT\n");
        return (-1);
    }
    hashval = hash(rawPktId);

    pthread_mutex_lock(&pktTimerListMutex);
    raw_timer_log_info(rawTimerLogLevel,"Grabbed pktTimerListMutex Grab\n");
    if (gHashTab[hashval].bucketPtr == NULL)
    {
       raw_timer_log_info(rawTimerLogLevel,
                          "Packet(PKT ID: %d) does not exist\n",  rawPktId);
       *context = NULL;
       *owner = RPT_OWNER_FIRST;
       pthread_mutex_unlock(&pktTimerListMutex);
       return (-1);
    }

    bucketHeadNode = gHashTab[hashval].bucketPtr;
    if (bucketHeadNode == NULL ) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "List is Empty...Delete RAW PKT ID %d\n",
                           rawPktId); 

    } else if (bucketHeadNode->pktId == rawPktId ) {
        /* Head Node in the Bucket to be deleted  */ 
        raw_timer_log_info(rawTimerLogLevel,
                           "Bucket Head Node Match Case, Delete RAW PKT ID %d\n",
                            rawPktId);

        *context = bucketHeadNode->context;
        *owner   = bucketHeadNode->owner;

        nodeFound = TRUE;

        prevTimerNode               =  bucketHeadNode; 
        bucketHeadNode              =  bucketHeadNode->next;
        gHashTab[hashval].bucketPtr =  bucketHeadNode;
         
        if (prevTimerNode->rawPkt != NULL) {
            free(prevTimerNode->rawPkt);
        } else {
            // Nothing for Now.
        }
  
        if (prevTimerNode != NULL) {
            free(prevTimerNode);  
        }
    }
    else if(bucketHeadNode->next)
    {
        /* Multi_Node Case, Some miiddle Node in the Bucket to be deleted  */ 
        timerNode     = bucketHeadNode->next;
        prevTimerNode = bucketHeadNode;

        if (timerNode->next) {
            raw_timer_log_info(rawTimerLogLevel, 
                               "Multi-Node Case:, Delete RAW PKT ID %d\n",
                                rawPktId);
        } else {
            raw_timer_log_info(rawTimerLogLevel, 
                               "Single-Node Case:, Delete RAW PKT ID %d\n",
                               rawPktId);
        }

        while (timerNode != NULL) {
            if ( timerNode->pktId == rawPktId ) {

                *context = timerNode->context;
                *owner   = timerNode->owner;

                prevTimerNode->next = timerNode->next;
                if (timerNode->rawPkt != NULL) {
                    free(timerNode->rawPkt);
                } else {
                    //Nothing for Now.
                }
 
                free(timerNode);  

                nodeFound = TRUE;

                break;
            } else {
                prevTimerNode = timerNode;
                timerNode = timerNode->next;
            }  // end if ..
        } //end while
    }

    if (!nodeFound) {
        raw_timer_log_info(rawTimerLogLevel, "PKT ID %d not found", rawPktId);
    }

    print_ht_timer_list(gHashTab[hashval].bucketPtr);

    raw_timer_log_info(rawTimerLogLevel, "Unlocking pktTimerListMutex\n");
    pthread_mutex_unlock(&pktTimerListMutex);

    return (0);
}

#endif // (RAW_DATA_STRUCTURE_HASH)


#if defined (RAW_DATA_STRUCTURE_HASH)
/*>>

    static void  raw_pkt_retransmit (void)

    DESCRIPTION:
    Will be called from PKT Retransmission Thread Entry function.
    This function will traverse through the Hash Table and re-transmit 
    those packets that are waiting in the Queue for >= RAW_PKT_RETRANSMIT_TIME
    This function will delete those packets in the list that have exceeded
    the maximum number of retransmits. 
  
    ARGS:
    (void)      

    RETURNS:
    (void)

    COMMENTS:

    EXAMPLE:


<<*/
static void  raw_pkt_retransmit (void)
{
  int i;
  /*
   *  timerNode points to the current node.
   *  prevTimerNode points to the previous node of the current node except at
   *  the begining of the list in which case both prevTimerNode and timerNode
   *  are set to the head of the list.
   */
  raw_proto_pkt_timer_t  *currTimerNode  = NULL;   /* current node */
  raw_proto_pkt_timer_t  *prevTimerNode  = NULL;  /* previous node */
  raw_proto_pkt_timer_t  *delNode = NULL;
  raw_proto_pkt_timer_t  *bucketHeadNode = NULL;

  //int rc;
  rpt_owner_t owner;

  for ( i = 0; i < MAX_HASH_BUCKET; i++ )
  {
      pthread_mutex_lock(&pktTimerListMutex);
      raw_timer_log_info(rawTimerLogLevel,"Grab pktTimerListMutex \n");

      if ( gHashTab[i].bucketPtr == NULL ) {
          raw_timer_log_info(rawTimerLogLevel,"Unlock pktTimerListMutex \n");
          pthread_mutex_unlock(&pktTimerListMutex);
          continue;
      }  else { 
          bucketHeadNode  = gHashTab[i].bucketPtr;
          currTimerNode   = bucketHeadNode;
          prevTimerNode   = currTimerNode;
          while (currTimerNode != NULL) 
          {

              /* Calculate the Re-Transmission Interval using the Exponential back-off
               * Algorithm: Time Gap for n th Retransmission is:
               *  E(n) =  3 +   {(2 ** n  - 1)/2 }
               *  We will use the round off value( the next integer value) of it because our 
               *  thread wakes up every second..
               */ 
              
              /*
              if (difftime (time(NULL), currTimerNode->lastSentTime) >= 
                  currTimerNode->retransmitInterval ) 
              */
              if (difftime (time(NULL), currTimerNode->lastSentTime) >= 
                  calculate_retry_interval(currTimerNode->transmitCount)) 
              {
                  if ( currTimerNode->transmitCount <= currTimerNode->maxNumRetransmit ) 
                  {
                      // Send the Pkt
                      raw_timer_log_info(rawTimerLogLevel,
                                         "Packet ID: %d   Transmit count: %d  No ACK received\n",
                                         currTimerNode->pktId, currTimerNode->transmitCount);
                      currTimerNode->transmitCount++;
                      currTimerNode->lastSentTime = time(NULL);
                      sendto(currTimerNode->sockFd, 
                             currTimerNode->rawPkt, 
                             currTimerNode->rawPktLen,
                             0, (struct sockaddr *)(&currTimerNode->addr), 
                             sizeof(struct sockaddr)); 
                      /* keep going */
                      
                      prevTimerNode = currTimerNode;
                      currTimerNode = currTimerNode->next;
                      continue;
                  } else {
                      // Call the Callback function..Notify the Consumer.
                      
                      raw_timer_log_info(rawTimerLogLevel,
                                         "Packet ID: %d   Transmit count: %d  No ACK received,"
                                         " Max number of retransmits reached\n",
                                         currTimerNode->pktId, currTimerNode->transmitCount);
                      // Remove  the PKT from Hash Table
                      prevTimerNode->next = currTimerNode->next;
                      delNode = currTimerNode;
                      currTimerNode = currTimerNode->next;             
                      // Check if it is the head node
                      if (delNode == bucketHeadNode)
                      {
                          gHashTab[i].bucketPtr = bucketHeadNode = currTimerNode;
                          prevTimerNode = currTimerNode;
                          
                      }

                      owner = delNode->owner;
                      pthread_mutex_unlock(&pktTimerListMutex);
                      // pthread_mutex_lock(&callbackPtrMutex) ;
                      if (IS_VALID_RPT_OWNER(owner)) 
                      {
                          if (rpt_callbacks[owner]) {
                              (*(rpt_callbacks[owner]))(RAW_PROTO_MAX_NUM_RETRANSMIT_EXCEEDED,
                                                        delNode->rawPkt, delNode->context,
                                                        owner);
                          } else {
                              ; /* TODO: handle error condition */
                          }

                      } else {
                          ; /* TODO: handle error condition */
                      }
                      pthread_mutex_lock(&pktTimerListMutex);
                      //pthread_mutex_unlock(&callbackPtrMutex) ;
                      
                      if (delNode->rawPkt != NULL) {
                          free(delNode->rawPkt);
                      }
                      free(delNode);

                      continue;
                  }  
                  
              } 
              else 
              {
                  /* Nothing sent, nothing deleted, just keep going */
                  prevTimerNode = currTimerNode;
                  currTimerNode = currTimerNode->next;
              }   /* end of else  Nothing sent, nothing deleted   */
          } //end while
      } /* end else */
      
      if(gHashTab[i].bucketPtr != NULL)  
          print_ht_timer_list(gHashTab[i].bucketPtr);

      raw_timer_log_info(rawTimerLogLevel,"Unlock pktTimerListMutex \n");
      pthread_mutex_unlock(&pktTimerListMutex);
  } // end for i < MAX_HASH_BUCKET
}
#endif // RAW_DATA_STRUCTURE_HASH

#if defined(CALLBACK_THREAD)
/*>>

    static void*   add_to_callback_q (raw_proto_pkt_timer_t  *) 

    DESCRIPTION:
    Add the deleted timer node to a callback list
  
    ARGS:
    (void*)      

    RETURNS:
    (void*)

    COMMENTS:

    EXAMPLE:


<<*/
static void add_to_callback_q(raw_proto_pkt_timer_t  *timerNode)
{
    pthread_mutex_lock(&callbackListMutex);
    timerNode->next = NULL;
    if (callbackListHead == NULL)
    {
        callbackListTail    = timerNode;
        callbackListHead    = timerNode;
    }
    else
    {
        callbackListTail->next = timerNode;
        callbackListTail       = timerNode;
    }
    pthread_mutex_unlock(&callbackListMutex)
}

static void raw_pkt_callback()
{
    rpt_owner_t owner;
    raw_proto_pkt_timer_t *delNode = NULL;

    pthread_mutex_lock(&callbackListMutex);
    if (callbackListHead == NULL)
    {
        raw_timer_log_info(rawTimerLogLevel, "Callback List is Empty...\n");
                         
    }
    else
    {
        while (callbackListHead != NULL)
        {
            delNode = callbackListHead;
            callbackListHead = callbackListHead->next;
            
            owner = delNode->owner;
            
            pthread_mutex_lock(&callbackPtrMutex) ;

            if (IS_VALID_RPT_OWNER(owner)) {
                if (rpt_callbacks[owner]) {
                    (*(rpt_callbacks[owner]))(RAW_PROTO_MAX_NUM_RETRANSMIT_EXCEEDED,
                                              delNode->rawPkt, delNode->context,
                                              owner);
                } else {
                    ; /* TODO: handle error condition */
                }
                
            } else {
                ; /* TODO: handle error condition */
            }
            
            pthread_mutex_unlock(&callbackPtrMutex) ;
            
            if (delNode->rawPkt != NULL) {
                free(delNode->rawPkt);
            } else {
                pktTimerListErrCnt_4++;
            }
            free(delNode);
            if (count == MAX_CALLBACKS_IN_INTERVAL)
            {
                if (callbackListHead == NULL)
                {
                    //adjust tail ptr
                    callbackListTail = callbackListHead;
                }
                break;
            }
        }
    }
    pthread_mutex_unlock(&callbackListMutex)
}

static void*  raw_pkt_callback_thread_entry (void *ptr) {
    int               rc;
    struct timespec   ts;
    struct timeval    tp;
    time_t t;

    while (TRUE) {
        rc = pthread_mutex_lock(&wait_mutex);

        rc =  gettimeofday(&tp, NULL);
        /* Convert from timeval to timespec */
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;
        rc = pthread_cond_timedwait(&wait_cond, &wait_mutex, &ts);
        if (rc == ETIMEDOUT) {
            time(&t);
            //printf("Wait timed out at %s\n", ctime(&t));
            rc = pthread_mutex_unlock(&wait_mutex);
            // Traverse the List and Retransmit the Pkts. 
            raw_pkt_callback();
        }
    }
    return 0;
}
#endif

/*>>

    static void*  raw_pkt_retransmision_thread_entry (void*) 

    DESCRIPTION:
    Entry function for PKT Retransmission Thread 
  
    ARGS:
    (void*)      

    RETURNS:
    (void*)

    COMMENTS:

    EXAMPLE:


<<*/
static void*  raw_pkt_retransmision_thread_entry (void *ptr) {
    int               rc;
    struct timespec   ts;
    struct timeval    tp;
    time_t t;

    while (TRUE) {
        rc = pthread_mutex_lock(&wait_mutex);

        rc =  gettimeofday(&tp, NULL);
        /* Convert from timeval to timespec */
        ts.tv_sec  = tp.tv_sec;
        ts.tv_nsec = tp.tv_usec * 1000;
        ts.tv_sec += WAIT_TIME_SECONDS;
        rc = pthread_cond_timedwait(&wait_cond, &wait_mutex, &ts);
        if (rc == ETIMEDOUT) {
            time(&t);
            //printf("Wait timed out at %s\n", ctime(&t));
            rc = pthread_mutex_unlock(&wait_mutex);
            // Traverse the List and Retransmit the Pkts. 
            raw_pkt_retransmit();
        }
    }
    return 0;
}


/*>>
    raw_proto_timer_return_status_t
    raw_proto_timer_init (rpt_callback_ptr raw_pkt_retransmit_callback, rpt_owner_t owner)

    DESCRIPTION:
    This function will Create All the Timer Threads for RAW Protocol Pkt 
    retransmission and initialize associated Mutexes/condition variables,
    and installs the callback function for the module that invokes this
    function.
  
    ARGS:
    raw_pkt_retransmit_callback       Callback function
    owner                             Caller module, callback function owner

    RETURNS:
    RAW_PROTO_TIMER_RETURN_OK            success
    RAW_PROTO_TIMER_RETURN_ERR           failure
    RAW_PROTO_TIMER_RETURN_INVALID_ARG   failure, invalid argument

    COMMENTS:

    EXAMPLE:


<<*/

raw_proto_timer_return_status_t
raw_proto_timer_init (rpt_callback_ptr raw_pkt_retransmit_callback, rpt_owner_t owner)
{
#if defined (RAW_DATA_STRUCTURE_HASH)
    int i;
#endif
    pthread_mutexattr_t   mta;
    // One for Start Thread, One for Stop, One for Retransmission
    int rc;
    const char *retryThreadName  = "PKT RETRANSMISSION THREAD";    
    static int init_done = 0;

    if (init_done) {
        goto register_callback;
    }

#if defined (RAW_DATA_STRUCTURE_HASH)
    for ( i = 0; i < MAX_HASH_BUCKET; i++ )
    {
        gHashTab[i].bucketPtr = NULL ;
    }
    memset(gHashTab,0,sizeof(gHashTab));
#endif

    if (!raw_pkt_retransmit_callback) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "ERROR; %s: raw_pkt_retransmit_callback = NULL\n",
                           __FUNCTION__);
        return (RAW_PROTO_TIMER_RETURN_INVALID_ARG);
    }

    if (IS_NOT_VALID_RPT_OWNER(owner)) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "ERROR; %s: owner = %d is out of range\n",
                           __FUNCTION__, owner);
        return (RAW_PROTO_TIMER_RETURN_INVALID_ARG);
    }

    // Initialize the mutex to recursive.
    if (pthread_mutexattr_init(&mta) != 0)
    {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    if (pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP) != 0)
    {
        pthread_mutexattr_destroy(&mta);
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    if (pthread_mutex_init(&callbackPtrMutex , &mta) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    if (pthread_mutex_init(&pktTimerListMutex, &mta) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    if (pthread_mutex_lock(&callbackPtrMutex) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    rpt_callbacks[owner] = raw_pkt_retransmit_callback;

    if (pthread_mutex_unlock(&callbackPtrMutex) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

#if 0 //TODO: Code Review -Sushma
    if (pthread_mutex_init(&raw_pkt_mutex, NULL) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }
    if (pthread_mutex_init(&raw_pkt_id_context_mutex, NULL) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }
#endif
    rc = pthread_create(&raw_proto_threads[0], NULL,
                        raw_pkt_retransmision_thread_entry,
                        (void *)retryThreadName);
    if (rc) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "ERROR; return code from pthread_create() is %d \n", 
                           rc);
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }
#if defined(CALLBACK_THREAD)
    if (pthread_mutex_init(&callbackListMutex, NULL) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }
    rc = pthread_create(&raw_proto_threads[1], NULL,
                        raw_pkt_callback_thread_entry,
                        (void *)"Callback Thread");
    if (rc) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "ERROR; return code from pthread_create() is %d \n", 
                           rc);
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }
#endif
    init_done = 1;

    return (RAW_PROTO_TIMER_RETURN_OK);

register_callback:

    if (pthread_mutex_lock(&callbackPtrMutex) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    rpt_callbacks[owner] = raw_pkt_retransmit_callback;

    if (pthread_mutex_unlock(&callbackPtrMutex) != 0) {
        return (RAW_PROTO_TIMER_RETURN_ERR);
    }

    return (RAW_PROTO_TIMER_RETURN_OK);
}

/*>>

    (int)raw_proto_timer_start(char *rawPkt, int rawPktLen, int rawPktId,
                                int retransmitInterval, int maxNumRetransmit,
                                int sockFd, struct sockaddr_in addr)

    DESCRIPTION:
    This function will start the Timer Thread for RAW Protocol Pkt 
    Retransmission.
  
    ARGS:
    rawPkt             Pointer to RAW PKT
    rawPktLen          RAW PKT  Length
    rawPktId           PKT  ID (Query ID)
    retransmitInterval Retransmission for the Packet (In Second), 
    maxNumRetransmit   Maximum Number of Retransmission for this Packet. 
    sockFd             Socket Dscriptor to send the Packet
    addr               Endpoint Address 

    RETURNS:
    0            success
    non-zero     failure

    COMMENTS:

    EXAMPLE:


<<*/

/*
    RETURNS:
    0            success
    non-zero     failure
    -1           invalid input parameter
    -2           timer list full
    -3           malloc() failure in insert_raw_pkt()
    -4           malloc() failure in this function
*/

int  raw_proto_timer_start(char *rawPkt, int rawPktLen, int rawPktId, 
                          int retransmitInterval, int maxNumRetransmit,
                          int sockFd, struct sockaddr *addr,
                          void *context,
                          rpt_owner_t owner)
{
    int rc = 0;
    raw_packet_t           tmpRawPkt = {0};
    raw_timer_log_info(rawTimerLogLevel,
                       "%s:%d >>>  PKT ID: %d\n",
                       __FUNCTION__, __LINE__, rawPktId);

    if (!rawPkt) {
        raw_timer_log_info(rawTimerLogLevel, "rawPkt = NULL in %s\n", __FUNCTION__);
        rc = -1;
    }
    if (!addr) {
        raw_timer_log_info(rawTimerLogLevel, "addr = NULL in %s\n", __FUNCTION__);
        rc = -1;
    }
    if (IS_NOT_VALID_RPT_OWNER(owner)) {
        raw_timer_log_info(rawTimerLogLevel, "owner = %d is out of range in %s\n",
                           owner, __FUNCTION__);
        rc = -1;
    }
    if (rc) {
        return (rc);
    }

    /* Check the value of condition variable and signal waiting thread 
     * when condition is reached. Note that this occurs while mutex is 
     * locked. 
     */

    /* Copy the Pkt in the Shared Memory, will be used by Timer 
       Start Thread
    */

    // This memory will be freed when stopping the Timer
    tmpRawPkt.data = (char *) malloc(rawPktLen);

    if (!tmpRawPkt.data) {
        raw_timer_log_info(rawTimerLogLevel,
                           "malloc() FAIL: %s:%d\n", __FUNCTION__, __LINE__);
        return (-4);
    }

    memcpy(tmpRawPkt.data, rawPkt, rawPktLen);
    
    tmpRawPkt.pktId              = rawPktId;
    tmpRawPkt.len                = rawPktLen;
    tmpRawPkt.retransmitInterval = retransmitInterval;
    tmpRawPkt.maxNumRetransmit   = maxNumRetransmit;
    tmpRawPkt.context            = context;
    tmpRawPkt.owner              = owner;
    tmpRawPkt.sockFd             = sockFd;
    tmpRawPkt.addr               = (struct sockaddr *)addr;         

    rc = insert_raw_pkt (&tmpRawPkt);
    
    if ((rc == -2) || (rc == -3)) {
        free(tmpRawPkt.data);  /* list full */
        tmpRawPkt.data = NULL;
    } 

    return (rc);
}

/*>>

    (raw_proto_timer_return_status_t)
    raw_proto_timer_stop(int rawPktId)

    DESCRIPTION:
    This function will stop the Timer for this RAW Protocol Pkt ID.
    It will remove the PKT from the List.
  
    ARGS:
    rawPktId       PKT  ID (Query ID)
    context        Get the context of this Pkt ID and copy it here.
    owner          Get the owner of this Pkt ID and copy it here.

    RETURNS:
    RAW_PROTO_TIMER_RETURN_OK            success
    RAW_PROTO_TIMER_RETURN_INVALID_ARG   failure, invalid argument

    COMMENTS:

    EXAMPLE:


<<*/
raw_proto_timer_return_status_t raw_proto_timer_stop(int rawPktId,
                                                     void **context, rpt_owner_t *owner)
{
    int rc;
    int tmpRawPktId;
    void *tmpRawPktContext = NULL;
    rpt_owner_t tmpRawPktOwner = RPT_OWNER_FIRST;

    raw_timer_log_info(rawTimerLogLevel, 
                       ".....raw_proto_timer_stop Called PKT_ID: %d", rawPktId);
    raw_timer_log_info(rawTimerLogLevel,
                           "....Entered raw_proto_timer_stop for PKT_ID: %d", rawPktId);
    if (rawPktId == 0) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "Invalid rawPktID = 0in %s\n", __FUNCTION__);
        return (RAW_PROTO_TIMER_RETURN_INVALID_ARG);
    }
    if (context == NULL) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "Invalid context = NULL in %s\n", __FUNCTION__);
        return (RAW_PROTO_TIMER_RETURN_INVALID_ARG);
    }
    if (!owner) {
        raw_timer_log_info(rawTimerLogLevel, 
                           "Invalid owner = NULL in %s\n", __FUNCTION__);
        return (RAW_PROTO_TIMER_RETURN_INVALID_ARG);
    }

 
    /* Check the value of condition variable and signal waiting thread 
     * when condition is reached. Note that this occurs while mutex is 
     * locked. 
     */

    tmpRawPktId = rawPktId;
    
    rc = delete_raw_pkt (tmpRawPktId, &tmpRawPktContext, &tmpRawPktOwner);
    if (rc < 0)
    {
        *context = NULL;
        *owner = tmpRawPktOwner;
    }
    else
    {     
         *context = tmpRawPktContext;
        *owner   = tmpRawPktOwner;
    }

    raw_timer_log_info(rawTimerLogLevel,
                               "....Exiting raw_proto_timer_stop for PKT_ID: %d", rawPktId);
    return (RAW_PROTO_TIMER_RETURN_OK);
}

//  Unique Query ID Generation for RAW Protocol Packets
/*>>

    (unsigned int) queryId = raw_proto_query_id_generate (void) 

    DESCRIPTION:
    This function will generate Unique Query ID for RAW Protocol Packets.
  
    ARGS:
    (void)      

    RETURNS:
    queryId          Query ID (Unqiue PKT ID)

    COMMENTS:

    EXAMPLE:


<<*/
unsigned int raw_proto_query_id_generate (void)
{
    static unsigned int queryId = 0;
    unsigned int generated_queryID;

    if ( queryId == UINT_MAX_VAL ) {
        queryId = 1;  // Start from Beginning Again
    } else {
        queryId++;
    }
    generated_queryID = queryId;

    return generated_queryID;
}


//  RAW PROTOCOL TIMER LOG FUNCTIONS.

static void raw_timer_log(int level, const char *fmt, va_list ap)
{
    char buf[MAX_BUFFER_LEN + 1];

    memset(buf, 0, MAX_BUFFER_LEN);
    vsnprintf(buf, MAX_BUFFER_LEN, fmt, ap);
    syslog(level, "%s", buf);
    if (1) {
        strcat(buf, "\n");
        fprintf(stdout, "%s", buf);
    }
    return;
}

static void _raw_timer_log_info(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_INFO, fmt, ap);
    va_end(ap);
}

#if 0
static void _raw_timer_log_die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_ERR, fmt, ap);
    va_end(ap);
    exit(-1);
}

static void _raw_timer_log_emergency(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_EMERG, fmt, ap);
    va_end(ap);
}

static void _raw_timer_log_alert(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_ALERT, fmt, ap);
    va_end(ap);
}

static void _raw_timer_log_critical(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_CRIT, fmt, ap);
    va_end(ap);
}


static void _raw_timer_log_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_ERR, fmt, ap);
    va_end(ap);
}

static void _raw_timer_log_warn(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_WARNING, fmt, ap);
    va_end(ap);
}

static void _raw_timer_log_notice(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_NOTICE, fmt, ap);
    va_end(ap);
}

static void _raw_timer_log_debug(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    raw_timer_log(LOG_DEBUG, fmt, ap);
    va_end(ap);
}
#endif /* 0 */


//  RAW PROTOCOL TIMER DEBUG FUNCTIONS.
void print_ht_timer_list(raw_proto_pkt_timer_t  *startNode)
{
    raw_proto_pkt_timer_t *currTimerNode = startNode;
    while (currTimerNode != NULL) {
        raw_timer_log_debug(rawTimerLogLevel,"PKT ID %d Context %p XmitCnt %d RexmitInt %d", currTimerNode->pktId,currTimerNode->context,currTimerNode->transmitCount, currTimerNode->retransmitInterval);
        currTimerNode = currTimerNode->next;
    }

}

