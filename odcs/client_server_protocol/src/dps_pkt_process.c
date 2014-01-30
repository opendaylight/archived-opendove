/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Source File:
 *      dps_pkt_process.c
 *      This file contains functions that processes DPS control protocol
 *
 *  Author:
 *      Sushma Anantharam
 *
 */

#if defined(DPS_SERVER)
#include "include.h"
#else
#include "sys/socket.h"
#endif
#include "raw_proto_timer.h"
#include "dps_client_common.h"
#include "dps_pkt.h"
#include "dps_log.h"


const char *dps_msg_name(uint8_t);

#if defined (NDEBUG) || defined (VMX86_DEBUG)
static void dump_client_info(dps_client_data_t *buff);
#else
#define dump_client_info(buff)
#endif
static void dump_tlv_info(uint8_t *buff, uint32_t len);
#if defined (NDEBUG) || defined (VMX86_DEBUG)
static void dump_pkt(uint8_t *buff, uint32_t len);
#else
#define dump_pkt(_buff, _len)
#endif

// Buffer that is passed to the protocol client
//static uint8_t dps_client_buff[DPS_MAX_BUFF_SZ];

// Macros

#define DPS_GET_CLIENT_HDR(client_req) \
	(dps_client_hdr_t *)&(((dps_client_data_t *)(client_req))->hdr)
#define DPS_GET_CLIENT_ENDPT_LOC_REQ(client_req) \
	(dps_endpoint_loc_req_t *)&(((dps_client_data_t *)(client_req))->endpoint_loc_req)
#define DPS_GET_CLIENT_ENDPT_LOC_REPLY(client_req) \
	(dps_endpoint_loc_reply_t *)&(((dps_client_data_t *)(client_req))->endpoint_loc_reply)
#define DPS_GET_CLIENT_POLICY_REQ(client_req) \
	(dps_policy_req_t *)&(((dps_client_data_t *)(client_req))->policy_req)
#define DPS_GET_CLIENT_POLICY_REPLY(client_req) \
	(dps_policy_reply_t *)&(((dps_client_data_t *)(client_req))->policy_reply)
#define DPS_GET_CLIENT_ENDPT_UPDATE(client_req) \
	(dps_endpoint_update_t *)&(((dps_client_data_t *)(client_req))->endpoint_update)
#define DPS_GET_CLIENT_ENDPT_UPDATE_REPLY(client_req) \
	(dps_endpoint_update_reply_t *)&(((dps_client_data_t *)(client_req))->endpoint_update_reply)
#define DPS_GET_CLIENT_INTERNAL_GW_REQ(client_req) \
	(dps_internal_gw_req_t *)&(((dps_client_data_t *)client_req)->internal_gw_req)
#define DPS_GET_CLIENT_GEN_MSG_REQ(client_req) \
	(dps_gen_msg_req_t *)&(((dps_client_data_t *)client_req)->gen_msg_req)
#define DPS_GET_IP_ADDR_TLV_LEN(family) (family == AF_INET ? DPS_IP4_TLV_LEN : (family == AF_INET6) ?DPS_IP6_TLV_LEN : DPS_IP4_TLV_LEN)

/*
 *****************************************************************************
 * DPS Client Server Protocol - Utility Functions                        *//**
 *
 * \addtogroup DPSClientServerProtocol
 * @{
 * \defgroup DPSProtocolPacket DPS Client Server Protocol - Packet Handling
 * @{
 *
 * This module handles Packets in the DPS Client Server Protocol
 *
 */

static uint8_t zero_mac[6] = {0x0,0x0,0x0,0x0,0x0,0x0};

uint8_t *dps_alloc_buff(uint32_t len)
{
	uint8_t *buff = NULL;
	if ((buff = (uint8_t *)malloc(DPS_MAX_BUFF_SZ)) != NULL)
	{
		memset(buff, 0, sizeof(dps_client_data_t));
	}
	return (buff);
}

void dps_free_buff(uint8_t *buff)
{
	free(buff);
}

/*
 ******************************************************************************
 * dps_get_int32                                                         *//**
 *
 * \brief - This routine gets 32 bits of integer data.
 *         
 *
 * \param[in] buff - The pointer to the beginning of the integer
 *  
 * \return Returns a 32 bit value.
 *
 ******************************************************************************
 */
static inline uint32_t dps_get_int32(uint8_t *buff)
{
	uint32_t value = 0;
	
	value = ((buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3]);
	return value;
}

/*
 ******************************************************************************
 * dps_set_int32                                                          *//**
 *
 * \brief - This routine sets 32 bits of integer data.
 *         
 *
 * \param[in] buff - The pointer to the beginning of the buffer
 * \param[in] val  - The pointer to the 32 bit value that copied to the buffer
 * 
 * \return 
 *
 ******************************************************************************
 */
static inline void dps_set_int32(uint8_t *buff, uint32_t val) 
{
	*buff++ = (val >> 24) & 0xff;
	*buff++ = (val >> 16) & 0xff;
	*buff++ = (val >> 8) & 0xff;
	*buff++ = (val & 0xff);
}

/*
 ******************************************************************************
 * dps_get_int16                                                          *//**
 *
 * \brief - This routine gets 16 bits of integer data.
 *         
 *
 * \param[in] buff - The pointer to the beginning of the integer
 *  
 * \return Returns a 16 bit value.
 *
 ******************************************************************************
 */
static inline uint16_t dps_get_int16(uint8_t *buff)
{
	uint16_t value = 0;
	
	value = ((buff[0] << 8) | buff[1]);
	return value;
}

/*
 ******************************************************************************
 * dps_set_int16                                                          *//**
 *
 * \brief - This routine sets 16 bits of integer data.
 *         
 *
 * \param[in] buff - The pointer to the beginning of the buffer
 * \param[in] val  - The pointer to the 16 bit value that copied to the buffer
 * 
 * \return 
 *
 ******************************************************************************
 */
static inline void dps_set_int16(uint8_t *buff, uint16_t val) 
{
  *buff++ = (val >> 8) & 0xff;
  *buff++ = (val & 0xff);
}

static uint8_t inline endpoint_info_present(dps_endpoint_info_t *info)
{
	ip_addr_t ipaddr;
	memset(&ipaddr, 0 , sizeof(ipaddr));
	if ((info->vnid == 0) && (!memcmp(info->mac, zero_mac, DPS_MAC_ADDR_LEN)) && !memcmp((uint8_t *)(&info->vm_ip_addr), (uint8_t *)&ipaddr, sizeof(ip_addr_t)))
		return 0;
	else
		return 1;
}

/*
 *****************************************************************************
 * DPS Client Server Protocol - Packet Handling                          *//**
 *
 * \addtogroup DPSClientServerProtocol
 * @{
 * \defgroup DPSProtocolPacket DPS Client Server Protocol - Packet Handling
 * @{
 *
 * This module handles Packets in the DPS Client Server Protocol
 *
 */

/**
 * \brief The Statistics Structure for every packet type
 */
typedef struct dps_pkt_stats_s{
	const char		*packet_name;
	unsigned long int	recv;
	unsigned long int	recv_error;
	unsigned long int	sent;
	unsigned long int	transmit_error;
} dps_pkt_stats_t;

/**
 * \brief The Statistics table for every Packet Type
 *
 * \remarks: This table MUST be kept in sync with dps_client_req_type
 */
static dps_pkt_stats_t dps_pkt_stats_tbl[] = {
	{"Unknown Packet", 0, 0, 0, 0},                                      // NONE
	{"Endpoint Location Request", 0, 0, 0, 0},                           // DPS_ENDPOINT_LOC_REQ
	{"Endpoint Location Reply", 0, 0, 0, 0},                             // DPS_ENDPOINT_LOC_REPLY
	{"Policy Request", 0, 0, 0, 0},                                      // DPS_POLICY_REQ
	{"Policy Reply", 0, 0, 0, 0},                                        // DPS_POLICY_REPLY
	{"Policy Invalidate", 0, 0, 0, 0},                                   // DPS_POLICY_INVALIDATE
	{"Endpoint Update", 0, 0, 0, 0},                                     // DPS_ENDPOINT_UPDATE
	{"Endpoint Ack", 0, 0, 0, 0},                                        // DPS_ENDPOINT_UPDATE_REPLY
	{"Address Resolve", 0, 0, 0, 0},                                     // DPS_ADDR_RESOLVE
	{"Address Reply", 0, 0, 0, 0},                                       // DPS_ADDR_REPLY
	{"Internal Gw Req", 0, 0, 0, 0},                                     // DPS_INTERNAL_GW_REQ
	{"Internal Gw Reply", 0, 0, 0, 0},                                   // DPS_INTERNAL_GW_REPLY
	{"Bulk Policy Transfer", 0, 0, 0, 0},                                // DPS_BULK_DVG_POLICY_XFER
	{"Broadcast Req", 0, 0, 0, 0},                                       // DPS_BCAST_LIST_REQ
	{"Broadcast Reply", 0, 0, 0, 0},                                     // DPS_BCAST_LIST_REPLY
	{"VM Migration Event", 0, 0, 0, 0},                                  // DPS_VM_MIGRATION_EVENT
	{"Mcast Sender Registration", 0, 0, 0, 0},                           // DPS_MCAST_SENDER_REGISTRATION
	{"Mcast Sender DeRegistration", 0, 0, 0, 0},                         // DPS_MCAST_SENDER_DEREGISTRATION
	{"Mcast Receiver Join", 0, 0, 0, 0},                                 // DPS_MCAST_RECEIVER_JOIN
	{"Mcast Receiver Leave", 0, 0, 0, 0},                                // DPS_MCAST_RECEIVER_LEAVE
	{"Mcast Receiver DS List", 0, 0, 0, 0},                              // DPS_MCAST_RECEIVER_DS_LIST
	{"Unsolicited Bcast List Reply", 0, 0, 0, 0},                        // DPS_UNSOLICITED_BCAST_LIST_REPLY
	{"Unsolicited Internal Gw Reply", 0, 0, 0, 0},                       // DPS_UNSOLICITED_INTERNAL_GW_REPLY
	{"General Ack", 0, 0, 0, 0},                                         // DPS_GENERAL_ACK
	{"Unsolicited External Gw List", 0, 0, 0, 0},                        // DPS_UNSOLICITED_EXTERNAL_GW_LIST
	{"Unsolicited Vlan Gw List", 0, 0, 0, 0},                            // DPS_UNSOLICITED_VLAN_GW_LIST
	{"Register Tunnels", 0, 0, 0, 0},                                    // DPS_TUNNEL_REGISTER
	{"Deregister Tunnels", 0, 0, 0, 0},                                  // DPS_TUNNEL_DEREGISTER
	{"Reg/Deregister Tunnel Ack", 0, 0, 0, 0},                           // DPS_TUNNEL_REGISTER_DEREGISTER_ACK
	{"External Gw List Req", 0, 0, 0, 0},                                // DPS_EXTERNAL_GW_LIST_REQ 
	{"External Gw List Reply", 0, 0, 0, 0},                              // DPS_EXTERNAL_GW_LIST_REPLY
	{"Vlan Gw List Req", 0, 0, 0, 0},                                    // DPS_VLAN_GW_LIST_REQ
	{"Vlan Gw List Reply", 0, 0, 0, 0},                                  // DPS_VLAN_GW_LIST_REPLY
	{"Unsolicited Endpoint Location Reply", 0, 0, 0, 0},                 // DPS_UNSOLICITED_ENDPOINT_LOC_REPLY
	{"Bulk VNID Policy Req", 0, 0, 0, 0},                                // DPS_VNID_POLICY_LIST_REQ
	{"Bulk VNID Policy Reply", 0, 0, 0, 0},                              // DPS_VNID_POLICY_LIST_REPLY
	{"Mcast Ctrl Gw Req", 0, 0, 0, 0},                                   // DPS_MCAST_CTRL_GW_REQ
	{"Mcast Ctrl Gw Reply", 0, 0, 0, 0},                                 // DPS_MCAST_CTRL_GW_REPLY
	{"Unsolicited Vnid Del Req", 0, 0, 0, 0},                            // DPS_UNSOLICITED_VNID_DEL_REQ
	{"Control Plane Heart Beat", 0, 0, 0, 0},                            // DPS_CTRL_PLANE_HB
	{"New DCS Node Req",  0, 0, 0, 0},                                   // DPS_GET_DCS_NODE
	{"Unsolicited VM Location Info", 0, 0, 0, 0},                        // DPS_UNSOLICITED_VM_LOC_INFO
};

/*
 ******************************************************************************
 * dps_send_to_protocol_client                                            *//**
 *
 * \brief - This routine sends the Message to be Processed by the Server/Client
 *          side Data Handler
 *
 * \param[in] msg - Pointer to a message (Type dps_client_data_t)
 *
 * \retval DPS_SUCCESS
 *
 ******************************************************************************
 */

static uint32_t dps_send_to_protocol_client(void *msg)
{
#if defined(DPS_SERVER)
	dps_protocol_send_to_server((dps_client_data_t *)msg);
#else
	dps_protocol_send_to_client((dps_client_data_t *)msg);
#endif
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_stop_retransmit_timer                                              *//**
 *
 * \brief - This routine is called when a response arrives for a request. The 
 *          retry timer is stopped and the timer returns the context of the
 *          request. 
 *
 * \param[in] client_buff   Pointer to the client buffer
 *
 * \retval None
 *
 *
 ******************************************************************************
 */
static uint32_t dps_stop_retransmit_timer(dps_client_data_t *client_buff)
{
	uint32_t ret_status = DPS_SUCCESS;
	void *context = NULL;
#if !defined(DPS_SERVER)
	rpt_owner_t owner;
#endif

	do
	{
		dps_log_debug(DpsProtocolLogLevel,"Context: %p", context);
		client_buff->context = NULL;
		if (client_buff->hdr.query_id == 0)
		{
			dps_log_debug(DpsProtocolLogLevel,"No qid");
			break;
		}
#if defined (DPS_SERVER)
		if (retransmit_timer_stop(client_buff->hdr.query_id, &context) != 0)
#else
		if (raw_proto_timer_stop(client_buff->hdr.query_id, &context,
		                         &owner) != RAW_PROTO_TIMER_RETURN_OK)
#endif
		{
			dps_log_debug(DpsProtocolLogLevel,
			              "Stop retransmit timer error pkt_type %d qid %d",
			              client_buff->hdr.type, client_buff->hdr.query_id);
			ret_status = DPS_ERROR;
		}
	} while (0);

	dps_log_debug(DpsProtocolLogLevel,
	              "Context: %p, Qid: %d",
	              context, client_buff->hdr.query_id);
	client_buff->context = context;
	return ret_status;
}

static inline uint32_t 
dps_calc_vip_tlv_len(uint32_t num_of_vip, ip_addr_t *vip)
{
	uint32_t i, j = 0, k = 0, len = 0;
	
	for (i = 0 ; i < num_of_vip; i++)
	{
	    if (vip[i].family == AF_INET)
	       j++;
	    else
	    {
	       if (vip[i].family == AF_INET6)
		  k++;
	       else
		  dps_log_debug(DpsProtocolLogLevel, "Tunnel Endpoint Invalid Family");
	    }
	}
	if (j)
	   len += (DPS_TLV_HDR_LEN + (j * DPS_IP4_ADDR_LEN));
	if (k)
	   len += (DPS_TLV_HDR_LEN + (k * DPS_IP6_ADDR_LEN));
	return len;
}

static inline uint32_t 
dps_calc_tunnel_tlv_len(uint32_t num_of_tunnels, dps_tunnel_endpoint_t *tunnel)
{
	uint32_t i, len = 0;
	
	if (num_of_tunnels)
		len += DPS_TLV_HDR_LEN;
	for (i =0 ; i < num_of_tunnels; i++)
	{
		if (tunnel[i].family == AF_INET)
			len += DPS_IP4_TUNNEL_INFO_LEN;
		else
		{
			if (tunnel[i].family == AF_INET6)
				len += DPS_IP6_TUNNEL_INFO_LEN;
			else
				dps_log_debug(DpsProtocolLogLevel, "Tunnel Endpoint Invalid Family");
		}
	}
	return len;
}

static inline uint32_t 
dps_calc_endpoint_loc_reply_tlv_len(dps_endpoint_loc_reply_t *client_info)
{
	uint32_t len = 0;

	//12 - endpoint_loc_reply_tlv hdr + euid tlv(tlvhdr+vnid+mac+2pad) + 4 version (virtual mac is part of euid for now so not added)
	len = DPS_TLV_HDR_LEN + DPS_EUID_TLV_LEN + 4;
	if ((client_info->vm_ip_addr.family == AF_INET) || (client_info->vm_ip_addr.family == 0))
 		len += DPS_IP4_TLV_LEN;
	else
		len += DPS_IP6_TLV_LEN;
	
	len += dps_calc_tunnel_tlv_len(client_info->tunnel_info.num_of_tunnels, client_info->tunnel_info.tunnel_list);

	return len;
}

static inline uint32_t
dps_calc_endpoint_info_tlv_len(	dps_endpoint_info_t *client_data)
{
	uint32_t len = 0;

	// Destination info TLV
	len += (DPS_TLV_HDR_LEN +  DPS_VNID_LEN + DPS_MAC_ADDR_LEN + 2);
	len += DPS_GET_IP_ADDR_TLV_LEN(client_data->vm_ip_addr.family);
	return len;
}

/*
 ******************************************************************************
 * calc_pkt_len                                                           *//**
 *
 * \brief - This routine is called by all send functions that need to send a
 *          DPS protocol packet. The length is calculated based on the type of
 *          packet and the information it needs to encode.
 *
 * \param[in] pkt_type - The DPS packet type
 * \param[in] req - The contents of the client data which needs to be encoded
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 *
 ******************************************************************************
 */

static uint32_t calc_pkt_len(uint8_t pkt_type, void *req)
{
	uint32_t len = DPS_PKT_HDR_LEN;

	switch (pkt_type)
	{
		case DPS_ENDPOINT_LOC_REQ:
		case DPS_ADDR_RESOLVE:
		{
			dps_endpoint_loc_req_t *msg = DPS_GET_CLIENT_ENDPT_LOC_REQ(req);
			len += DPS_EUID_TLV_LEN;
			if (msg->vm_ip_addr.family == AF_INET)
			{
				len += DPS_IP4_TLV_LEN;
			}
			else if (msg->vm_ip_addr.family == AF_INET6)
			{
				len += DPS_IP6_TLV_LEN;
			}
			if (msg->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (msg->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
			break;
		}
		
		case DPS_ENDPOINT_LOC_REPLY:
		case DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
		{
			dps_endpoint_loc_reply_t *msg = DPS_GET_CLIENT_ENDPT_LOC_REPLY(req);
			
			len += dps_calc_endpoint_loc_reply_tlv_len(msg);
			break;
		}


		case DPS_POLICY_REQ:
		{
			dps_policy_req_t *msg = DPS_GET_CLIENT_POLICY_REQ(req);
		

			//calculate endpoint_req tlv
			// 1. Destination Endpoint TLV
			len += dps_calc_endpoint_info_tlv_len(&msg->dst_endpoint);
			// 2. Source Endpoint TLV			
			len += dps_calc_endpoint_info_tlv_len(&msg->src_endpoint);

			// 3. DPS Client TLV
			if (msg->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (msg->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
			break;
		}

		case DPS_POLICY_REPLY:
		{
			dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(req);
			dps_policy_reply_t *msg =  DPS_GET_CLIENT_POLICY_REPLY(req);
			dps_policy_info_t *plcy_info = &(msg->dps_policy_info);
		
			//Calculate Endpoint Location reply tlv len
			
			len += dps_calc_endpoint_loc_reply_tlv_len(&msg->dst_endpoint_loc_reply);
			if (client_hdr->resp_status == DPS_NO_ERR)
			{ 
				//Calculate Policy
				len += (DPS_TLV_HDR_LEN + DPS_POLICY_ID_TLV_LEN);
				len += DPS_TLV_HDR_LEN; //policy type tlv header
				if (plcy_info->dps_policy.policy_type == DPS_POLICY_TYPE_CONNECTIVITY) 
				{
					len += 4; //4 bytes for ver+type+tot_num_of_rules
				
					len += (((plcy_info->dps_policy.vnid_policy.num_permit_rules + 
					          plcy_info->dps_policy.vnid_policy.num_deny_rules) * 8));
					if (plcy_info->dps_policy.vnid_policy.num_permit_rules)
						len += 4; // 4 bytes for [permit =1 , # of permit rules
					if (plcy_info->dps_policy.vnid_policy.num_deny_rules)
						len += 4; // 4 bytes for [permit =0 , # of deny rules
				}
				else if ( plcy_info->dps_policy.policy_type == DPS_POLICY_TYPE_SOURCE_ROUTING)
				{
					len += 4; //4 bytes for ver+type+ip_number
					if (plcy_info->dps_policy.version == AF_INET) 
					{
						len += (4*plcy_info->dps_policy.source_routing.address_count);
					}
					else
					{
						len += (16*plcy_info->dps_policy.source_routing.address_count);
					}
				}
			}
			break;
		}

		case DPS_ENDPOINT_UPDATE:
		{
			dps_endpoint_update_t *msg = DPS_GET_CLIENT_ENDPT_UPDATE(req);
			len += (DPS_TLV_HDR_LEN + DPS_EUID_TLV_LEN);
			len += DPS_DATA_VER_LEN;

			if ((msg->vm_ip_addr.family == AF_INET) || 
			    (msg->vm_ip_addr.family == 0))
			{
				len += DPS_IP4_TLV_LEN;
			}
			else
			{
				len += DPS_IP6_TLV_LEN;
			}

			len += dps_calc_tunnel_tlv_len(msg->tunnel_info.num_of_tunnels, msg->tunnel_info.tunnel_list);

			if (msg->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (msg->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
			break;
		}

		case DPS_ENDPOINT_UPDATE_REPLY:
		{
			dps_endpoint_update_reply_t *msg = DPS_GET_CLIENT_ENDPT_UPDATE_REPLY(req);
			len += (DPS_TLV_HDR_LEN + DPS_EUID_TLV_LEN + DPS_DATA_VER_LEN);
			len += dps_calc_vip_tlv_len(msg->num_of_vip, msg->vm_ip_addr);

			len += dps_calc_tunnel_tlv_len(msg->tunnel_info.num_of_tunnels, msg->tunnel_info.tunnel_list);

			break;
		}
		case DPS_INTERNAL_GW_REPLY:
		case DPS_UNSOLICITED_INTERNAL_GW_REPLY:
		{
			dps_internal_gw_t *client_data = &((dps_client_data_t *)req)->internal_gw_list;
			if (client_data->num_v4_gw)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->num_v4_gw * DPS_IP4_ADDR_LEN));
			}
			if (client_data->num_v6_gw)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->num_v6_gw * DPS_IP6_ADDR_LEN));
			}
			break;
		}

		case DPS_BCAST_LIST_REQ:
		case DPS_EXTERNAL_GW_LIST_REQ:
		case DPS_VLAN_GW_LIST_REQ:
		case DPS_INTERNAL_GW_REQ:
		case DPS_VNID_POLICY_LIST_REQ:
		case DPS_UNSOLICITED_VNID_DEL_REQ:
		case DPS_MCAST_CTRL_GW_REQ:
		case DPS_CTRL_PLANE_HB:
		{
			dps_gen_msg_req_t *client_data = DPS_GET_CLIENT_GEN_MSG_REQ(req);
			// DPS Client TLV
			if (client_data->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (client_data->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
			break;
		}


		case DPS_BCAST_LIST_REPLY:
		case DPS_UNSOLICITED_BCAST_LIST_REPLY:
		{
			dps_pkd_tunnel_list_t *client_data = &((dps_client_data_t *)req)->dove_switch_list;
			if (client_data->num_v4_tunnels)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->num_v4_tunnels * DPS_IP4_ADDR_LEN));
			}
			if (client_data->num_v6_tunnels)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->num_v6_tunnels * DPS_IP6_ADDR_LEN));
			}
			break;
		}

		case DPS_UNSOLICITED_VNID_POLICY_LIST:
		case DPS_VNID_POLICY_LIST_REPLY:
		{
			dps_bulk_vnid_policy_t *client_data = &((dps_client_data_t *)req)->bulk_vnid_policy;
			len += 4; // number of permit rules and num of deny rules
			// each vnid is 4 bytes svnid,dvnid rule takes up 8 bytes
			len += ((client_data->num_permit_rules + client_data->num_deny_rules) * 8);
			break;
		}
		
		case DPS_VM_MIGRATION_EVENT:
		{
			dps_vm_migration_event_t *client_data = &(((dps_client_data_t *)req)->vm_migration_event);
			len += dps_calc_endpoint_loc_reply_tlv_len(&client_data->src_vm_loc);
			len += dps_calc_endpoint_info_tlv_len(&client_data->migrated_vm_info);
			break;
		}
		case DPS_MCAST_SENDER_REGISTRATION:
		case DPS_MCAST_SENDER_DEREGISTRATION:
		{
			dps_mcast_sender_t *client_data = &(((dps_client_data_t *)req)->mcast_sender);
			len += DPS_GET_IP_ADDR_TLV_LEN(client_data->tunnel_endpoint.family);
			len += dps_calc_endpoint_info_tlv_len(&client_data->mcast_src_vm);

			// mcast_addr len tlv
			len += (DPS_TLV_HDR_LEN + DPS_VMAC_TLV_LEN);
			if (client_data->mcast_addr.mcast_addr_type == MCAST_ADDR_V6)
				len += DPS_IP6_TLV_LEN;
			else
				len += DPS_IP4_TLV_LEN;

			// DPS Client TLV
			if (client_data->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (client_data->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
			break;
		}
	    case DPS_MCAST_RECEIVER_JOIN:
	    case DPS_MCAST_RECEIVER_LEAVE:
	    {
		    dps_mcast_receiver_t *client_data = &(((dps_client_data_t *)req)->mcast_receiver);
		    dps_mcast_group_record_t *grp_rec = &client_data->mcast_group_rec;

		    len += DPS_GET_IP_ADDR_TLV_LEN(client_data->tunnel_endpoint.family);

		    // mcast_grp_rec tlv len calc.
		    len += DPS_TLV_HDR_LEN; // grp rec tlv hdr
		    // mcast_addr_t tlv calc
		    len += (DPS_TLV_HDR_LEN + DPS_VMAC_TLV_LEN);
		    if (grp_rec->mcast_addr.mcast_addr_type == MCAST_ADDR_V6)
			    len += DPS_IP6_TLV_LEN;
		    else 
			    len += DPS_IP4_TLV_LEN;
		    // num of v4 or v6 src list tlv calc
		    if (grp_rec->num_of_srcs)
		    {
			    if (grp_rec->family == AF_INET)
			    {
				    len += (DPS_TLV_HDR_LEN + (grp_rec->num_of_srcs * DPS_IP4_ADDR_LEN));
			    }
			    if (grp_rec->family  == AF_INET6)
			    {
				    len += (DPS_TLV_HDR_LEN + (grp_rec->num_of_srcs * DPS_IP6_ADDR_LEN));
			    }
		    }
		     // DPS Client TLV
			if (client_data->dps_client_addr.family == AF_INET)
			{
				len += DPS_SVCLOC4_TLV_LEN;
			}
			else if (client_data->dps_client_addr.family == AF_INET6)
			{
				len += DPS_SVCLOC6_TLV_LEN;
			}
		    break;
	    }
	    case DPS_UNSOLICITED_EXTERNAL_GW_LIST:
	    case DPS_UNSOLICITED_VLAN_GW_LIST: 
	    case DPS_EXTERNAL_GW_LIST_REPLY:
	    case DPS_VLAN_GW_LIST_REPLY:
	    case DPS_MCAST_CTRL_GW_REPLY:
			
		{
			dps_tunnel_list_t *client_data = &((dps_client_data_t *)req)->tunnel_info;
			//We don't know the size so allocate more then we need, since it is a mix
			//of v4 and v6 addresses
			if (client_data->num_of_tunnels)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->num_of_tunnels * sizeof(dps_tunnel_endpoint_t)));
			}
			break;
		}
	    case DPS_TUNNEL_REGISTER:
	    case DPS_TUNNEL_DEREGISTER:
		{
			dps_tunnel_reg_dereg_t *client_data = &((dps_client_data_t *)req)->tunnel_reg_dereg;
			//We don't know the size so allocate more then we need, since it is a mix
			//of v4 an dv6 addresses
			if (client_data->tunnel_info.num_of_tunnels)
			{
				len += (DPS_TLV_HDR_LEN + (client_data->tunnel_info.num_of_tunnels * sizeof(dps_tunnel_endpoint_t)));
			}
			// Allocate for the largest value
			len += DPS_SVCLOC6_TLV_LEN;
			break;
		}
		default:
			dps_log_info(DpsProtocolLogLevel,"Invalid Msg Type %d", pkt_type);
			break;
	}

	dps_log_debug(DpsProtocolLogLevel,"Msg Type %s Pkt Len: %d", dps_msg_name(pkt_type),len);
	return len;

}



/******************************************************************************
 *                             TLV Construction Functions
 ******************************************************************************/
static inline void dps_set_tlv_hdr(uint8_t *buff, uint8_t ver,
                                   uint8_t data_type, uint16_t len)
{
	dps_tlv_hdr_t *tlv = (dps_tlv_hdr_t *)buff; 
	tlv->s.ver = ver;
	tlv->s.type = data_type;
	tlv->s.len = htons(len);
}

/*
 ******************************************************************************
 * dps_set_pkt_hdr                                                        *//**
 *
 * \brief Sets the pkt hdr.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|  Type |              Length                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Query Id                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         VNID                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Sub Type                |         Resp Status             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Client ID | Transaction Type|         Reserved                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - The buffer contains the pkt that is to be transmitted.
 * \param[in] pkt_type - The pkt type
 * \param[in] client_hdr - Pointer to info passed from the client
 * \param[in] len - The total length of the pkt to be transmitted
 *
 * \retval: The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_pkt_hdr(uint8_t *buff, uint8_t pkt_type,
                                dps_client_data_t *client_data, uint16_t len)
{
	dps_pkt_hdr_t *hdr = (dps_pkt_hdr_t *)buff;
	dps_client_hdr_t *client_hdr = &client_data->hdr;

	hdr->ver = DPS_PROTOCOL_VER;
	hdr->type = (uint8_t)pkt_type;

	//Encode len as 32 bit words, with the header but w/o the hdr tlv

	hdr->len = htons((len - DPS_PKT_HDR_TLV_LEN)/4);

#if defined(DPS_SERVER)
	// The DPS Server Data Handler MUST generate Query ID itself if the
	if ((client_data->context) && (client_hdr->query_id == 0))
	{
		dps_log_emergency(DpsProtocolLogLevel,
		                  "Coding Error: Data Handler setting context but not query ID"
		                  );
	}
#else
	if (client_hdr->query_id == 0)
	{
		// Generate query id only if you need a retry.
		// Dove Switches sends msgs which require retries
		client_hdr->query_id = raw_proto_query_id_generate();
	}
	client_hdr->transaction_type = DPS_TRANSACTION_NORMAL;
#endif
	hdr->vnid = htonl(client_hdr->vnid);
	hdr->sub_type = htons((uint16_t)client_hdr->sub_type);
	hdr->resp_status = htons((uint16_t)client_hdr->resp_status);
	hdr->client_id = client_hdr->client_id;
	hdr->transaction_type = client_hdr->transaction_type;
	hdr->query_id = htonl(client_hdr->query_id);
	return (sizeof(dps_pkt_hdr_t));
}

/*
 ******************************************************************************
 *  dps_set_ip4_tlv                                                       *//**
 *
 * \brief Encode the ip4 address tlv. The first byte of the tlv indicates whether
 *        it is ipv4 or ipv6. The tlv type is IP_ADDR_TLV
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  04   |     IP_ADDR_TLV     |              Length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv4 Address                          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 *
 * \param[in] buff - The tlv is constructed in buff from the info passed in 
 *                   ip4.
 * \param[in] ip4 - The ip4 address.
 *
 * \retval  DPS_IP4_TLV_LEN
 *
 *******************************************************************************
 */

static uint32_t dps_set_ip4_tlv(uint8_t *buff, uint32_t ip4)
{
#if defined (NDEBUG)
	struct in_addr ip_addr;
	ip_addr.s_addr = htonl(ip4);
	dps_log_debug(DpsProtocolLogLevel,"VIP %s", inet_ntoa(ip_addr));
#endif
	dps_set_tlv_hdr(buff,0x04,IP_ADDR_TLV, (DPS_IP4_TLV_LEN-DPS_TLV_HDR_LEN));
	buff += DPS_TLV_HDR_LEN;
	(*(uint32_t *)buff) = htonl(ip4);
	return DPS_IP4_TLV_LEN;
}

/*
 ******************************************************************************
 *  dps_set_ip6_tlv                                                       *//**
 *
 * \brief Encode the ipv6 tlv. The first byte of the tlv indicates whether
 *        it is ipv4 or ipv6. The tlv type is IP_ADDR_TLV.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  06   |     IP_ADDR_TLV     |              Length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      IPv6 Address (16 bytes)                  | 
 *  |                                                               |
 *  |                                                               |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 * \param[in] buff - The tlv is constructed in buff from the info passed in 
 *                   client_euid.
 * \param[in] ip6 - The ip6 address.
 *
 * \retval  DPS_IP6_TLV_LEN
 *
 *******************************************************************************
 */
static uint32_t dps_set_ip6_tlv(uint8_t *buff, uint8_t *ip6)
{
	dps_set_tlv_hdr(buff,0x06,IP_ADDR_TLV, (DPS_IP6_TLV_LEN-DPS_TLV_HDR_LEN));
	buff += DPS_TLV_HDR_LEN;
	memcpy(buff, ip6,DPS_IP6_ADDR_LEN);
	return DPS_IP6_TLV_LEN;
}

/*
 ******************************************************************************
 *  dps_set_vmac_tlv                                                       *//**
 *
 * \brief Encode the mac tlv. The mac tlv consists of 6 bytes of mac addresss
 *        with 2 bytes of padding.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Version   |      VMAC_TLV     |              Length          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      MAC Address                              |   
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      MAC Address              |        Padding                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - The tlv is constructed in buff from the info passed in 
 *                   mac.
 * \param[in] mac - The mac address.
 *
 * \retval  DPS_VMAC_TLV_LEN
 *
 *******************************************************************************
 */
static uint32_t dps_set_vmac_tlv(uint8_t *buff,  uint8_t *mac)
{
	dps_set_tlv_hdr(buff,0x01, VMAC_TLV, (DPS_VMAC_TLV_LEN-DPS_TLV_HDR_LEN));
	buff += DPS_TLV_HDR_LEN;
	memcpy(buff, mac, DPS_MAC_ADDR_LEN);
	return DPS_VMAC_TLV_LEN;
}

static uint32_t dps_set_ip_tlv(uint8_t *buff, ip_addr_t *ip)
{
	uint32_t len = 0;
	if ((ip->family == AF_INET) || (ip->family == 0))
	{
		len = dps_set_ip4_tlv(buff, ip->ip4);
	}
	if (ip->family == AF_INET6)
	{
		len = dps_set_ip6_tlv(buff, ip->ip6);
	}
	return len;
}

/*
 ******************************************************************************
 * dps_set_euid_tlv                                                       *//**
 *
 * \brief Encode the euid tlv. The euid tlv consists of the vnid and the mac
 *        address of the VM.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|     EUID_TLV     |              Length                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         VNID                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         MAC Address                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      MAC Address              |        Padding                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - The tlv is constructed in buff from the info passed in 
 *                   client_euid.
 * \param[in] client_euid - The client euid.
 *
 * \retval DPS_EUID_TLV_LEN
 *
 *******************************************************************************
 */

uint32_t dps_set_euid_tlv(uint8_t *buff, uint32_t *client_euid)
{
	dps_euid_tlv_t *euid = (dps_euid_tlv_t *)buff;

	dps_set_tlv_hdr(buff,0x01, EUID_TLV, (DPS_EUID_TLV_LEN-DPS_TLV_HDR_LEN));
	buff += DPS_TLV_HDR_LEN;	
	euid->vnid = htonl(*client_euid);
	client_euid++;
	memcpy( euid->mac, (uint8_t *)client_euid, DPS_MAC_ADDR_LEN);
	return DPS_EUID_TLV_LEN;
}

/*
 ******************************************************************************
 * dps_set_svcloc_tlv                                                     *//**
 *
 * \brief Encode the service location tlv. The location includes the ipv4 or v6
 *        address followed by the port number
 *      
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |04/06|     SERVICE_LOC_TLV    |              Length            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                 ipv4 (4 bytes) or ipv6 (16 bytes)             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Port Number             |        Padding                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - The tlv is constructed in buff from the info passed in 
 *                   phy_loc.
 * \param[in] phy_loc - The location of the service.
 *
 * \retval DPS_SVCLOC4_TLV_LEN/DPS_SVCLOC6_TLV_LEN
 *
 *******************************************************************************
 */

static uint32_t dps_set_svcloc_tlv(uint8_t *buff, ip_addr_t *phy_loc)
{
	dps_svcloc4_tlv_t *ip4 = NULL;
	dps_svcloc6_tlv_t *ip6 = NULL;
	uint32_t len;

	if (phy_loc->family == AF_INET || phy_loc->family == 0)
	{
		dps_set_tlv_hdr(buff,0x04, SERVICE_LOC_TLV,
		                (DPS_SVCLOC4_TLV_LEN-DPS_TLV_HDR_LEN));
		ip4 = (dps_svcloc4_tlv_t *)buff;
	}
	else
	{
		dps_set_tlv_hdr(buff, 0x06, SERVICE_LOC_TLV,
		                (DPS_SVCLOC6_TLV_LEN-DPS_TLV_HDR_LEN));
		ip6 = (dps_svcloc6_tlv_t *)buff;
	}
	if (ip4 != NULL)
	{
		ip4->ip = htonl(phy_loc->ip4);
		ip4->port = htons(phy_loc->port);
		len = DPS_SVCLOC4_TLV_LEN;
	}
	else
	{
		memcpy(ip6->ip6, phy_loc->ip6,DPS_IP6_ADDR_LEN);
		ip6->port = htons(phy_loc->port);
		len = DPS_SVCLOC6_TLV_LEN;
	}

	return len;
}

/*
 ******************************************************************************
 * dps_set_ipv4_tunnel_info                                               *//**
 *
 * \brief Construct the v4 tunnel information
 *      
 * \param[in] buff - The tunnel information is constructed in buff from the info 
 *                   passed in by client_buff
 * \param[in] client_buff - The location of the tunnel information.
 *
 * \retval  DPS_IP4_TUNNEL_INFO_LEN
 *
 *******************************************************************************
 */
static uint32_t dps_set_ipv4_tunnel_info(uint8_t *buff, dps_tunnel_endpoint_t *client_buff)
{
	(*((uint16_t *)buff)) = htons(4);   //family
	buff += SZ_OF_INT16;
	*((uint16_t *)buff) = htons(client_buff->port);
	buff += SZ_OF_INT16;
	(*((uint16_t *)buff)) = htons(client_buff->tunnel_type);
	buff += SZ_OF_INT16;
	(*((uint16_t *)buff)) = htons(client_buff->flags);	
	buff += SZ_OF_INT16;
	(*((uint32_t *)buff)) = htonl(client_buff->vnid);
	buff += DPS_VNID_LEN;
	*((uint32_t *)buff) = htonl(client_buff->ip4);
	return DPS_IP4_TUNNEL_INFO_LEN;
}

/*
 ******************************************************************************
 * dps_set_ipv6_tunnel_info                                               *//**
 *
 * \brief Construct the v6 tunnel information
 *      
 * \param[in] buff - The tunnel information is constructed in buff from the info 
 *                   passed in by client_buff
 * \param[in] client_buff - The location of the tunnel information.
 *
 * \retval  DPS_IP6_TUNNEL_INFO_LEN
 *
 *******************************************************************************
 */
static uint32_t dps_set_ipv6_tunnel_info(uint8_t *buff, dps_tunnel_endpoint_t *client_buff)
{
	(*((uint16_t *)buff)) = htons(6);   //family
	buff += SZ_OF_INT16;
	*((uint16_t *)buff) = htons(client_buff->port);
	buff += SZ_OF_INT16;
	(*((uint16_t *)buff)) = htons(client_buff->tunnel_type);
	buff += SZ_OF_INT16;
	(*((uint16_t *)buff)) = htons(client_buff->flags);	
	buff += SZ_OF_INT16;
	(*((uint32_t *)buff)) = htonl(client_buff->vnid);
	buff += DPS_VNID_LEN;
	memcpy(buff, client_buff->ip6, DPS_IP6_ADDR_LEN);
	return DPS_IP6_TUNNEL_INFO_LEN;
}

/*
 ******************************************************************************
 * dps_set_tunnel_list_tlv                                                *//**
 *
 * \brief Encode the tunnel list tlv. The tunnel list tlv has information about
 *        the kind of tunnel as well as whether it is a v4 or v6.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|     TUNNEL_LIST_TLV    |              Length          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |              4(ipv4)           |          Port                |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Tunnel Type               |        Flags                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            IPv4 Address                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |              4                 |          Port                |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Tunnel Type               |        Flags                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            IPv4 Address                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               .                               |
 *  |                               .                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |              6 (ipv6)          |          Port                |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Tunnel Type               |        Flags                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            IPv6 Address                       |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |              6                 |          Port                |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |      Tunnel Type               |        Flags                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            IPv6 Address                       |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                               .                               |
 *  |                               .                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - To be filled in after parsing the ipv4 addr list
 * \param[in] client_buff - Points to the beginning of the ipv4 address list with vnid
 * \param[in] num_of_tunnels - Number of tunnels to fill in.
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_tunnel_list_tlv(uint8_t *buff, dps_tunnel_endpoint_t *client_buff, uint16_t num_of_tunnels)
{
	uint8_t *recvbuff; 
	uint16_t i;
	uint32_t len = 0;
	
	recvbuff = buff;
	
	if (num_of_tunnels)
		buff += DPS_TLV_HDR_LEN;
	else
		return len;
	for (i = 0; i < num_of_tunnels; i++)
	{
		if (client_buff[i].family == AF_INET)
			buff += dps_set_ipv4_tunnel_info(buff, &client_buff[i]); 
		else 
		{
			if (client_buff[i].family == AF_INET6)
				buff += dps_set_ipv6_tunnel_info(buff, &client_buff[i]); 
			else
				dps_log_debug(DpsProtocolLogLevel, "Tunnel Endpoint Invalid Family");
		}
	}
	len = buff - recvbuff;
	dps_set_tlv_hdr(recvbuff, 1, TUNNEL_LIST_TLV, len - DPS_TLV_HDR_LEN);
	return (len);
	
}

/*
 ******************************************************************************
 * dps_set_ipv4_list_tlv                                                   *//**
 *
 * \brief Construct a list of ipv4 addresses.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|  IPV4_ADDR_LIST_TLV    |          Length              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv4 address                          |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv4 address                          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             .                                 |
 *  |                             .                                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - To be filled in after parsing the ipv4 addr list
 * \param[in] client_buff - Points to the beginning of the ipv4 address list
 * \param[in] num_v4_addr - Number of v4 addresses to fill in.
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_ipv4_list_tlv(uint8_t *buff, uint32_t *client_buff, uint32_t num_v4_addr)
{
	uint8_t *recvbuf; 
	uint32_t i;

	recvbuf = buff;

	dps_set_tlv_hdr(buff, 1, IPV4_ADDR_LIST_TLV, (num_v4_addr * DPS_IP4_ADDR_LEN));
	buff += DPS_TLV_HDR_LEN;
	
	for (i = 0; i < num_v4_addr; i++)
	{
		(*((uint32_t *)buff)) = htonl(client_buff[i]);
		buff += DPS_IP4_ADDR_LEN;
	}
	
	return (buff - recvbuf);
	
}

/*
 ******************************************************************************
 * dps_set_ipv6_list_tlv                                                   *//**
 *
 * \brief Construct a list of ipv6 addresses.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|  IPV6_ADDR_LIST_TLV    |          Length              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv6 address(16 bytes)                |  
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv6 address(16 bytes)                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             .                                 |
 *  |                             .                                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - To be filled in after parsing the ipv6 addr list
 * \param[in] client_buff - Points to the beginning of the ipv6 address list
 * \param[in] num_v6_addr - Number of v6 addresses to fill in.
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_ipv6_list_tlv(uint8_t *buff, uint8_t *client_buff, uint32_t num_v6_addr)
{
	uint8_t *recvbuf; 
	uint32_t i;

	recvbuf = buff;

	dps_set_tlv_hdr(buff, 1, IPV6_ADDR_LIST_TLV, (num_v6_addr * DPS_IP6_ADDR_LEN));
	buff += DPS_TLV_HDR_LEN;
	for (i = 0; i < num_v6_addr; i++)
	{
		memcpy(buff, client_buff, DPS_IP6_ADDR_LEN);
		client_buff += DPS_IP6_ADDR_LEN;
		buff += DPS_IP6_ADDR_LEN;
	}
	return (buff - recvbuf);
	
}
/*
 ******************************************************************************
 *  dps_set_ip4_info_tlv                                                   *//**
 *
 * \brief Encode a list of ip4 addresses with flags and reserved info..
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|IPV4_INFO_LIST_TLV|              Length                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv4                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Flags               |      Reserved                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv4                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Flags               |      Reserved                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             .                                 |
 *  |                             .                                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *
 * \param[in] buff - To be filled in after parsing the ipv4 info list
 * \param[in] client_buff - Points to the beginning of the ipv4 info list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_ip4_info_tlv(uint8_t *buff, dps_ip4_info_list_t *client_buff)
{
	uint8_t *buff_start; 
	uint32_t i;
	dps_ip4_info_t *pkt_buff;

	buff_start = buff;

	dps_set_tlv_hdr(buff, 1, IP4_INFO_LIST_TLV, (client_buff->num_of_ips * sizeof(dps_ip4_info_t)));
	buff += DPS_TLV_HDR_LEN;
	pkt_buff = (dps_ip4_info_t *)buff;
	for (i = 0; i < client_buff->num_of_ips; i++)
	{
		pkt_buff->ip = htonl(client_buff->ip_info[i].ip);
		pkt_buff->flags = htons(client_buff->ip_info[i].flags);
		pkt_buff->reserved = htons(client_buff->ip_info[i].reserved);
		pkt_buff++;
	}	
	return ((uint8_t *)pkt_buff - buff_start);
	
}

/*
 ******************************************************************************
 * dps_set_ip6_info_tlv                                                   *//**
 *
 * \brief Encode a list of ip4 addresses with flags and reserved info..
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|IPV6_INFO_LIST_TLV|              Length                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv6 (16 bytes)                       |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Flags               |      Reserved                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IPv6 (16 bytes)                       |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Flags               |      Reserved                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             .                                 |
 *  |                             .                                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *
 * \param[in] buff - To be filled in after parsing the ipv6 info list
 * \param[in] client_buff - Points to the beginning of the ipv6 info list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_ip6_info_tlv(uint8_t *buff, dps_ip6_info_list_t  *client_buff)
{
	uint8_t *buff_start; 
	uint32_t i;
	dps_ip6_info_t *pkt_buff;

	buff_start = buff;

	dps_set_tlv_hdr(buff, 1, IP6_INFO_LIST_TLV, (client_buff->num_of_ips * sizeof(dps_ip6_info_t)));
	buff += DPS_TLV_HDR_LEN;
	pkt_buff = (dps_ip6_info_t *)buff;
	for (i = 0; i < client_buff->num_of_ips; i++)
	{
		memcpy(pkt_buff->ip, client_buff->ip_info[i].ip, DPS_IP6_ADDR_LEN);
		pkt_buff->flags = htons(client_buff->ip_info[i].flags);
		pkt_buff->reserved = htons(client_buff->ip_info[i].reserved);
		pkt_buff++;
	}
	
	return ((uint8_t *)pkt_buff - buff_start);
}

/*
 ******************************************************************************
 * dps_set_endpoint_info_tlv                                              *//**
 *
 * \brief Encodes information about the VM. The  ENDPOINT_INFO_TLV contains
 *        a sub-tlv of type IP_ADDR_TLV. The Length field of the ENDPOINT_INFO_TLV
 *        includes the length of the sub-tlv.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|ENDPOINT_INFO_TLV|              Length                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         VNID                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        MAC Address                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         MAC Address         |     Padding                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | 04/06 |     IP_ADDR_TLV     |              Length             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                IPv4 (4 bytes)/v6 Address (16 bytes)           | 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *
 * \param[in] buff - To be filled in after parsing the ipv6 info list
 * \param[in] client_buff - Points to the beginning of the ipv6 info list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */

static uint32_t dps_set_endpoint_info_tlv(uint8_t *buff, dps_endpoint_info_t *client_buff)
{
	dps_endpoint_info_tlv_t *endpoint = (dps_endpoint_info_tlv_t *)buff;
	uint8_t *bufptr = buff;

	buff += DPS_TLV_HDR_LEN;
	endpoint->vnid = htonl(client_buff->vnid);
	buff += DPS_VNID_LEN;
	memcpy(endpoint->mac, client_buff->mac, DPS_MAC_ADDR_LEN);
	buff += DPS_MAC_ADDR_LEN + 2;
	if (client_buff->vm_ip_addr.family == AF_INET6)
	{
		dps_set_tlv_hdr(bufptr, 0x01, ENDPOINT_INFO_TLV, DPS_ENDPOINT6_INFO_TLV_LEN - DPS_TLV_HDR_LEN);
		buff += dps_set_ip6_tlv(buff, client_buff->vm_ip_addr.ip6);
	}	
	else
	{
		dps_set_tlv_hdr(bufptr, 0x01, ENDPOINT_INFO_TLV, DPS_ENDPOINT4_INFO_TLV_LEN - DPS_TLV_HDR_LEN);
		buff += dps_set_ip4_tlv(buff, client_buff->vm_ip_addr.ip4);
	}
	return (buff - bufptr);
}

/*
 ******************************************************************************
 * dps_set_policy_type_tlv                                                *//**
 *
 * \brief Encodes policy information. The Policy Type is CONNECTIVITY.
 *        CONNECTIVITY policy consists of a set of permit and deny rules as
 *        encoded below.  
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|POLICY_ID_TLV|              Length                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Policy Id                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Version                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         TTL                                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - To be filled in after parsing the policy id inforamtion
 * \param[in] id        - Policy id number
 * \param[in] version   - Policy version
 * \param[in] ttl       - How long the policy is valid.
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */

static uint32_t dps_set_policy_id_tlv(uint8_t *buff, uint32_t id, uint32_t version, uint32_t ttl)
{
	dps_policy_id_tlv_t *policyid = (dps_policy_id_tlv_t *)buff;

	dps_set_tlv_hdr(buff, 0x01, POLICY_ID_TLV, (DPS_POLICY_ID_TLV_LEN-DPS_TLV_HDR_LEN));
	policyid->policy_id = htonl(id);
	policyid->version = htonl(version);
	policyid->ttl = htonl(ttl);
	return DPS_POLICY_ID_TLV_LEN;
}

/*
 ******************************************************************************
 * dps_set_policy_type_tlv                                                *//**
 *
 * \brief Encodes policy information. The Policy Type is CONNECTIVITY.
 *        CONNECTIVITY policy consists of a set of permit and deny rules as
 *        encoded below.  
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|POLICY_TYPE_TLV|              Length                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|Policy Type    |     Total Num of Rules                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Permit=1 Rules     |     Number of Permit Rules            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Source VNID                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dest VNID                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Source VNID                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dest VNID                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            .                                  |
 *  |                            .                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Deny=0 Rules     |     Number of Deny Rules                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Source VNID                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dest VNID                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Source VNID                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dest VNID                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            .                                  |
 *  |                            .                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - To be filled in after parsing the ipv6 info list
 * \param[in] client_buff - Points to the policy inforamtion
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_policy_type_tlv(uint8_t *buff, dps_policy_t *client_info)
{
	uint16_t i, j = 0,len = 0;
	uint8_t *data;
	dps_policy_type_tlv_t *policytype = (dps_policy_type_tlv_t *)buff;

	len = sizeof(dps_policy_type_tlv_t);
	policytype->ver = client_info->version;
	policytype->policy_type = client_info->policy_type;

	dps_log_debug(DpsProtocolLogLevel, "Policy type %d", policytype->policy_type);
	if (client_info->policy_type == DPS_POLICY_TYPE_CONNECTIVITY)
	{
		policytype->total_num_of_rules = htons(client_info->vnid_policy.num_permit_rules + client_info->vnid_policy.num_deny_rules);
		dps_log_debug(DpsProtocolLogLevel, "Policy tot_num_of_rules %d", policytype->total_num_of_rules);
		data = policytype->data;
		if (client_info->vnid_policy.num_permit_rules)
		{
			(*((uint16_t *)data)) = htons(1);  //Permit =1
			data += SZ_OF_INT16;
			(*((uint16_t *)data)) =  htons(client_info->vnid_policy.num_permit_rules);
			data += SZ_OF_INT16;
			len += 4;
			for (i = 0; i < client_info->vnid_policy.num_permit_rules; i++)
			{
				(*((uint32_t *)data)) = htonl(client_info->vnid_policy.src_dst_vnid[j].svnid);
				data += SZ_OF_INT32;
				(*((uint32_t *)data)) = htonl(client_info->vnid_policy.src_dst_vnid[j].dvnid);
				data += SZ_OF_INT32;
				j++;
				len += 8;
			}
		}
		if (client_info->vnid_policy.num_deny_rules)
		{
			(*((uint16_t *)data)) = htons(0);  // Deny =0
			data += SZ_OF_INT16;
			(*((uint16_t *)data)) = htons(client_info->vnid_policy.num_deny_rules);
			data += SZ_OF_INT16;
			len += 4;
			for (i = 0; i < client_info->vnid_policy.num_deny_rules; i++)
			{
				(*((uint32_t *)data)) = htonl(client_info->vnid_policy.src_dst_vnid[j].svnid);
				data += SZ_OF_INT32;
				(*((uint32_t *)data)) = htonl(client_info->vnid_policy.src_dst_vnid[j].dvnid);
				data += SZ_OF_INT32;
				j++;
				len += 8;
			}			
		}
		
	}
	dps_set_tlv_hdr((uint8_t *)&policytype->tlv, 0x01, POLICY_TYPE_TLV, len-DPS_TLV_HDR_LEN);
	dps_log_debug(DpsProtocolLogLevel, "Dump Policy Info");
	dump_tlv_info(buff, len);
	return len;
}

/*
 ******************************************************************************
 * dps_set_policy_tlv                                                     *//**
 *
 * \brief The POLICY_TLV consists of 2 sub-tlvs:POLICY_ID_TLV and POLICY_TYPE_TLV.
 *        The POLICY_ID_TLV identifies the policy. The POLICY_TYPE_TLV contains
 *        inforamtion about the policy. 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|POLICY_TLV     |              Length                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|POLICY_ID_TLV  |              Length                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|POLICY_TYPE_TLV|              Length                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - To be filled in after parsing the policy
 * \param[in] policy_info - Information about the policy
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_set_policy_tlv(uint8_t *buff, dps_policy_info_t *policy_info)
{

	uint32_t len = 0;
	uint8_t *tmpbuff;
	
	tmpbuff = buff;
	// Make space for policy structure tlv 
	buff += DPS_TLV_HDR_LEN;

	buff += dps_set_policy_id_tlv(buff, policy_info->policy_id, policy_info->version, policy_info->ttl);

	buff += dps_set_policy_type_tlv(buff, &policy_info->dps_policy);

	len = buff - tmpbuff;
    //len is not known before so set it at this time
	dps_set_tlv_hdr(tmpbuff, 0x01, POLICY_TLV, len-DPS_TLV_HDR_LEN);
	return (len);
}

/*
 ******************************************************************************
 * dps_set_endpoint_loc_reply_tlv                                         *//**
 *
 * \brief The ENDPOINT_LOC_TLV consists of Data Version field of 4 bytes and 3
 *        sub-tlvs as shown below. The TLV contains all the inforamtion needed
 *        to reach an endpoint.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|ENDPOINT_LOC_TLV|              Length                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Data Version                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       EUID_TLV                                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       IP_ADDR_TLV                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       TUNNEL_LIST_TLV                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] buff - To be filled in after parsing the policy
 * \param[in] client_data - Information about the endpoint location
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t
dps_set_endpoint_loc_reply_tlv(uint8_t *buff,
                               dps_endpoint_loc_reply_t *client_data)
{
	uint32_t len = 0;
	uint8_t *tmpbuff = buff;

    // Make space for tlv hdr 
	buff += DPS_TLV_HDR_LEN;
	// Set version in the beginning easier to process the rest of the tlvs
	*((uint32_t *)buff) = htonl(client_data->version);
	buff += DPS_DATA_VER_LEN;

	buff += dps_set_euid_tlv(buff, &client_data->vnid);

	if ((client_data->vm_ip_addr.family == AF_INET) || 
	   (client_data->vm_ip_addr.family == 0))
	{
		buff += dps_set_ip4_tlv(buff,client_data->vm_ip_addr.ip4);
	}
	else if (client_data->vm_ip_addr.family == AF_INET6)
	{
		buff += dps_set_ip6_tlv(buff, client_data->vm_ip_addr.ip6);
	}

	// encode tunnel endpoints
	buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_info.tunnel_list, client_data->tunnel_info.num_of_tunnels);

	len = buff - tmpbuff;

	dps_set_tlv_hdr(tmpbuff, 1, ENDPOINT_LOC_TLV, len - DPS_TLV_HDR_LEN);

	return (len); 
}

/*
 ******************************************************************************
 * dps_set_policy_reply_tlv                                               *//**
 *
 * \brief The policy reply msg consists of ENDPOINT_LOC_TLV and POLICY_TLV.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       ENDPOINT_LOC_TLV                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       POLICY_TLV                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - To be filled in after parsing the policy
 * \param[in] client_data - Information about the endpoint location
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t
dps_set_policy_reply_tlv(uint8_t *buff,
                         dps_policy_reply_t *client_data,
                         dps_client_hdr_t *client_hdr,
                         uint32_t len)
{
	dps_endpoint_loc_reply_t *endpoint_loc_reply = &client_data->dst_endpoint_loc_reply;
	dps_policy_info_t *policy_info = &client_data->dps_policy_info;
	uint8_t *tmpbuff = buff;

	buff += dps_set_endpoint_loc_reply_tlv(buff, endpoint_loc_reply);

	// set policy struct tlv
	if (client_hdr->resp_status == DPS_NO_ERR)
		buff += dps_set_policy_tlv(buff, policy_info);

	dps_log_debug(DpsProtocolLogLevel,"Policy Reply Buff Len %d", buff - tmpbuff);
	return (buff - tmpbuff); 
}

/*
 ******************************************************************************
 * dps_set_endpoint_update_tlv                                            *//**
 *
 * \brief The endpoint_update_tlv function uses the endpoint_loc_tlv to encode
 *        the endpoint information as shown below.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|ENDPOINT_LOC_TLV|              Length                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Data Version                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       EUID_TLV                                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       IP_ADDR_TLV                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       TUNNEL_LIST_TLV                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] buff - To be filled in after parsing the policy
 * \param[in] client_data - Information about the endpoint location
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t
dps_set_endpoint_update_tlv(uint8_t *buff,
                            dps_endpoint_update_t *client_data,
                            uint32_t buff_len)
{
	uint8_t *tmp_buff = buff;
	uint32_t len = 0;

	dps_log_debug(DpsProtocolLogLevel,"endpoint_update_tlv avail buff len %d",buff_len);

	buff += DPS_TLV_HDR_LEN;

	*((uint32_t *)buff) = htonl(client_data->version);

	buff += DPS_DATA_VER_LEN;

	buff += dps_set_euid_tlv(buff, &client_data->vnid);

	if ((client_data->vm_ip_addr.family == AF_INET) || 
	   (client_data->vm_ip_addr.family == 0))
	{
		buff += dps_set_ip4_tlv(buff,client_data->vm_ip_addr.ip4);
	}
	else if (client_data->vm_ip_addr.family == AF_INET6)
	{
		buff += dps_set_ip6_tlv(buff, client_data->vm_ip_addr.ip6);
	}
	buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_info.tunnel_list, client_data->tunnel_info.num_of_tunnels);

	len = buff - tmp_buff;

	dps_log_debug(DpsProtocolLogLevel,"endpoint_update_tlv: buff_len %d tlv_len %d",
	              buff_len, len);
	dps_set_tlv_hdr(tmp_buff, 1, ENDPOINT_LOC_TLV, len - DPS_TLV_HDR_LEN);
	return (len);
}

/*
 ******************************************************************************
 * dps_set_endpoint_update_reply_tlv                                      *//**
 *
 * \brief The endpoint_update_reply_tlv has a list of sub-tlvs as shown below.
 *        The VMs can have a combination of ipv4 or ipv6 addresses, based on
 *        the presence of the vips the IPV4_ADDR_LIST_TLV or IPV6_ADDR_LIST_TLV
 *        may or may not be present. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|ENDPOINT_UPDATE_REPLY_TLV|              Length         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Data Version                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       EUID_TLV                                |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       IPV4_ADDR_LIST_TLV (optional)           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       IPV6_ADDR_LIST_TLV (optional)           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       TUNNEL_LIST_TLV                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] buff - To be filled in after parsing the policy
 * \param[in] client_data - Information about the endpoint location
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t
dps_set_endpoint_update_reply_tlv(uint8_t *buff,
                                  dps_endpoint_update_reply_t *client_data,
                                  uint32_t len)
{

	uint8_t *start_buff = buff;
	uint32_t vip4_buff[MAX_VIP_ADDR];
	uint32_t vip6_buff[MAX_VIP_ADDR * 4];
	uint32_t i, buff_len, j=0,k=0;
	uint32_t *vip6 = vip6_buff;

	dps_log_debug(DpsProtocolLogLevel, "endpoint_update_reply_tlv buff len %d",len);

	buff += DPS_TLV_HDR_LEN;

	*((uint32_t *)buff) = htonl(client_data->version);

	buff += DPS_DATA_VER_LEN;

	buff += dps_set_euid_tlv(buff, &client_data->vnid);

	//Construct a list of vip4 addresses followed by ipv6

	for (i=0; i < client_data->num_of_vip; i++)
	{
	  if (client_data->vm_ip_addr[i].family == AF_INET) 
	  {
	      vip4_buff[j++] = client_data->vm_ip_addr[i].ip4;
	      
	  }
	  else if (client_data->vm_ip_addr[i].family == AF_INET6)
	  {
	    memcpy((uint8_t *)vip6, client_data->vm_ip_addr[i].ip6, DPS_IP6_ADDR_LEN);
	      k++;
	      vip6 += 4; 
	  }
	}
	if (j)
	  buff += dps_set_ipv4_list_tlv(buff,vip4_buff, j);
	if (k)
	  buff += dps_set_ipv6_list_tlv(buff, (uint8_t *)vip6_buff, k);
	

	buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_info.tunnel_list, client_data->tunnel_info.num_of_tunnels);
	
	buff_len = buff - start_buff;
	dps_log_debug(DpsProtocolLogLevel,"endpoint_update_reply_tlv: buff_len %d cal_tlv_len %d",
	              buff_len, len);
	dps_set_tlv_hdr(start_buff, 1, ENDPOINT_UPDATE_REPLY_TLV, buff_len - DPS_TLV_HDR_LEN);
	return (buff_len); 
}


/*
 ******************************************************************************
 * dps_set_bulk_policy_xfer                                             *//**
 *
 * \brief This function creates a msg which includes all dvg policies and is sent
 *        by the DPS Server. This message is only used for connectivity policy.
 *
 * \param[in] client_dvg_policy - A pointer to the dvg policy information
 * \param[in] len - Used for vlaidation. The amount of memory available to set 
 *                  dvg policies.
 *
 * \retval - The length of the buffer 
 *
 ******************************************************************************
 */

static uint32_t
dps_set_bulk_policy_xfer(uint8_t *buff,  dps_bulk_vnid_policy_t *client_vnid_policy, uint32_t len)
{
	uint8_t *bufptr = buff;
	uint32_t i,j;

	(*((uint16_t *)buff)) = htons(client_vnid_policy->num_permit_rules);
	buff += SZ_OF_INT16;
	(*((uint16_t *)buff)) =  htons(client_vnid_policy->num_deny_rules);
	buff += SZ_OF_INT16;
	for (i = 0, j = 0; i < client_vnid_policy->num_permit_rules; i++, j++)
	{
		(*((uint32_t *)buff)) = htonl(client_vnid_policy->src_dst_vnid[j].svnid);
		buff += SZ_OF_INT32;
		(*((uint32_t *)buff)) = htonl(client_vnid_policy->src_dst_vnid[j].dvnid);
		buff += SZ_OF_INT32;
	}
	for (i = 0; i < client_vnid_policy->num_deny_rules; i++, j++)
	{
		(*((uint32_t *)buff)) = htonl(client_vnid_policy->src_dst_vnid[j].svnid);
		buff += SZ_OF_INT32;
		(*((uint32_t *)buff)) = htonl(client_vnid_policy->src_dst_vnid[j].dvnid);
		buff += SZ_OF_INT32;
	}
	if (((uint32_t)(buff - bufptr)) != len)
		dps_log_error(DpsProtocolLogLevel,"dps_set_bulk_policy_xfer passed len %d set len %d",len, buff-bufptr);
	return((uint32_t)(buff - bufptr));
}

/*
 ******************************************************************************
 * dps_set_mcast_addr_tlv                                                 *//**
 *
 * \brief Processing of mcast receiver tlv
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|MCAST_ADDR_MAC_TLV/MCAST_ADDR_V4_TLV/MCAST_ADDR_V6_TLV|Length|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         VMAC_TLV                                    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |          IP_ADDR_TLV (v4 or v6 in the ver field of tlv header)      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] buff - A pointer to a buffer received from the network that has
 *                   encoded list of mcast grp re chis msg is sent when a
 *                   receiver wants to join a mcast group. The ipv4/ipv6
 *                   addr list tlv is for source specific joins.
 * \param[out] client_data - Parse the buff and set it in client_data.
 *
 * \retval The amount of bytes processed in the buff
 * 
 *
 ******************************************************************************
 */
static uint32_t
dps_set_mcast_addr_tlv(uint8_t *buff, dps_mcast_addr_t *client_data)
{
	uint32_t len = 0;
	uint8_t tlv_type;
	uint8_t *buff_start = buff;

	// The mcast_addr_tlv contains the mac tlv followed by ip4/ipv6 tlv
    // Make space for mcast addr tlv hdr 
	buff += DPS_TLV_HDR_LEN;

	buff += dps_set_vmac_tlv(buff, client_data->mcast_mac);

	// If mcast_addr_type is mac only then encode the ip address as v4 with
	// ip value of all 0s

	if (client_data->mcast_addr_type == MCAST_ADDR_V6)
	{
		tlv_type = MCAST_ADDR_V6_TLV;
		buff += dps_set_ip6_tlv(buff, client_data->u.mcast_ip6);
	}
	else
	{
		if (client_data->mcast_addr_type == MCAST_ADDR_MAC)
			tlv_type = MCAST_ADDR_MAC_TLV;
		else 
		{
			if (client_data->mcast_addr_type == MCAST_ADDR_V4)
				tlv_type = MCAST_ADDR_V4_TLV;
			else
				tlv_type = MCAST_ADDR_V4_ICB_RANGE_TLV;
		}
		buff += dps_set_ip4_tlv(buff, client_data->u.mcast_ip4);
	}

	len = buff - buff_start;
	
	dps_set_tlv_hdr(buff_start, 1, tlv_type, len - DPS_TLV_HDR_LEN);

	dps_log_debug(DpsProtocolLogLevel,"Mcast Addr TLV");
	dump_tlv_info(buff_start,len);
	return (len); 
}

/*
 ******************************************************************************
 * dps_set_mcast_grp_rec_tlv                                              *//**
 *
 * \brief Processing of mcast receiver tlv
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|MCAST_GRP_REC_TLV|              Length                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         MCAST_ADDR_TLV                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV4_ADDR_LIST_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV6_ADDR_LIST_TLV                     | 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - A pointer to a buffer received from the network that has
 *                   encoded list of mcast grp rec This msg is sent when a
 *                   receiver wants to join a mcast group. The ipv4/ipv6
 *                   addr list tlv is for source specific joins.
 * \param[out] client_data - Parse the buff and set it in client_data.
 *
 * \retval The amount of bytes processed in the buff
 * 
 *
 ******************************************************************************
 */
static uint32_t
dps_set_mcast_grp_rec_tlv(uint8_t *buff, dps_mcast_group_record_t *client_data)
{
	uint32_t len = 0;
	uint8_t *tmpbuff;
	
	tmpbuff = buff;
	// Make space for grp rec tlv hdr
	buff += DPS_TLV_HDR_LEN;

	buff += dps_set_mcast_addr_tlv(buff, &client_data->mcast_addr);
	if (client_data->num_of_srcs)
	{
		if (client_data->family == AF_INET)
		{
			buff +=  dps_set_ipv4_list_tlv(buff, client_data->src_addr, client_data->num_of_srcs);
		}
		else if (client_data->family == AF_INET6)
		{
			buff += dps_set_ipv6_list_tlv(buff, (uint8_t *)client_data->src_addr, client_data->num_of_srcs);
		}
	}

	len = buff - tmpbuff;
    //len is not known before so set it at this time
	dps_set_tlv_hdr(tmpbuff, 0x01, MCAST_GRP_REC_TLV, len-DPS_TLV_HDR_LEN);
	return (len);
}

/*
 ******************************************************************************
 * dps_set_vnid_tunnel_list_tlv                                           *//**
 *
 * \brief Processing of mcast receiver tlv
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|VNID_TUNNEL_LIST_TLV|              Length              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Vnid                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV4_ADDR_LIST_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV6_ADDR_LIST_TLV                     | 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - A pointer to a buffer received from the network that has
 *                   encoded list of mcast receiver list.
 * \param[out] client_data - Parse the buff and set it in client_data.
 *
 * \retval The amount of bytes processed in the buff
 * 
 *
 ******************************************************************************
 */
static uint32_t
dps_set_vnid_tunnel_list_tlv(uint8_t *buff, dps_pkd_tunnel_list_t *client_data)
{
	uint32_t len = 0;
	uint8_t *buff_start;
	uint32_t *tunnels;

	buff_start = buff;
	
	buff += DPS_TLV_HDR_LEN;

	*((uint32_t *)buff) = htonl(client_data->vnid);

	buff += DPS_VNID_LEN;

	tunnels = client_data->tunnel_list;
	if (client_data->num_v4_tunnels)
	{
		buff += dps_set_ipv4_list_tlv(buff, tunnels, client_data->num_v4_tunnels);
		//Advance tunnel list by the number of v4
		tunnels += client_data->num_v4_tunnels;
	}
	if (client_data->num_v6_tunnels)
	{
		buff += dps_set_ipv6_list_tlv(buff, (uint8_t *)tunnels, client_data->num_v6_tunnels);
	}

	len = buff - buff_start;
	dps_log_debug(DpsProtocolLogLevel,"Vnid Tunnel List TLV %d len", len);

    //len is not known before so set it at this time
	dps_set_tlv_hdr(buff_start, 0x01, VNID_TUNNEL_LIST_TLV, len-DPS_TLV_HDR_LEN);

	dump_tlv_info(buff_start,len);
	return (len);
}

/*
 ******************************************************************************
 * dps_set_epri_tlv                                                       *//**
 *
 * \brief Processing of mcast receiver tlv
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|EPRI_TLV|              Length                          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         EUID_TLV                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV4_INFO_LIST_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV6_INFO_LIST_TLV                     | 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        TUNNEL_LIST_TLV                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] buff - A pointer to a buffer received from the network that has
 *                   encoded list of mcast receiver list.
 * \param[out] client_data - Parse the buff and set it in client_data.
 *
 * \retval The amount of bytes processed in the buff
 * 
 *
 ******************************************************************************
 */
static uint32_t
dps_set_epri_tlv(uint8_t *buff, dps_epri_t *client_data)
{
	uint32_t len = 0;
	uint8_t *buff_start = buff;

    // Make space for epri tlv hdr 
	buff += DPS_TLV_HDR_LEN;

	buff += dps_set_euid_tlv(buff, &client_data->vnid);

	buff += dps_set_ip4_info_tlv(buff, &client_data->vm_ip4_addr);

	buff += dps_set_ip6_info_tlv(buff, &client_data->vm_ip6_addr);
	// encode tunnel endpoints
	buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_info.tunnel_list, client_data->tunnel_info.num_of_tunnels);

	len = buff - buff_start;

	dps_set_tlv_hdr(buff_start, 1, EPRI_TLV, len - DPS_TLV_HDR_LEN);

	return (len); 
}

/*
 ******************************************************************************
 * dps_get_pkt_hdr                                                        *//**
 *
 * \brief - This routine gets the contents of the packet hdr and returns the
 *          length of the data.
 *
 * \param[in] pkt_hdr - The pointer to the beginning of the pkt hdr
 * \param[in] client_hdr - The destination to assign the information parsed
 *                         from the header
 *
 * \return Length of the data
 *
 ******************************************************************************
 */

uint32_t dps_get_pkt_hdr(dps_pkt_hdr_t *pkt_hdr, dps_client_hdr_t *client_hdr)
{
	client_hdr->type = pkt_hdr->type;
	client_hdr->vnid = ntohl(pkt_hdr->vnid);
	client_hdr->query_id = ntohl(pkt_hdr->query_id);
	client_hdr->resp_status = (uint32_t)ntohs(pkt_hdr->resp_status);
	client_hdr->sub_type = (uint8_t)ntohs(pkt_hdr->sub_type);
	client_hdr->client_id = pkt_hdr->client_id;
	client_hdr->transaction_type = pkt_hdr->transaction_type;
	// the len of the packet does not include hdr tlv len(4) but the 
	// DPS_PKT_HDR_LEN(20) includes the 4 bytes of tlv len.
	return ((ntohs(pkt_hdr->len)<<2) - DPS_PKT_HDR_LEN + DPS_PKT_HDR_TLV_LEN);
}

static uint32_t dps_get_tlv_hdr(uint8_t *buff, dps_tlv_hdr_t *client_tlv)
{
	dps_tlv_hdr_t *tlv = (dps_tlv_hdr_t *)buff;

 	client_tlv->s.ver = tlv->s.ver;
	client_tlv->s.type = tlv->s.type;
	client_tlv->s.len = ntohs(tlv->s.len);
	return DPS_TLV_HDR_LEN;
}

static uint32_t dps_get_vmac_tlv(uint8_t *buff, uint8_t *vmac)
{
	dps_vmac_tlv_t *vmac_tlv =  (dps_vmac_tlv_t *)buff;
	memcpy((uint8_t *)vmac, vmac_tlv->mac, DPS_MAC_ADDR_LEN);
	return DPS_VMAC_TLV_LEN;
}

static uint32_t dps_get_euid_tlv(uint8_t *buff, uint32_t *client_euid)
{
	dps_euid_tlv_t *euid = (dps_euid_tlv_t *)buff;
	*(client_euid++) = ntohl(euid->vnid);
	memcpy((uint8_t *)client_euid, euid->mac, DPS_MAC_ADDR_LEN);
	return DPS_EUID_TLV_LEN;
}

static uint32_t dps_get_policy_id_tlv(uint8_t *buff, dps_policy_info_t *client_plcy)
{
	dps_policy_id_tlv_t *policyid = (dps_policy_id_tlv_t *)buff;

	client_plcy->policy_id = ntohl(policyid->policy_id);
	client_plcy->version = ntohl(policyid->version);
	client_plcy->ttl = ntohl(policyid->ttl);

	return DPS_POLICY_ID_TLV_LEN;
}

static uint32_t dps_get_ip4_tlv(uint8_t *buff, uint32_t *ip4)
{
	dps_vip4_tlv_t *ip4tlv = (dps_vip4_tlv_t *)buff;

	*ip4 = ntohl(ip4tlv->ip);
	return DPS_IP4_TLV_LEN;
}

static uint32_t dps_get_ip6_tlv(uint8_t *buff, uint8_t *ip6)
{
	dps_vip6_tlv_t *ip6tlv = (dps_vip6_tlv_t *)buff;
	memcpy(ip6, ip6tlv->ip, DPS_IP6_ADDR_LEN);
	return DPS_IP6_TLV_LEN;
}

static uint32_t dps_get_ip_tlv(uint8_t *buff, ip_addr_t *client_ip)
{
	uint32_t len = 0;

	if (buff[0] == 4)
	{
		len = dps_get_ip4_tlv(buff, &client_ip->ip4);
		client_ip->family = AF_INET;
	}
	else
	{
		len = dps_get_ip6_tlv(buff, (uint8_t *)client_ip->ip6);
		client_ip->family = AF_INET6;
	}	
	return len;

}

static uint32_t dps_get_svcloc_tlv(uint8_t *buff, ip_addr_t *svc_loc)
{
	dps_svcloc4_tlv_t *ip4;
	dps_svcloc6_tlv_t *ip6;
	uint32_t len;

	if (buff[0] == 4)
	{
		ip4 = (dps_svcloc4_tlv_t *)buff;
		svc_loc->family = AF_INET;
		svc_loc->xport_type = ntohs(ip4->xport_type);
		svc_loc->port = ntohs(ip4->port);
		svc_loc->ip4 = ntohl(ip4->ip);
		len = DPS_SVCLOC4_TLV_LEN;
	}
	else
	{
		ip6 = (dps_svcloc6_tlv_t *)buff;
		svc_loc->family = AF_INET6;
		svc_loc->xport_type = ntohs(ip6->xport_type);
		svc_loc->port = ntohs(ip6->port);
		memcpy(svc_loc->ip6, ip6->ip6, DPS_IP6_ADDR_LEN);
		len = DPS_SVCLOC6_TLV_LEN;
	}

	return len;
}

/*
 ******************************************************************************
 * dps_get_ipv4_list_tlv                                                   *//**
 *
 * \brief TBD.
 *
 * \param[in] buff - Points to the beginning of ipv4 address list
 * \param[in] client_buff - To be filled in after parsing the ipv4 address list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_ipv4_list_tlv(uint8_t *buff, uint32_t *client_buff)
{
	uint8_t *bufptr, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;

	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;
	
	dps_log_debug(DpsProtocolLogLevel, "v4 addr list len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while ((buff - bufptr) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		*client_buff = ntohl(*((uint32_t *)buff));
		client_buff++;
		buff += DPS_IP4_ADDR_LEN;
	}
	return (buff - recvbuf);
	
}

/*
 ******************************************************************************
 * dps_get_ipv6_list_tlv                                                   *//**
 *
 * \brief TBD.
 *
 * \param[in] buff - Points to the beginning of ipv6 address list
 * \param[in] client_buff - To be filled in after parsing the ipv6 address list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_ipv6_list_tlv(uint8_t *buff, uint8_t *client_buff)
{
	uint8_t *bufptr, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;

	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;
	
	dps_log_debug(DpsProtocolLogLevel, "v6 addr list len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while ((buff - bufptr) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		memcpy(client_buff,buff, DPS_IP6_ADDR_LEN);
		client_buff += DPS_IP6_ADDR_LEN;
		buff += DPS_IP6_ADDR_LEN;
	}
	return (buff - recvbuf);
	
}


static uint32_t dps_get_ipv4_list_to_ipaddr(uint8_t *buff, ip_addr_t **dst_buff, uint32_t *num_of_vip, uint32_t max_vip)
{
	uint8_t *bufptr, *buff_start = buff; 
	dps_tlv_hdr_t tlv_hdr;
	uint32_t tlv_len = 0;
	ip_addr_t *client_buff = *dst_buff;

	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;

	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);

	while ((uint32_t)(buff - bufptr) < tlv_len)
	{
		if (*num_of_vip <= max_vip)
		{
			client_buff->ip4 = ntohl(*((uint32_t *) buff));
			client_buff->family = AF_INET;
			client_buff++;
			buff += DPS_IP4_ADDR_LEN;
			(*num_of_vip)++;
		}
		else
		{
			//Increment it to the end of the tlv
			buff = bufptr + tlv_len;
			dps_log_debug(DpsProtocolLogLevel, "Not enough memory for all vip");
			break;
		}
	}
	*dst_buff = client_buff;
	return (buff - buff_start);
}

static uint32_t dps_get_ipv6_list_to_ipaddr(uint8_t *buff, ip_addr_t **dst_buff, uint32_t *num_of_vip, uint32_t max_vip)
{
	uint8_t *bufptr, *buff_start = buff; 
	dps_tlv_hdr_t tlv_hdr;
	uint32_t tlv_len = 0;
	ip_addr_t *client_buff = *dst_buff;

	buff_start = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;
	
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);

	dps_log_debug(DpsProtocolLogLevel, "v4 addr list len %d", tlv_len);

	while ((uint32_t)(buff - bufptr) < tlv_len)
	{
		if (*num_of_vip <= max_vip)
		{
			memcpy(client_buff, buff, DPS_IP6_ADDR_LEN);
			client_buff->family = AF_INET6;
			client_buff++;
			buff += DPS_IP6_ADDR_LEN;
			(*num_of_vip)++;
		}
		else
		{
			//Increment it to the end of the tlv
			buff = bufptr + tlv_len;
			dps_log_debug(DpsProtocolLogLevel, "Not enough memory for all vip");
			break;
		}
	}
	*dst_buff = client_buff;
	return (buff - buff_start);
}

static uint32_t dps_get_ipv4_tunnel_info(uint8_t *buff, dps_tunnel_endpoint_t *client_buff)
{
	client_buff->family = AF_INET;
	buff += SZ_OF_INT16;
	client_buff->port = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->tunnel_type = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->flags = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->vnid = ntohl(*((uint32_t *)buff));
	buff += DPS_VNID_LEN;
	client_buff->ip4 = ntohl(*((uint32_t *)buff));
	return DPS_IP4_TUNNEL_INFO_LEN;
}

static uint32_t dps_get_ipv6_tunnel_info(uint8_t *buff, dps_tunnel_endpoint_t *client_buff)
{
	client_buff->family = AF_INET6;
	buff += SZ_OF_INT16;
	client_buff->port = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->tunnel_type = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->flags = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_buff->vnid = ntohl(*((uint32_t *)buff));
	buff += DPS_VNID_LEN;
	memcpy(client_buff->ip6, buff, DPS_IP6_ADDR_LEN);
	return DPS_IP6_TUNNEL_INFO_LEN;
}

/*
 ******************************************************************************
 * dps_get_ip4_info_list_tlv                                                   *//**
 *
 * \brief The function parses the ip4 address and flags to the client buffer.
 *        It also sets the number of ip4 address present.
 *
 * \param[in] buff - Points to the beginning of ipv4 address list
 * \param[in] client_buff - To be filled in after parsing the ipv4 address list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_ip4_info_list_tlv(uint8_t *buff, dps_ip4_info_list_t *client_buff)
{
	uint8_t *buff_start, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;
	dps_ip4_info_t *buff_ptr, *client_ip = client_buff->ip_info;
	
	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	buff_start = buff;
	buff_ptr = (dps_ip4_info_t *)buff;
	client_buff->num_of_ips = 0;
	
	dps_log_debug(DpsProtocolLogLevel, "v4 addr list len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while (((uint8_t *)buff_ptr - buff_start) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		client_ip->ip = ntohl(buff_ptr->ip);
		client_ip->flags =  ntohs(buff_ptr->flags);
		client_ip->reserved =  ntohs(buff_ptr->reserved);	
		client_ip++;
		buff_ptr++;
		client_buff->num_of_ips++;
	}
	return ((uint8_t *)buff_ptr - recvbuf);
	
}

/*
 ******************************************************************************
 * dps_get_ip6_info_list_tlv                                                   *//**
 *
 * \brief The function parses the ip4 address and flags to the client buffer.
 *        It also sets the number of ip4 address present.
 *
 * \param[in] buff - Points to the beginning of ipv6 address list
 * \param[in] client_buff - To be filled in after parsing the ipv6 address list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_ip6_info_list_tlv(uint8_t *buff, dps_ip6_info_list_t *client_buff)
{
	uint8_t *buff_start, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;
	dps_ip6_info_t *buff_ptr, *client_ip = client_buff->ip_info;
	
	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	buff_start = buff;
	buff_ptr = (dps_ip6_info_t *)buff;
	client_buff->num_of_ips = 0;
	
	dps_log_debug(DpsProtocolLogLevel, "v6 addr list len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while (((uint8_t *)buff_ptr - buff_start) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		memcpy(client_ip->ip, buff_ptr->ip, DPS_IP6_ADDR_LEN);
		client_ip->flags =  ntohs(buff_ptr->flags);
		client_ip->reserved =  ntohs(buff_ptr->reserved);	
		client_ip++;
		buff_ptr++;
		client_buff->num_of_ips++;
	}
return ((uint8_t *)buff_ptr - recvbuf);
	
}


/*
 ******************************************************************************
 * dps_get_tunnel_list_tlv                                                *//**
 *
 * \brief TBD.
 *
 * \param[in] buff - Points to the beginning of ipv4 address list
 * \param[in] client_buff - To be filled in after parsing the ipv4 address list
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_tunnel_list_tlv(uint8_t *buff, dps_tunnel_endpoint_t *client_buff, uint16_t *num_of_gw)
{
	uint8_t *bufptr, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;
	uint16_t i = 0
;
	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;
	
	dps_log_debug(DpsProtocolLogLevel, "addr list len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while ((buff - bufptr) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		if (ntohs(*((uint16_t *)buff)) == 4) // AF_INET
		{
			buff += dps_get_ipv4_tunnel_info(buff, &client_buff[i]);
		}
		else
		{
			if (ntohs(*((uint16_t *)buff)) == 6) //AF_INET6
			{
				buff += dps_get_ipv6_tunnel_info(buff, &client_buff[i]);
			}
			else
			{
				dps_log_debug(DpsProtocolLogLevel, "Invalid Tunnel Addr Format");
				*num_of_gw = i;
				return (DPS_GET_TLV_LEN((&tlv_hdr)) + 4);
			}
		}
		i++;
	}
	*num_of_gw = i;
	return (buff - recvbuf);
	
}

static uint32_t
dps_get_endpoint_loc_reply_tlv(uint8_t *buff,
                               dps_endpoint_loc_reply_t *endpoint_reply)
{
	uint8_t *buff_end, *buff_start = buff; 
	dps_tlv_hdr_t tlv_hdr;
	uint32_t tlv_len = 0;

	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);
	//Incr tlv hdr len
	buff += DPS_TLV_HDR_LEN;
	//get version which is not in a tlv
	endpoint_reply->version = ntohl(*(uint32_t *)buff);
	buff += DPS_DATA_VER_LEN;
	tlv_len = tlv_len - DPS_DATA_VER_LEN;
	buff_end = buff+tlv_len;
	dps_log_debug(DpsProtocolLogLevel,"dps_get_endpoint_loc_reply_tlv buff %p, buff_end %p tlv len %d",
	              buff, buff_end, tlv_len +4);
	while (buff < buff_end)
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case EUID_TLV:
			    // Get euid (vnid and mac)
			    buff += dps_get_euid_tlv(buff, &(endpoint_reply->vnid));
			    break;
		    case IP_ADDR_TLV:
			    // Get VIP
			    buff += dps_get_ip_tlv(buff, &(endpoint_reply->vm_ip_addr));
			    break;
		    case TUNNEL_LIST_TLV:
			    // Get tunnel list
			    buff += dps_get_tunnel_list_tlv(buff, endpoint_reply->tunnel_info.tunnel_list,  &(endpoint_reply->tunnel_info.num_of_tunnels));
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel,"Invalid TLV");
			    goto error;
			    break;
		}
	}
error:  //TODO:Return a value
	return ((buff - buff_start));
}

static uint32_t
dps_get_endpoint_update_tlv(uint8_t *buff,
                            uint32_t pkt_len,
                            dps_endpoint_update_t *endpoint_update,
                            ip_addr_t *sender_addr)
{
	uint8_t *buff_start = buff; 
	uint8_t *buff_end = buff + pkt_len - 4; // -4 ignores the version which is always present
	dps_tlv_hdr_t tlv_hdr;
	uint32_t tlv_len = 0;

	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);
	dps_log_debug(DpsProtocolLogLevel,"dps_get_endpoint_update_tlv len %d, buff %p, buff_end %p",
	              tlv_len, buff, buff_end);
	buff += DPS_TLV_HDR_LEN;
	endpoint_update->version = ntohl(*(uint32_t *)buff);
	buff += DPS_DATA_VER_LEN;
	tlv_len = tlv_len - DPS_DATA_VER_LEN;
	buff_end = buff+tlv_len;

	while (buff < buff_end)
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case EUID_TLV:
			    // Get euid (vnid and mac)
			    buff += dps_get_euid_tlv(buff, &(endpoint_update->vnid));
			    break;
		    case IP_ADDR_TLV:
			    // Get VIP
			    buff += dps_get_ip_tlv(buff, &(endpoint_update->vm_ip_addr));
			    dps_log_debug(DpsProtocolLogLevel,"endpoint_update->vip %x",endpoint_update->vm_ip_addr.ip4);
			    break;
		    case TUNNEL_LIST_TLV:
			    // Get tunnel list
			    buff += dps_get_tunnel_list_tlv(buff, endpoint_update->tunnel_info.tunnel_list,  &(endpoint_update->tunnel_info.num_of_tunnels));
			    break;
			default:
			    dps_log_info(DpsProtocolLogLevel,"Invalid TLV");
			    goto error;
			    break;
		}
	}

error:
	return (buff - buff_start);
}

static uint32_t
dps_get_endpoint_update_reply_tlv(uint8_t *buff,
                                  dps_endpoint_update_reply_t *endpoint_update)
{
        uint32_t tlv_len = 0;
	dps_tlv_hdr_t tlv_hdr;
	ip_addr_t *vip = NULL;
	uint8_t *buff_end, *buff_start = buff; 

	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);
	buff += DPS_TLV_HDR_LEN;
	// get version
	endpoint_update->version = ntohl(*(uint32_t *)buff);
	buff += DPS_DATA_VER_LEN;
	tlv_len -= DPS_DATA_VER_LEN;
	buff_end = buff + tlv_len;
	vip = endpoint_update->vm_ip_addr;
	while (buff < buff_end)
	{
	    dps_get_tlv_hdr(buff, &tlv_hdr);
	    switch (DPS_GET_TLV_TYPE(&tlv_hdr))
	    {
	       case EUID_TLV:
		       // Get euid (vnid and mac)
		       buff += dps_get_euid_tlv(buff, &(endpoint_update->vnid));
		       break;
	       case IPV4_ADDR_LIST_TLV:
		       // Get VIP
		       buff += dps_get_ipv4_list_to_ipaddr(buff, &vip, &endpoint_update->num_of_vip, MAX_VIP_ADDR);
		       break;
	       case IPV6_ADDR_LIST_TLV:
		       // Get VIP
		       buff += dps_get_ipv6_list_to_ipaddr(buff, &vip,&endpoint_update->num_of_vip, MAX_VIP_ADDR);
		       break;
	       case TUNNEL_LIST_TLV:
		       // Get tunnel list
		       buff += dps_get_tunnel_list_tlv(buff, endpoint_update->tunnel_info.tunnel_list,  &(endpoint_update->tunnel_info.num_of_tunnels));
		       break;
	        default:
		        dps_log_info(DpsProtocolLogLevel,"Invalid TLV");
		        goto error;
		        break;
	    }
	}
error:
	return (buff - buff_start);
}

static uint32_t
dps_get_endpoint_info_tlv(uint8_t *buff, dps_endpoint_info_t *client_info)
{
	dps_endpoint_info_tlv_t *endpoint = (dps_endpoint_info_tlv_t *)buff;
	uint8_t *bufptr = buff;
	
	buff += DPS_TLV_HDR_LEN;
	client_info->vnid = ntohl(endpoint->vnid);
	buff += DPS_VNID_LEN;
	memcpy(client_info->mac, endpoint->mac, DPS_MAC_ADDR_LEN);
	buff += DPS_MAC_ADDR_LEN + 2;
	buff += dps_get_ip_tlv(buff, &client_info->vm_ip_addr);
	return(buff - bufptr);	
}

static uint32_t
dps_get_mcast_addr_tlv(uint8_t *buff, dps_mcast_addr_t *mcast_addr)
{
	dps_tlv_hdr_t tlv_hdr;
	uint8_t *bufptr = buff;

	dps_get_tlv_hdr(buff, &tlv_hdr);
	buff += DPS_TLV_HDR_LEN;
	buff += dps_get_vmac_tlv(buff, mcast_addr->mcast_mac);
	// If the mcast addres is a pure mac address the tlv will still contain an ip address of type v4
	if (buff[0] == 4)
	{
		buff += dps_get_ip4_tlv(buff, &mcast_addr->u.mcast_ip4);
		if (DPS_GET_TLV_TYPE(&tlv_hdr) == MCAST_ADDR_MAC_TLV)
			mcast_addr->mcast_addr_type = MCAST_ADDR_MAC;
		else 
		{
			if (DPS_GET_TLV_TYPE(&tlv_hdr) == MCAST_ADDR_V4_TLV)
				mcast_addr->mcast_addr_type = MCAST_ADDR_V4;
			else
				mcast_addr->mcast_addr_type = MCAST_ADDR_V4_ICB_RANGE;
		}
	}
	else
	{ 
		mcast_addr->mcast_addr_type = MCAST_ADDR_V6;
		buff += dps_get_ip6_tlv(buff, mcast_addr->u.mcast_ip6);
	}	
	return(buff - bufptr);	
}

static uint32_t
dps_get_mcast_grp_rec_tlv(uint8_t *buff, dps_mcast_group_record_t *client_data)
{
	uint32_t tlv_len = 0, len = 0;
	uint8_t *buff_start;
	dps_tlv_hdr_t tlv_hdr;

	buff_start = buff;

	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);

	buff += DPS_TLV_HDR_LEN;

	while (buff < ((uint8_t *)buff_start + DPS_TLV_HDR_LEN + tlv_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
			case MCAST_ADDR_MAC_TLV: 
  		    case MCAST_ADDR_V4_TLV: 
		    case MCAST_ADDR_V6_TLV: 
		    case MCAST_ADDR_V4_ICB_RANGE_TLV:
		    {
			    buff += dps_get_mcast_addr_tlv(buff, &client_data->mcast_addr);
			    break;
		    }
		    case IPV4_ADDR_LIST_TLV:
		    {
			    buff += dps_get_ipv4_list_tlv(buff, client_data->src_addr);
			    client_data->num_of_srcs = ((DPS_GET_TLV_LEN(&tlv_hdr))/DPS_IP4_ADDR_LEN);
			    client_data->family = AF_INET;
			    break;
		    }
		    case IPV6_ADDR_LIST_TLV:
		    {
			    buff += dps_get_ipv6_list_tlv(buff, (uint8_t *)client_data->src_addr);
			    client_data->num_of_srcs = (DPS_GET_TLV_LEN(&tlv_hdr)/DPS_IP6_ADDR_LEN);
			    client_data->family = AF_INET6;
			    break;
		    }
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    break;
		}
	}
	len = buff - buff_start;

	return (len);
}

static uint32_t
dps_get_vnid_tunnel_list_tlv(uint8_t *buff, dps_pkd_tunnel_list_t *client_data)
{
	uint32_t tlv_len = 0;
	uint8_t *buff_start;
	dps_tlv_hdr_t tlv_hdr, ip_tlv_hdr;

	buff_start = buff;
	// Get the vnid tunnel list tlv hdr
	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);

	buff += DPS_TLV_HDR_LEN;	
	client_data->vnid = ntohl(*(uint32_t *)buff);
	buff += DPS_VNID_LEN;
	tlv_len -= DPS_VNID_LEN;       //the vnid len;
	while (buff < ((uint8_t *)buff_start +  DPS_TLV_HDR_LEN + tlv_len))
	{
		// Get the ip list tlv hdr
		dps_get_tlv_hdr(buff, &ip_tlv_hdr);
		dps_log_debug(DpsProtocolLogLevel, "tlv_type %d tlv_len %d", DPS_GET_TLV_TYPE(&ip_tlv_hdr), DPS_GET_TLV_LEN(&ip_tlv_hdr));
		switch (DPS_GET_TLV_TYPE(&ip_tlv_hdr))
		{
		    case IPV4_ADDR_LIST_TLV:
		    {
			    buff += dps_get_ipv4_list_tlv(buff, client_data->tunnel_list);
			    client_data->num_v4_tunnels = ((DPS_GET_TLV_LEN(&ip_tlv_hdr))/DPS_IP4_ADDR_LEN);
			    break;
		    }
		    case IPV6_ADDR_LIST_TLV:
		    {
			    buff += dps_get_ipv6_list_tlv(buff, (uint8_t *)client_data->tunnel_list);
			    client_data->num_v6_tunnels = (DPS_GET_TLV_LEN(&ip_tlv_hdr)/DPS_IP6_ADDR_LEN);
			    break;
		    }
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    break;
		}
	}
	return(buff - buff_start);
}

static uint32_t
dps_get_epri_tlv(uint8_t *buff, dps_epri_t *epri)
{
	uint8_t *buff_start; 
	dps_tlv_hdr_t tlv_hdr;
	uint32_t tlv_len = 0;

	buff_start = buff;

	dps_get_tlv_hdr(buff, &tlv_hdr);
	tlv_len = DPS_GET_TLV_LEN(&tlv_hdr);

	buff += DPS_TLV_HDR_LEN;

	while (buff < ((uint8_t *)buff_start + DPS_TLV_HDR_LEN + tlv_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case EUID_TLV:
			    // Get euid (vnid and mac)
			    buff += dps_get_euid_tlv(buff, &(epri->vnid));
			    break;
		    case IP4_INFO_LIST_TLV:
			    // Get VIP info
			    buff += dps_get_ip4_info_list_tlv(buff, &(epri->vm_ip4_addr));

			    break;
		    case IP6_INFO_LIST_TLV:
			    buff += dps_get_ip6_info_list_tlv(buff, &(epri->vm_ip6_addr));
			    break;
		    case TUNNEL_LIST_TLV:
			    // Get tunnel list
			    buff += dps_get_tunnel_list_tlv(buff, epri->tunnel_info.tunnel_list,  &(epri->tunnel_info.num_of_tunnels));
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "TLV type %d not suppoted len %d", 
			                 DPS_GET_TLV_TYPE(&tlv_hdr), DPS_GET_TLV_LEN(&tlv_hdr));
			    buff += (DPS_GET_TLV_LEN(&tlv_hdr) + DPS_TLV_HDR_LEN);
			    break;
		}
	}
	return ((buff - buff_start));
}

/*******************************************************************************
 *                             Packet Send Functions
 *******************************************************************************/

/*
 ******************************************************************************
 * dps_send_endpoint_loc_req                                              *//**
 *
 * \brief This routine is called by clients of the dps protocol handler to send
 *        packets on the network. This encodes the endpoint_req type packet.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         EUID_TLV                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IP_ADDR_TLV (optional)                 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        SERVICE_LOC_TLV (optional)             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_endpoint_loc_req(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_endpoint_loc_req_t *client_data = DPS_GET_CLIENT_ENDPT_LOC_REQ(client_req);
	
	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len(DPS_ENDPOINT_LOC_REQ, (void *)client_req);
	if ((buff = dps_alloc_buff(len)) == NULL) 
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += dps_set_pkt_hdr(buff, DPS_ENDPOINT_LOC_REQ, (dps_client_data_t *)client_req, len);

	buff += dps_set_euid_tlv(buff, &client_data->vnid);

	// Request for endpoint is indexed via vip/vMac/EUID sent by the client
	if (client_data-> vm_ip_addr.family == AF_INET)
	{
		buff += dps_set_ip4_tlv(buff,client_data-> vm_ip_addr.ip4);
	}
	else if (client_data-> vm_ip_addr.family == AF_INET6)
	{
		buff += dps_set_ip6_tlv(buff, client_data-> vm_ip_addr.ip6);
	}


	// DPS Client Location - If provided
	if ((client_data->dps_client_addr.family == AF_INET) ||
	    (client_data->dps_client_addr.family == AF_INET6))
	{
		buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
	}

	dump_pkt(bufptr, len);

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;

}

/*
 ******************************************************************************
 * dps_send_endpoint_loc_reply                                            *//**
 *
 * \brief This routine is called by the DPS Server to send a endpoint_reply
 *        packet in response to a endpoint_req. In case of error the
 *        endpoint_reply packet will just contain the DPS packet header with
 *        the error code if it is needed.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         ENDPOINT_LOC_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_endpoint_loc_reply(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_endpoint_loc_reply_t *client_data = DPS_GET_CLIENT_ENDPT_LOC_REPLY(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	do
	{
		dump_client_info((dps_client_data_t *)client_req);

		// If there is an error dont encode anything
		if (client_hdr->resp_status)
		{
			len = DPS_PKT_HDR_LEN;
		}
		else
		{
			len = calc_pkt_len(client_hdr->type, (void *)client_req);
		}
		dps_log_debug(DpsProtocolLogLevel, "dps_send_endpoint_reply pkt len %d",len);
		if ((buff = dps_alloc_buff(len)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR;
			break;
		}

		bufptr = buff;
		buff += dps_set_pkt_hdr(buff, client_hdr->type, (dps_client_data_t *)client_req, len);
		if (len != DPS_PKT_HDR_LEN)
		{
			// set endpoint location tlv which has a number of sub-tlvs
			buff += dps_set_endpoint_loc_reply_tlv(buff, client_data);
		}

		status = dps_protocol_xmit(bufptr, (buff - bufptr),
		                           &client_hdr->reply_addr,
		                           ((dps_client_data_t *)client_req)->context);
		dps_free_buff(bufptr);
	}while(0);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return status;
}

/*
 ******************************************************************************
 * dps_send_endpoint_update                                               *//**
 *
 * \brief This routine is called by Dove Switches to reqister endpoint (VM port)
 *        information. The endpoint can be identified by an IP/MAC/EUID.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         ENDPOINT_LOC_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        SERVICE_LOC_TLV (optional)             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_endpoint_update(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_endpoint_update_t *client_data = DPS_GET_CLIENT_ENDPT_UPDATE(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len(DPS_ENDPOINT_UPDATE, (void *)client_req);

	if (len == DPS_PKT_HDR_LEN)
	{
		dps_log_error(DpsProtocolLogLevel,"Endpoint Update Data Incorrect");
		return DPS_ERROR;
	}
	
	if ((buff = dps_alloc_buff(len)) == NULL) 
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += DPS_PKT_HDR_LEN;

	// set endpoint location tlv which has a number of sub-tlvs
	buff += dps_set_endpoint_update_tlv(buff, client_data, len - DPS_PKT_HDR_LEN);
	// DPS Client Location if provided
	if ((client_data->dps_client_addr.family == AF_INET) ||
	    (client_data->dps_client_addr.family == AF_INET6))
	{
		buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
	}
	
	dps_set_pkt_hdr(bufptr, DPS_ENDPOINT_UPDATE, (dps_client_data_t *)client_req, (buff - bufptr));
 	
	dump_pkt(bufptr, (buff - bufptr));
	
	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_endpoint_update_reply                                         *//**
 *
 * \brief This routine is called by the DPS Server in response to an
 *        endpoint_update message. This message is a ACK/NACK on the success
 *        of the enpoint_update.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        ENDPOINT_UPDATE_REPLY_TLV              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_endpoint_update_reply(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_endpoint_update_reply_t *client_data = DPS_GET_CLIENT_ENDPT_UPDATE_REPLY(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len(client_hdr->type, (void *)client_req);
	if ((buff = dps_alloc_buff(len)) == NULL) 
	{
		dps_log_error(DpsProtocolLogLevel, "Exit: No memory: DPS_ERROR");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += dps_set_pkt_hdr(buff, DPS_ENDPOINT_UPDATE_REPLY, (dps_client_data_t *)client_req, len);

	if (len != DPS_PKT_HDR_LEN) 
	{
		// set endpoint location tlv which has a number of sub-tlvs
		buff += dps_set_endpoint_update_reply_tlv(buff, client_data, len-DPS_PKT_HDR_LEN);
 	}
	
	dump_pkt(bufptr, len);
	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_log_debug(DpsProtocolLogLevel, "Free bufptr %p len %d", bufptr, len);
	dps_free_buff(bufptr);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return status;
}

/*
 ******************************************************************************
 * dps_send_policy_req                                                    *//**
 *
 * \brief This routine is called by Dove Switches to request a policy
 *        The message contains the src and destination endpoint information
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        ENDPOINT_INFO_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        ENDPOINT_INFO_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        SERVICE_LOC_TLV (optional)             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_policy_req(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;


	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);

	dps_policy_req_t *client_data = DPS_GET_CLIENT_POLICY_REQ(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len(DPS_POLICY_REQ, (void *)client_req);

	if ((buff = dps_alloc_buff(len)) == NULL) 
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += dps_set_pkt_hdr(buff, DPS_POLICY_REQ, (dps_client_data_t *)client_req, len);

	//encode dest_endpoint info_tlv
	buff += dps_set_endpoint_info_tlv(buff, &client_data->dst_endpoint);
	
	//encode src_endpoint info_tlv
	buff += dps_set_endpoint_info_tlv(buff, &client_data->src_endpoint);
	
	//encode dps client address if applicable
	if ((client_data->dps_client_addr.family == AF_INET) ||
	    (client_data->dps_client_addr.family == AF_INET6))
	{
		buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
	}

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_policy_reply                                                  *//**
 *
 * \brief This routine is called by the DPS Server to send a policy_reply
 *        packet in response to a policy_req. The policy reply msg contains
 *        endpoint information as well as policy information.
 *        In case of error the policy_reply packet will just contain the DPS 
 *        packet header with the error code if it is needed.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        ENDPOINT_LOC_TLV                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        POLICY_TLV                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_policy_reply(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;

	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	
	dps_policy_reply_t *client_data = DPS_GET_CLIENT_POLICY_REPLY(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);


	len = calc_pkt_len(DPS_POLICY_REPLY, (void *)client_req);

	dps_log_debug(DpsProtocolLogLevel, "dps_send_policy_reply pkt len %d",len);

	if ((buff = dps_alloc_buff(len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += dps_set_pkt_hdr(buff, DPS_POLICY_REPLY, (dps_client_data_t *)client_req, len);

	// set policy reply tlv which has a number of sub-tlvs
	buff += dps_set_policy_reply_tlv(buff, client_data, client_hdr, len - DPS_PKT_HDR_LEN);

	dump_pkt(bufptr, len);
	dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_internal_gw_reply                                               *//**
 *
 * \brief This func is called for 2 different types of msgs, DPS_BCAST_LIST_REPLY
 *        and 	DPS_UNSOLICITED_BCAST_LIST_REPLY.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV4_ADDR_LIST_TLV (optional)          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV6_ADDR_LIST_TLV (optional)          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_internal_gw_reply(void *client_req, void *cli_addr)
{
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_internal_gw_t *client_data = &((dps_client_data_t *)client_req)->internal_gw_list;
	uint32_t *client_gw, len;
	
	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	if (client_hdr->resp_status)
	{
		len = DPS_PKT_HDR_LEN;
	}
	else
	{
		len = calc_pkt_len(client_hdr->type, (void *)client_req);
	
	}

	if ((buff = dps_alloc_buff(len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;
	buff += dps_set_pkt_hdr(buff, client_hdr->type, (dps_client_data_t *)client_req, len);

	if (len != DPS_PKT_HDR_LEN)
	{
		client_gw = client_data->gw_list;
		
		if (client_data->num_v4_gw)
		{
			buff += dps_set_ipv4_list_tlv(buff, client_gw, client_data->num_v4_gw);
			//Advance gw list by the number of v4 gws
			client_gw += client_data->num_v4_gw;
		}
		if (client_data->num_v6_gw)
		{
			buff += dps_set_ipv6_list_tlv(buff, (uint8_t *)client_gw, client_data->num_v6_gw);
		}
	}

	dump_pkt(bufptr,len);
	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	dps_free_buff(bufptr);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;	
}

/*
 ******************************************************************************
 * dps_send_gen_msg_req                                                   *//**
 *
 * \brief This function sends a general msg body qualified by a distinct msg type
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_gen_msg_req(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_gen_msg_req_t *client_data = DPS_GET_CLIENT_GEN_MSG_REQ(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	len = calc_pkt_len(client_hdr->type, (void *)client_req);
	if ((buff = dps_alloc_buff(len)) == NULL)
	{
	    dps_log_error(DpsProtocolLogLevel,"No memory");
	    return DPS_ERROR;
	}

	bufptr = buff;
	buff += dps_set_pkt_hdr(buff, client_hdr->type, (dps_client_data_t *)client_req, len);


	if ((client_data->dps_client_addr.family == AF_INET) ||
	    (client_data->dps_client_addr.family == AF_INET6))
	{
	    buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
	}

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;	
}

/*
 ******************************************************************************
 * dps_send_bcast_list_reply                                               *//**
 *
 * \brief This function is called by DPS to send a msg to a DS. This msg contains 
 *        a list DS which all have VMs belonging to a specific vnid.
 *        This function is called to send 2 types of msgs, DPS_BCAST_LIST_REPLY
 *        and DPS_UNSOLICITED_BCAST_LIST_REPLY.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV4_ADDR_LIST_TLV (optional)          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IPV6_ADDR_LIST_TLV (optional)          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_bcast_list_reply(void *client_req, void *cli_addr)
{
	uint8_t *buff, *bufptr;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_pkd_tunnel_list_t *client_data = &((dps_client_data_t *)client_req)->dove_switch_list;
	uint32_t *client_switch, len, status = DPS_SUCCESS;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	status = DPS_SUCCESS;
	do
	{
		dump_client_info((dps_client_data_t *)client_req);

		if (client_hdr->resp_status)
		{
			len = DPS_PKT_HDR_LEN;
		}
		else
		{
			len = calc_pkt_len(client_hdr->type, (void *)client_req);
		
		}

		if ((buff = dps_alloc_buff(len)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR;
			break;
		}

		bufptr = buff;
		buff += dps_set_pkt_hdr(buff, client_hdr->type, (dps_client_data_t *)client_req, len);

		if (len != DPS_PKT_HDR_LEN)
		{
			client_switch = client_data->tunnel_list;

			if (client_data->num_v4_tunnels)
			{
				buff += dps_set_ipv4_list_tlv(buff, client_switch, client_data->num_v4_tunnels);
				//Advance switch list by the number of v4 switches
				client_switch += client_data->num_v4_tunnels;
			}
			if (client_data->num_v6_tunnels)
			{
				buff += dps_set_ipv6_list_tlv(buff, (uint8_t *)client_switch, client_data->num_v6_tunnels);
			}
		}

		dump_pkt(bufptr,len);
		status = dps_protocol_xmit(bufptr,
		                           (buff - bufptr),
		                           &client_hdr->reply_addr,
		                           ((dps_client_data_t *)client_req)->context);
		dps_free_buff(bufptr);
	}while(0);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return status;
}



/*
 ******************************************************************************
 * dps_send_bulk_policy_xfer                                             *//**
 *
 * \brief This routine is called by DPS when it wants to send a list of policies
 *        to a DS. The policies are of the type permit/deny between vnids.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    Number of Permit Rules     |  Number of Deny Rules         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Src VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dst VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           .                                   |
 *  |                           .                                   |
 *  |                      Upto Number of Permit Rules Pair         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Src VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Dst VNID                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           .                                   |
 *  |                           .                                   |
 *  |                      Upto Number of Deny Rules Pair           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_bulk_policy_xfer(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_bulk_vnid_policy_t *client_data = &((dps_client_data_t *)client_req)->bulk_vnid_policy;

	dps_log_info(DpsProtocolLogLevel,"Enter");

	do
	{
		dump_client_info((dps_client_data_t *)client_req);
		len = calc_pkt_len(client_hdr->type, (void *)client_req);

		dps_log_debug(DpsProtocolLogLevel, "dps_send_bulk_policy_xfer len %d",len);

		if ((buff = dps_alloc_buff(len)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR_NO_RESOURCES;
			break;
		}

		bufptr = buff;
		buff += dps_set_pkt_hdr(buff, client_hdr->type,
		                        (dps_client_data_t *)client_req, len);
		buff += dps_set_bulk_policy_xfer(buff, client_data, len - DPS_PKT_HDR_LEN);

		dump_pkt(bufptr, len);
		status = dps_protocol_xmit(bufptr,
		                           (buff - bufptr),
		                           &client_hdr->reply_addr,
		                           ((dps_client_data_t *)client_req)->context);
		dps_free_buff(bufptr);
	} while(0);
	dps_log_info(DpsProtocolLogLevel,"Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_gw_list                                                       *//**
 *
 * \brief This func is used to push unsolicited external gw list to the dove 
 *        switches. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         TUNNEL_LIST_TLV                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_gw_list(void *client_req, void *cli_addr)
{
	uint8_t *buff, *bufptr;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_tunnel_list_t *client_data = &((dps_client_data_t *)client_req)->tunnel_info;
	uint32_t len = 0;
	uint32_t status = DPS_SUCCESS;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	do
	{
		dump_client_info((dps_client_data_t *)client_req);

		if (client_hdr->resp_status)
		{
			len = DPS_PKT_HDR_LEN;
		}
		else
		{
			len = calc_pkt_len(client_hdr->type, (void *)client_req);
		}

		if ((buff = dps_alloc_buff(len)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR;
		}

		bufptr = buff;
		buff += DPS_PKT_HDR_LEN;
		if (len != DPS_PKT_HDR_LEN)
		{
			buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_list, client_data->num_of_tunnels);
		}
		len = buff - bufptr;
		dps_set_pkt_hdr(bufptr, client_hdr->type, (dps_client_data_t *)client_req, len);
		dump_pkt(bufptr,len);
		status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
		dps_free_buff(bufptr);
	}while(0);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_gw_reg_dereg                                                  *//**
 *
 * \brief This func is used to register/deregister tunnel endpoints with the
 *        DPS Server. All users of DPS for example dove switches, vlan/external 
 *        gateways need to first register their tunnel endpoints before any other
 *        msgs can be sent. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         TUNNEL_LIST_TLV                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         SERVICE_LOC_TLV                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_tunnel_reg_dereg(void *client_req, void *cli_addr)
{
	uint8_t *buff, *bufptr;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_tunnel_reg_dereg_t *client_data = &((dps_client_data_t *)client_req)->tunnel_reg_dereg;
	uint32_t len = 0;
	uint32_t status = DPS_SUCCESS;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	if (client_hdr->resp_status)
	{
		len = DPS_PKT_HDR_LEN;
	}
	else
	{
		len = calc_pkt_len(client_hdr->type, (void *)client_req);
	
	}

	if ((buff = dps_alloc_buff(len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;
	buff += DPS_PKT_HDR_LEN;

	if (len != DPS_PKT_HDR_LEN)
	{
		buff += dps_set_tunnel_list_tlv(buff, client_data->tunnel_info.tunnel_list, client_data->tunnel_info.num_of_tunnels);
		if ((client_data->dps_client_addr.family == AF_INET) ||
		    (client_data->dps_client_addr.family == AF_INET6))
		{
			buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
		}
	}
	len = buff - bufptr;
	dps_set_pkt_hdr(bufptr, client_hdr->type, (dps_client_data_t *)client_req, len);
	dump_pkt(bufptr,len);
	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;	
}

/*
 ******************************************************************************
 * dps_send_vm_migration_event                                            *//**
 *
 * \brief This routine is called by the Dove switch to let the DPS know that
 *        it is receiving data packets destined for a VM that no longer exists
 *        or has moved. The Dove switch will send to the DPS information about the
 *        vm that has moved as well as information about the vm that is the source 
 *        of the data. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         ENDPOINT_INFO_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         ENDPOINT_LOC_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_vm_migration_event(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_vm_migration_event_t *client_data = &(((dps_client_data_t *)client_req)->vm_migration_event);

	dps_log_info(DpsProtocolLogLevel, "Enter");
	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len(DPS_VM_MIGRATION_EVENT, (void *)client_req);

	dps_log_debug(DpsProtocolLogLevel, " dps_send_vm_migration_event pkt len %d",len);

	if ((buff = dps_alloc_buff(len)) == NULL) 
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;

	buff += dps_set_pkt_hdr(buff, DPS_VM_MIGRATION_EVENT, (dps_client_data_t *)client_req, len);

	buff += dps_set_endpoint_info_tlv(buff, &client_data->migrated_vm_info);
	// set endpoint location tlv which has a number of sub-tlvs
	buff += dps_set_endpoint_loc_reply_tlv(buff, &client_data->src_vm_loc);

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);

	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_mcast_sender_reg_dereg                                        *//**
 *
 * \brief This routine is called when a DS wants to register or deregister a 
 *        multicast sender.
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IP_ADDR_TLV                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         ENDPOINT_INFO_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       MCAST_ADDR_MAC_TLV/MCAST_ADDR_V4_TLV/MCAST_ADDR_V6_TLV  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         SERVICE_LOC_TLV(optional)             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_mcast_sender_reg_dereg(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_mcast_sender_t *client_data = &(((dps_client_data_t *)client_req)->mcast_sender);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len((dps_client_req_type)client_hdr->type, (void *)client_req);
	if ((buff = dps_alloc_buff(len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;
	buff += dps_set_pkt_hdr(buff, (dps_client_req_type)client_hdr->type, (dps_client_data_t *)client_req, len);

	while (((uint32_t)(buff - bufptr)) < len)
	{
		buff += dps_set_ip_tlv(buff, &client_data->tunnel_endpoint);
		buff += dps_set_endpoint_info_tlv(buff, &client_data->mcast_src_vm);
		buff += dps_set_mcast_addr_tlv(buff, &client_data->mcast_addr);
		if ((client_data->dps_client_addr.family == AF_INET) ||
		    (client_data->dps_client_addr.family == AF_INET6))
		{
			buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
		}
	}

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;	
}

/*
 ******************************************************************************
 * dps_send_mcast_receiver_join_leave                                     *//**
 *
 * \brief This routine is called when a DS wants to subscribe or un-subscribe 
 *        from multicast traffic. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         IP_ADDR_TLV                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         MCAST_GRP_REC_TLV                     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         SERVICE_LOC_TLV (optional)            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_mcast_receiver_join_leave(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_mcast_receiver_t *client_data = &(((dps_client_data_t *)client_req)->mcast_receiver);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	dump_client_info((dps_client_data_t *)client_req);

	len = calc_pkt_len((dps_client_req_type)client_hdr->type, (void *)client_req);
	
	if ((buff = dps_alloc_buff(len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	bufptr = buff;
	buff += dps_set_pkt_hdr(buff, (dps_client_req_type)client_hdr->type, (dps_client_data_t *)client_req, len);

	while (((uint32_t)(buff - bufptr)) < len)
	{
		//Set tunnel endpoint
		buff += dps_set_ip_tlv(buff, &client_data->tunnel_endpoint);
		// Set grp rec tlv
		buff += dps_set_mcast_grp_rec_tlv(buff, &client_data->mcast_group_rec);
		//Set svc loc tlv
		if ((client_data->dps_client_addr.family == AF_INET) ||
		    (client_data->dps_client_addr.family == AF_INET6))
		{
			buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
		}
	}

	dump_pkt(bufptr,len);

	status = dps_protocol_xmit(bufptr, (buff - bufptr), &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	dps_free_buff(bufptr);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;	
}


/*
 ******************************************************************************
 * dps_send_mcast_receiver_ds_list                                        *//**
 *
 * \brief This routine is called when DPS needs to send a list dove switches (DS) 
 *        receivers that have subscribed to receive traffic for a multicast
 *        address. The first part of the message has the multicast address
 *        followed by a record which has a vnid followed by list of DS.
 *        The message could have multiple such records. 
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    MCAST_ADDR_MAC_TLV/MCAST_ADDR_V4_TLV/MCAST_ADDR_V6_TLV     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  VNID_TUNNEL_LIST_TLV                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  VNID_TUNNEL_LIST_TLV                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                            .                                  |
 *  |                            .                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr  - The address of the dove switch
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_mcast_receiver_ds_list(void *client_req, void *cli_addr)
{
	uint8_t *buff, *buff_start;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_mcast_receiver_ds_list_t *client_data = &((dps_client_data_t *)client_req)->mcast_receiver_ds_list;
	dps_pkd_tunnel_list_t *mcast_recvr_list;
	uint32_t i;
	uint8_t *tmp_char_ptr;
	uint32_t len = 0;
	uint32_t status = DPS_SUCCESS;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	buff_start = NULL;

	do
	{
		dump_client_info((dps_client_data_t *)client_req);
		// Setting aside a large buffer for mcast
		if ((buff = dps_alloc_buff(DPS_SEND_BUFF_SZ)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR;
			break;
		}

		buff_start = buff;
		buff += DPS_PKT_HDR_LEN;
		buff += dps_set_mcast_addr_tlv(buff, &client_data->mcast_addr);

		mcast_recvr_list = client_data->mcast_recvr_list;

		for (i = 0; i < client_data->num_of_rec; i++)
		{
			if ((buff-buff_start) > DPS_SEND_BUFF_SZ)
			{
				dps_log_error(DpsProtocolLogLevel,"Not enough memory to encode data");
				status = DPS_ERROR_EXCEEDS_CAP;
				break;
			}
			buff += dps_set_vnid_tunnel_list_tlv(buff, mcast_recvr_list);
			tmp_char_ptr = (uint8_t *)mcast_recvr_list;
			tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[mcast_recvr_list->num_v4_tunnels + (mcast_recvr_list->num_v6_tunnels << 2)]);
			mcast_recvr_list = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
		}
		len = buff-buff_start;
		dps_set_pkt_hdr(buff_start, client_hdr->type, (dps_client_data_t *)client_req, len);
		dump_pkt(buff_start, len);
		status = dps_protocol_xmit(buff_start, len, &client_hdr->reply_addr,
		                           ((dps_client_data_t *)client_req)->context);
	}while(0);

	if (buff_start != NULL)
	{
		dps_free_buff(buff_start);
	}

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_client_create_general_ack                                          *//**
 *
 * \brief Create an ack packet as if it was created by the client with the msg
 *        type set to DPS_GENERAL_ACK with the sub_type field set to the msg type
 *        that needs to be acked. If qid is 0 dont send an ack msg.
 *        The reply_addr field in the client_buff hdr should be set correctly
 *        to the address to which the pkt needs to be sent.
 *
 * \param[in] client_buff - A pointer to the parsed copy of the original msg that
 *                          needs to be acked.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_client_create_general_ack(dps_client_data_t *client_buff)
{
	dps_client_data_t client_data;

	client_data.hdr = ((dps_client_data_t *)client_buff)->hdr;

	if (client_data.hdr.query_id != 0)
	{
		// For the general ack the sub_type is the original msg type that it is trying to ack
		client_data.hdr.sub_type = client_data.hdr.type; 
		if ((client_data.hdr.type == DPS_TUNNEL_REGISTER) || 
		    (client_data.hdr.type == DPS_TUNNEL_DEREGISTER) ||
		    (client_data.hdr.type == DPS_MCAST_SENDER_REGISTRATION) ||
		    (client_data.hdr.type == DPS_MCAST_SENDER_DEREGISTRATION) ||
		    (client_data.hdr.type == DPS_MCAST_RECEIVER_JOIN) ||
		    (client_data.hdr.type == DPS_MCAST_RECEIVER_LEAVE))
		{
			client_data.hdr.type = DPS_REG_DEREGISTER_ACK;
		}
		else
		{
			client_data.hdr.type = DPS_GENERAL_ACK;
		}
		client_data.hdr.resp_status = DPS_NO_ERR;
		dps_protocol_client_send(&client_data);
	}
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_send_general_ack                                                   *//**
 *
 * \brief The ack is sent in response to an unsolicited msg from the DPS. The
 *        msg is of type DPS_GENERAL_ACK with the sub_type field of the msg set 
 *        to the msg type it is acking. 
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] senders_addr - The address of the DPS.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_general_ack(void *client_req, void *cli_addr)
{
	dps_pkt_hdr_t *pkt_hdr, pkt_hdr_buff = {0};
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);

	dps_log_info(DpsProtocolLogLevel, "Enter");
	pkt_hdr = &pkt_hdr_buff;
	dps_set_pkt_hdr((uint8_t *)pkt_hdr, client_hdr->type, (dps_client_data_t *)client_req, DPS_PKT_HDR_LEN);
	dump_pkt((uint8_t *)pkt_hdr,DPS_PKT_HDR_LEN);
	status = dps_protocol_xmit((uint8_t *)pkt_hdr, DPS_PKT_HDR_LEN, &client_hdr->reply_addr, NULL);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_policy_invalidate                                             *//**
 *
 * \brief TBD.
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_policy_invalidate(void *client_req, void *cli_addr)
{
	dps_log_notice(DpsProtocolLogLevel,"Not implemented");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_send_addr_resolve                                                  *//**
 *
 * \brief DPS sends this msg to all Dove Switches requesting them to resolve a
 *        vIP address. The DS on receiving this msg should send an arp request
 *        to all the VMs registered with the DS. If a VM responds to the arp 
 *        request, the DS should register the VM with a endpoint_update msg.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        EUID_TLV                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        IP_ADDR_TLV                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        SERVICE_LOC_TLV                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - The address of the dove switch.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_addr_resolve(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *bufptr;
	uint32_t status = DPS_SUCCESS;
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	dps_endpoint_loc_req_t *client_data = (dps_endpoint_loc_req_t *)&(((dps_client_data_t *)(client_req))->address_resolve);

	dps_log_info(DpsProtocolLogLevel, "Enter");
	do
	{
		dump_client_info((dps_client_data_t *)client_req);
		len = calc_pkt_len(DPS_ADDR_RESOLVE, (void *)client_req);
		if ((buff = dps_alloc_buff(len)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			status = DPS_ERROR;
			break;
		}

		bufptr = buff;
		buff += dps_set_pkt_hdr(buff, client_hdr->type, (dps_client_data_t *)client_req, len);
		buff += dps_set_euid_tlv(buff, &client_data->vnid);

		// Request for endpoint is indexed via vip/vMac/EUID sent by the client
		if (client_data-> vm_ip_addr.family == AF_INET)
		{
			buff += dps_set_ip4_tlv(buff,client_data-> vm_ip_addr.ip4);
		}
		else if (client_data->vm_ip_addr.family == AF_INET6)
		{
			buff += dps_set_ip6_tlv(buff, client_data-> vm_ip_addr.ip6);
		}

		// DPS Client Location - If provided
		if ((client_data->dps_client_addr.family == AF_INET) ||
		    (client_data->dps_client_addr.family == AF_INET6))
		{
			buff += dps_set_svcloc_tlv(buff, &(client_data->dps_client_addr));
		}
		dump_pkt(bufptr, len);
		status = dps_protocol_xmit(bufptr, (buff - bufptr),
		                           &client_hdr->reply_addr,
		                           ((dps_client_data_t *)client_req)->context);
		dps_free_buff(bufptr);
	}while(0);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*
 ******************************************************************************
 * dps_send_addr_reply                                                    *//**
 *
 * \brief TBD.
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_send_addr_reply(void *client_req, void *cli_addr)
{
	dps_log_notice(DpsProtocolLogLevel,"Not implemented");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_send_invalidate_vm                                                 *//**
 *
 * \brief This msg is sent to all DOVE switches in the domain to update the VMs
 *        location if present in the forwarding table. This message is sent
 *        in response to a migration_in event sent by the DOVE switch to which
 *        the VM has moved.
 *
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Header                                |
 *  |                                                               |
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        EPRI_TLV                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * \param[in] client_req - A pointer to a message that the client wants to send
 * \param[in] cli_addr - Not used, it is a NULL value.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_send_vm_loc_info(void *client_req, void *cli_addr)
{
	uint32_t len = 0;
	uint8_t *buff, *buff_start;
	uint32_t status = DPS_SUCCESS;

	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(client_req);
	
	dps_epri_t *client_data = (dps_epri_t *)&(((dps_client_data_t *)(client_req))->vm_invalidate_msg.epri);

	dps_log_info(DpsProtocolLogLevel, "Enter");

	do
	{
		dump_client_info((dps_client_data_t *)client_req);
		
		if ((buff = dps_alloc_buff(DPS_MAX_BUFF_SZ)) == NULL)
		{
			dps_log_error(DpsProtocolLogLevel,"No memory");
			return DPS_ERROR;
		}
	
		buff_start = buff;
		buff += DPS_PKT_HDR_LEN;
		
		// set epri_tlv which has a number of sub-tlvs
		buff += dps_set_epri_tlv(buff, client_data);

		len = buff-buff_start;
		dps_set_pkt_hdr(buff_start, client_hdr->type, (dps_client_data_t *)client_req, len);
		dump_pkt(buff_start, len);
		dps_protocol_xmit(buff_start, len, &client_hdr->reply_addr, ((dps_client_data_t *)client_req)->context);
	} while(0);

	dps_free_buff(buff_start);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return status;
}

/*******************************************************************************
 *                             Packet Recv Functions
 *******************************************************************************/

/*
 ******************************************************************************
 * dps_process_endpoint_loc_req                                               *//**
 *
 * \brief This routine processes the endpoint request sent by a DOVE switch or
 *        Dove GW The senders address is needed so that the DPS server can
 *        reply to the correct requestor of the endpoint request
 *
 * \param[in] recv_buff - A pointer to the endpoint req pkt.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_endpoint_loc_req(void *recv_buff, void *senders_addr)
{
	dps_endpoint_loc_req_t *endpoint_req = NULL;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	endpoint_req = (dps_endpoint_loc_req_t *)&(client_buff->endpoint_loc_req);

	// Set hdr info
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// Assume DPS Client to be the Sender's Location
	endpoint_req->dps_client_addr = *((ip_addr_t *)senders_addr);

	dps_log_debug(DpsProtocolLogLevel, "packet len %d", pkt_len);
	buff += DPS_PKT_HDR_LEN;

	// Go to the end of buffer
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
			case VMAC_TLV:
				buff += dps_get_vmac_tlv(buff, endpoint_req->mac);
				break;
			case IP_ADDR_TLV:
				buff += dps_get_ip_tlv(buff, &endpoint_req->vm_ip_addr);
				break;
			case EUID_TLV:
				buff += dps_get_euid_tlv(buff, &endpoint_req->vnid);
				break;
			case REPLY_SERVICE_LOC_TLV:
			case SERVICE_LOC_TLV:
				// Update the DPS Client Location
				buff += dps_get_svcloc_tlv(buff, &endpoint_req->dps_client_addr);
				break;
			default:
				dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	
	}

	dump_client_info(client_buff);

	dps_send_to_protocol_client((void *)client_buff);
error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_endpoint_loc_reply                                         *//**
 *
 * \brief This routine processes the endpoint reply sent from the DPS server.
 *        It contains the the necessary data needed to communicate with the VM.
 *        In the case of error the error code is set in the header and no TLV
 *        is encoded. The senders address is not used since this is a response
 *        from the DPS server to a request from the Dove switch/gateway
 *        requester of the endpoint request
 *
 * \param[in] recv_buff - A pointer to the endpoint req pkt.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_endpoint_loc_reply(void *recv_buff,
                                               void *senders_addr)
{
	dps_endpoint_loc_reply_t *endpoint_reply;
	dps_client_data_t *client_buff;
	dps_client_hdr_t hdr;
	uint32_t pkt_len, ret_status = DPS_SUCCESS;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);

	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + (pkt_len))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	endpoint_reply = (dps_endpoint_loc_reply_t *)&(client_buff->endpoint_loc_reply);

	// Set hdr info
	client_buff->hdr = hdr;

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// Stop retranmsit timer
	if (client_buff->hdr.type == DPS_ENDPOINT_LOC_REPLY)
	{
		ret_status = dps_stop_retransmit_timer(client_buff);
	}
	
	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		buff += dps_get_endpoint_loc_reply_tlv(buff, endpoint_reply);
	}

	dump_client_info(client_buff);
	// call the client
	if (client_buff->hdr.type == DPS_UNSOLICITED_ENDPOINT_LOC_REPLY)
	{
		client_buff->hdr.type = DPS_ENDPOINT_LOC_REPLY;
	}
	dps_send_to_protocol_client((void *)client_buff);

	if (hdr.type == DPS_UNSOLICITED_ENDPOINT_LOC_REPLY)
	{
		dps_client_create_general_ack(client_buff);
	}

	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return ret_status;
}

/*
 ******************************************************************************
 * dps_process_endpoint_update                                            *//**
 *
 * \brief This routine processes the endpoint_update message sent by a DOVE
 *        switch. The senders address is needed so that the DPS server can
 *        reply to the correct requester of the endpoint request. The message
 *        contains VM information like IP/MAC/EUID.
 *
 * \param[in] recv_buff - A pointer to the endpoint update pkt.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_endpoint_update(void *recv_buff, void *senders_addr)
{
	dps_endpoint_update_t *endpoint_update;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	dps_tlv_hdr_t tlv_hdr;	
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	dps_log_info(DpsProtocolLogLevel, "Alloc Buff Len %d", pkt_len + sizeof(dps_client_data_t));
	endpoint_update = (dps_endpoint_update_t *)&(client_buff->endpoint_update);

	// Set hdr info
	client_buff->hdr = hdr;
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

   // Assume DPS Client to be the Sender's Location
	endpoint_update->dps_client_addr = *((ip_addr_t *)senders_addr);

	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case ENDPOINT_LOC_TLV:
			    buff += dps_get_endpoint_update_tlv(buff,
			                                        pkt_len,
			                                        endpoint_update,
			                                        (ip_addr_t *)senders_addr);
			    break;
		    case SERVICE_LOC_TLV:
		    case REPLY_SERVICE_LOC_TLV:
			    // Update the DPS Client IP
			    buff += dps_get_svcloc_tlv(buff, &endpoint_update->dps_client_addr);
			    break;
			default:
				dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;  
			    break;
		}
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);
error:
	dps_free_buff((uint8_t *)client_buff);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_endpoint_update_reply                                      *//**
 *
 * \brief This routine processes the endpoint_update reply sent from the DPS
 *        server. The reply ACKS/NACKS in response to the enpoint_update. The
 *        DOVE Switch should examine the response_error field in the header to
 *        determine whether the endpoint_update was a success.
 *
 * \param[in] recv_buff - A pointer to the endpoint update pkt.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_endpoint_update_reply(void *recv_buff,
                                                  void *senders_addr)
{
	dps_endpoint_update_reply_t *endpoint_reply;
	dps_client_data_t *client_buff;
	uint32_t pkt_len, ret_status = DPS_SUCCESS;	
	dps_client_hdr_t hdr;
	dps_tlv_hdr_t tlv_hdr;
	uint8_t *buff = (uint8_t *)recv_buff;
	
	dps_log_info(DpsProtocolLogLevel, "Enter");

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	
	endpoint_reply = (dps_endpoint_update_reply_t *)&(client_buff->endpoint_update_reply);
	// Set hdr info
	client_buff->hdr = hdr;
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// Stop retransmit timer
	ret_status = dps_stop_retransmit_timer(client_buff);

	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
	   dps_get_tlv_hdr(buff, &tlv_hdr);
	   switch (DPS_GET_TLV_TYPE(&tlv_hdr))
	   {
	      case ENDPOINT_UPDATE_REPLY_TLV:
		      buff += dps_get_endpoint_update_reply_tlv(buff, endpoint_reply);
		      break;
	      default:
		      dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
		      goto error;
		      break;
	   }
	}
	dump_client_info(client_buff);
	
	// call the client
	dps_send_to_protocol_client((void *)client_buff);
error:
	dps_free_buff((uint8_t *)client_buff);
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return ret_status;
}

/*
 ******************************************************************************
 * dps_process_policy_req                                                 *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_policy_req(void *recv_buff, void *senders_addr)
{
	dps_endpoint_info_t *endpoint_info;
	dps_policy_req_t *policy_req = NULL;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	policy_req = (dps_policy_req_t *)&(client_buff->policy_req);

	// Set hdr info
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// Assume DPS Client's IP to be the Sender's IP
	policy_req->dps_client_addr = *((ip_addr_t *)senders_addr);

	dps_log_debug(DpsProtocolLogLevel, "Plcy Req len %d", pkt_len);

	dump_pkt((uint8_t *)recv_buff, pkt_len+DPS_PKT_HDR_LEN);

	buff += DPS_PKT_HDR_LEN;

	// Go to the end of buffer
	endpoint_info = &policy_req->dst_endpoint;
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
			case ENDPOINT_INFO_TLV:
				buff += dps_get_endpoint_info_tlv(buff, endpoint_info);
				endpoint_info++;
				break;
			case SERVICE_LOC_TLV:
			case REPLY_SERVICE_LOC_TLV:
				// Update the DPS Client IP
				buff += dps_get_svcloc_tlv(buff, &policy_req->dps_client_addr);
				break;
		}
	}

	dump_client_info(client_buff);

	dps_send_to_protocol_client((void *)client_buff);

	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;
}

void dps_bit_set(uint8_t *plcy, uint8_t idx)
{
	*plcy |= (1 << idx);
}

void dps_bit_unset(uint8_t *plcy, uint8_t idx)
{
	*plcy &= (1 << idx);
}


/*
 ******************************************************************************
 * dps_get_policy_type_tlv                                                *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The policy rules
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_get_policy_type_tlv(uint8_t *buff, dps_policy_t *client_data)
{
	dps_tlv_hdr_t tlv_hdr, plcy_type_hdr;
	uint16_t i = 0, j = 0, action, tot_num_of_rules = 0, num_of_rules = 0;
	uint8_t *buffptr = buff; //**plcy;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	// tlv header with tlv policy_type
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	//Not really a tlv hdr but info is encoded in this way
	buff += dps_get_tlv_hdr(buff, &plcy_type_hdr);
	client_data->version = DPS_GET_TLV_VER(&(plcy_type_hdr));

	client_data->policy_type = DPS_GET_TLV_TYPE(&(plcy_type_hdr));
	

	if (client_data->policy_type == DPS_POLICY_TYPE_CONNECTIVITY)
	{
		tot_num_of_rules = DPS_GET_TLV_LEN(&(plcy_type_hdr));
		dps_log_debug(DpsProtocolLogLevel, "Tot rules %d",tot_num_of_rules); 

		while (j < tot_num_of_rules)
		{
			// Get the action permit/deny and number of rules
			action = ntohs(*((uint16_t *)buff));
			buff += SZ_OF_INT16;
			num_of_rules = ntohs(*((uint16_t *)buff));
			buff += SZ_OF_INT16;
			if (action == DPS_DVG_PERMIT)
			{
				dps_log_debug(DpsProtocolLogLevel, "Permit rules %d",num_of_rules); 
				client_data->vnid_policy.num_permit_rules = num_of_rules;
			}
			else
			{
				dps_log_debug(DpsProtocolLogLevel, "Deny rules %d",num_of_rules); 
				client_data->vnid_policy.num_deny_rules = num_of_rules;
			}
			for (i = 0; i < num_of_rules; i++, j++)
			{
				client_data->vnid_policy.src_dst_vnid[j].svnid = ntohl(*((uint32_t *)buff));
				buff += SZ_OF_INT32;
				client_data->vnid_policy.src_dst_vnid[j].dvnid = ntohl(*((uint32_t *)buff));
				buff += SZ_OF_INT32;
				// dps_set_policy_matrix(plcy, action,
				//                      client_data->dvg_policy.src_dst_dvg[j].sdvg,
				//                      client_data->dvg_policy.src_dst_dvg[j].ddvg);
			}
		}
	}
	dps_log_info(DpsProtocolLogLevel, "Exit");
	return (buff - buffptr);
}

/*
 ******************************************************************************
 * dps_get_policy_tlv                                                     *//**
 *
 * \brief TBD.
 *
 * \param[in] buff - Points to the beginning of policy data
 * \param[in] client_plcy_info - To be filled in after parsing the policy tlv
 * \param[in] domain - The domain the policy belongs to.
 *
 * \retval  The number of bytes processed
 *
 ******************************************************************************
 */
static uint32_t dps_get_policy_tlv(uint8_t *buff, dps_policy_info_t *client_plcy_info)
{
	uint8_t *bufptr, *recvbuf; 
	dps_tlv_hdr_t tlv_hdr;

	recvbuf = buff;
	buff += dps_get_tlv_hdr(buff, &tlv_hdr);
	bufptr = buff;
	
	dps_log_debug(DpsProtocolLogLevel, "Policy len %d",
	              DPS_GET_TLV_LEN((&tlv_hdr)));
	while ((buff - bufptr) < DPS_GET_TLV_LEN((&tlv_hdr)))
	{
		dps_log_debug(DpsProtocolLogLevel, "Process policy rules enter");
		buff += dps_get_policy_id_tlv(buff, client_plcy_info);
		buff += dps_get_policy_type_tlv(buff, &client_plcy_info->dps_policy);
		dps_log_debug(DpsProtocolLogLevel, "Process policy rules exit %d", (buff - bufptr));
	}
	return (buff - recvbuf);
	
}


/*
 ******************************************************************************
 * dps_process_policy_reply                                               *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_policy_reply(void *recv_buff, void *senders_addr)
{
	dps_policy_info_t *plcy_info;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	dps_tlv_hdr_t tlv_hdr;
	dps_endpoint_loc_reply_t *endpoint_reply;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, policy_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);
	// Stop retranmsit timer
	dps_stop_retransmit_timer(client_buff);

	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;
	
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case ENDPOINT_LOC_TLV:
			    //Get endpoint loc reply tlv
			    endpoint_reply = &(client_buff->policy_reply.dst_endpoint_loc_reply);
			    buff += dps_get_endpoint_loc_reply_tlv(buff, endpoint_reply);
			    break;

		    case POLICY_TLV:
			    plcy_info = (dps_policy_info_t *)&(client_buff->policy_reply.dps_policy_info);
			    buff += dps_get_policy_tlv(buff, plcy_info);
			    break;
		    
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_internal_gw_reply                                            *//**
 *
 * \brief This function is called for processing both DPS_INTERNAL_GW_REPLY and
 *        DPS_UNSOLICITED_INTERNAL_GW_REPLY.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_process_internal_gw_reply(void *recv_buff, void *senders_addr)
{
	dps_internal_gw_t *gw_info;
	dps_client_data_t *client_buff;
	dps_tlv_hdr_t tlv_hdr;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	uint32_t *gw_list;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, gw_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	buff += DPS_PKT_HDR_LEN;
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// The rexmit timer was started as a result of sending internal_gw_req msg.
	// A reply was received so stop the retranmsit timer.
	if (client_buff->hdr.type == DPS_INTERNAL_GW_REPLY)
		dps_stop_retransmit_timer(client_buff);

	gw_info = &client_buff->internal_gw_list;
	gw_list = gw_info->gw_list;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{ 
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case IPV4_ADDR_LIST_TLV:
			    buff += dps_get_ipv4_list_tlv(buff, gw_list);
			    gw_info->num_v4_gw = ((DPS_GET_TLV_LEN(&tlv_hdr))/DPS_IP4_ADDR_LEN);				
			    gw_list += gw_info->num_v4_gw;
			break;	
		    case IPV6_ADDR_LIST_TLV:
			    buff += dps_get_ipv6_list_tlv(buff, (uint8_t *)gw_list);
			    gw_info->num_v6_gw = (DPS_GET_TLV_LEN(&tlv_hdr)/DPS_IP6_ADDR_LEN);
			break;	
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
		
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);
    // Send a DPS_GENERAL_ACK msg to the Server if required
	if (client_buff->hdr.type == DPS_UNSOLICITED_INTERNAL_GW_REPLY)
	    dps_client_create_general_ack(client_buff);
error:	
    dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_gen_msg_req                                                *//**
 *
 * \brief This function processes the general msg request. Based on the msg type
 *        the appropriate response is sent by the DPS.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_gen_msg_req(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	dps_gen_msg_req_t *msg_req = NULL;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	msg_req = (dps_gen_msg_req_t *)&(client_buff->gen_msg_req);

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);

	dps_log_debug(DpsProtocolLogLevel, "General Msg Req len %d", pkt_len);
	
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);
	msg_req->dps_client_addr = *((ip_addr_t *)senders_addr);
	
	buff += DPS_PKT_HDR_LEN;
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case SERVICE_LOC_TLV:
		    case REPLY_SERVICE_LOC_TLV:
			    buff += dps_get_svcloc_tlv(buff, &msg_req->dps_client_addr);
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	if (client_buff->hdr.type == DPS_CTRL_PLANE_HB)
	{
		dps_client_create_general_ack(client_buff);
	}
	else
	{
		dps_send_to_protocol_client((void *)client_buff);
	}

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;
}



/*
 ******************************************************************************
 * dps_process_bcast_list_reply                                            *//**
 *
 * \brief This function is called for both DPS_BCAST_LIST_REPLY and 
 *        DPS_UNSOLICITED_BCAST_LIST_REPLY. In the case of unsolicited_reply the
 *        DPS pushes the list of DSs in the bcast domain.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_process_bcast_list_reply(void *recv_buff, void *senders_addr)
{
	dps_pkd_tunnel_list_t *switch_info;
	dps_client_data_t *client_buff;
	dps_tlv_hdr_t tlv_hdr;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	uint32_t *switch_list;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, bcast_list_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	buff += DPS_PKT_HDR_LEN;
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// The rexmit timer was started as a result of sending
	// a bcast_list_req. A reply was received so stop the retranmsit timer
	if (client_buff->hdr.type == DPS_BCAST_LIST_REPLY)
		dps_stop_retransmit_timer(client_buff);

	switch_info = &client_buff->dove_switch_list;
	switch_list = switch_info->tunnel_list;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{ 
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case IPV4_ADDR_LIST_TLV:
			    buff += dps_get_ipv4_list_tlv(buff, switch_list);
			    switch_info->num_v4_tunnels = ((DPS_GET_TLV_LEN(&tlv_hdr))/DPS_IP4_ADDR_LEN);				
			    switch_list += switch_info->num_v4_tunnels;
			break;	
		    case IPV6_ADDR_LIST_TLV:
			    buff += dps_get_ipv6_list_tlv(buff, (uint8_t *)switch_list);
			    switch_info->num_v6_tunnels = (DPS_GET_TLV_LEN(&tlv_hdr)/DPS_IP6_ADDR_LEN);
			break;	
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
		
	}

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);
	
	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);

	// Send a DPS_GENERAL_ACK msg to the Server if required
	if (client_buff->hdr.type == DPS_UNSOLICITED_BCAST_LIST_REPLY)
	{
		dps_client_create_general_ack(client_buff);
	}
error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

static uint32_t
dps_get_bulk_policy_xfer(uint8_t *buff,  dps_bulk_vnid_policy_t *client_vnid_policy)
{
	uint32_t i,j;
	uint8_t *bufptr = buff;
	src_dst_vnid_t *policy_rules = client_vnid_policy->src_dst_vnid;

	client_vnid_policy->num_permit_rules = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;
	client_vnid_policy->num_deny_rules = ntohs(*((uint16_t *)buff));
	buff += SZ_OF_INT16;

	for (i = 0, j = 0; i < client_vnid_policy->num_permit_rules; i++, j++)
	{
		policy_rules[j].svnid = ntohl(*((uint32_t *)buff));
		buff += SZ_OF_INT32;
		policy_rules[j].dvnid = ntohl(*((uint32_t *)buff));
		buff += SZ_OF_INT32;

	}
	for (i = 0; i < client_vnid_policy->num_deny_rules; i++, j++)
	{
		policy_rules[j].svnid = ntohl(*((uint32_t *)buff));
		buff += SZ_OF_INT32;
		policy_rules[j].svnid = ntohl(*((uint16_t *)buff));
		buff += SZ_OF_INT32;

	}
	
	return (bufptr - buff);
}

/*
 ******************************************************************************
 * dps_process_bulk_policy_xfer                                           *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 * \todo Determine if needed or not
 *
 ******************************************************************************
 */

uint32_t dps_process_bulk_policy_xfer(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	dps_client_hdr_t hdr;
	uint32_t pkt_len;
	uint8_t  *buff = (uint8_t *)recv_buff; 

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, bulk_policy_xfer is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);

	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	if (client_buff->hdr.type == DPS_VNID_POLICY_LIST_REPLY)
		dps_stop_retransmit_timer(client_buff);

	buff += DPS_PKT_HDR_LEN;

	buff += dps_get_bulk_policy_xfer(buff,  &client_buff->bulk_vnid_policy);

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);

	// Send a DPS_GENERAL_ACK msg to the Server if required
	if (client_buff->hdr.type == DPS_UNSOLICITED_VNID_POLICY_LIST)
		dps_client_create_general_ack(client_buff);

	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;

}

/*
 ******************************************************************************
 * dps_process_gw_list                                                    *//**
 *
 * \brief This func is used to process unsolicited external gw list to the dove 
 *        switches.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_process_gw_list(void *recv_buff, void *senders_addr)
{
	dps_tunnel_list_t *gw_info;
	dps_client_data_t *client_buff;
	dps_tlv_hdr_t tlv_hdr;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, gw_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	buff += DPS_PKT_HDR_LEN;
	// To get the exact number of ipv4/ipv6 addr we need to parse deep into the pkt. To avoid
	// this problem use the default as v4 entries and find out how many v4/vnid combinations are present.
	// Then multiply it by the number of extra bytes needed for ipv6 address which is 12 bytes
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + 
	                                                       ((pkt_len/DPS_IP4_TUNNEL_INFO_LEN)*sizeof(dps_tunnel_endpoint_t)))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	dps_log_debug(DpsProtocolLogLevel, "Allocateded memory: %d",
	              sizeof(dps_client_data_t) + ((pkt_len/DPS_IP4_TUNNEL_INFO_LEN)*sizeof(dps_tunnel_endpoint_t)));
	              
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	// The rexmit timer was started as a result of sending external or vlan gateway req msg.
	// A reply was received so stop the retranmsit timer.
	if ((client_buff->hdr.type == DPS_EXTERNAL_GW_LIST_REPLY) || 
	    (client_buff->hdr.type == DPS_VLAN_GW_LIST_REPLY) ||
	    (client_buff->hdr.type == DPS_MCAST_CTRL_GW_REPLY)
	   )
	{
		dps_stop_retransmit_timer(client_buff);
	}

	gw_info = &client_buff->tunnel_info;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{ 
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case  TUNNEL_LIST_TLV:
			    buff += dps_get_tunnel_list_tlv(buff, gw_info->tunnel_list, &gw_info->num_of_tunnels);
			break;	

  		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			break;
		}
		
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);
	// Send a DPS_GENERAL_ACK msg to the Server if required
	if ((client_buff->hdr.type == DPS_UNSOLICITED_EXTERNAL_GW_LIST) ||
	    (client_buff->hdr.type == DPS_UNSOLICITED_VLAN_GW_LIST)
	   )
	{
	  dps_client_create_general_ack(client_buff);
	}

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_tunnel_reg_dereg                                               *//**
 *
 * \brief This func is used to process unsolicited external gw list to the dove 
 *        switches.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_process_tunnel_reg_dereg(void *recv_buff, void *senders_addr)
{
	dps_tunnel_list_t *tunnel_info;
	dps_client_data_t *client_buff;
	dps_tlv_hdr_t tlv_hdr;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, gw_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	buff += DPS_PKT_HDR_LEN;
	// To get the exact number of ipv4/ipv6 addr we need to parse deep into the pkt. To avoid
	// this problem use the default as v4 entries and find out how many v4/vnid combinations are present.
	// Then multiply it by the number of extra bytes needed for ipv6 address which is 12 bytes
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + 
	                                                       ((pkt_len/DPS_IP4_TUNNEL_INFO_LEN)*sizeof(dps_tunnel_endpoint_t)))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	dps_log_debug(DpsProtocolLogLevel, "Allocateded memory: %d",
	              sizeof(dps_client_data_t) + ((pkt_len/DPS_IP4_TUNNEL_INFO_LEN)*sizeof(dps_tunnel_endpoint_t)));
	              
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	client_buff->tunnel_reg_dereg.dps_client_addr = *((ip_addr_t *)senders_addr);

	tunnel_info = &client_buff->tunnel_reg_dereg.tunnel_info;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{ 
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case  TUNNEL_LIST_TLV:
			    buff += dps_get_tunnel_list_tlv(buff, tunnel_info->tunnel_list, &tunnel_info->num_of_tunnels);
			break;	
			case SERVICE_LOC_TLV:
			case REPLY_SERVICE_LOC_TLV:
				// Update the DPS Client IP
				buff += dps_get_svcloc_tlv(buff, &client_buff->tunnel_reg_dereg.dps_client_addr);
				break;
  		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			break;
		}
		
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);
 
error:	
    dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_vm_migration_event                                         *//**
 *
 * \brief This routine processes the vm migration information sent from the Dove.
 *        switch. It contains inforamtion about the src vm location and information 
 *        about the destination VM which no longer exists on the Dove switch that
 *        sent this message. 
 *
 * \param[in] recv_buff - A pointer to the vm migration message.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_vm_migration_event(void *recv_buff,
                                               void *senders_addr)
{
	dps_vm_migration_event_t *vm_migration;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;	
	dps_tlv_hdr_t tlv_hdr;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	vm_migration = (dps_vm_migration_event_t *)&(client_buff->vm_migration_event);

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);
	
	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case ENDPOINT_LOC_TLV:
			    buff += dps_get_endpoint_loc_reply_tlv(buff, &vm_migration->src_vm_loc);
			    break;
		    case ENDPOINT_INFO_TLV:
			    buff += dps_get_endpoint_info_tlv(buff, &vm_migration->migrated_vm_info);
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	dump_client_info(client_buff);
	// call the client
	dps_send_to_protocol_client((void *)client_buff);

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_mcast_sender_reg_dereg                                     *//**
 *
 * \brief This routine processes mcast sender registration and deregistration.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 * \todo Determine if needed or not
 *
 ******************************************************************************
 */

uint32_t dps_process_mcast_sender_reg_dereg(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	dps_mcast_sender_t *sender_reg = NULL;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	sender_reg = (dps_mcast_sender_t *)&(client_buff->mcast_sender);

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);

	dps_log_debug(DpsProtocolLogLevel, "Mcast Sender Pkt Type %d len %d", 
	              client_buff->hdr.type, pkt_len);
	
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);
	sender_reg->dps_client_addr = *((ip_addr_t *)senders_addr);
	
	buff += DPS_PKT_HDR_LEN;
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case IP_ADDR_TLV:
			    buff += dps_get_ip_tlv(buff, &sender_reg->tunnel_endpoint);
			    break;
			case ENDPOINT_INFO_TLV:
				buff += dps_get_endpoint_info_tlv(buff, &sender_reg->mcast_src_vm);
				break;  
		    case MCAST_ADDR_MAC_TLV: 
  		    case MCAST_ADDR_V4_TLV: 
		    case MCAST_ADDR_V6_TLV: 
			    buff += dps_get_mcast_addr_tlv(buff, &sender_reg->mcast_addr);
			    break;
			case SERVICE_LOC_TLV:
		    case REPLY_SERVICE_LOC_TLV:
			    buff += dps_get_svcloc_tlv(buff, &sender_reg->dps_client_addr);
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	dump_client_info(client_buff);
	dps_send_to_protocol_client((void *)client_buff);

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;

}

/*
 ******************************************************************************
 *  dps_process_mcast_receiver_join                                       *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 * \todo Determine if needed or not
 *
 ******************************************************************************
 */

uint32_t dps_process_mcast_receiver_join_leave(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;
	dps_client_hdr_t hdr;
	dps_mcast_receiver_t *receiver_reg = NULL;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	// Get the header len, mcast receiver join/leave is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);

	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}
	
	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	buff += DPS_PKT_HDR_LEN;

	receiver_reg = (dps_mcast_receiver_t *)&(client_buff->mcast_receiver);

	receiver_reg->dps_client_addr = *((ip_addr_t *)senders_addr);

	dps_log_debug(DpsProtocolLogLevel, "Mcast Receiver Join Pkt Type %d len %d", 
	              client_buff->hdr.type, pkt_len);

	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case IP_ADDR_TLV:
			    // Tunnel Endpoint
			    buff += dps_get_ip_tlv(buff, &receiver_reg->tunnel_endpoint);
			    break;
		    case MCAST_GRP_REC_TLV: 
			    buff += dps_get_mcast_grp_rec_tlv(buff, &receiver_reg->mcast_group_rec);
			    break;
			case SERVICE_LOC_TLV:
		    case REPLY_SERVICE_LOC_TLV:
			    buff += dps_get_svcloc_tlv(buff, &receiver_reg->dps_client_addr);
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	dump_client_info(client_buff);
	dps_send_to_protocol_client((void *)client_buff);

error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 *  dps_process_mcast_receiver_ds_list                                    *//**
 *
 * \brief Processing of DPS_MCAST_RECEIVER_DS_LIST message type
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Message Hdr 20 bytes                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     MCAST_ADDR_MAC_TLV/MCAST_ADDR_V4_TLV/MCAST_ADDR_V6_TLV    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  MCAST_RECEIVER_LIST_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  MCAST_RECEIVER_LIST_TLV                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           .                                   |
    |                           .                                   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                  MCAST_RECEIVER_LIST_TLV                      | 
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 * \todo Determine if needed or not
 *
 ******************************************************************************
 */

uint32_t dps_process_mcast_receiver_ds_list(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	dps_client_hdr_t hdr;
	dps_mcast_receiver_ds_list_t *mcast_recvr_info = NULL;
	dps_pkd_tunnel_list_t *mcast_recvr_list = NULL;
	uint8_t *buff = (uint8_t *)recv_buff;
	uint8_t *tmp_char_ptr;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t) + pkt_len)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	dps_log_debug(DpsProtocolLogLevel, "Mcast Recvr List Len %d", pkt_len); 

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);	
	client_buff->hdr = hdr;

	mcast_recvr_info = (dps_mcast_receiver_ds_list_t *)&(client_buff->mcast_receiver_ds_list);
	mcast_recvr_list = mcast_recvr_info->mcast_recvr_list;

	buff += DPS_PKT_HDR_LEN;
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case MCAST_ADDR_MAC_TLV: 
  		    case MCAST_ADDR_V4_TLV: 
		    case MCAST_ADDR_V6_TLV: 
			    buff += dps_get_mcast_addr_tlv(buff, &mcast_recvr_info->mcast_addr);
			    break;
		    case VNID_TUNNEL_LIST_TLV:
     			mcast_recvr_info->num_of_rec++;
			    tmp_char_ptr = (uint8_t *)mcast_recvr_list;
			    buff += dps_get_vnid_tunnel_list_tlv(buff, mcast_recvr_list);
			    tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[mcast_recvr_list->num_v4_tunnels+(mcast_recvr_list->num_v6_tunnels << 2)]);
			    mcast_recvr_list = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
			    break;
		    default:
			    dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	}

	dps_send_to_protocol_client((void *)client_buff);
	dps_client_create_general_ack(client_buff);
error:
	dump_client_info(client_buff);

	dps_free_buff((uint8_t *)client_buff);


	dps_log_info(DpsProtocolLogLevel,"Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_general_ack                                               *//**
 *
 * \brief This routine processes an ack received for unsolicited msgs sent from
 *        the DPS. The Option1 field in the header contains the message type it
 *        is acking
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 * \todo Determine if needed or not
 *
 ******************************************************************************
 */

uint32_t dps_process_general_ack(void *recv_buff, void *senders_addr)
{

	dps_client_data_t *client_buff;
	uint32_t ret_status = DPS_SUCCESS;	

	dps_log_info(DpsProtocolLogLevel, "Enter");

	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"Exit: DPS_ERROR dps_alloc_buff failed");
		return DPS_ERROR;
	}

	dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);

	// Stop retransmit timer
	ret_status = dps_stop_retransmit_timer(client_buff);

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);

	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return ret_status;

}

/*
 ******************************************************************************
 * dps_process_policy_invalidate                                          *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_policy_invalidate(void *recv_buff,
                                              void *senders_addr)
{
	dps_log_notice(DpsProtocolLogLevel,"Not implemented");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_process_addr_resolve                                               *//**
 *
 * \brief This msg is sent from the DPS in response to not being able to find
 *        the location of a requested VMs address. The DS on processing this msg
 *        should send an arp request to the registered VMs to see if any of VMs
 *        have the ip address.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_addr_resolve(void *recv_buff, void *senders_addr)
{
	dps_endpoint_loc_req_t *endpoint_req = NULL;
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_tlv_hdr_t tlv_hdr;	
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(sizeof(dps_client_data_t))) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	endpoint_req = (dps_endpoint_loc_req_t *)&(client_buff->address_resolve);
	// Assume DPS Client to be the Sender's Location
	endpoint_req->dps_client_addr = *((ip_addr_t *)senders_addr);

	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &client_buff->hdr);

	dps_log_debug(DpsProtocolLogLevel, "packet len %d", pkt_len);
	buff += DPS_PKT_HDR_LEN;

	// Go to the end of buffer
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
			case VMAC_TLV:
				buff += dps_get_vmac_tlv(buff, endpoint_req->mac);
				break;
			case IP_ADDR_TLV:
				buff += dps_get_ip_tlv(buff, &endpoint_req->vm_ip_addr);
				break;
			case EUID_TLV:
				buff += dps_get_euid_tlv(buff, &endpoint_req->vnid);
				break;
			case REPLY_SERVICE_LOC_TLV:
			case SERVICE_LOC_TLV:
				// Update the DPS Client Location
				buff += dps_get_svcloc_tlv(buff, &endpoint_req->dps_client_addr);
				break;
			default:
				dps_log_info(DpsProtocolLogLevel, "Invalid TLV");
			    goto error;
			    break;
		}
	
	}

	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);

	dump_client_info(client_buff);

	dps_send_to_protocol_client((void *)client_buff);
	// Send a general ack to DPS
	dps_client_create_general_ack(client_buff);
error:
	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");

	return DPS_SUCCESS;

}

/*
 ******************************************************************************
 * dps_process_invalidate_vm                                              *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_vm_loc_info(void *recv_buff, void *senders_addr)
{
	dps_client_data_t *client_buff;
	uint32_t pkt_len;
	dps_client_hdr_t hdr;
	dps_tlv_hdr_t tlv_hdr;
	dps_epri_t *epri;
	uint8_t *buff = (uint8_t *)recv_buff;

	dps_log_info(DpsProtocolLogLevel, "Enter");

	// Get the header len, policy_reply is of variable len so have to alloc the client buffer
	pkt_len = dps_get_pkt_hdr((dps_pkt_hdr_t *)recv_buff, &hdr);
	
	if ((client_buff = (dps_client_data_t *)dps_alloc_buff(DPS_MAX_BUFF_SZ)) == NULL)
	{
		dps_log_error(DpsProtocolLogLevel,"No memory");
		return DPS_ERROR;
	}

	memcpy(&(client_buff->hdr), &hdr, sizeof(dps_client_hdr_t));
	client_buff->hdr.reply_addr = *((ip_addr_t *)senders_addr);
	
	//Increment by pkt hdr len
	buff += DPS_PKT_HDR_LEN;
	
	while (buff < ((uint8_t *)recv_buff + DPS_PKT_HDR_LEN + pkt_len))
	{
		dps_get_tlv_hdr(buff, &tlv_hdr);
		switch (DPS_GET_TLV_TYPE(&tlv_hdr))
		{
		    case EPRI_TLV:
			    epri = &(client_buff->vm_invalidate_msg.epri);
			    buff += dps_get_epri_tlv(buff, epri);
			    break;

		    default:
			    dps_log_info(DpsProtocolLogLevel, "TLV type %d not suppoted len %d", 
			                 DPS_GET_TLV_TYPE(&tlv_hdr), DPS_GET_TLV_LEN(&tlv_hdr));
			    buff += (DPS_GET_TLV_LEN(&tlv_hdr) + DPS_TLV_HDR_LEN);
			    break;
		}
	}

	dump_client_info(client_buff);

	// call the client
	dps_send_to_protocol_client((void *)client_buff);

	dps_client_create_general_ack(client_buff);

	dps_free_buff((uint8_t *)client_buff);

	dps_log_info(DpsProtocolLogLevel, "Exit");
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_noop_func                                                          *//**
 *
 * \brief This function is a holder for a msg type that is never seen on the wire.
 *        It is currently used for DPS_GET_DCS_NODE msg type and is only seen
 *        internally on the DS client.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
static uint32_t dps_noop_func (void *recv_buff, void *senders_addr)
{
	return DPS_SUCCESS;
}

static void dps_construct_noop_func(dps_client_data_t *generic_data)
{
	return;
}

/*
 ******************************************************************************
 * dps_process_addr_reply                                                 *//**
 *
 * \brief TBD.
 *
 * \param[in] recv_buff - TBD.
 * \param[in] senders_addr - The senders address
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

static uint32_t dps_process_addr_reply(void *recv_buff, void *senders_addr)
{
	dps_log_notice(DpsProtocolLogLevel,"Not implemented");
	return DPS_SUCCESS;
}

/**
 * \brief The jump table functions are indexed using dps_client_req_type.
 *        When the corresponding functions are added they should be in the
 *        appropriate indexed location. The dps_send_func_tbl is used by
 *        the DOVE switch client or the DPS server client to send a
 *        packet out on to the network
 */

static dps_func_tbl_t dps_send_func_tbl[] = {
	{NULL, NULL},
	{"Endpoint Loc Req", dps_send_endpoint_loc_req},
	{"Endpoint Loc Reply", dps_send_endpoint_loc_reply},
	{"Policy Req", dps_send_policy_req},
	{"Policy Reply", dps_send_policy_reply},
	{"Policy Invalidate", dps_send_policy_invalidate},
	{"Endpoint Update", dps_send_endpoint_update},
	{"Endpoint Update Reply", dps_send_endpoint_update_reply},
	{"Addr Resolve", dps_send_addr_resolve},
	{"Addr Reply", dps_send_addr_reply},
	{"Internal Gw Req", dps_send_gen_msg_req},
	{"Internal Gw Reply", dps_send_internal_gw_reply},
	{"Bulk Policy Xfer", dps_send_bulk_policy_xfer},
	{"Bcast List Req", dps_send_gen_msg_req},
	{"Bcast List Reply", dps_send_bcast_list_reply},
	{"VM Migration Info", dps_send_vm_migration_event},
	{"Mcast Sender Registration", dps_send_mcast_sender_reg_dereg},
	{"Mcast Sender De-Registration", dps_send_mcast_sender_reg_dereg},
	{"Mcast Receiver Join", dps_send_mcast_receiver_join_leave},
 	{"Mcast Receiver Leave", dps_send_mcast_receiver_join_leave},
 	{"Mcast Receiver Dove Switch List", dps_send_mcast_receiver_ds_list},
	{"Unsolicited Bcast List Reply",dps_send_bcast_list_reply},
	{"Unsolicited Internal Gw Reply", dps_send_internal_gw_reply},
	{"General Ack", dps_send_general_ack},
	{"Unsolicited External Gw List", dps_send_gw_list},
	{"Unsolicited Vlan Gw List", dps_send_gw_list},
	{"Register Tunnel", dps_send_tunnel_reg_dereg},
	{"Deregister Tunnel", dps_send_tunnel_reg_dereg},
	{"Reg/Dereg Tunnel Ack", dps_send_general_ack},
	{"External Gw List Req", dps_send_gen_msg_req},
	{"External Gw List Reply", dps_send_gw_list},
	{"Vlan Gw List Req", dps_send_gen_msg_req},
	{"Vlan Gw List Reply", dps_send_gw_list},
	{"Unsolicited Endpoint Loc Reply", dps_send_endpoint_loc_reply},
	{"Vnid Policy List Req", dps_send_gen_msg_req},
	{"Vnid Policy List Reply", dps_send_bulk_policy_xfer},
	{"Mcast Ctrl Gw Req", dps_send_gen_msg_req},
	{"Mcast Ctrl Gw Reply", dps_send_gw_list},
	{"Unsolicited Vnid Del Req", dps_send_gen_msg_req},
	{"Control Plane heart beat", dps_send_gen_msg_req},
	{"New DCS Node Req", dps_noop_func},
	{"Unsolicited VM Location Info", dps_send_vm_loc_info},
};

/**
 * \brief The jump table dps_recv_func_tbl is used to process packets
 *        received from the network. The packets are received by the DOVE
 *        switch or the DPS server
 */

static dps_func_tbl_t dps_recv_func_tbl[] = {
	{NULL, NULL},
	{"Endpoint Loc Req", dps_process_endpoint_loc_req},
	{"Endpoint Loc Reply", dps_process_endpoint_loc_reply},
	{"Policy Req", dps_process_policy_req},
	{"Policy Reply", dps_process_policy_reply},
	{"Policy Invalidate", dps_process_policy_invalidate},
	{"Endpoint Update", dps_process_endpoint_update},
	{"Endpoint Update Reply", dps_process_endpoint_update_reply},
	{"Addr Resolve", dps_process_addr_resolve},
	{"Addr Reply", dps_process_addr_reply},
	{"Internal Gw Req", dps_process_gen_msg_req},
	{"Internal Gw Reply", dps_process_internal_gw_reply},
	{"Bulk Policy Xfer", dps_process_bulk_policy_xfer},
	{"Bcast List Req", dps_process_gen_msg_req},
	{"Bcast List Reply", dps_process_bcast_list_reply},
	{"VM Migration Info", dps_process_vm_migration_event},
	{"Mcast Sender Registration", dps_process_mcast_sender_reg_dereg},
	{"Mcast Sender De-Registration", dps_process_mcast_sender_reg_dereg},
	{"Mcast Receiver Join", dps_process_mcast_receiver_join_leave},
 	{"Mcast Receiver Leave", dps_process_mcast_receiver_join_leave},
 	{"Mcast Receiver Dove Switch List", dps_process_mcast_receiver_ds_list},
	{"Unsolicited Bcast List Reply",dps_process_bcast_list_reply},
	{"Unsolicited Internal Gw Reply", dps_process_internal_gw_reply},
	{"General Ack", dps_process_general_ack},
	{"Unsolicited External Gw List", dps_process_gw_list},
	{"Unsolicited Vlan Gw List", dps_process_gw_list},
	{"Register Tunnel", dps_process_tunnel_reg_dereg},
	{"Deregister Tunnel", dps_process_tunnel_reg_dereg},
	{"Reg/Dereg Tunnel Ack", dps_process_general_ack},
	{"External Gw List Req", dps_process_gen_msg_req},
	{"External Gw List Reply", dps_process_gw_list},
	{"Vlan Gw List Req", dps_process_gen_msg_req},
	{"Vlan Gw List Reply", dps_process_gw_list},
	{"Unsolicited Endpoint Loc Reply", dps_process_endpoint_loc_reply},
	{"Vnid Policy List Req", dps_process_gen_msg_req},
	{"Vnid Policy List Reply", dps_process_bulk_policy_xfer},
	{"Mcast Ctrl Gw Req", dps_process_gen_msg_req},
	{"Mcast Ctrl Gw Reply", dps_process_gw_list},
	{"Unsolicited Vnid Del Req", dps_process_gen_msg_req},
	{"Control Plane Heart Beat", dps_process_gen_msg_req},
	{"New DCS Node Req", dps_noop_func},
	{"Unsolicited VM Location Info", dps_process_vm_loc_info},
};

const char *dps_msg_name(uint8_t pkt_type)
{
	const char *msg_str;
	msg_str = dps_send_func_tbl[pkt_type].dps_fn_name;
	return(msg_str);
}

/**
 * \brief This routine constructs the Endpoint location_reply
 *
 * \param[in] location_request The Endpoint Location Request
 */

static void dps_construct_reply_endpoint_loc_req(dps_client_data_t *location_request)
{
	dps_client_data_t *location_reply = location_request;

	// The Header remains mostly the same
	location_reply->hdr.type = DPS_ENDPOINT_LOC_REPLY;

	// Change the body
	// TODO: The 1st 4 elements of the location_request and location_reply
	//       are the same. Nothing to do there.

	location_reply->endpoint_loc_reply.vnid = 0;
	location_reply->endpoint_loc_reply.version = 0;
	location_reply->endpoint_loc_reply.tunnel_info.num_of_tunnels = 0;
	return;
}

/**
 * \brief This routine constructs the Policy Reply
 *
 * \param[in] policy_req The Policy Request
 */

static void dps_construct_reply_policy_req(dps_client_data_t *policy_req)
{
	dps_client_data_t *policy_reply = policy_req;
	dps_endpoint_info_t dst_endpoint_info;

	// The Header remains mostly the same
	policy_reply->hdr.type = DPS_POLICY_REPLY;

	// Change the body
	memcpy(&dst_endpoint_info,
	       &policy_req->policy_req.dst_endpoint,
	       sizeof(dps_endpoint_info_t));

	policy_reply->policy_reply.dst_endpoint_loc_reply.vnid = policy_reply->hdr.vnid;
	policy_reply->policy_reply.dst_endpoint_loc_reply.version = 0;
	memcpy(&policy_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr,
	       &dst_endpoint_info.vm_ip_addr,
	       sizeof(ip_addr_t));
	memcpy(policy_reply->policy_reply.dst_endpoint_loc_reply.mac,
	       dst_endpoint_info.mac,
	       6);

	policy_reply->policy_reply.dps_policy_info.ttl = 0;
	policy_reply->policy_reply.dps_policy_info.policy_id = 0;
	policy_reply->policy_reply.dps_policy_info.version = 0;
	policy_reply->policy_reply.dps_policy_info.dps_policy.policy_type = DPS_POLICY_TYPE_CONNECTIVITY;
	policy_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_deny_rules = 0;
	policy_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_permit_rules = 0;

	return;

}

/**
 * \brief This routine constructs the Endpoint Update Reply
 *
 * \param[in] endpoint_update The Endpoint Update Request
 */

static void dps_construct_reply_endpoint_update(dps_client_data_t *endpoint_update)
{
	dps_client_data_t *endpoint_reply = endpoint_update;

	// The Header remains mostly the same
	endpoint_reply->hdr.type = DPS_ENDPOINT_UPDATE_REPLY;

	// Change the body
	// TODO: The bodies of endpoint update and reply are exactly the same
	//       Nothing to do here.

	return;
}

/**
 * \brief This routine constructs the Internal Gateway List
 *
 * \param[in] gw_request The Gateway Request
 */

static void dps_construct_reply_internal_gw_req(dps_client_data_t *gw_request)
{
	dps_client_data_t *gw_reply = gw_request;

	// The Header remains mostly the same
	gw_reply->hdr.type = DPS_INTERNAL_GW_REPLY;

	// TODO: Reconfirm
	gw_reply->internal_gw_list.num_v4_gw = 0;
	gw_reply->internal_gw_list.num_v6_gw = 0;

	return;
}

/**
 * \brief This routine constructs the Broadcast Reply
 *
 * \param[in] broadcast_req The Broadcast Request
 */

static void dps_construct_reply_bcast_list_req(dps_client_data_t *broadcast_req)
{
	dps_client_data_t *broadcast_reply = broadcast_req;

	// The Header remains mostly the same
	broadcast_reply->hdr.type = DPS_BCAST_LIST_REPLY;

	return;
}

/**
 * \brief This routine constructs the External Gateway List
 *
 * \param[in] gw_request The Gateway Request
 */

static void dps_construct_reply_external_gw_req(dps_client_data_t *gw_request)
{
	dps_client_data_t *gw_reply = gw_request;

	// The Header remains mostly the same
	gw_reply->hdr.type = DPS_EXTERNAL_GW_LIST_REPLY;

	gw_reply->tunnel_info.num_of_tunnels = 0;

	return;
}

/**
 * \brief This routine constructs the Vlan Gateway List
 *
 * \param[in] gw_request The Gateway Request
 */

static void dps_construct_reply_vlan_gw_req(dps_client_data_t *gw_request)
{
	dps_client_data_t *gw_reply = gw_request;

	// The Header remains mostly the same
	gw_reply->hdr.type = DPS_VLAN_GW_LIST_REPLY;

	gw_reply->tunnel_info.num_of_tunnels = 0;

	return;
}

/**
 * \brief This routine constructs the Policy List
 *
 * \param[in] plcy_request The Policy Request
 */

static void dps_construct_reply_policy_list_req(dps_client_data_t *plcy_request)
{
	dps_client_data_t *plcy_reply = plcy_request;

	// The Header remains mostly the same
	plcy_reply->hdr.type = DPS_VNID_POLICY_LIST_REPLY;

	return;
}


/**
 * \brief This routine constructs the general ack
 *
 * \param[in] generic_data Point to the general client information
 */

static void dps_construct_reply_generic_ack(dps_client_data_t *generic_data)
{
	dps_client_data_t *generic_reply = generic_data;
	if ((generic_reply->hdr.type == DPS_TUNNEL_REGISTER) ||
	    (generic_reply->hdr.type == DPS_TUNNEL_DEREGISTER) ||
	    (generic_reply->hdr.type == DPS_MCAST_SENDER_REGISTRATION) ||
	    (generic_reply->hdr.type == DPS_MCAST_SENDER_DEREGISTRATION) ||
	    (generic_reply->hdr.type == DPS_MCAST_RECEIVER_JOIN) ||
	    (generic_reply->hdr.type == DPS_MCAST_RECEIVER_LEAVE))
	{
		generic_reply->hdr.type = DPS_REG_DEREGISTER_ACK;
	}
	else
	{
		generic_reply->hdr.type = DPS_GENERAL_ACK;
	}
	return;
}

/**
 * \brief This routine constructs the reply to DPS_MCAST_CTRL_GW_REQ
 *
 * \param[in] gw_request The Gateway Request
 */

static void dps_construct_reply_mcast_ctrl_gw_req(dps_client_data_t *gw_request)
{
	dps_client_data_t *gw_reply = gw_request;

	// The Header remains mostly the same
	gw_reply->hdr.type = DPS_MCAST_CTRL_GW_REPLY;

	gw_reply->tunnel_info.num_of_tunnels = 0;

	return;
}

/**
 * \brief The jump table to construct a reply packet from the original
 *        requesting packet. Only packet type which have replies need
 *        to construct the reply messages. The original packet will
 *        be replaced with the reply packet and the error code provided
 */
static dps_construct_reply_func_t dps_construct_msg_reply_table[] = {
	{"None", NULL},
	{"Endpoint Loc Request", dps_construct_reply_endpoint_loc_req},
	{"Endpoint Loc Reply", NULL},
	{"Policy Req", dps_construct_reply_policy_req},
	{"Policy Reply", NULL},
	{"Policy Invalidate", NULL},
	{"Endpoint Update", dps_construct_reply_endpoint_update},
	{"Endpoint Update Reply", NULL},
	{"Addr Resolve", dps_construct_reply_generic_ack},
	{"Addr Reply", NULL},
	{"Internal Gw Req", dps_construct_reply_internal_gw_req},
	{"Internal Gw Reply", NULL},
	{"Bulk Policy Xfer", dps_construct_reply_generic_ack},
	{"Bcast List Req", dps_construct_reply_bcast_list_req},
	{"Bcast List Reply", NULL},
	{"VM Migration Info", NULL},
	{"Mcast Sender Registration",dps_construct_reply_generic_ack},
	{"Mcast Sender De-Registration", dps_construct_reply_generic_ack},
	{"Mcast Receiver Join", dps_construct_reply_generic_ack},
	{"Mcast Receiver Leave", dps_construct_reply_generic_ack},
	{"Mcast Receiver Dove Switch List", dps_construct_reply_generic_ack},
	{"Unsolicited Bcast List Reply",dps_construct_reply_generic_ack},
	{"Unsolicited Internal Gw Reply", dps_construct_reply_generic_ack},
	{"General Ack", NULL},
	{"Unsolicited External Gw List", dps_construct_reply_generic_ack},
	{"Unsolicited Vlan Gw List", dps_construct_reply_generic_ack},
	{"Tunnel Register", dps_construct_reply_generic_ack},
	{"Tunnel Deregister", dps_construct_reply_generic_ack},
	{"Tunnel Reg/Dereg Ack", dps_construct_reply_generic_ack},
	{"External Gw Req", dps_construct_reply_external_gw_req},
	{"External Gw Reply", NULL},
	{"Vlan Gw Req", dps_construct_reply_vlan_gw_req},
	{"Vlan Gw Reply", NULL},
	{"Unsolicited Endpoint Loc Reply",dps_construct_reply_generic_ack},
	{"Vnid Policy List Req",dps_construct_reply_policy_list_req},
	{"Vnid Policy List Reply", NULL},
	{"Mcast Ctrl Gw Req", dps_construct_reply_mcast_ctrl_gw_req},
	{"Mcast Ctrl Gw Reply", NULL},
	{"Unsolicited Vnid Del Req", dps_construct_reply_generic_ack},
	{"Control Plane Heart Beat", dps_construct_reply_generic_ack},
	{"New DCS Node Req", dps_construct_noop_func},
	{"VM Invalidate Msg", dps_construct_reply_generic_ack},
};

/*
 ******************************************************************************
 * dps_retransmit_callback                                                *//**
 *
 * \brief - This routine is called when retry timer expires. It is called with 
 *          the original packet that was being sent and the context that was
 *          given by the DPS client. The routine on receiving this callback
 *          calls the DPS client with the context.
 *
 * \param[in] status        RAW PKT Restransmission Status
 * \param[in] pRawPkt       Pointer to the packet that was retried.
 * \param[in] context       Context that was sent by the client
 * \param[in] owner         The owner of the request DPS(DPS) or Dove Switch (DPSA)
 *
 * \retval None
 *
 *
 ******************************************************************************
 */
void dps_retransmit_callback(raw_proto_retransmit_status_t status, char *pRawPkt, 
                             void *context, rpt_owner_t owner)
{
	dps_client_data_t data;
	dps_pkt_hdr_t *pkt_hdr = (dps_pkt_hdr_t *) pRawPkt;

	if (status == RAW_PROTO_MAX_NUM_RETRANSMIT_EXCEEDED)
	{
		memset((void *) (&data), 0, sizeof(data));
		dps_get_pkt_hdr(pkt_hdr, &data.hdr);
		dps_log_info(DpsProtocolLogLevel,
		              "Re-tranmission exceeded for Domain %d, PktType %d, Context %p, QID %d",
		              data.hdr.vnid, data.hdr.type, context, data.hdr.query_id);
#if !defined (DPS_SERVER)
		dps_req_new_dps_node(data.hdr.vnid);
#endif		
		if ((data.hdr.type < DPS_ENDPOINT_LOC_REQ) || (data.hdr.type >= DPS_MAX_MSG_TYPE))
		{
		  dps_pkt_stats_tbl[0].transmit_error++;
		  dps_log_error(DpsProtocolLogLevel, "Incorrect msg type %d", pkt_hdr->type);
		}
		else
		{
		  if (dps_construct_msg_reply_table[data.hdr.type].dps_reply_fn != NULL)
		  {
		      dps_construct_msg_reply_table[data.hdr.type].dps_reply_fn(&data);
		  }
		  data.context = (void *) context;
		  data.hdr.resp_status = DPS_NO_RESPONSE;
		  data.hdr.sub_type = pkt_hdr->type;
		  dps_send_to_protocol_client((void *) (&data));
		}

	}
	else if (status == RAW_PROTO_MAX_NUM_OUTSTANDING_PKTS_EXCEEDED)
	{
		dps_log_debug(DpsProtocolLogLevel, "Error: Max Number of Outstanding Packets Exceeded\n");
	}

}

/*
 ******************************************************************************
 * dps_process_rcvd_pkt                                                   *//**
 *
 * \brief This routine is called by a network entity that received a packet
 *        from the network Based on the type of packet the corresponding
 *        function is called to process packet and send it to the protocol
 *        client.
 *
 * \param[in] recv_buff - A pointer to a message that the socket received.
 *                        Type - dps_pkt_hdr_t
 * \param[in] cli_addr - The address of the sender in sockaddr_in format. The
 *                       addresses are in HOST ORDER at this point.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

uint32_t dps_process_rcvd_pkt(void *recv_buff, void *cli_addr)
{
	dps_pkt_hdr_t *hdr = (dps_pkt_hdr_t *)recv_buff;
	uint32_t ret = DPS_ERROR;

	dps_log_debug(DpsProtocolLogLevel, "Enter: Msg type %d", hdr->type);

	do
	{
		if ((hdr->type < DPS_ENDPOINT_LOC_REQ) || (hdr->type >= DPS_MAX_MSG_TYPE))
		{
			dps_pkt_stats_tbl[0].recv_error++;
			dps_log_debug(DpsProtocolLogLevel, "Incorrect msg type %d received", hdr->type);
			break;
		}
		// TODO: Verify Packet Length - Confirm that the RECV FROM length is >= the length
		//       stated in the packet
		ret = dps_recv_func_tbl[hdr->type].dps_fn(recv_buff, cli_addr);
		if (ret == DPS_SUCCESS)
		{
			dps_pkt_stats_tbl[hdr->type].recv++;
		}
		else
		{
			dps_pkt_stats_tbl[hdr->type].recv_error++;
		}

	}while(0);

	dps_log_debug(DpsProtocolLogLevel, "Exit: Status %d", ret);

	return ret;
}

/*
 ******************************************************************************
 * dps_protocol_client_send                                               *//**
 *
 * \brief This routine is called by clients of the dps protocol handler to send
 *        packets on the network. The clients of the protocol handler are the
 *        Dove Switches Dove Gateway and the DPS Server. Based on the type of
 *        packet the corresponding function is called to create a packet and
 *        send it out on the network.
 *
 * \param[in] msg - A pointer to a message that the client wants to send.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */

dps_return_status dps_protocol_client_send(dps_client_data_t *msg)
{
	dps_return_status ret = DPS_ERROR;

	dps_log_info(DpsProtocolLogLevel, "Enter: Msg type %d", msg->hdr.type);

	do
	{
#if defined (DPS_SERVER)
		if(!dps_cluster_is_local_node_active())
		{
			ret = DPS_SUCCESS;
			break;
		}
#endif
		if ((msg->hdr.type < DPS_ENDPOINT_LOC_REQ) || (msg->hdr.type >= DPS_MAX_MSG_TYPE))
		{
			dps_pkt_stats_tbl[0].transmit_error++;
			dps_log_notice(DpsProtocolLogLevel, "Incorrect msg type %d sent from client",
			               msg->hdr.type);
			break;
		}
		ret = (dps_return_status)(dps_send_func_tbl[msg->hdr.type].dps_fn((void *)msg, (void *)NULL));
		if (ret == DPS_SUCCESS)
		{
			dps_pkt_stats_tbl[msg->hdr.type].sent++;
		}
		else
		{
			dps_pkt_stats_tbl[msg->hdr.type].transmit_error++;
		}
	}while(0);

	dps_log_info(DpsProtocolLogLevel, "Exit: Status %d", ret);

	return ret;

}

#if defined(DPS_SERVER)
/*
 ******************************************************************************
 * dps_packet_stats_show                                                  *//**
 *
 * \brief This routine is called to show the packet statistics of DPS Client
 *        Server packets
 *
 * \param[in] pkt_type - DPS Client Server Protocol Message Type i.e.
 *                       dps_client_req_type
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */

dove_status dps_packet_stats_show(uint32_t pkt_type)
{
	int i;

	dps_log_debug(DpsProtocolLogLevel, "Enter: packet type %d", pkt_type);

	if ((pkt_type < DPS_ENDPOINT_LOC_REQ) || (pkt_type > DPS_ADDR_REPLY))
	{
		// Show all statistics
		show_print("");
		for (i = DPS_ENDPOINT_LOC_REQ; i <= DPS_ADDR_REPLY; i++)
		{
			show_print("Packet Type: %s", dps_pkt_stats_tbl[i].packet_name);
			show_print("Sent %25ld: Transmit Error %25ld",
			           dps_pkt_stats_tbl[i].sent, dps_pkt_stats_tbl[i].transmit_error);
			show_print("Recv %25ld: Receive Error  %25ld",
			           dps_pkt_stats_tbl[i].recv, dps_pkt_stats_tbl[i].recv_error);
			show_print("");
		}
	}
	else
	{
		i = pkt_type;
		show_print("");
		show_print("Packet Type: %s", dps_pkt_stats_tbl[i].packet_name);
		show_print("Sent %25ld: Transmit Error %25ld",
		           dps_pkt_stats_tbl[i].sent, dps_pkt_stats_tbl[i].transmit_error);
		show_print("Recv %25ld: Receive Error  %25ld",
		           dps_pkt_stats_tbl[i].recv, dps_pkt_stats_tbl[i].recv_error);
		show_print("");
	}

	dps_log_debug(DpsProtocolLogLevel, "Exit");
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_packet_stats_clear                                                 *//**
 *
 * \brief This routine is called to clear the packet statistics of DPS Client
 *        Server packets
 *
 * \param[in] pkt_type - DPS Client Server Protocol Message Type i.e.
 *                       dps_client_req_type
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */
dove_status dps_packet_stats_clear(uint32_t pkt_type)
{
	int i;

	dps_log_debug(DpsProtocolLogLevel, "Enter: packet type %d", pkt_type);
	if ((pkt_type < DPS_ENDPOINT_LOC_REQ) || (pkt_type > DPS_ADDR_REPLY))
	{
		// Clear all statistics
		for (i = DPS_ENDPOINT_LOC_REQ; i <= DPS_ADDR_REPLY; i++)
		{
			dps_pkt_stats_tbl[i].sent = 0;
			dps_pkt_stats_tbl[i].transmit_error = 0;
			dps_pkt_stats_tbl[i].recv = 0;
			dps_pkt_stats_tbl[i].recv_error = 0;
		}
	}
	else
	{
		i = pkt_type;
		dps_pkt_stats_tbl[i].sent = 0;
		dps_pkt_stats_tbl[i].transmit_error = 0;
		dps_pkt_stats_tbl[i].recv = 0;
		dps_pkt_stats_tbl[i].recv_error = 0;
	}
	dps_log_debug(DpsProtocolLogLevel, "Exit");
	return DOVE_STATUS_OK;
}
#endif // #if defined(DPS_SERVER)

/*
 ******************************************************************************
 * Print Functions                                                        *//**
 *
 ******************************************************************************
 */

#if defined (NDEBUG) || defined (VMX86_DEBUG)
static void dump_pkt_hdr(uint8_t *buff, uint32_t len)
{
	dps_log_debug(DpsProtocolLogLevel, "Entire pkt len: %d", len);
	dps_log_debug(DpsProtocolLogLevel, "DPS Hdr [Ver:%x,Type:%x,Len:%x%x,Query:%x%x%x%x,Vnid:%x%x%x%x,SubType:%x%x,RespStatus:%x%x,Sender:%x%x, Resv:%x%x]",
	              buff[0], buff[1], buff[2], buff[3],
	              buff[4], buff[5], buff[6], buff[7],
	              buff[8], buff[9], buff[10], buff[11],
	              buff[12], buff[13], buff[14], buff[15],
	              buff[16], buff[17], buff[18], buff[19]);
	return;
}
static void dump_pkt(uint8_t *buff, uint32_t len)
{
	uint32_t i;


	dump_pkt_hdr(buff, len);
	dps_log_debug(DpsProtocolLogLevel,"DPS Payload:");
	for (i = DPS_PKT_HDR_LEN; i < len; i++)
	{
		dps_log_debug(DpsProtocolLogLevel,"%02x ",buff[i]);
	}
}
#else
#define dump_pkt(_buff, _len)
#endif

#if defined (NDEBUG) || defined (VMX86_DEBUG)

const char *str_policy_type[] = {
	"Unknown Policy Type",
	"Connectivity",
	"Source Routing",
	"Unknown Policy Type"
};

const char *str_resp_err[] = {
	"No Error",
	"Invalid Source IP",
	"Invalid Dst IP",
	"Invalid Src DVG",
	"Invalid Dst DVG",
	"Invalid EUID",
	"Invalid Policy Id",
	"Policy Tracking Mismatch",
	"Invalid Domain Id",
	"No Memory",
	"No internal Gw",
	"Invalid Query Id",
	"No Bcast List",
	"Operation Not Supported",
	"No Response",
	"Endpoint Conflict",
	"Retry Error",
	"No Route",
	"Invalid Tunnel Endpoint",
	"Unknown Response Error Code"
};

const char *str_endpoint_update_resp_code[] = {
	"Unknown Endpoint Update Code",
	"Endpoint Update Add",
	"Endpoint Update Delete",
	"Endpoint VIP Add",
	"Endpoint VIP Delete",
	"Endpoint Update Migrate In",
	"Endpoint Update Migrate Out",
	"Unknown Endpoint Update Code"
};


static void print_client_hdr(dps_client_hdr_t *hdr, void *context)
{
	const char *resp_str, *subtype_str = "\0";
	const char *pkt_type_str = "Unknown Packet Type";
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];

	if (hdr->type == DPS_ENDPOINT_UPDATE)
	{
		if (hdr->sub_type >= DPS_ENDPOINT_UPDATE_MAX)
		{
			subtype_str = str_endpoint_update_resp_code[DPS_ENDPOINT_UPDATE_MAX];
		}
		else
		{
			subtype_str = str_endpoint_update_resp_code[hdr->sub_type];
		}
	}
	else
	{
		if (hdr->type == DPS_GENERAL_ACK)
		{
			if ((hdr->sub_type > 0) && (hdr->sub_type < DPS_MAX_MSG_TYPE))
				subtype_str = dps_send_func_tbl[hdr->sub_type].dps_fn_name;
		}

	}
	if (hdr->resp_status >= DPS_RESP_ERR_MAX)
		resp_str = str_resp_err[DPS_RESP_ERR_MAX];
	else
		resp_str = str_resp_err[hdr->resp_status];

	if ((hdr->type > 0) && (hdr->type < DPS_MAX_MSG_TYPE))
		pkt_type_str = dps_send_func_tbl[hdr->type].dps_fn_name;
	
	dps_log_info(DpsProtocolLogLevel,"Pkt Type: %s, Sub-Type: %d %s", pkt_type_str, hdr->sub_type, subtype_str);
	dps_log_info(DpsProtocolLogLevel,
	             "Pkt Hdr: Vnid:%d, Client_id:%d, Transaction Type:%d, Resp_Status:%s, QID:%d, Context:%p\n",
	             hdr->vnid, hdr->client_id, hdr->transaction_type, resp_str, hdr->query_id, context);
	if (hdr->reply_addr.family == AF_INET)
	{
		uint32_t ip4;
		ip4 = htonl(hdr->reply_addr.ip4);
		inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"DPS Client Sender: [%s:%d]",
		              str_v4, hdr->reply_addr.port);
	}
	else if (hdr->reply_addr.family == AF_INET6)
	{
		inet_ntop(AF_INET6, hdr->reply_addr.ip6, str_v6, INET6_ADDRSTRLEN);
		dps_log_info(DpsProtocolLogLevel,"DPS Client Sender [%s:%d]",
		              str_v6, hdr->reply_addr.port);
	}
	else
	{
		dps_log_info(DpsProtocolLogLevel,"DPS Client Sender Invalid Family!!!");
	}

}

void print_ip_addr(ip_addr_t *ip)
{
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];

	if (ip->family == AF_INET)
	{
		uint32_t ip4 = htonl(ip->ip4);
		inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"vIP: addr %s", str_v4);
	}
	else if (ip->family == AF_INET6)
	{
		inet_ntop(AF_INET6, &ip->ip6, str_v6, INET6_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"vIP addr %s", str_v6);

	}
	else
	{
		dps_log_debug(DpsProtocolLogLevel,"invalid vIP family");
	}
}

static void print_mcast_addr(dps_mcast_addr_t *mcast_addr)
{
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];

	dps_log_debug(DpsProtocolLogLevel, "Mcast Mac: %02x:%02x:%02x:%02x:%02x:%02x", 
	              mcast_addr->mcast_mac[0],mcast_addr->mcast_mac[1], mcast_addr->mcast_mac[2],
	              mcast_addr->mcast_mac[3],mcast_addr->mcast_mac[4],mcast_addr->mcast_mac[5]);

	if(mcast_addr->mcast_addr_type == MCAST_ADDR_V4)
	{
		uint32_t ip4 = htonl(mcast_addr->u.mcast_ip4);
		inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"Mcast IP: addr %s", str_v4);	
	}
	if(mcast_addr->mcast_addr_type == MCAST_ADDR_V6)
	{
		inet_ntop(AF_INET6, &mcast_addr->u.mcast_ip6, str_v6, INET6_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"Mcast IP %s", str_v6);

	}
	
}

static void print_endpoint_info(dps_endpoint_info_t *endpoint_info)
{
	
	dps_log_debug(DpsProtocolLogLevel, "Endpoint Info:");
	dps_log_debug(DpsProtocolLogLevel, "vnid: %d", endpoint_info->vnid);
	
	print_ip_addr(&endpoint_info->vm_ip_addr);

	dps_log_debug(DpsProtocolLogLevel, "vMac: %02x:%02x:%02x:%02x:%02x:%02x", 
	              endpoint_info->mac[0],endpoint_info->mac[1],endpoint_info->mac[2],
	              endpoint_info->mac[3],endpoint_info->mac[4],endpoint_info->mac[5]);

}

static void print_tunnel_info(uint32_t num_of_tunnels, dps_tunnel_endpoint_t *tunnel_list)
{
	uint32_t i = 0;
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];
	
	dps_log_debug(DpsProtocolLogLevel, "Num of Tunnels: %d",num_of_tunnels);
	for (i =0 ; i< num_of_tunnels; i++)
	{

		if (tunnel_list[i].family == AF_INET)
		{
			uint32_t ip4 = htonl(tunnel_list[i].ip4);
			inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel, "DOVE Switch family AF_INET, pIP addr %s vnid %d port %d ttype %d flags %d",
			              str_v4, tunnel_list[i].vnid, tunnel_list[i].port, tunnel_list[i].tunnel_type,tunnel_list[i].flags);
		}
		else if (tunnel_list[i].family == AF_INET6)
		{
			inet_ntop(AF_INET6, tunnel_list[i].ip6, str_v6, INET6_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel, "DOVE Switch family AF_INET6, pIP addr %s vnid %d port %d ttype %d flags %d",
			              str_v6, tunnel_list[i].vnid, tunnel_list[i].port, tunnel_list[i].tunnel_type,tunnel_list[i].flags);
		}
		else
		{
		  dps_log_debug(DpsProtocolLogLevel,"%d DOVE Switch family invalid!!!");
		}
	}
}

static void print_endpoint_loc_reply(dps_endpoint_loc_reply_t *endpoint_loc_reply)
{	
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];


	dps_log_debug(DpsProtocolLogLevel, "Endpoint Loc Reply Info");
	dps_log_debug(DpsProtocolLogLevel, "Vnid %d vMac %02x:%02x:%02x:%02x:%02x:%02x",
	              endpoint_loc_reply->vnid,
	              endpoint_loc_reply->mac[0],endpoint_loc_reply->mac[1],
	              endpoint_loc_reply->mac[2], endpoint_loc_reply->mac[3],
	              endpoint_loc_reply->mac[4], endpoint_loc_reply->mac[5]);
	
	print_tunnel_info(endpoint_loc_reply->tunnel_info.num_of_tunnels, endpoint_loc_reply->tunnel_info.tunnel_list);

	if (endpoint_loc_reply->vm_ip_addr.family == AF_INET)
	{
		uint32_t ip4 = htonl(endpoint_loc_reply->vm_ip_addr.ip4);
		inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel, "Virtual IP family AF_INET, vIP addr %s",
		              str_v4);
	}
	else if (endpoint_loc_reply->vm_ip_addr.family == AF_INET6)
	{
		inet_ntop(AF_INET6, endpoint_loc_reply->vm_ip_addr.ip6, str_v6, INET6_ADDRSTRLEN);
		dps_log_debug(DpsProtocolLogLevel,"Virtual IP family AF_INET6, vIP addr %s",
		              str_v6);
	}
	else
	{
		dps_log_debug(DpsProtocolLogLevel,"Virtual IP family invalid!!!");
	}
}

static void dump_client_info(dps_client_data_t *buff)
{
	dps_client_hdr_t *client_hdr = DPS_GET_CLIENT_HDR(buff);
	char str_v4[INET_ADDRSTRLEN];
	char str_v6[INET6_ADDRSTRLEN];

	switch (client_hdr->type)
	{
	    case DPS_ENDPOINT_LOC_REQ:
	    case DPS_ADDR_RESOLVE:	
			print_client_hdr(&buff->hdr, buff->context);

			if (buff->endpoint_loc_req.vm_ip_addr.family == AF_INET)
			{
				uint32_t ip4 = htonl(buff->endpoint_loc_req.vm_ip_addr.ip4);
				inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"family AF_INET, vIP addr %s", str_v4);
			}
			else if (buff->endpoint_loc_req.vm_ip_addr.family == AF_INET6)
			{
				inet_ntop(AF_INET6, buff->endpoint_loc_req.vm_ip_addr.ip6, str_v6, INET6_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"family AF_INET6, vIP addr %s", str_v6);
			}
			else
			{
				dps_log_debug(DpsProtocolLogLevel,"invalid vIP family");
			}
			dps_log_debug(DpsProtocolLogLevel, "vnid %d mac %02x:%02x:%02x:%02x:%02x:%02x",
			              buff->endpoint_loc_req.vnid,
			              buff->endpoint_loc_req.mac[0],buff->endpoint_loc_req.mac[1],
			              buff->endpoint_loc_req.mac[2],buff->endpoint_loc_req.mac[3],
			              buff->endpoint_loc_req.mac[4],buff->endpoint_loc_req.mac[5]);
		break;

		case DPS_ENDPOINT_LOC_REPLY:
    	case DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
			print_client_hdr(&buff->hdr, buff->context);
			
			print_endpoint_loc_reply(&buff->endpoint_loc_reply);


		break;

		case DPS_ENDPOINT_UPDATE:
		{

			print_client_hdr(&buff->hdr,buff->context );
		
			dps_log_debug(DpsProtocolLogLevel, "Data EUID Vnid %d mac %02x:%02x:%02x:%02x:%02x:%02x",
			              buff->endpoint_update.vnid,
			              buff->endpoint_update.mac[0],buff->endpoint_update.mac[1],
			              buff->endpoint_update.mac[2], buff->endpoint_update.mac[3],
			              buff->endpoint_update.mac[4], buff->endpoint_update.mac[5]);
		
			print_tunnel_info(buff->endpoint_update.tunnel_info.num_of_tunnels,buff->endpoint_update.tunnel_info.tunnel_list);
		
			if (buff->endpoint_update.vm_ip_addr.family == AF_INET)
			{
				uint32_t ip4 = htonl(buff->endpoint_update.vm_ip_addr.ip4);
				inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel, "Virtual IP family AF_INET, vIP addr %s",
				              str_v4);
			}
			else if (buff->endpoint_update.vm_ip_addr.family == AF_INET6)
			{
				inet_ntop(AF_INET6, buff->endpoint_update.vm_ip_addr.ip6, str_v6, INET6_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"Virtual IP family AF_INET6, vIP addr %s",
				              str_v6);
			}
			else
			{
				dps_log_debug(DpsProtocolLogLevel,"Virtual IP family invalid!!!");
			}
		}
		break;

	case DPS_ENDPOINT_UPDATE_REPLY:
	{	
	    uint32_t i;
	    print_client_hdr(&buff->hdr, buff->context);
	    
	    dps_log_debug(DpsProtocolLogLevel, "Data EUID Vnid %d mac %02x:%02x:%02x:%02x:%02x:%02x",
			  buff->endpoint_update_reply.vnid,
			  buff->endpoint_update_reply.mac[0],buff->endpoint_update_reply.mac[1],
			  buff->endpoint_update_reply.mac[2], buff->endpoint_update_reply.mac[3],
			  buff->endpoint_update_reply.mac[4], buff->endpoint_update_reply.mac[5]);
		
	    print_tunnel_info(buff->endpoint_update_reply.tunnel_info.num_of_tunnels,buff->endpoint_update_reply.tunnel_info.tunnel_list);
	    dps_log_debug(DpsProtocolLogLevel, "Num of VIPs %d", buff->endpoint_update_reply.num_of_vip);
	    for (i = 0; i < buff->endpoint_update_reply.num_of_vip; i++)
	    {
		if (buff->endpoint_update_reply.vm_ip_addr[i].family == AF_INET)
		{
		    uint32_t ip4 = htonl(buff->endpoint_update_reply.vm_ip_addr[i].ip4);
		    inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		    dps_log_debug(DpsProtocolLogLevel, "Virtual IP family AF_INET, vIP addr %s",
				  str_v4);
		}
		else if (buff->endpoint_update_reply.vm_ip_addr[i].family == AF_INET6)
		{
		    inet_ntop(AF_INET6, buff->endpoint_update_reply.vm_ip_addr[i].ip6, str_v6, INET6_ADDRSTRLEN);
		    dps_log_debug(DpsProtocolLogLevel,"Virtual IP family AF_INET6, vIP addr %s",
				  str_v6);
		}
		else
		{
		    dps_log_debug(DpsProtocolLogLevel,"Virtual IP family invalid!!!");
		}
	    }
	    break;
	}
	case DPS_POLICY_REQ:
	    print_client_hdr(&buff->hdr, buff->context);
		dps_log_debug(DpsProtocolLogLevel,"Dst Endpoint Info");
		print_endpoint_info(&buff->policy_req.dst_endpoint);
		dps_log_debug(DpsProtocolLogLevel,"Src Endpoint Info");
		print_endpoint_info(&buff->policy_req.src_endpoint);
		
		break;
	case DPS_POLICY_REPLY:
	{
	    uint32_t i;
	    dps_policy_info_t *plcy_info = &(buff->policy_reply.dps_policy_info);
	    uint32_t type = (plcy_info->dps_policy.policy_type > DPS_POLICY_TYPE_MAX) ? DPS_POLICY_TYPE_MAX : plcy_info->dps_policy.policy_type;
	    const char *plcy = str_policy_type[type];
	    dps_policy_t *rules = &(plcy_info->dps_policy);
	    src_dst_vnid_t *vnid_rules = rules->vnid_policy.src_dst_vnid;
	    
	    print_client_hdr(&buff->hdr, buff->context);
	    print_endpoint_loc_reply(&buff->policy_reply.dst_endpoint_loc_reply);
	    dps_log_debug(DpsProtocolLogLevel,"Policy Id: %d Version: %d TTL: %d Policy Type %s",
			  plcy_info->policy_id, plcy_info->version,  plcy_info->ttl, plcy);
	    
	    dps_log_debug(DpsProtocolLogLevel,"Permit Rules %d",rules->vnid_policy.num_permit_rules);
	    for (i = 0;i < rules->vnid_policy.num_permit_rules; i++)
	    {
		dps_log_debug(DpsProtocolLogLevel,"sVNID %d, dVNID %d", vnid_rules[i].svnid, vnid_rules[i].dvnid);
	    }
	    dps_log_debug(DpsProtocolLogLevel,"Deny Rules %d",rules->vnid_policy.num_deny_rules);
	    for (i = rules->vnid_policy.num_permit_rules;i < rules->vnid_policy.num_deny_rules; i++)
	    {
		dps_log_debug(DpsProtocolLogLevel,"sVNID %d, dVNID %d", vnid_rules[i].svnid, vnid_rules[i].dvnid);
	    }
	}
	break;
	case DPS_INTERNAL_GW_REPLY:
	case DPS_UNSOLICITED_INTERNAL_GW_REPLY:
	{
		uint32_t i;
		uint32_t *ip_addr, tmp_addr;
		dps_internal_gw_t *gw_info =  &(buff->internal_gw_list);
		ip_addr = gw_info->gw_list;

		print_client_hdr(&buff->hdr,buff->context);
		dps_log_debug(DpsProtocolLogLevel,"V4 Gw Addr: %d", gw_info->num_v4_gw);
		for (i = 0;i < gw_info->num_v4_gw; i++, ip_addr++)
		{
			tmp_addr =  htonl(*ip_addr);
			inet_ntop(AF_INET, &tmp_addr, str_v4, INET_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"v4 addr: %s", str_v4);
		}
		dps_log_debug(DpsProtocolLogLevel,"V6 Gw Addr: %d", gw_info->num_v6_gw);
		for (i = 0;i < gw_info->num_v6_gw; i++, ip_addr+=4)
		{
			inet_ntop(AF_INET6, (uint8_t *)ip_addr, str_v6, INET6_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"v6 addr: %s", str_v6);
		}
		break;
	}
	case DPS_BCAST_LIST_REPLY:
	case DPS_UNSOLICITED_BCAST_LIST_REPLY:
	{
		uint32_t i;
		uint32_t *ip_addr, tmp_addr;
		dps_pkd_tunnel_list_t *switch_info =  &(buff->dove_switch_list);
		ip_addr = switch_info->tunnel_list;

		print_client_hdr(&buff->hdr, buff->context);
		dps_log_debug(DpsProtocolLogLevel,"V4 Switch Addr: %d", switch_info->num_v4_tunnels);
		for (i = 0; i < switch_info->num_v4_tunnels; i++, ip_addr++)
		{
			tmp_addr =  htonl(*ip_addr);
			inet_ntop(AF_INET, &tmp_addr, str_v4, INET_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"v4 addr: %s", str_v4);
		}
		dps_log_debug(DpsProtocolLogLevel,"V6 Switch Addr: %d", switch_info->num_v6_tunnels);
		for (i = 0;i < switch_info->num_v6_tunnels; i++, ip_addr += 4)
		{
			inet_ntop(AF_INET6, (uint8_t *)ip_addr, str_v6, INET6_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"v6 addr: %s", str_v6);
		}
		break;
	}
	case DPS_UNSOLICITED_VNID_POLICY_LIST:
	case DPS_VNID_POLICY_LIST_REPLY:
	{
		uint32_t i;
		src_dst_vnid_t *vnid_rules = buff->bulk_vnid_policy.src_dst_vnid;
		print_client_hdr(&buff->hdr,buff->context);

		dps_log_debug(DpsProtocolLogLevel,"Permit Rules %d",buff->bulk_vnid_policy.num_permit_rules);

		for (i = 0;i < buff->bulk_vnid_policy.num_permit_rules; i++)
		{
			dps_log_debug(DpsProtocolLogLevel,"sDVG %d, dDVG %d", vnid_rules[i].svnid, vnid_rules[i].dvnid);
		}
		dps_log_debug(DpsProtocolLogLevel,"Deny Rules %d",buff->bulk_vnid_policy.num_deny_rules);
		for (i = buff->bulk_vnid_policy.num_permit_rules; i < buff->bulk_vnid_policy.num_deny_rules; i++)
		{
			dps_log_debug(DpsProtocolLogLevel,"sDVG %d, dDVG %d", vnid_rules[i].svnid, vnid_rules[i].dvnid);
		}
		break;
	}
	
	case DPS_INTERNAL_GW_REQ:
	case DPS_BCAST_LIST_REQ:
	case DPS_EXTERNAL_GW_LIST_REQ:
	case DPS_VLAN_GW_LIST_REQ:
	case DPS_VNID_POLICY_LIST_REQ:
    case DPS_MCAST_CTRL_GW_REQ:
	case DPS_CTRL_PLANE_HB:
	{
		uint32_t tmp_addr;
		print_client_hdr(&buff->hdr,buff->context);
		if (buff->gen_msg_req.dps_client_addr.family == AF_INET)
		{
			tmp_addr = htonl(buff->gen_msg_req.dps_client_addr.ip4);
			inet_ntop(AF_INET, &tmp_addr, str_v4, INET_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"Client Addr: [%s:%d]", str_v4,buff->gen_msg_req.dps_client_addr.port);
		}
		else if (buff->gen_msg_req.dps_client_addr.family == AF_INET6)
		{
			inet_ntop(AF_INET6, (uint8_t *)buff->gen_msg_req.dps_client_addr.ip6, str_v6, INET6_ADDRSTRLEN);
			dps_log_debug(DpsProtocolLogLevel,"Client Addr: [%s:%d]", str_v6, buff->gen_msg_req.dps_client_addr.port);
		}
	
		break;
	}

	case DPS_VM_MIGRATION_EVENT:
	{		
		print_client_hdr(&buff->hdr,buff->context);
		dps_log_debug(DpsProtocolLogLevel,"Migrated VM Endpoint Info");
		print_endpoint_info(&buff->vm_migration_event.migrated_vm_info);
		dps_log_debug(DpsProtocolLogLevel,"Src VM Location Info");
		print_endpoint_loc_reply(&buff->vm_migration_event.src_vm_loc);
		break;
	}
	
	case DPS_MCAST_SENDER_REGISTRATION:
	case DPS_MCAST_SENDER_DEREGISTRATION:
	{
		print_client_hdr(client_hdr, buff->context);
		dps_log_debug(DpsProtocolLogLevel,"Mcast Sender VM Info");
		print_endpoint_info(&buff->mcast_sender.mcast_src_vm);
		dps_log_debug(DpsProtocolLogLevel,"Mcast Sender Tunnel Endpoint");
		print_ip_addr(&buff->mcast_sender.tunnel_endpoint);
		print_mcast_addr(&buff->mcast_sender.mcast_addr);
		break;
		
	}
	case DPS_MCAST_RECEIVER_JOIN:
	case DPS_MCAST_RECEIVER_LEAVE: 
	{
		uint32_t i = 0;
		uint32_t *ip_addr, tmp_addr;

		dps_mcast_group_record_t *grp_rec = &buff->mcast_receiver.mcast_group_rec;
		print_client_hdr(client_hdr,buff->context);
		dps_log_debug(DpsProtocolLogLevel,"Mcast Sender Tunnel Endpoint");
		print_ip_addr(&buff->mcast_sender.tunnel_endpoint);
		print_mcast_addr(&grp_rec->mcast_addr);
		ip_addr = grp_rec->src_addr;
		if ((grp_rec->family == AF_INET) && (grp_rec->num_of_srcs))
		{
			for (i = 0; i < grp_rec->num_of_srcs; i++, ip_addr++)
			{
				tmp_addr =  htonl(*ip_addr);
				inet_ntop(AF_INET, &tmp_addr, str_v4, INET_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"v4 addr: %s", str_v4);
			}
		}
		else if ((grp_rec->family == AF_INET6) && (grp_rec->num_of_srcs))
		{
			for (i = 0;i < grp_rec->num_of_srcs; i++, ip_addr += 4)
			{
				inet_ntop(AF_INET6, (uint8_t *)ip_addr, str_v6, INET6_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"v6 addr: %s", str_v6);
			}
		}
		break;
	}
	case DPS_UNSOLICITED_EXTERNAL_GW_LIST:
	case DPS_UNSOLICITED_VLAN_GW_LIST:
	case DPS_EXTERNAL_GW_LIST_REPLY:
	case DPS_VLAN_GW_LIST_REPLY:
	case DPS_MCAST_CTRL_GW_REPLY:
	{
		dps_tunnel_list_t *gw_info =  &(buff->tunnel_info);
	
		print_client_hdr(&buff->hdr,buff->context);
		print_tunnel_info(gw_info->num_of_tunnels, gw_info->tunnel_list);
		
		break;
	}
	case DPS_TUNNEL_REGISTER:
	case DPS_TUNNEL_DEREGISTER:
	{
		dps_tunnel_list_t *gw_info =  &(buff->tunnel_reg_dereg.tunnel_info);
	
		print_client_hdr(&buff->hdr,buff->context);
		print_tunnel_info(gw_info->num_of_tunnels, gw_info->tunnel_list);
		break;
	}
	case DPS_GENERAL_ACK:
	case DPS_REG_DEREGISTER_ACK:
	{
		print_client_hdr(client_hdr,buff->context);
		break;
	}
	case DPS_MCAST_RECEIVER_DS_LIST:
	{
		uint32_t j, i = 0;
		uint32_t *ip_addr, tmp_addr;
		dps_mcast_receiver_ds_list_t *grp_recvr = &buff->mcast_receiver_ds_list;
		dps_pkd_tunnel_list_t *switch_info;
		uint8_t *tmp_char_ptr;

		print_client_hdr(&buff->hdr,buff->context);
		print_mcast_addr(&grp_recvr->mcast_addr);
		switch_info = grp_recvr->mcast_recvr_list;
		for (j =0; j < grp_recvr->num_of_rec; j++)
		{
			ip_addr = switch_info->tunnel_list;

			dps_log_debug(DpsProtocolLogLevel, "Vnid:%d", switch_info->vnid);
			dps_log_debug(DpsProtocolLogLevel,"Num of V4 Tunnels: %d",switch_info->num_v4_tunnels);
			for (i = 0; i < switch_info->num_v4_tunnels; i++, ip_addr++)
			{
				tmp_addr =  htonl(*ip_addr);
				inet_ntop(AF_INET, &tmp_addr, str_v4, INET_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"v4 addr: %s", str_v4);
			}
			dps_log_debug(DpsProtocolLogLevel,"Num of V6 Tunnels: %d", switch_info->num_v6_tunnels);
			for (i = 0;i < switch_info->num_v6_tunnels; i++, ip_addr += 4)
			{
				inet_ntop(AF_INET6, (uint8_t *)ip_addr, str_v6, INET6_ADDRSTRLEN);
				dps_log_debug(DpsProtocolLogLevel,"v6 addr: %s", str_v6);
			}
			tmp_char_ptr = (uint8_t *)switch_info;
			tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[switch_info->num_v4_tunnels+ (switch_info->num_v6_tunnels << 2)]);
			switch_info = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
		}
		break;
	}
	case DPS_UNSOLICITED_VM_LOC_INFO:
	{	
	    uint32_t i;

	    print_client_hdr(&buff->hdr, buff->context);
	    
	    dps_log_debug(DpsProtocolLogLevel, "Data EUID Vnid %d mac %02x:%02x:%02x:%02x:%02x:%02x",
			  buff->vm_invalidate_msg.epri.vnid,
			  buff->vm_invalidate_msg.epri.mac[0],buff->vm_invalidate_msg.epri.mac[1],
			  buff->vm_invalidate_msg.epri.mac[2], buff->vm_invalidate_msg.epri.mac[3],
			  buff->vm_invalidate_msg.epri.mac[4], buff->vm_invalidate_msg.epri.mac[5]);
		
	    dps_log_debug(DpsProtocolLogLevel, "Num of VIP4 %d", buff->vm_invalidate_msg.epri.vm_ip4_addr.num_of_ips);
	    for (i = 0; i < buff->vm_invalidate_msg.epri.vm_ip4_addr.num_of_ips; i++)
	    {
		    uint32_t ip4 = htonl(buff->vm_invalidate_msg.epri.vm_ip4_addr.ip_info[i].ip);
		    inet_ntop(AF_INET, &ip4, str_v4, INET_ADDRSTRLEN);
		    dps_log_debug(DpsProtocolLogLevel, "%s",str_v4);
				  
		}
	    
		for (i = 0; i < buff->vm_invalidate_msg.epri.vm_ip6_addr.num_of_ips; i++)
		{
		    inet_ntop(AF_INET6, buff->vm_invalidate_msg.epri.vm_ip6_addr.ip_info[i].ip, str_v6, INET6_ADDRSTRLEN);
		    dps_log_debug(DpsProtocolLogLevel,"%s", str_v6);
		}

	    print_tunnel_info(buff->vm_invalidate_msg.epri.tunnel_info.num_of_tunnels, buff->vm_invalidate_msg.epri.tunnel_info.tunnel_list);
	    
	    break;
	}
	default:
		dps_log_info(DpsProtocolLogLevel, "Invalid Msg Type %d", client_hdr->type);
		break;

	}

}
#else
#define dump_client_info(buff)
#endif

static void dump_tlv_info(uint8_t *buff, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len; i++)
	{
		dps_log_debug(DpsProtocolLogLevel,"%02x ",buff[i]);
	}
}

/** @} */
/** @} */

