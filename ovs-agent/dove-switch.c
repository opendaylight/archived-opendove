/*
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dove-switch.c
 *      This file is the user space portion of the DOVE switch.  The
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

#include <config.h>
#include "dove-switch.h"

#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>

#include "byte-order.h"
#include "classifier.h"
#include "flow.h"
#include "hmap.h"
#include "ofpbuf.h"
#include "ofp-actions.h"
#include "ofp-errors.h"
#include "ofp-msgs.h"
#include "ofp-parse.h"
#include "ofp-print.h"
#include "ofp-util.h"
#include "openflow/openflow.h"
#include "poll-loop.h"
#include "rconn.h"
#include "shash.h"
#include "simap.h"
#include "timeval.h"
#include "vconn.h"
#include "vlog.h"
#include "dhcp.h"

#include "dps_client_common.h"
#include "dps_pkt.h"

VLOG_DEFINE_THIS_MODULE(learning_switch);

struct dove_switch_port {
  struct hmap_node hmap_node; /* Hash node for port number. */
  uint16_t port_no;           /* OpenFlow port number, in host byte order. */
  uint32_t queue_id;          /* OpenFlow queue number. */
};

enum lswitch_state {
    S_CONNECTING,               /* Waiting for connection to complete. */
    S_FEATURES_REPLY,           /* Waiting for features reply. */
    S_SWITCHING,                /* Switching flows. */
};

struct tunnel_ep {
    in_addr_t pIP;
    uint32_t vnid;
};

#define MAX_TUNNEL_EP 128

struct dove_switch {
    struct rconn *rconn;
    enum lswitch_state state;

    /* If nonnegative, the switch sets up flows that expire after the given
     * number of seconds (or never expire, if the value is OFP_FLOW_PERMANENT).
     * Otherwise, the switch processes every packet. */
    int max_idle;

    enum ofputil_protocol protocol;
    unsigned long long int datapath_id;

    struct flow_wildcards wc;   /* Wildcards to apply to flows. */

    /* Queue distribution. */
    uint32_t default_queue;     /* Default OpenFlow queue, or UINT32_MAX. */
    struct hmap queue_numbers;  /* Map from port number to lswitch_port. */
    struct shash queue_names;   /* Map from port name to lswitch_port. */

    /* Number of outgoing queued packets on the rconn. */
    struct rconn_packet_counter *queued;

    /* If true, do not reply to any messages from the switch (for debugging
     * fail-open mode). */
    bool mute;

    /* Optional "flow mod" requests to send to the switch at connection time,
     * to set up the flow table. */
    const struct ofputil_flow_mod *default_flows;
    size_t n_default_flows;
    uint16_t dove_port;
    DC_Address host;

    size_t n_eps;
    struct tunnel_ep eps[MAX_TUNNEL_EP];
};

struct packet_in_params {
  struct dove_switch *sw;
  struct ofpbuf *msg;
  DC_Address src_vIP;
};

/* The log messages here could actually be useful in debugging, so keep the
 * rate limit relatively high. */
static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(30, 300);

static void queue_tx(struct dove_switch *, struct ofpbuf *);
static void send_features_request(struct dove_switch *);

static bool dswitch_process_packet(struct dove_switch *, struct ofpbuf *);
static void process_port_status(struct dove_switch *sw, struct ofp_header *oh);
static enum ofperr process_switch_features(struct dove_switch *,
                                           struct ofp_header *);
static bool process_packet_in(struct dove_switch *, struct ofpbuf *msg);
static void process_echo_request(struct dove_switch *, const struct ofp_header *);

static int register_tunnel(uint32_t pIP, uint32_t vnid, uint32_t on);
static int known_tunnel_ep(struct dove_switch * sw, uint32_t vnid);
static void add_tunnel_ep(struct dove_switch * sw, uint32_t vnid);
static int loc_update(uint32_t pIP, uint32_t vnid, uint32_t vIP, uint8_t * vMAC, void * ctx);
static int pol_resolve(uint32_t vnid, uint32_t vIP, void * ctx);
int dpsa_response(void* rsp);

/* Creates and returns a new learning switch whose configuration is given by
 * 'cfg'.
 *
 * 'rconn' is used to send out an OpenFlow features request. */
struct dove_switch *
dove_switch_create(struct rconn *rconn,
		   const struct dove_switch_config *cfg,
		   ovs_be32 remote_ip)
{
    struct dove_switch *sw;
    uint32_t ofpfw;

    sw = xzalloc(sizeof *sw);
    sw->rconn = rconn;
    sw->state = S_CONNECTING;
    sw->max_idle = cfg->max_idle;
    sw->datapath_id = 0;
    sw->host.kind = IPv4;
    sw->host.addr.ipv4.s_addr = remote_ip;
    sw->dove_port = -1;

    switch (cfg->wildcards) {
    case 0:
        ofpfw = 0;
        break;

    case UINT32_MAX:
        /* Try to wildcard as many fields as possible, but we cannot
         * wildcard all fields.  We need in_port to detect moves.  We need
         * Ethernet source and dest and VLAN VID to do L2 learning. */
        ofpfw = (OFPFW10_DL_TYPE | OFPFW10_DL_VLAN_PCP
                 | OFPFW10_NW_SRC_ALL | OFPFW10_NW_DST_ALL
                 | OFPFW10_NW_TOS | OFPFW10_NW_PROTO
                 | OFPFW10_TP_SRC | OFPFW10_TP_DST);
        break;

    default:
        ofpfw = cfg->wildcards;
        break;
    }
    ofputil_wildcard_from_ofpfw10(ofpfw, &sw->wc);

    sw->default_queue = cfg->default_queue;
    hmap_init(&sw->queue_numbers);
    shash_init(&sw->queue_names);
    if (cfg->port_queues) {
        struct simap_node *node;

        SIMAP_FOR_EACH (node, cfg->port_queues) {
            struct dove_switch_port *port = xmalloc(sizeof *port);
            hmap_node_nullify(&port->hmap_node);
            port->queue_id = node->data;
            shash_add(&sw->queue_names, node->name, port);
        }
    }

    sw->default_flows = cfg->default_flows;
    sw->n_default_flows = cfg->n_default_flows;

    sw->queued = rconn_packet_counter_create();

    return sw;
}

static void
lswitch_handshake(struct dove_switch *sw)
{
    enum ofputil_protocol protocol;

    send_features_request(sw);

    protocol = ofputil_protocol_from_ofp_version(rconn_get_version(sw->rconn));
    if (sw->default_flows) {
        enum ofputil_protocol usable_protocols;
        struct ofpbuf *msg = NULL;
        int error = 0;
        size_t i;

        /* If the initial protocol isn't good enough for default_flows, then
         * pick one that will work and encode messages to set up that
         * protocol.
         *
         * This could be improved by actually negotiating a mutually acceptable
         * flow format with the switch, but that would require an asynchronous
         * state machine.  This version ought to work fine in practice. */
        usable_protocols = ofputil_flow_mod_usable_protocols(
            sw->default_flows, sw->n_default_flows);
        if (!(protocol & usable_protocols)) {
            enum ofputil_protocol want = rightmost_1bit(usable_protocols);
            while (!error) {
                msg = ofputil_encode_set_protocol(protocol, want, &protocol);
                if (!msg) {
                    break;
                }
                error = rconn_send(sw->rconn, msg, NULL);
            }
        }
        if (protocol & usable_protocols) {
            for (i = 0; !error && i < sw->n_default_flows; i++) {
                msg = ofputil_encode_flow_mod(&sw->default_flows[i], protocol);
                error = rconn_send(sw->rconn, msg, NULL);
            }

            if (error) {
                VLOG_INFO_RL(&rl, "%s: failed to queue default flows (%s)",
                             rconn_get_name(sw->rconn), strerror(error));
            }
        } else {
            VLOG_INFO_RL(&rl, "%s: failed to set usable protocol",
                         rconn_get_name(sw->rconn));
        }
    }
    sw->protocol = protocol;
}

bool
dswitch_is_alive(const struct dove_switch *sw)
{
    return rconn_is_alive(sw->rconn);
}

/* Destroys 'sw'. */
void
dswitch_destroy(struct dove_switch *sw)
{
    if (sw) {
        int i;
        struct dove_switch_port *node, *next;

        rconn_destroy(sw->rconn);
        HMAP_FOR_EACH_SAFE (node, next, hmap_node, &sw->queue_numbers) {
            hmap_remove(&sw->queue_numbers, &node->hmap_node);
            free(node);
        }
        shash_destroy(&sw->queue_names);
        rconn_packet_counter_destroy(sw->queued);

        for(i = 0; i < sw->n_eps; i++)
            register_tunnel(sw->host.addr.ipv4.s_addr, sw->eps[i].vnid, 0);

        free(sw);
    }
}

/* Takes care of necessary 'sw' activity, except for receiving packets (which
 * the caller must do). */
void
dswitch_run(struct dove_switch *sw)
{
    int i;
    bool keep = false;

    rconn_run(sw->rconn);

    if (sw->state == S_CONNECTING) {
        if (rconn_get_version(sw->rconn) != -1) {
	    lswitch_handshake(sw);
            sw->state = S_FEATURES_REPLY;
        }
        return;
    }

    for (i = 0; i < 50; i++) {
        struct ofpbuf *msg;

        msg = rconn_recv(sw->rconn);
        if (!msg) {
            break;
        }

        if (!sw->mute) {
            keep = dswitch_process_packet(sw, msg);
        }
	if(!keep) {
	  ofpbuf_delete(msg);
	}
    }
}

void
dswitch_wait(struct dove_switch *sw)
{
    rconn_run_wait(sw->rconn);
    rconn_recv_wait(sw->rconn);
}

/* Processes 'msg', which should be an OpenFlow received on 'rconn', according
 * to the learning switch state in 'sw'.  The most likely result of processing
 * is that flow-setup and packet-out OpenFlow messages will be sent out on
 * 'rconn'.  */
static bool
dswitch_process_packet(struct dove_switch *sw, struct ofpbuf *msg)
{
    enum ofptype type;
    struct ofpbuf b;
    bool keep = false;

    b = *msg;
    if (ofptype_pull(&type, &b)) {
        return false;
    }

    if (sw->state == S_FEATURES_REPLY
        && type != OFPTYPE_ECHO_REQUEST
        && type != OFPTYPE_FEATURES_REPLY) {
        return false;
    }

    switch (type) {
    case OFPTYPE_ECHO_REQUEST:
        process_echo_request(sw, msg->data);
        break;

    case OFPTYPE_FEATURES_REPLY:
        if (sw->state == S_FEATURES_REPLY) {
            if (!process_switch_features(sw, msg->data)) {
                sw->state = S_SWITCHING;
            } else {
                rconn_disconnect(sw->rconn);
            }
        }
        break;

    case OFPTYPE_PACKET_IN:
        keep = process_packet_in(sw, msg);
        break;

    case OFPTYPE_FLOW_REMOVED:
        /* Nothing to do. */
        break;

    case OFPTYPE_HELLO:
    case OFPTYPE_ERROR:
    case OFPTYPE_ECHO_REPLY:
    case OFPTYPE_FEATURES_REQUEST:
    case OFPTYPE_GET_CONFIG_REQUEST:
    case OFPTYPE_GET_CONFIG_REPLY:
    case OFPTYPE_SET_CONFIG:
    case OFPTYPE_PORT_STATUS:
      process_port_status(sw, msg->data);
      break;

    case OFPTYPE_PACKET_OUT:
    case OFPTYPE_FLOW_MOD:
    case OFPTYPE_PORT_MOD:
    case OFPTYPE_BARRIER_REQUEST:
    case OFPTYPE_BARRIER_REPLY:
    case OFPTYPE_QUEUE_GET_CONFIG_REQUEST:
    case OFPTYPE_QUEUE_GET_CONFIG_REPLY:
    case OFPTYPE_DESC_STATS_REQUEST:
    case OFPTYPE_DESC_STATS_REPLY:
    case OFPTYPE_FLOW_STATS_REQUEST:
    case OFPTYPE_FLOW_STATS_REPLY:
    case OFPTYPE_AGGREGATE_STATS_REQUEST:
    case OFPTYPE_AGGREGATE_STATS_REPLY:
    case OFPTYPE_TABLE_STATS_REQUEST:
    case OFPTYPE_TABLE_STATS_REPLY:
    case OFPTYPE_PORT_STATS_REQUEST:
    case OFPTYPE_PORT_STATS_REPLY:
    case OFPTYPE_QUEUE_STATS_REQUEST:
    case OFPTYPE_QUEUE_STATS_REPLY:
    case OFPTYPE_PORT_DESC_STATS_REQUEST:
    case OFPTYPE_PORT_DESC_STATS_REPLY:
    case OFPTYPE_ROLE_REQUEST:
    case OFPTYPE_ROLE_REPLY:
    case OFPTYPE_SET_FLOW_FORMAT:
    case OFPTYPE_FLOW_MOD_TABLE_ID:
    case OFPTYPE_SET_PACKET_IN_FORMAT:
    case OFPTYPE_FLOW_AGE:
    case OFPTYPE_SET_CONTROLLER_ID:
    case OFPTYPE_FLOW_MONITOR_STATS_REQUEST:
    case OFPTYPE_FLOW_MONITOR_STATS_REPLY:
    case OFPTYPE_FLOW_MONITOR_CANCEL:
    case OFPTYPE_FLOW_MONITOR_PAUSED:
    case OFPTYPE_FLOW_MONITOR_RESUMED:
    case OFPTYPE_GET_ASYNC_REQUEST:
    case OFPTYPE_GET_ASYNC_REPLY:
    case OFPTYPE_SET_ASYNC_CONFIG:
    case OFPTYPE_METER_MOD:
    case OFPTYPE_GROUP_REQUEST:
    case OFPTYPE_GROUP_REPLY:
    case OFPTYPE_GROUP_DESC_REQUEST:
    case OFPTYPE_GROUP_DESC_REPLY:
    case OFPTYPE_GROUP_FEATURES_REQUEST:
    case OFPTYPE_GROUP_FEATURES_REPLY:
    case OFPTYPE_METER_REQUEST:
    case OFPTYPE_METER_REPLY:
    case OFPTYPE_METER_CONFIG_REQUEST:
    case OFPTYPE_METER_CONFIG_REPLY:
    case OFPTYPE_METER_FEATURES_REQUEST:
    case OFPTYPE_METER_FEATURES_REPLY:
    case OFPTYPE_TABLE_FEATURES_REQUEST:
    case OFPTYPE_TABLE_FEATURES_REPLY:
    default:
        if (VLOG_IS_DBG_ENABLED()) {
            char *s = ofp_to_string(msg->data, msg->size, 2);
            VLOG_DBG_RL(&rl, "%016llx: OpenFlow packet ignored: %s",
                        sw->datapath_id, s);
            free(s);
        }
    }

    return keep;
}

static void
send_features_request(struct dove_switch *sw)
{
    struct ofpbuf *b;
    struct ofp_switch_config *osc;
    int ofp_version = rconn_get_version(sw->rconn);

    ovs_assert(ofp_version > 0 && ofp_version < 0xff);

    /* Send OFPT_FEATURES_REQUEST. */
    b = ofpraw_alloc(OFPRAW_OFPT_FEATURES_REQUEST, ofp_version, 0);
    queue_tx(sw, b);

    /* Send OFPT_SET_CONFIG. */
    b = ofpraw_alloc(OFPRAW_OFPT_SET_CONFIG, ofp_version, sizeof *osc);
    osc = ofpbuf_put_zeros(b, sizeof *osc);
    osc->miss_send_len = htons(OFP_DEFAULT_MISS_SEND_LEN);
    queue_tx(sw, b);

    /* Set PACKET_IN to NXM */
    b = ofputil_make_set_packet_in_format(rconn_get_version(sw->rconn),
					  NXPIF_NXM);
    queue_tx(sw, b);
}

static void
queue_tx(struct dove_switch *sw, struct ofpbuf *b)
{
    int retval = rconn_send_with_limit(sw->rconn, b, sw->queued, 10);
    if (retval && retval != ENOTCONN) {
        if (retval == EAGAIN) {
            VLOG_INFO_RL(&rl, "%016llx: %s: tx queue overflow",
                         sw->datapath_id, rconn_get_name(sw->rconn));
        } else {
            VLOG_WARN_RL(&rl, "%016llx: %s: send: %s",
                         sw->datapath_id, rconn_get_name(sw->rconn),
                         strerror(retval));
        }
    }
}

static void
process_port_status(struct dove_switch *sw, struct ofp_header *oh)
{
  struct ofputil_port_status ps;
  enum ofperr error;
		
  error = ofputil_decode_port_status(oh, &ps);
  if (error) {
    VLOG_ERR("received invalid port status (%s)",
	     ofperr_to_string(error));
    return;
  }
	
  /* We only want to look for the dove port */
  if (strncmp(ps.desc.name, "dove", 4) == 0) {
    switch (ps.reason) {
    case OFPPR_ADD:
    case OFPPR_MODIFY:

      printf("Found dove port %d\n", ps.desc.port_no);
      sw->dove_port = ps.desc.port_no;
      break;
    case OFPPR_DELETE:
      printf("Remove dove port\n");
      sw->dove_port = -1;
      break;
    }
  }
  return;
}

static enum ofperr
process_switch_features(struct dove_switch *sw, struct ofp_header *oh)
{
    struct ofputil_switch_features features;
    struct ofputil_phy_port port;
    enum ofperr error;
    struct ofpbuf b;

    error = ofputil_decode_switch_features(oh, &features, &b);
    if (error) {
        VLOG_ERR("received invalid switch feature reply (%s)",
                 ofperr_to_string(error));
        return error;
    }

    sw->datapath_id = features.datapath_id;

    while (!ofputil_pull_phy_port(oh->version, &b, &port)) {
        struct dove_switch_port *lp = shash_find_data(&sw->queue_names, port.name);
        if (lp && hmap_node_is_null(&lp->hmap_node)) {
            lp->port_no = port.port_no;
            hmap_insert(&sw->queue_numbers, &lp->hmap_node,
                        hash_int(lp->port_no, 0));
        }

	if (strncmp(port.name, "dove", 4) == 0)
	  {
	    sw->dove_port = port.port_no;
	  }
    }
    return 0;
}

static bool extract_ids(struct dove_switch *sw,
			struct ofputil_packet_in *pi,
			uint32_t *domainID,
			uint32_t *groupID)
{
  if (pi->fmd.regs[0] != 0L) {
    // Non zero reg0 means that it contains domainID
    *domainID = pi->fmd.regs[0];
    *groupID = pi->fmd.regs[1];
    return true;
  } else if ((pi->fmd.in_port == sw->dove_port) &&
	     (pi->fmd.tun_id != 0)) {
    uint32_t tun_id = ntohll(pi->fmd.tun_id);
    
    *domainID = 0; // dummy value
    *groupID = tun_id;
    return true;
  }
  else {
    printf("logical ID extraction failed for in_port = %d, rule.flow.regs[0] = %x, rule.flow.regs[1] = %dtun id %Lx\n", pi->fmd.in_port, pi->fmd.regs[0],
	   pi->fmd.regs[1], pi->fmd.tun_id);

    return false;
  }
}

static bool
process_packet_in(struct dove_switch *sw, struct ofpbuf *msg)
{
    const struct ofp_header *oh = msg->data;
    struct ofputil_packet_in pi;
    uint32_t domainID, groupID;
    enum ofperr error;
    struct ofpbuf pkt;
    struct flow flow;
    DC_PolicyKey key;
    struct packet_in_params * params;

    error = ofputil_decode_packet_in(&pi, oh);
    if (error) {
        VLOG_WARN_RL(&rl, "failed to decode packet-in: %s",
                     ofperr_to_string(error));

	return false;
    }

    /* Ignore packets sent via output to OFPP_CONTROLLER.  This library never
     * uses such an action.  You never know what experiments might be going on,
     * though, and it seems best not to interfere with them. */
    if (pi.reason != OFPR_NO_MATCH) {
      return false;
    }

    /* Extract flow data from 'opi' into 'flow'. */
    ofpbuf_use_const(&pkt, pi.packet, pi.packet_len);
    flow_extract(&pkt, 0, 0, NULL, pi.fmd.in_port, &flow);
    flow.tunnel.tun_id = pi.fmd.tun_id;

    if(!extract_ids(sw, &pi, &domainID, &groupID)) {
      return false;
    }

    if (!known_tunnel_ep(sw, groupID))
    {
        register_tunnel(ntohl(sw->host.addr.ipv4.s_addr), groupID, 1);
        add_tunnel_ep(sw, groupID);
    }

    if(ntohs(flow.dl_type) == ETH_TYPE_IP &&
       flow.nw_proto == IPPROTO_UDP &&
       ntohs(flow.tp_src) == DHCP_CLIENT_PORT &&
       ntohs(flow.tp_dst) == DHCP_SERVER_PORT) {
      // char vNIC_ID[8] = {0, 0, ETH_ADDR_ARGS(flow.dl_src)};
      // DC_Address address = {.kind = IPv4};

      printf("[DISABLED for DCS] Catching dhcp request - ignore\n");

      // enqueue_vip_request(dps, domainID, *((uint64_t *)vNIC_ID), params);

      return false;
    }

    /* Extract flow data from 'opi' into 'flow'. */
    if (flow.nw_src == 0L || flow.nw_dst == 0L) {
      printf("Ignoring the packet: flow.nw_src == 0L || flow.nw_dst == 0L\n");
      return false;
    }


    // 1. if not done yet for this key, QUERY DPS
    // 2. proceed with code below after policy arrives

    key.domainID = domainID;
    key.source.kind = IPv4;
    key.target.kind = IPv4;
    key.source.addr.ipv4.s_addr = flow.nw_src;
    key.target.addr.ipv4.s_addr = flow.nw_dst;

    if (pi.fmd.in_port != sw->dove_port) {
        // do location update

        printf("%s location update:vMAC "ETH_ADDR_FMT" vIP "IP_FMT
                " on pIP "IP_FMT"\n",
                (flow.nw_src == 0 ? "PARTIAL" : "FULL"),
                ETH_ADDR_ARGS(flow.dl_src),
                IP_ARGS(key.source.addr.ipv4.s_addr),
                IP_ARGS(sw->host.addr.ipv4.s_addr));

        loc_update(
            ntohl(sw->host.addr.ipv4.s_addr),
            groupID,
            ntohl(flow.nw_src),
            flow.dl_src,
            0
        );
    }

    printf("Query DPS %ld:%d "IP_FMT" ==> "IP_FMT"\n",
	   (long int) domainID,
	   groupID,
	   IP_ARGS(key.source.addr.ipv4.s_addr),
	   IP_ARGS(key.target.addr.ipv4.s_addr));

    params = malloc(sizeof(struct packet_in_params));

    params->msg = msg;
    params->sw = sw;
    params->src_vIP = key.source;

    pol_resolve(groupID, ntohl(flow.nw_dst), params);

    return true;
}

static void
forward_pkt(struct dove_switch *sw,
	    const struct ofpbuf *msg,
	    const DC_Policy * policy)
{
  uint16_t out_port = OFPP_NONE;
  uint64_t ofpacts_stub[64 / 8];
  struct ofpbuf ofpacts;
  struct flow flow;
  const struct ofp_header *oh = msg->data;
  struct ofputil_packet_out po;
  struct ofputil_packet_in pi;
  struct ofpbuf pkt;
  enum ofperr error;
  struct ofputil_flow_mod fm;
  struct ofpbuf *buffer;
  uint32_t domainID, groupID;
  bool fromDove;

  error = ofputil_decode_packet_in(&pi, oh);
  if (error) {
    VLOG_WARN_RL(&rl, "failed to decode packet-in: %s",
		 ofperr_to_string(error));

    return;
  }

  fromDove = pi.fmd.in_port == sw->dove_port;

  if(!extract_ids(sw, &pi, &domainID, &groupID)) {
       VLOG_ERR("Failed to extract IDs on policy resolution\n");
       return;
  }

  ofpbuf_use_const(&pkt, pi.packet, pi.packet_len);
  flow_extract(&pkt, 0, 0, NULL, pi.fmd.in_port, &flow);

  /* generate ARP reply to in_port */
  if(ntohs(flow.dl_type) == ETH_TYPE_ARP) {
    /* push back ARP reply */
    struct eth_header *eth;
    struct arp_eth_header* reply;
    //struct ofp_action_output ao;
    ovs_be32 ar_spa;

    eth = (struct eth_header *)pkt.data;
    reply = (struct arp_eth_header *)((char *)eth + sizeof(*eth));

    memcpy(eth->eth_dst, eth->eth_src, ETH_ADDR_LEN);
    memcpy(eth->eth_src, policy->vMAC, ETH_ADDR_LEN);

    reply->ar_op = htons(ARP_OP_REPLY);

    memcpy(reply->ar_tha, reply->ar_sha, ETH_ADDR_LEN);
    memcpy(reply->ar_sha, policy->vMAC, ETH_ADDR_LEN);

    ar_spa = reply->ar_tpa;
    reply->ar_tpa = reply->ar_spa;
    reply->ar_spa = ar_spa;

    ofpbuf_use_stack(&ofpacts, ofpacts_stub, sizeof ofpacts_stub);
    ofpact_put_OUTPUT(&ofpacts)->port = pi.fmd.in_port;
    ofpact_pad(&ofpacts);

    po.buffer_id = UINT32_MAX; // What happens with the saved buffer on switch?
    po.packet = pkt.data;
    po.packet_len = pkt.size;

    po.in_port = OFPP_NONE;
    po.ofpacts = ofpacts.data;
    po.ofpacts_len = ofpacts.size;

    queue_tx(sw, ofputil_encode_packet_out(&po, sw->protocol));

    printf("Forward ARP reply to port %d: "IP_FMT" is at "ETH_ADDR_FMT"\n",
	   pi.fmd.in_port,
	   IP_ARGS(reply->ar_spa),
	   ETH_ADDR_ARGS(reply->ar_sha));

    return;
  }


  /* Make actions. */
  ofpbuf_use_stack(&ofpacts, ofpacts_stub, sizeof ofpacts_stub);

  // Set the correct action according to dst host address
  if(sw->host.addr.ipv4.s_addr == policy->host.addr.ipv4.s_addr) {
    // Forward pkt on local switch by resubmit to table 1
    struct ofpact_resubmit *resubmit;
    char next_h_mac[ETH_ADDR_LEN] = {0x0, 0x18, 0xb1, 0xaa, 0xaa, 0x00};

    /* when we have next hop MAC in local forwarding we overwrite MAC */
    if(memcmp(flow.dl_dst, next_h_mac,ETH_ADDR_LEN) == 0) {
      printf("Replace MAC "ETH_ADDR_FMT" with MAC "ETH_ADDR_FMT" \n", 
	     ETH_ADDR_ARGS(flow.dl_dst), ETH_ADDR_ARGS(policy->vMAC));
      
      memcpy(ofpact_put_SET_ETH_DST(&ofpacts)->mac,
	     policy->vMAC, ETH_ADDR_LEN);
    }

    printf("Fwd to local switch\n");
    resubmit = ofpact_put_RESUBMIT(&ofpacts);
    resubmit->ofpact.compat = OFPUTIL_NXAST_RESUBMIT_TABLE;
    resubmit->in_port = pi.fmd.in_port;
    resubmit->table_id = 1;
  } else {
    /* Forward pkt to remote switch via tunnel */
    struct ofpact_tunnel *tunnel;
    struct ofpact_reg_load *load;
    const char *error;
    const struct mf_field *mf;
    union mf_value mf_value;
    char *addr = inet_ntoa(policy->host.addr.ipv4);

    if(sw->dove_port == -1) {
      ovs_fatal(0, "DOVE port is not configured on switch on %s\n",
		inet_ntoa(sw->host.addr.ipv4));
    }

    tunnel = ofpact_put_SET_TUNNEL(&ofpacts);
    tunnel->ofpact.compat = OFPUTIL_NXAST_SET_TUNNEL;
    tunnel->tun_id =groupID;
    
    printf("Fwd pkt via DOVE tunnel through port %d tun_id %Ld\n",
	   sw->dove_port, (long long int) tunnel->tun_id);

    /* Set TUNNEL DST */
    load = ofpact_put_REG_LOAD(&ofpacts);
    mf = mf_from_name("tun_dst");

    if (!mf) {
      ovs_fatal(0, "tun_dst is not valid oxm field name");
    }
    if (!mf->writable) {
      ovs_fatal(0, "tun_dst is not allowed to set");
    }

    error = mf_parse_value(mf, addr, &mf_value);
    if (error) {
        ovs_fatal(0, "%s", error);
    }
    if (!mf_is_value_valid(mf, &mf_value)) {
      ovs_fatal(0, "%s is not valid valid for field tun_dst", addr);
    }
    ofpact_set_field_init(load, mf, &mf_value);

    /* Now set output action to the dove tunnel */
    out_port = fromDove ? OFPP_IN_PORT : sw->dove_port;
    ofpact_put_OUTPUT(&ofpacts)->port = out_port;
  }
  ofpact_pad(&ofpacts);

  /* Prepare packet_out in case we need one. */
  po.buffer_id = pi.buffer_id;
  if (po.buffer_id == UINT32_MAX) {
    po.packet = pkt.data;
    po.packet_len = pkt.size;
  } else {
    po.packet = NULL;
    po.packet_len = 0;
  }
  po.in_port = pi.fmd.in_port;
  po.ofpacts = ofpacts.data;
  po.ofpacts_len = ofpacts.size;

  /* Send the packet with a new flow with the actions above*/
  memset(&fm, 0, sizeof fm);
  match_init_catchall(&fm.match);
  match_set_in_port(&fm.match, pi.fmd.in_port);
  match_set_dl_type(&fm.match, flow.dl_type);
  match_set_nw_src(&fm.match, flow.nw_src);
  match_set_nw_dst(&fm.match, flow.nw_dst);
  if (fromDove) match_set_tun_id(&fm.match, groupID);

  fm.priority = 65535;
  fm.table_id = 0xff;
  fm.command = OFPFC_ADD;
  fm.idle_timeout = 5;
  fm.buffer_id = pi.buffer_id;
  fm.out_port = OFPP_NONE;
  fm.ofpacts = ofpacts.data;
  fm.ofpacts_len = ofpacts.size;
  buffer = ofputil_encode_flow_mod(&fm, sw->protocol);

  queue_tx(sw, buffer);

  /* If the switch didn't buffer the packet, we need to send a copy. */
  if (pi.buffer_id == UINT32_MAX && out_port != OFPP_NONE) {
    queue_tx(sw, ofputil_encode_packet_out(&po, sw->protocol));
  }
}

static void
process_echo_request(struct dove_switch *sw, const struct ofp_header *rq)
{
    queue_tx(sw, make_echo_reply(rq));
}

static void mac2str( const uint8_t * src, char * trg )
{
  sprintf(trg, "%02x:%02x:%02x:%02x:%02x:%02x", src[0], src[1], src[2], src[3], src[4], src[5]);
}

void dove_policy_cb( const DC_PolicyKey * key, const DC_Policy * result, int status, void * opaque )
{
  struct packet_in_params * params = (struct packet_in_params *) opaque;

  if (key == NULL);

  if (status == 0)
    {
      char buf2[100], buf3[20];
      mac2str(result->vMAC, buf3);

      if (result->host.kind == IPv6)
        {
	  inet_ntop(AF_INET6, &result->host.addr.ipv6, buf2, 100);
	  printf("policy resolved: <action:%s, host:"
		 "(IPv6) %s, vMAC: %s>\n",
		 result->action == DROP ? "DROP" : "FORWARD", buf2, buf3);
        }
      else
        {
	  inet_ntop(AF_INET, &result->host.addr.ipv4, buf2, 100);
	  printf("policy resolved: <%s, host:"
		 "(IPv4) %s, vMAC: %s> "IP_FMT" ==> "IP_FMT"\n",
		 result->action == DROP ? "DROP" : "FORWARD", buf2, buf3,
		 IP_ARGS(key->source.addr.ipv4.s_addr),
		 IP_ARGS(key->target.addr.ipv4.s_addr));
        }

      if (result->action == FORWARD)
        forward_pkt(params->sw, params->msg, result);

    }
  else
    {
      printf("policy resolution failed: from vIP "IP_FMT" to vIP "IP_FMT" status = %d\n",
	     IP_ARGS(key->source.addr.ipv4.s_addr),
	     IP_ARGS(key->target.addr.ipv4.s_addr),
	     status);
    }

  ofpbuf_delete(params->msg);
  free(params);
}

static int register_tunnel(uint32_t pIP, uint32_t vnid, uint32_t on)
{
    dps_tunnel_reg_dereg_t dps_tunnel_reg_dereg;
    dps_client_data_t      dps_client_data;

    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&dps_tunnel_reg_dereg, 0, sizeof(dps_tunnel_reg_dereg_t));

    dps_tunnel_reg_dereg.tunnel_info.num_of_tunnels = 1;
    dps_tunnel_reg_dereg.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_tunnel_reg_dereg.tunnel_info.tunnel_list[0].ip4 = pIP; // host order
    dps_tunnel_reg_dereg.tunnel_info.tunnel_list[0].port = 12345;
    dps_tunnel_reg_dereg.tunnel_info.tunnel_list[0].vnid = vnid;
    dps_tunnel_reg_dereg.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_NVGRE;

    memcpy((void *)&dps_client_data.tunnel_reg_dereg.tunnel_info, (void *)&dps_tunnel_reg_dereg.tunnel_info, sizeof(dps_client_data.tunnel_reg_dereg.tunnel_info));

    dps_client_data.hdr.type = on ? DPS_TUNNEL_REGISTER : DPS_TUNNEL_DEREGISTER;
    dps_client_data.hdr.sub_type = on ? DPS_TUNNEL_REG: DPS_TUNNEL_DEREG;
    dps_client_data.hdr.vnid = vnid;
    dps_client_data.hdr.client_id = DPS_SWITCH_AGENT_ID;
    dps_client_data.context = NULL;

    return dps_protocol_client_send(&dps_client_data);
}

static int known_tunnel_ep(struct dove_switch * sw, uint32_t vnid)
{
    int i;
    uint32_t pIP = sw->host.addr.ipv4.s_addr;

    for( i = 0; i < sw->n_eps; i++ )
    {
        struct tunnel_ep * ep = sw->eps + i;
        if (ep->pIP == pIP && ep->vnid == vnid) return 1;
    }

    return 0;
}

static void add_tunnel_ep(struct dove_switch * sw, uint32_t vnid)
{
    uint32_t pIP = sw->host.addr.ipv4.s_addr;
    if (sw->n_eps < MAX_TUNNEL_EP)
    {
        int i = sw->n_eps++;
        sw->eps[i].pIP = pIP;
        sw->eps[i].vnid = vnid;
        printf("registering tunnel ep #%d\n", (int) sw->n_eps);
    }
    else
    {
        printf("too many end-points registered, bailing");
        exit(1);
    }
}

static int loc_update(uint32_t pIP, uint32_t vnid, uint32_t vIP, uint8_t * vMAC, void * ctx)
{
    dps_client_data_t dps_data;

    memset((uint8_t *)&dps_data, 0, sizeof(dps_client_data_t));
    dps_data.context = ctx;

    dps_data.hdr.type = DPS_ENDPOINT_UPDATE;
    dps_data.hdr.vnid = vnid;
    dps_data.hdr.sub_type = DPS_ENDPOINT_UPDATE_ADD;
    dps_data.hdr.client_id = DPS_SWITCH_AGENT_ID;

    dps_data.endpoint_update.vnid = vnid;
    dps_data.endpoint_update.tunnel_info.num_of_tunnels = 1;
    dps_data.endpoint_update.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_data.endpoint_update.tunnel_info.tunnel_list[0].ip4 = pIP;
    dps_data.endpoint_update.vm_ip_addr.family = AF_INET;
    dps_data.endpoint_update.vm_ip_addr.ip4 = vIP;
    memcpy(dps_data.endpoint_update.mac, vMAC, 6);

    return dps_protocol_client_send(&dps_data) != DPS_SUCCESS;
}

int pol_resolve(uint32_t vnid, uint32_t vIP, void * ctx)
{
    dps_client_data_t dps_client_data;

    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_POLICY_REQ;
    dps_client_data.hdr.client_id = DPS_SWITCH_AGENT_ID;
    dps_client_data.hdr.vnid = vnid;
    dps_client_data.policy_req.src_endpoint.vnid = vnid;
    dps_client_data.policy_req.dst_endpoint.vm_ip_addr.family = AF_INET;
    dps_client_data.policy_req.dst_endpoint.vm_ip_addr.ip4 = vIP;
    dps_client_data.context = ctx;

    //memcpy(dps_client_data.policy_req.dst_endpoint.mac,
    //        p_policy_req->dest_vm_mac,
    //        VDP_ETH_ADDR_LEN);

    return dps_protocol_client_send(&dps_client_data);
}

int dpsa_response(void* rsp)
{
    uint32_t    u4RetVal = 0;
    uint32_t    rspType;
    uint32_t    vnid;
    uint32_t    rsp_status;
    //uint32_t  context;
    //uint32_t    rspSubType;
    void*   p_context;

    rspType = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).type;
    //rspSubType = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).sub_type;
    vnid = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).vnid;
    p_context = (void*)((dps_client_data_t*) rsp)->context;
    //context = (uint32_t) p_context;
    rsp_status = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).resp_status;

    if (rspType > DPS_MAX_MSG_TYPE)
    {
        u4RetVal = 1;
        goto dpsa_response_exit;
    }

    printf("Received Response from DPS Client, response type = 0x%x, response status = 0x%x \n", rspType, rsp_status);

    switch (rspType)
    {
        case DPS_ENDPOINT_LOC_REPLY:
        case DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
            break;

        case DPS_POLICY_REPLY:
        {
            if (rsp_status == DPS_NO_ERR)
            {
                dps_policy_reply_t * policy_reply = &((dps_client_data_t*) rsp)->policy_reply;
                dps_endpoint_loc_reply_t* p_loc_reply = &((dps_client_data_t*) rsp)->policy_reply.dst_endpoint_loc_reply;
                int allow = policy_reply->dps_policy_info.dps_policy.vnid_policy.num_permit_rules > 0;

                if (allow)
                {
                    struct packet_in_params * params = (struct packet_in_params *) p_context;

                    DC_PolicyKey key = {
                        .domainID = vnid,
                        .source = params->src_vIP,
                        .target.kind = IPv4,
                        .target.addr.ipv4.s_addr = ntohl(p_loc_reply->vm_ip_addr.ip4)
                    };
                    DC_Policy policy = {.action = FORWARD};

                    memcpy(policy.vMAC, p_loc_reply->mac, 6);
                    policy.host.addr.ipv4.s_addr = ntohl(p_loc_reply->tunnel_info.tunnel_list[0].ip4);

                    dove_policy_cb(&key, &policy, 0, p_context);
                }
                else
                {
                    printf("communication prohibited by policy\n");
                }
            }
            else
                printf("Policy resolution failed\n");
        }
        break;

        case DPS_ENDPOINT_UPDATE_REPLY:
        case DPS_ADDR_RESOLVE:
        case DPS_INTERNAL_GW_REPLY:
        case DPS_BCAST_LIST_REPLY:
        case DPS_UNSOLICITED_BCAST_LIST_REPLY:
        case DPS_UNSOLICITED_VNID_POLICY_LIST:
        case DPS_VNID_POLICY_LIST_REPLY:
        case DPS_UNSOLICITED_EXTERNAL_GW_LIST:
        case DPS_EXTERNAL_GW_LIST_REPLY:
        case DPS_UNSOLICITED_VLAN_GW_LIST:
        case DPS_VLAN_GW_LIST_REPLY:
        case DPS_MCAST_RECEIVER_DS_LIST:
        case DPS_REG_DEREGISTER_ACK:
        case DPS_UNSOLICITED_VNID_DEL_REQ:

            break;

        default:
            u4RetVal = 1;
        break;
    }
dpsa_response_exit:
    return (u4RetVal ? DPS_ERROR : DPS_SUCCESS);
}


static unsigned short checksum(unsigned short * buffer, int bytes)
{
    unsigned long sum = 0;
    int i = bytes;

    while(i>0)
    {
            sum+=*buffer;
            buffer+=1;
            i-=2;
    }
    sum = (sum >> 16) + (sum & 0x0000ffff);
    sum += (sum >> 16);
    return ~sum;
}

static unsigned char msg_type = 2;

void vip_cb(DC_Address srcIp, void *opaque)
{
  struct packet_in_params * params = (struct packet_in_params *) opaque;
  const struct ofp_header *oh = params->msg->data;
  struct ofputil_packet_in pi;
  struct ofputil_packet_out po;
  uint64_t ofpacts_stub[64 / 8];
  struct ofpbuf ofpacts;
  struct ofpbuf pkt;
  struct flow flow;
  struct eth_header *eth;
  struct ip_header *ip;
  struct udp_header *udp;
  struct dhcp_header *dhcp;
  unsigned char * options;
  uint32_t domainID, groupID;
  enum ofperr error;

  error = ofputil_decode_packet_in(&pi, oh);
  if (error) {
    VLOG_WARN_RL(&rl, "failed to decode packet-in: %s",
		 ofperr_to_string(error));

    return;
  }

  if(!extract_ids(params->sw, &pi, &domainID, &groupID)) {
       VLOG_ERR("Failed to extract IDs on policy resolution\n");
       return;
  }

  ofpbuf_use_const(&pkt, pi.packet, pi.packet_len);
  flow_extract(&pkt, 0, 0, NULL, pi.fmd.in_port, &flow);

  if(!(ntohs(flow.dl_type) == ETH_TYPE_IP &&
     flow.nw_proto == IPPROTO_UDP &&
     ntohs(flow.tp_src) == DHCP_CLIENT_PORT &&
       ntohs(flow.tp_dst) == DHCP_SERVER_PORT)) {
    printf("Error vip CB on non DHCP request\n");
  } else {
    //   ofpbuf_use_const(&pkt, opi,  sizeof *opi + 590);
/*     ofpbuf_use_const(&pkt, opi,  sizeof *opi +  */
/* 		     ROUND_UP(ntohs(opi->match_len),8) + 2 +  */
/* 		     IP_HEADER_LEN + ETH_HEADER_LEN +  */
/* 		     sizeof(struct udp_header) + sizeof *dhcp + ); */

    eth = pkt.data;

    memcpy(eth->eth_dst, eth->eth_src, ETH_ADDR_LEN);

    eth->eth_src[3]++;
    eth->eth_src[4]++;
    eth->eth_src[5]++;

    ip =(struct ip_header *)((char *)eth + ETH_HEADER_LEN);
    ip->ip_src = inet_addr("10.0.2.2"); //srcIp.addr.ipv4.s_addr + 1;
    ip->ip_dst = srcIp.addr.ipv4.s_addr;

    ip->ip_csum = 0;
    ip->ip_csum = checksum((unsigned short *)ip, IP_HEADER_LEN);

    udp = (struct udp_header *)((char *)eth + ETH_HEADER_LEN + IP_HEADER_LEN);
    udp->udp_csum = 0;
    //udp->udp_csum = checksum(ip, IP_HEADER_LEN + udp->udp_len);


    printf("src port %d dst port %d\n", ntohs(udp->udp_src), ntohs(udp->udp_dst));

    udp->udp_src = htons(DHCP_SERVER_PORT);
    udp->udp_dst = htons(DHCP_CLIENT_PORT);

    dhcp = ofpbuf_pull(&pkt, DHCP_HEADER_LEN);

    options = ((unsigned char *)dhcp) + DHCP_HEADER_LEN; //pkt.data - 10; //ofpbuf_pull(&pkt, 0);//pkt.size);

    *(uint32_t *)options = htonl(0x63825363);
    options += 4;

    options[0] = 53;
    options[1] = 1;
    options[2] = msg_type;

    if(msg_type == 2) {
      msg_type = 5;
    } else {
      msg_type = 2;
    }

    options += 3;

    options[0] = 1;
    options[1] = 4;
    options += 2;
    *(uint32_t *)options = inet_addr("0.0.0.0");
    options += 4;

    options[0] = 3;
    options[1] = 4;
    options += 2;
    *(uint32_t *)options = inet_addr("0.0.0.0"); //10.0.2.2");
    options += 4;

    //options[0] = 6;
    //options[1] = 4;
    //options += 2;
    //*(uint32_t *)options = inet_addr("10.0.2.2");
    //options += 4;

    //options[0] = 15;
    //options[1] = 13;
    //options += 2;
    //strncpy(options, "haifa.ibm.com", 13);
    //options += 13;

    options[0] = 51;
    options[1] = 4;
    options += 2;
    *(uint32_t *)options = htonl(86400);
    options += 4;

    //options[0] = 54;
    //options[1] = 4;
    //options += 2;
    //*(uint32_t *)options = inet_addr("10.0.2.2");
    //options += 4;

    *options = 0xff;

    dhcp->op = 2;
    dhcp->secs = 0;
    dhcp->yiaddr = srcIp.addr.ipv4.s_addr;
    dhcp->siaddr = inet_addr("10.0.2.4"); //dhcp->yiaddr + 1;

    ofpbuf_use_stack(&ofpacts, ofpacts_stub, sizeof ofpacts_stub);
    ofpact_put_OUTPUT(&ofpacts)->port = htons(pi.fmd.in_port);
    ofpact_pad(&ofpacts);

    po.buffer_id = UINT32_MAX;
    po.packet = (void *)eth;
    po.packet_len =  590; //ETH_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN +
    //DHCP_HEADER_LEN;

    po.in_port = OFPP_NONE;
    po.ofpacts = ofpacts.data;
    po.ofpacts_len = ofpacts.size;

    queue_tx(params->sw, ofputil_encode_packet_out(&po, params->sw->protocol));

    printf("Forward DHCP reply to port %d\n", pi.fmd.in_port);
  }

  ofpbuf_delete(params->msg);
  free(params);
}
