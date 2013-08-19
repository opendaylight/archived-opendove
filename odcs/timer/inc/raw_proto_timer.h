/*
 *
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      raw_proto_timer.h
 *      This file declares Timer/RAW Pkt retransmission related functions.
 *
 *  Author:
 *      Dove Development Team
 *
 */

#ifndef _RAW_PROTO_TIMER_
#define _RAW_PROTO_TIMER_

// Default Retransmission Interval
#define  RAW_PKT_DEF_RETRANSMIT_INTERVAL      4 

// Default MAX Number of Retransmission
#define  MAX_NUM_DEF_RETRANSMIT               3

// Maximum number of allowable outstanding packets
#define  MAX_OUTSTANDING_PKT_CNT              1000

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif



/*
 *  * Return values used by DC_AGENT/DPS_AGENT/CONTROLLER/DPS Server
 * 
 */
typedef enum raw_proto_retransmit_status_e
{
    RAW_PROTO_RETRANSMIT_ERR                    = -1,  // Generic Error
    RAW_PROTO_MAX_NUM_RETRANSMIT_EXCEEDED       = -2,  // MAX NUM Retransmission
    RAW_PROTO_MAX_NUM_OUTSTANDING_PKTS_EXCEEDED = -3,  // MAX NUM Outstanding packets
    RAW_PROTO_RETRANSMIT_OK                     =  1   // RETRANSMISSION OK
} raw_proto_retransmit_status_t;

typedef enum {
    RAW_PROTO_TIMER_RETURN_OK          = 0,
    RAW_PROTO_TIMER_RETURN_ERR         = -1,
    RAW_PROTO_TIMER_RETURN_INVALID_ARG = -2
} raw_proto_timer_return_status_t;

/* Packet owner */
typedef enum {
    RPT_OWNER_FIRST = 1,
    RPT_OWNER_DCA = RPT_OWNER_FIRST,
    RPT_OWNER_DPSA,
    RPT_OWNER_DC,
    RPT_OWNER_DPS,
    RPT_OWNER_LAST = RPT_OWNER_DPS,
    RPT_OWNER_MAXNUM = RPT_OWNER_LAST
} rpt_owner_t;

#define IS_VALID_RPT_OWNER(owner)     ((owner >= RPT_OWNER_FIRST) && (owner <= RPT_OWNER_LAST))
#define IS_NOT_VALID_RPT_OWNER(owner) (!(IS_VALID_RPT_OWNER(owner)))

/* RPT Callback function pointer */
typedef void (*rpt_callback_ptr) \
             (raw_proto_retransmit_status_t, char*, void *context, rpt_owner_t);

// Timer Related Functions
/*>>

    raw_proto_timer_return_status_t
    raw_proto_timer_init( void(*raw_pkt_retransmit_callback) \
                         (raw_proto_retransmit_status, char*, unsigned int context))

    DESCRIPTION:
    This function will Create All the Timer Threads for RAW Protocol Pkt 
    retransmission and initialize associated Mutexes/condition variables.
  
    ARGS:
    raw_pkt_retransmit_callback       Callback function

    RETURNS:
    RAW_PROTO_TIMER_RETURN_OK            success
    RAW_PROTO_TIMER_RETURN_ERR           failure
    RAW_PROTO_TIMER_RETURN_INVALID_ARG   failure, invalid argument

    COMMENTS:

    EXAMPLE:
          raw_proto_timer_init(&raw_pkt_retransmit_callback);


<<*/

raw_proto_timer_return_status_t
raw_proto_timer_init(rpt_callback_ptr, rpt_owner_t owner);

/*>>

    (void)raw_proto_timer_start(char *rawPkt, int rawPktLen, int rawPktId,
                                int retransmitInterval, int maxNumRetransmit,
                                int sockFd, struct sockaddr_in addr)
    (int)raw_proto_timer_start(char *rawPkt, int rawPktLen, int rawPktId,
                               int retransmitInterval, int maxNumRetransmit,
                               int sockFd, struct sockaddr *addr,
                               unsigned int context, rpt_owner_t owner)

    DESCRIPTION:
    This function will start the Timer for the RAW Protocol Pkt Retransmission.
  
    ARGS:
    rawPkt             Pointer to RAW PKT
    rawPktLen          RAW PKT  Length
    rawPktId           PKT  ID (Query ID)
    retransmitInterval Retransmission Interval for the Packet (In Seconds), 
    maxNumRetransmit   Maximum Number of Retransmission for this Packet. 
    sockFd             Socket Dscriptor to send the Packet
    addr               Endpoint Address 
    context
    owner              Module that owns this Packet

    RETURNS:
    (void)

    COMMENTS:

    EXAMPLE:
          raw_proto_timer_start(send_buff, send_len, pHdr->query_id,
                                RAW_PKT_DEF_RETRANSMIT_INTERVAL, 
                                MAX_NUM_DEF_RETRANSMIT,
                                dove_controller.sock_fd,
                                (struct sockaddr *)&dove_controller.svr_ip4);


<<*/

int  raw_proto_timer_start(char *rawPkt, int rawPktLen, int rawPktId,
                          int retransmitInterval, int maxNumRetransmit,
                          int sockFd, struct sockaddr *addr,
                          void *context, rpt_owner_t owner);

/*>>

    (raw_proto_timer_return_status_t) raw_proto_timer_stop(int rawPktId, unsigned int *context)

    DESCRIPTION:
    This function will stop the Retransmission Timer for this RAW Protocol Pkt.
    It will remove the PKT from the List.
  
    ARGS:
    rawPktId       PKT  ID (Query ID)
    context        Get the context of this Pkt ID and copy it here.

    RETURNS:
    RAW_PROTO_TIMER_RETURN_OK            success
    RAW_PROTO_TIMER_RETURN_INVALID_ARG   failure, invalid argument

    COMMENTS:

    EXAMPLE:
          raw_proto_timer_stop(hdr->query_id, &context);


<<*/
raw_proto_timer_return_status_t raw_proto_timer_stop(int rawPktId, 
                                                     void **context, 
                                                     rpt_owner_t *owner);


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
unsigned int raw_proto_query_id_generate (void) ;

#endif
