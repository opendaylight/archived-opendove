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
 *      dps_pkt.h
 *      This file defines structures and msg types used for interacting with dove switches
 *      gateways and dove policy server (DPS).
 *
 *  Author:
 *      Sushma Anantharam
 *
 */

#ifndef _DPS_PKT_
#define _DPS_PKT_

#include "dps_client_common.h"

#define DPS_SEND_BUFF_SZ            4096

#define SZ_OF_INT16         2 
#define SZ_OF_INT32         4 
#define DPS_DVG_DENY        0
#define DPS_DVG_PERMIT      1

#define DPS_PKT_HDR_TLV_LEN	4
#define DPS_PKT_HDR_LEN		20
#define DPS_MAC_ADDR_LEN 	6
#define DPS_PROTOCOL_VER	1
#define DPS_VNID_LEN		4
#define DPS_DATA_VER_LEN	4
#define DPS_IP4_ADDR_LEN    4
#define DPS_IP6_ADDR_LEN	16
#define DPS_IP4_TUNNEL_INFO_LEN    (DPS_IP4_ADDR_LEN + DPS_VNID_LEN + 8) // 8 = family(2),port(2),tunnel_type(2),flags(2)
#define DPS_IP6_TUNNEL_INFO_LEN    (DPS_IP6_ADDR_LEN + DPS_VNID_LEN + 8) // 8 = family(2),port(2),tunnel_type(2),flags(2)

// TLV lengths (tlv_hdr + data)

#define DPS_TLV_HDR_LEN		    4
#define DPS_VMAC_TLV_LEN	    DPS_TLV_HDR_LEN + 8
#define DPS_IP4_TLV_LEN         DPS_TLV_HDR_LEN + 4
#define DPS_IP6_TLV_LEN	        DPS_TLV_HDR_LEN + 16
#define DPS_EUID_TLV_LEN	    DPS_TLV_HDR_LEN + 12
#define DPS_PHYLOC4_TLV_LEN	    DPS_TLV_HDR_LEN + 8
#define DPS_PHYLOC6_TLV_LEN	    DPS_TLV_HDR_LEN + 20
#define DPS_SVCLOC4_TLV_LEN	    DPS_TLV_HDR_LEN + 8
#define DPS_SVCLOC6_TLV_LEN	    DPS_TLV_HDR_LEN + 20
#define DPS_POLICY_ID_TLV_LEN	    DPS_TLV_HDR_LEN + 12
#define DPS_ENDPOINT4_INFO_TLV_LEN DPS_TLV_HDR_LEN + 12 + DPS_IP4_TLV_LEN
#define DPS_ENDPOINT6_INFO_TLV_LEN DPS_TLV_HDR_LEN + 12 + DPS_IP6_TLV_LEN

// TLV Base Type 

#define	VMAC_TLV                     1
#define	IP_ADDR_TLV                  2
#define	EUID_TLV                     3
#define	PHYLOC_TLV                   4
#define	ENDPOINT_LOC_TLV             5
#define	SERVICE_LOC_TLV              6
#define POLICY_TLV                   7
#define POLICY_ID_TLV                8
#define POLICY_TYPE_TLV              9
#define REPLY_SERVICE_LOC_TLV       10
#define ENDPOINT_INFO_TLV           11
#define IPV4_ADDR_LIST_TLV          12
#define IPV6_ADDR_LIST_TLV          13
#define MCAST_ADDR_MAC_TLV          14
#define MCAST_ADDR_V4_TLV           15
#define MCAST_ADDR_V4_ICB_RANGE_TLV 16
#define MCAST_ADDR_V6_TLV           17
#define MCAST_GRP_REC_TLV           18
#define TUNNEL_LIST_TLV             19
#define VNID_TUNNEL_LIST_TLV        20
#define ENDPOINT_UPDATE_REPLY_TLV   21
#define EPRI_TLV                    22
#define IP4_INFO_LIST_TLV           23
#define IP6_INFO_LIST_TLV           24

/*
 ******************************************************************************
 * DPS Client Server Protocol - The Packet Headers                        *//**
 *
 * \ingroup DPSProtocolPacket
 * @{
 */

/*
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  Type |              Length                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Query Id                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         VNID                                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Sub Type                |         Resp Status             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Client ID | Transaction Type|         Reserved                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct  dps_pkt_hdr_s {
	/**
	 * \brief Version of DPS Protocol
	 */
	uint8_t         ver;
	/**
	 * \brief Packet Type: dps_client_req_type
	 */
	uint8_t         type;
	/**
	 * \brief Length of the Packet without the header
	 */
	uint16_t        len;
	/**
	 * \brief The Query ID of the packet. The response to a query
	 *        must have the same Query ID as the query
	 */
	uint32_t        query_id;
	/**
	 * \brief The VNID associated with the header
	 */
	uint32_t        vnid;
	/**
	 * \brief The Sub Type of the Type
	 */
	uint16_t        sub_type;
	/**
	 * \brief The response status: dps_resp_status_t
	 */
	uint16_t        resp_status;
	/**
	 * \brief The DPS Client ID: dps_client_id
	 */
	uint8_t         client_id;
	/**
	 * \brief The Transaction Type: dps_transaction_type
	 */
	uint8_t         transaction_type;
	/**
	 * \brief Reserved
	 */
	uint16_t        reserved;
} __attribute__((__packed__)) dps_pkt_hdr_t;

/*
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++
   |Version|  Type |              Length                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-++
*/

typedef union dps_tlv_hdr_s {
	uint32_t tlv_hdr;
	struct {
		uint8_t ver;
		uint8_t type;
		uint16_t len;
	} s;	
#define DPS_GET_TLV_VER(tlv_hdr) ((tlv_hdr)->s.ver)
#define DPS_GET_TLV_TYPE(tlv_hdr) ((tlv_hdr)->s.type)
#define DPS_GET_TLV_LEN(tlv_hdr) ((tlv_hdr)->s.len)
} dps_tlv_hdr_t;

typedef struct dps_vmac_tlv_s {
	dps_tlv_hdr_t tlv;
	uint8_t mac[6];
	uint8_t pad[2];
} dps_vmac_tlv_t;

typedef struct dps_vip4_tlv_s {
	dps_tlv_hdr_t tlv;
	uint32_t ip;
} dps_vip4_tlv_t;

typedef struct dps_vip6_tlv_s {
	dps_tlv_hdr_t tlv;
	uint8_t ip[16];
} dps_vip6_tlv_t;

typedef struct dps_euid_tlv_s {
	dps_tlv_hdr_t tlv;
	uint32_t vnid;
	uint8_t mac[6];
	uint8_t pad[2];
} dps_euid_tlv_t;

// Reply Service Location
typedef struct dps_svcloc4_tlv_s {
	dps_tlv_hdr_t tlv;
	uint16_t xport_type;
	uint16_t port;	
	uint32_t ip;
} dps_svcloc4_tlv_t;

typedef struct dps_svcloc6_tlv_s {
	dps_tlv_hdr_t tlv;
	uint16_t xport_type;
	uint16_t port;
	uint8_t ip6[16];
} dps_svcloc6_tlv_t;

typedef struct dps_endpoint_info_tlv_s {
	dps_tlv_hdr_t tlv;
	uint32_t vnid;
	uint8_t mac[6];
	uint8_t pad[2];
	//vm_ip address v4 or v6
}  dps_endpoint_info_tlv_t;

// Dove Switch Policy 

typedef struct dps_policy_id_tlv_s {
	dps_tlv_hdr_t tlv;
	uint32_t policy_id;
	uint32_t version;
	uint32_t ttl;
} dps_policy_id_tlv_t;

typedef struct dps_policy_type_tlv_s {
	dps_tlv_hdr_t tlv;
	uint8_t ver;         // for policy_type 1 ver is 1 for policy_type 2 either 4 or 6 depending on the ip address type
	uint8_t policy_type; // 1 - permit/deny 2 -src routing
	uint16_t total_num_of_rules; // for policy_type 1, total number of rules, for type 2 total number of ip addresses
	uint8_t  data[0];
} dps_policy_type_tlv_t;

// Jump Table used for processing packet types

typedef struct dps_func_s {
	const char	*dps_fn_name;
	uint32_t	(*dps_fn) (void *, void *);
} dps_func_tbl_t;

typedef struct tlv_func_tbl_s {
	const char *name;
	uint32_t (*pTlvFn)(uint8_t *, dps_client_data_t *client_buff);
} tlv_func_tbl_t;

/*
 * \brief Jump table for constructing reply packet
 */

typedef struct dps_construct_reply_func_s {
	const char	*dps_msg_name;
	void	(*dps_reply_fn) (dps_client_data_t *msg);
} dps_construct_reply_func_t;

// Bit function prototype
typedef void (*dps_bit_func)(uint8_t *, uint8_t);

/*
 * Function Prototypes
 */

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
 * \param[in] cli_addr - The address of the sender in sockaddr_in format
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
uint32_t dps_process_rcvd_pkt(void *, void *);

/*
 ******************************************************************************
 * dps_protocol_xmit                                                      *//**
 *
 * \brief - This routine should to be called to send a Buffer/Packet to a
 *          Remote destination
 *
 * \param[in] buff - Pointer to a char buffer
 * \param[in] buff_len - Size of the Buffer
 * \param[in] addr - The Remote End to send the Buffer To
 *
 * \retval DPS_SUCCESS
 *
 ******************************************************************************
 */
uint32_t dps_protocol_xmit(uint8_t *buff, uint32_t buff_len, ip_addr_t *addr, void *context);

uint32_t dps_get_pkt_hdr(dps_pkt_hdr_t *pkt_hdr, dps_client_hdr_t *client_hdr);


// General Function prototypes

#if !defined(DPS_SERVER)
extern uint32_t DpsReqNewNodeInterval;

#define dps_offsetof(_type, _member) ((size_t) &((_type *)0)->_member)

/*
 ******************************************************************************
 * dps_req_new_dps_node --                                                *//**
 *
 * \brief This routine is called to request a new DPS node. The old DPS node  
 *        mapped to the vnid is deleted. This routine is called when after a
 *        number of retries there is no response from the DPS. It is assumed
 *        that the DPS is not reachable and a new DPS node is requested.
 *
 * \param[in] vnid - The DPS node to be deleted from the vnid
 *
 * \retval None
 *
 ******************************************************************************
 */
void dps_req_new_dps_node(uint32_t vnid);

/*
 ******************************************************************************
 * dps_client_init --                                                     *//**
 *
 * \brief This routine is called to initialize the dps client.
 *
 * \retval None
 *
 ******************************************************************************
 */
dps_return_status dps_client_init(void);

/*
 ******************************************************************************
 * dps_server_add                                                         *//**
 *
 * \brief - This function adds DPS Server information to be used by the DPS
 *          protocol client. The DPS protocol uses one of the DPS Servers to
 *          send the information to, when requested by the DPS client
 *
 * \param[in] domain - The Domain associated with the DPS Server Node. A
 *                     value of 0 indicates that this dps node is not associated
 *                     with any particular domain.
 * \param[in] dps_svr_node - Address of the DPS Server (Network Byte Order)
 *
 * \retval ISWITCH_STATUS_OK
 *
 * \todo Return "int" instead of iswitch_status
 *
 ******************************************************************************
 */
dps_return_status dps_server_add(uint32_t domain, ip_addr_t *dps_svr_node);

/*
 ******************************************************************************
 * dps_server_del                                                         *//**
 *
 * \brief - This function deletes DPS Server information. The message to delete
 *          comes from from the Dove Controller
 *
 * \param[in] dps_svr_node - Address of the DPS Server (Network Byte Order)
 *
 * \retval None
 *
 ******************************************************************************
 */
void dps_server_del(ip_addr_t *dps_svr_node);

#endif

extern int32_t DpsProtocolCustomerLogLevel;
//Debug
extern int32_t DpsProtocolLogLevel;
#endif

/** @} */

