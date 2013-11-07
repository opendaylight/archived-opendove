/*
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dove-switch.h
 *      Header file for the user space portion of the DOVE switch.  The
 *      implementation is based on the learning switch in the Open vSwitch
 *      project.
 *
 *
 *  Author:
 *      Liran Schour
 *
 * Portions of this code are
 * Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013 Nicira, Inc.
 * and have been relicensed under the Eclipse Public License v1.0
 * 
 */

#ifndef DOVE_SWITCH_H
#define DOVE_SWITCH_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <netdb.h>
#include "ofp-util.h"

struct ofpbuf;
struct rconn;

enum DC_Address_Kind {IPv4, IPv6, DC_Address_UnixDomain};

typedef struct {
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
	char *path;
    } addr;
    enum DC_Address_Kind kind;
} DC_Address;

typedef struct
{
    unsigned long domainID;
    DC_Address source;
    DC_Address target;

} DC_PolicyKey;

/* policy actions*/
enum DC_Policy_action { FORWARD=1, DROP=0 };

typedef struct
{
    enum DC_Policy_action action;
    DC_Address host;
	uint64_t vmID; // added by Nadav
    unsigned long id;
    unsigned int version;
    unsigned int ttl;
    uint8_t vMAC[6];

} DC_Policy;

enum lswitch_mode {
    LSW_NORMAL,                 /* Always use OFPP_NORMAL. */
    LSW_FLOOD,                  /* Always use OFPP_FLOOD. */
    LSW_LEARN                   /* Learn MACs at controller. */
};

struct dove_switch_config {

    /* 0 to use exact-match flow entries,
     * a OFPFW10_* bitmask to enable specific wildcards,
     * or UINT32_MAX to use the default wildcards (wildcarding as many fields
     * as possible.
     *
     * Ignored when max_idle < 0 (in which case no flows are set up). */
    uint32_t wildcards;

    /* <0: Process every packet at the controller.
     * >=0: Expire flows after they are unused for 'max_idle' seconds.
     * OFP_FLOW_PERMANENT: Set up permanent flows. */
    int max_idle;

    /* Optional "flow mod" requests to send to the switch at connection time,
     * to set up the flow table. */
    const struct ofputil_flow_mod *default_flows;
    size_t n_default_flows;
    enum ofputil_protocol usable_protocols;
    
    /* The OpenFlow queue to use by default.  Use UINT32_MAX to avoid
     * specifying a particular queue. */
    uint32_t default_queue;

    /* Maps from a port name to a queue_id. */
    const struct simap *port_queues;

    /* If true, do not reply to any messages from the switch (for debugging
     * fail-open mode). */
    bool mute;
};

struct packet_in_params {
  struct dove_switch *sw;
  struct ofpbuf *msg;
  DC_Address src_vIP;
};

struct dove_switch *dove_switch_create(struct rconn *, 
				       const struct dove_switch_config *,
				       uint32_t);
bool dswitch_is_alive(const struct dove_switch *);
void lswitch_set_queue(struct dove_switch *sw, uint32_t queue);
void dswitch_run(struct dove_switch *);
void dswitch_wait(struct dove_switch *);
void dswitch_destroy(struct dove_switch *);

void lswitch_mute(struct dove_switch *);

void dove_notify_cb(unsigned long domainID __attribute__((unused)));
void dove_update_cb(uint64_t vmID __attribute__((unused)),
		    unsigned long domainID __attribute__((unused))
		    , int status __attribute__((unused)));
void dove_policy_cb( const DC_PolicyKey * key, const DC_Policy * result, int status, void * opaque );
void vip_cb(struct in_addr srcIp, struct in_addr gw, struct in_addr mask, void *opaque);

#endif /* learning-switch.h */
