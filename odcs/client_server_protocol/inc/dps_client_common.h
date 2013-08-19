/*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dps_client_common.h
 *      This file defines structures and msg types used for interacting with dove
 *      switches gateways and dove policy server (DPS).
 *
 *  ALERT:
 *      The structures in this FILE are used by Python Code too. Any changes in this
 *      file MUST be reflected in the Python Code
 *
 *  Author:
 *      Sushma Anantharam
 *
 */

#ifndef _DPS_CLIENT_COMMON_
#define _DPS_CLIENT_COMMON_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <assert.h>
#include <semaphore.h>
#include "dps_list.h"

/*
 * All integer fields *MUST* be in host byte order including ipv4 addresses.
 *
 */
#define MAX_VIP_ADDR 16

/*
 ******************************************************************************
 * DPS Client Server Protocol - The Packet Headers                        *//**
 *
 * \ingroup DPSClientServerProtocol
 * @{
 */

/*
 * Return value seen by the client
 */
typedef enum {
	DPS_SUCCESS  = 0,
	DPS_ERROR    = 1,
	DPS_ERROR_EXCEEDS_CAP = 2,
	DPS_ERROR_NO_RESOURCES = 3,
	DPS_XMIT_ERROR = 4,
} dps_return_status;

/*
 * Following message types are used by the dps agent to interact with dove switch client (DSC) 
 * and dove gateway client (DGC). 
 * The dove switch or the dove gateway requests the dps agent to send msgs on their behalf to the DPS Server
 * to resolve policy and address resolution requests. 
 * DPS Agent <---> Dove Switch Client (DSC) / Dove Gateway Client (DGC)
 */
typedef enum {
	DPS_ENDPOINT_LOC_REQ = 1,              // Request for endpoint location of a VM sent by DPS Client
	DPS_ENDPOINT_LOC_REPLY = 2,            // Response from DPS with information about the VM
	DPS_POLICY_REQ = 3,                    // Request for policy from DPS Client to Dove Policy Server
	DPS_POLICY_REPLY = 4,                  // Reply from DPS to DPS Client
	DPS_POLICY_INVALIDATE = 5,             // Request sent from DPS to DPS Client to invalidate a policy
	DPS_ENDPOINT_UPDATE = 6,               // Update sent from Dove Agent to DPS to register a VMs information
	DPS_ENDPOINT_UPDATE_REPLY = 7,         // Ack/Nack of the endpoint update msg
	DPS_ADDR_RESOLVE = 8,                  // Request from DPS to resolve a vIP [Used in silent VM case]
	DPS_ADDR_REPLY = 9,                    // Not used: client must respond with a endpoint update 
	DPS_INTERNAL_GW_REQ = 10,              // Request for internal gateway information for a specific tenant
	DPS_INTERNAL_GW_REPLY = 11,            // List of internal gateway
	DPS_UNSOLICITED_VNID_POLICY_LIST = 12, // List of vnid policies
	DPS_BCAST_LIST_REQ = 13,               // Request for a list of Dove switches for a specific tenant
	DPS_BCAST_LIST_REPLY = 14,             // List of Dove switches in the specific vnid
	DPS_VM_MIGRATION_EVENT = 15,           // VM Migration hint sent by the Old Dove Switch
	DPS_MCAST_SENDER_REGISTRATION = 16,    // Register the source of the mcast stream
	DPS_MCAST_SENDER_DEREGISTRATION = 17,  // Register the source of the mcast stream
	DPS_MCAST_RECEIVER_JOIN = 18,          // Join the mcast stream
	DPS_MCAST_RECEIVER_LEAVE = 19,         // Leave the mcast stream
	DPS_MCAST_RECEIVER_DS_LIST = 20,       // List of Dove Switches receivers for a multicast stream
	DPS_UNSOLICITED_BCAST_LIST_REPLY = 21, // Msg sent by DPS when the bcast list of Dove Switches changes
	DPS_UNSOLICITED_INTERNAL_GW_REPLY = 22,// Msg sent by DPS when there is a change in internal gw list
	DPS_GENERAL_ACK = 23,                  // A general ack message
	DPS_UNSOLICITED_EXTERNAL_GW_LIST = 24, // A list of external gws with the vnids sent from DPS
	DPS_UNSOLICITED_VLAN_GW_LIST = 25,     // A list of vlan gws with the vnids sent from DPS
	DPS_TUNNEL_REGISTER = 26,              // Registration for Tunnel Endpoint
	DPS_TUNNEL_DEREGISTER = 27,            // Deregisration for Tunnel Endpoint
	DPS_REG_DEREGISTER_ACK = 28,           // ACK msg for tunnel and multicast reg/dereg
	DPS_EXTERNAL_GW_LIST_REQ = 29,
	DPS_EXTERNAL_GW_LIST_REPLY = 30,
	DPS_VLAN_GW_LIST_REQ = 31,
	DPS_VLAN_GW_LIST_REPLY = 32,
	DPS_UNSOLICITED_ENDPOINT_LOC_REPLY = 33, // Sent by DCS to DS after address resolution and in response to DPS_VM_MIGRATION_EVENT
	DPS_VNID_POLICY_LIST_REQ = 34,
	DPS_VNID_POLICY_LIST_REPLY = 35,
	DPS_MCAST_CTRL_GW_REQ = 36,             // Requesting an external gateway to which igmp control msgs should be 
	                                        // sent for multicast addresses in the ICB block
	DPS_MCAST_CTRL_GW_REPLY = 37,
	DPS_UNSOLICITED_VNID_DEL_REQ = 38,	    // VNID is being deleted, block ports in VNID
	DPS_CTRL_PLANE_HB = 39,                 // Heart Beat sent from the DCS node to Dove Switches
	DPS_GET_DCS_NODE = 40,                  // Used to request a new DCS node ip address. Never seen on the wire. 
	DPS_UNSOLICITED_INVALIDATE_VM = 41,    // Invalidate the VM.This msg is sent by DCS in response to a VM migration
	DPS_MAX_MSG_TYPE                       // This MUST be the Final Message Type
} dps_client_req_type;

// Error Codes is present in option1 of the packet header. The error code is 
// optional and is valid only in certain packet types.
// Always add new error codes just above DPS_RESP_ERR_MAX

typedef enum {
	DPS_NO_ERR = 0,
	DPS_INVALID_SRC_IP = 1,
	DPS_INVALID_DST_IP = 2,
	DPS_INVALID_SRC_VNID = 3,
	DPS_INVALID_DST_VNID = 4,
	DPS_INVALID_EUID = 5,
	DPS_INVALID_PLCY_ID = 6,
	DPS_PLCY_TRACKING_MISMATCH = 7,
	DPS_INVALID_VNID_ID = 8,
	DPS_NO_MEMORY = 9,
	DPS_NO_INTERNAL_GW = 10,
	DPS_INVALID_QUERY_ID = 11,
	DPS_NO_BCAST_LIST = 12,
	DPS_OPERATION_NOT_SUPPORTED = 13,
	DPS_NO_RESPONSE = 14,
	DPS_ERROR_ENDPOINT_CONFLICT = 15,
	DPS_ERROR_RETRY = 16,
	DPS_ERROR_NO_ROUTE = 17,
	DPS_INVALID_TUNNEL_ENDPOINT = 18,
	DPS_RESP_ERR_MAX
} dps_resp_status_t;

/**
 * \brief Describes the type of Endpoint Update. When endpoint_update request
 *        is sent the client needs to fill the sub_type field in the
 *        dps_client_hdr_t with what is requested of the endpoint update.
 */
typedef enum {
	DPS_ENDPOINT_UPDATE_ADD = 1,
	DPS_ENDPOINT_UPDATE_DELETE = 2,
	DPS_ENDPOINT_UPDATE_VIP_ADD = 3,
	DPS_ENDPOINT_UPDATE_VIP_DEL = 4,
	DPS_ENDPOINT_UPDATE_MIGRATE_IN = 5,
	DPS_ENDPOINT_UPDATE_MIGRATE_OUT = 6,
	DPS_ENDPOINT_UPDATE_MAX = 7
} dps_endpoint_update_subtype;

/**
 * \brief Describes the type of Endpoint location reply. The endpoint location reply
 *        message is qualified to indicate the attribute of the information carried in
 *        the reply. The sub_type is valid for the DPS_ENDPOINT_LOC_REPLY message only.
 */
typedef enum {
	DPS_ENDPOINT_LOC_REPLY_VM = 0,
	DPS_ENDPOINT_LOC_REPLY_GW = 1,
	DPS_ENDPOINT_LOC_REPLY_MAX = 2
} dps_endpoint_loc_reply_subtype;

/**
 * \brief Describes the sub_type for DPS_EXTERNAL_GW_REG_DEREG and
 *        DPS_VLAN_GW_REG_DEREG. 
 *        The client needs to fill the sub_type field in the
 *        dps_client_hdr_t with what is requested for the type of the msg
 */

typedef enum {
	DPS_TUNNEL_REG = 1,              // if the ext/vlan gws are to be registered
	DPS_TUNNEL_DEREG = 2,            // if the ext/vlan gw are to be deregistered
} dps_tunnel_reg_dereg_subtype;


/**
 * \brief Describes the Type of DPS Client. Note that the DPS Server itself
 *        may be a DPS Client for some operations
 */
typedef enum {
	/**
	 * \brief Message originated from DPS Policy Server
	 */
	DPS_POLICY_SERVER_ID = 1,
	/**
	 * \brief Message originated from DOVE Switch
	 */
	DPS_SWITCH_AGENT_ID = 2,
	/**
	 * \brief Message originated from VLAN Gateway
	 */
	DOVE_VLAN_GATEWAY_AGENT_ID = 3,
	/**
	 * \brief Message originated from External Gateway
	 */
	DOVE_EXTERNAL_GATEWAY_AGENT_ID = 4,
	/**
	 * \brief Message originated from the Controller. NOT USED at this
	 *        time.
	 */
	DOVE_CONTROLLER_AGENT_ID = 5,
} dps_client_id;

/**
 * \brief Describes the DPS Transaction Type related to Requests.
 *        For e.g. an request could have been initiated due to a replication
 *        from a DPS Server or a mass copy related transaction.
 */

typedef enum {
	/**
	 * \brief Normal transaction initiated by DPS Server or Client
	 */
	DPS_TRANSACTION_NORMAL = 1,
	/**
	 * \brief To be used for Replication Requests(DPS Server ONLY)
	 */
	DPS_TRANSACTION_REPLICATION = 2,
	/**
	 * \brief To be used during Mass Copy of Data from one DPS
	 *        Server to another (DPS Server ONLY)
	 */
	DPS_TRANSACTION_MASS_COPY = 3,
} dps_transaction_type;

/**
 * \brief Policy Type:
 *        MUST match with data_handler/python/dcs_objects/Policy.py
 */
typedef enum {
	DPS_POLICY_TYPE_CONNECTIVITY = 1,
	DPS_POLICY_TYPE_SOURCE_ROUTING = 2,
	DPS_POLICY_TYPE_MAX
} dps_policy_type;

/**
 * \brief This describes the structure of a IP Address Location
 */
typedef struct ip_addr_s {
	/**
	 * \brief Address family: AF_INET or AF_INET6. 0 means invalid.
	 */
	uint16_t family;
	/**
	 * \brief The Port number
	 */
	uint16_t port;
	/**
	 * \brief SOCK_STREAM or SOCK_DGRAM
	 */
	uint16_t xport_type;
	union{
		/**
		 * \brief RESERVED: DPS Client must not use this field
		 */
		uint16_t port_http;
		/**
		 * \brief RESERVED: DPS Client must not use this field
		 */
		uint16_t status;
	};
	union{
		/**
		 * \brief IPv4 address
		 */
		uint32_t ip4;
		/**
		 * \brief IPv6 Address
		 */
		uint8_t  ip6[16];
	};
	/* Need UUID ?????????? */
	//char uuid[128];
} ip_addr_t;

/**
 * \brief This describes the structure of a IPv4 Address
 */
typedef struct dps_ip4_info_s {
	uint32_t ip;
	uint16_t flags;
	uint16_t reserved;
} dps_ip4_info_t;

typedef struct dps_ip6_info_s {
	uint8_t ip[16];
	uint16_t flags;
	uint16_t reserved;
} dps_ip6_info_t;

typedef struct dps_ip4_info_list_s {
	/**
	 * \brief The number of ips present. If none present set it to 0
	 */
	uint32_t num_of_ips;

	/**
	 * \brief The list of ip addresses. 
	 */
	dps_ip4_info_t ip_info[MAX_VIP_ADDR];
} dps_ip4_info_list_t;

typedef struct dps_ip6_info_list_s {
	/**
	 * \brief The number of ips present. If none present set it to 0
	 */
	uint32_t num_of_ips;

	/**
	 * \brief The list of ip addresses. 
	 */
	dps_ip6_info_t ip_info[MAX_VIP_ADDR];
} dps_ip6_info_list_t;

/**
 * \brief The tunnel endpoint is a generic structure that contains information about 
 *        the tunnel. 
 */

typedef struct dps_tunnel_endpoint_s {
	/**
	 * \brief Address family: AF_INET or AF_INET6. 0 means invalid.
	 */
	uint16_t family;
	/**
	 * \brief The Port number
	 */
	uint16_t port;
		/**
	 * \brief The virtual network id
	 */
	uint32_t vnid;
	/**
	 * \brief The VXLAN/NVGRE
	 */	
#define TUNNEL_TYPE_VXLAN 0
#define TUNNEL_TYPE_NVGRE 1
	uint16_t tunnel_type;

	/**
	 * \brief The flags field is a bit mask indicating properties of a tunnel
	 *        When TUNNEL_IS_DESIGNATED_MCAST_GW bit is set it means all mcast control
	 *        traffic should be sent to this tunnel.
	 */	
#define TUNNEL_IS_DESIGNATED_MCAST_GW 0x01

	uint16_t flags;

	union{
		/**
		 * \brief IPv4 address
		 */
		uint32_t ip4;
		/**
		 * \brief IPv6 Address
		 */
		uint8_t  ip6[16];
	};

} dps_tunnel_endpoint_t;

typedef struct dps_tunnel_list_s {
	/**
	 * \brief The number of gateways present
	 */
	uint16_t num_of_tunnels;

	/**
	 * \brief The list of tunnel ip addresses. 
	 */
	dps_tunnel_endpoint_t tunnel_list[1];
} dps_tunnel_list_t;

typedef	struct {
		uint16_t euid_type;
		union {
			uint32_t euid_int;
			struct {
				uint32_t vnid;
				uint8_t  vmac[6];
				uint8_t  pad[2];
			} euid_vnid_mac;
		}u;
#define EUID_INT u.euid_int
#define EUID_VNID u.euid_dom_mac.vnid
#define EUID_VMAC u.euid_dom_mac.vmac
} euid_t;

/**
 * \brief Common header for all client requests
 */
typedef struct dps_client_hdr {
	/**
	 * \brief enum of type dps_client_req_type
	 */
	uint8_t type;
	/**
	 * \brief For Endpoint Update: Enum of type dps_endpoint_update_option
	 *        Some messages like dps_endpoint_update_option need a type qualifier
	 */
	uint8_t sub_type;
	/**
	 * \brief Enum of type dps_client_id.
	 */
	uint8_t client_id;
	/**
	 * \brief Enum of type dps_transaction_type. DPS Client MUST set this
	 *        to DPS_TRANSACTION_NORMAL
	 */
	uint8_t transaction_type;
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief Enum of type dps_resp_status_t
	 */
	uint32_t resp_status;
	/**
	 * \brief DPS Clients MUST set this to 0
	 *        DPS Server should set this field if it's replying to a request by
	 *        copying the query id from the request.
	 */
	uint32_t query_id;
	/**
	 * \brief DPS Clients MUST NOT set this field i.e. Reply_Addr.family MUST be set to 0
	 *        DPS Server should set this field for replication AND forwarding.
	 */
	ip_addr_t reply_addr;
} dps_client_hdr_t;

typedef struct dps_endpoint_loc_req_s {
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief The MAC address of Lookup. If none, this field must be
	 *        zeroed out by DPS Clients
	 */
	uint8_t mac[6];
	/**
	 * \brief Reserved: DO NOT USE
	 */
	uint8_t pad[2];
	/**
	 * \brief The Virtual IP Address of the Endpoint being looked up.
	 */
	ip_addr_t vm_ip_addr;
	/**
	 * \brief The DPS Client Address where the Endpoint Location Request
	 *        reply should be sent to.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 *        DPS Servers MUST SET this field while forwarding.
	 */
	ip_addr_t dps_client_addr;
}dps_endpoint_loc_req_t;

typedef struct dps_endpoint_loc_reply_s {
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief The MAC Address of the Endpoint (Must follow vnid)
	 */
	uint8_t mac[6];
	/**
	 * \brief Reserved: DO NOT USE
	 */
	uint8_t pad[2];
	/**
	 * \brief The VM Virtual IP Address
	 */
	ip_addr_t vm_ip_addr;
	/**
	 * \brief Version of the Endpoint (Registration)
	 */
	uint32_t version;
	/**
	 * \brief The IP Address of the DOVE Switch hosting the Endpoint
	 */
	dps_tunnel_list_t tunnel_info;

}dps_endpoint_loc_reply_t;

typedef struct dps_endpoint_update_s {
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief VALID MAC Address: Cannot be 0s
	 */
	uint8_t mac[6];
	/**
	 * \brief RESERVED
	 */
	uint8_t padding[2];
	/**
	 * \brief The Version of the Endpoint as seen by the DOVE Switch.
	 *        For the 1st Update, the version number MUST be set to 0.
	 *        From that point on every update should increment version
	 *        by 1. DCS will return DPS_ERROR_RETRY if it detects
	 *        an incorrect version number.
	 */
	uint32_t version;
	/**
	 * \brief The Virtual IP Address of the Endpoint
	 * TODO: Make this an ARRAY
	 */
	ip_addr_t vm_ip_addr;
	/*
	 * \brief The DPS Client Service Location (IP Address + Port) of the
	 *        Endpoint.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 *        DPS Server (during replication) MUST fill this field in
	 *        based on the address provided by the Protocol Handler.
	 */
	ip_addr_t dps_client_addr;
	/*
	 * \brief The DOVE Switch IP hosting the Endpoint. This MUST be
	 *        filled by the DPS Client
	 */
	dps_tunnel_list_t tunnel_info;
} dps_endpoint_update_t;



typedef struct dps_endpoint_update_reply_s {
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief VALID MAC Address: Cannot be 0s
	 */
	uint8_t mac[6];
	/**
	 * \brief RESERVED
	 */
	uint8_t padding[2];
	
	/**
	 * \brief The Version of the Endpoint as seen by the DPS Servers
	 */
	uint32_t version;
	/**
	 * \brief List of Virtual IP Address(es) of the Endpoint.
	 *        Set the  num_of_vip field to the number of valid
	 *        vip addresses present in the vm_ip_addr array.
	 */
	uint32_t num_of_vip;
	ip_addr_t vm_ip_addr[MAX_VIP_ADDR];
	/*
	 * \brief The DOVE Switch IP hosting the Endpoint. This MUST be
	 *        filled by the DPS Client
	 */
	dps_tunnel_list_t tunnel_info;
} dps_endpoint_update_reply_t;

typedef struct dps_endpoint_info_s {
	/**
	 * \brief The vnid of the Endpoint
	 */
	uint32_t vnid;
	/**
	 * \brief MAC Address if present - otherwise 0s
	 */
	uint8_t mac[6];
	/**
	 * \brief RESERVED
	 */
	uint8_t padding[2];
	/**
	 * \brief The Virtual IP Address of the Request
	 */
	ip_addr_t vm_ip_addr;

} dps_endpoint_info_t;

// Policy related structures

typedef struct src_dst_vnid_s {
	/**
	 * \brief The Source VNID (32 bits)
	 */
	uint32_t svnid;
	/**
	 * \brief The Destination VNID (32 bits)
	 */
	uint32_t dvnid;
} src_dst_vnid_t;

typedef struct dps_policy_s {
	/**
	 * \brief The Policy Version (Depends on Type)
	 */
	uint8_t version;
	/**
	 * \brief Policy Type: 1 = Connectivity, 2 = Source Routing
	 *        ABiswas:
	 *        At this time source routing is not yet supported
	 */
	uint8_t policy_type;
	union {
		struct {
			/**
			 * \brief The number of permit rules
			 */
			uint16_t num_permit_rules;
			/**
			 * \brief The number of deny rules
			 */
			uint16_t num_deny_rules;
			/**
			 * \brief The List of rules. If there are permit rules,
			 *        they will be indexed first followed by deny
			 *        rules.
			 */
			src_dst_vnid_t src_dst_vnid[1];
		} vnid_policy;
		struct {
			/**
			 * \brief The IP Address Count
			 */
			uint16_t address_count;
			/**
			 * \brief List of IP Addresses
			 */
			uint8_t ip_list[0];
		} source_routing;
	};
} dps_policy_t;

typedef struct dps_policy_info_s {
	/**
	 * \brief The Policy ID
	 */
	uint32_t policy_id;
	/**
	 * \brief The Policy Version
	 */
	uint32_t version;
	/**
	 * \brief The Time to Live
	 */
	uint32_t ttl;
	/**
	 * \brief The Policy
	 */
	dps_policy_t dps_policy;
} dps_policy_info_t;

typedef struct dps_policy_req_s {
	/**
	 * \brief The Destination Endpoint Information
	 */
	dps_endpoint_info_t dst_endpoint;
	/**
	 * \brief The Source Endpoint Information
	 */
	dps_endpoint_info_t src_endpoint;
	/**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;
} dps_policy_req_t;

typedef struct dps_policy_reply_s {
	/**
	 * \brief The Policy Details
	 */
	dps_policy_info_t dps_policy_info;
	/**
	 * \brief The destination Endpoint Location
	 */
	dps_endpoint_loc_reply_t dst_endpoint_loc_reply;
} dps_policy_reply_t;

/**
 * \brief The DPS server sends this msg when a EndpointUpdate msg
 *        is received and is the first registration  for the vnid
 *        specified in the EndpointUpdate msg. This is to expedite
 *        policy processing for the Dove Switch and is an optimization.
 *        Subsequently only delta changes in policy rules will be sent
 *        to the Dove Switches.
 *        The default vnid policy is a deny.
 */
typedef struct dps_bulk_vnid_policy_s {
	/**
	 * \brief The number of permit rules
	 */
	uint16_t num_permit_rules;
	/**
	 * \brief The number of deny rules
	 */
	uint16_t num_deny_rules;
	/**
	 * \brief The List of rules. If there are permit rules,
	 *        they will be indexed first followed by deny
	 *        rules.
	 */
	src_dst_vnid_t src_dst_vnid[1];
} dps_bulk_vnid_policy_t;

/**
 * \brief The dps_epri_t structure contains endpoint reachability information. It has information about
 *        all the configured ip addresses of the VM as well as the tunnel endpoints.  
 *        a VM moving location. 
 */
typedef struct dps_epri_s {
	/**
	 * \brief The Tenant being looked up
	 */
	uint32_t vnid;
	/**
	 * \brief The MAC Address of the Endpoint (Must follow vnid)
	 */
	uint8_t mac[6];
	/**
	 * \brief Reserved: DO NOT USE
	 */
	uint8_t pad[2];
	/**
	 * \brief The VM Virtual IPv4 Address. 
	 */
	dps_ip4_info_list_t vm_ip4_addr;

	/**
	 * \brief The VM Virtual IPv6 Address
	 */
	dps_ip6_info_list_t vm_ip6_addr;

	/**
	 * \brief The IP Address of the DOVE Switch hosting the Endpoint
	 */
	dps_tunnel_list_t tunnel_info;
} dps_epri_t;

// Gateway Data Structure
/**
 * \brief A request for a list of internal gateways 
 */
typedef  struct dps_internal_gw_req_s {
    /**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;

} dps_internal_gw_req_t;

/**
 * \brief Internal gateways are virtual gateways in the overlay.
 *        They are used when traffic needs to be routed between
 *        VMs on different subnets.
 */
typedef struct dps_internal_gw_s {
	/**
	 * \brief The number of v4 internal gateways
	 */
	uint16_t num_v4_gw;
	/**
	 * \brief The number of v6 internal gateways
	 */
	uint16_t num_v6_gw;
	/**
	 * \brief The list of internal gateways. The size of the gateway list
	 *        will be the sum of the num_v4_gw + num_v6_gw. The first
	 *        elements of the gw_list are v4 gateways up to a max of num_v4_gw 
	 *        followed by a list of v6 gateways up to a max of num_v6_gw 
	 */
	uint32_t gw_list[0];
} dps_internal_gw_t;

/**
 * \brief A general request used by differnt message types. The msg types
 *        are the following-DPS_BCAST_LIST_REQ, DPS_EXTERNAL_GW_LIST_REQ,
 *        DPS_VLAN_GW_LIST_REQ, DPS_VNID_POLICY_LIST_REQ, DPS_CTRL_PLANE_HB
 *        For example, when requesting a bcast list the following
 *        structure is used with the a msg type of DPS_BCAST_LIST_REQ.
 *        The message type is set in the client header.
 */
typedef struct dps_gen_msg_req_t {
	/**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;

} dps_gen_msg_req_t;

// Dove Switch List Structure
/**
 * \brief The DPS server sends a list of Dove switches in the domain.
 *        The list is used by the Dove switches for broadcast traffic received
 *        from the VM.
 */
typedef struct dps_pkd_tunnel_list_t {
	/**
	 * \brief The vnid value used by the tunnels
	 */
	uint32_t vnid;
	/**
	 * \brief The number of Dove switches with v4 ip addresses
	 */
	uint16_t num_v4_tunnels;
	/**
	 * \brief The number of Dove switches with v6 ip addresses
	 */
	uint16_t num_v6_tunnels;
	/**
	 * \brief The list of dove switches. The size of the bcast list
	 *        will be the sum of the num_v4_switch + num_v6_switch. The first
	 *        elements of the bcast_list are v4 switches up to a max of num_v4_switch 
	 *        followed by a list of v6 switches up to a max of num_v6_switches 
	 */
	uint32_t tunnel_list[0];
} dps_pkd_tunnel_list_t;


/**
 * \brief A Dove switch on receiving data traffic for a VM that has moved, will send the migrated
 *        VMs info (IP address and mac address) and the source VMs location to the DPS.
 *        The DPS on receiving this message will send the migrated VMs new location if present to the
 *        Dove switch on which the source VM resides, so that it can update its cache with the new 
 *        location of the the destination VM.
 */
typedef struct dps_vm_migration_event_s {
	/**
	 * \brief Information about the vm that has moved. If vnid information for the VM is 
	 *        not available set it to 0.
	 */
	dps_endpoint_info_t migrated_vm_info;

	/**
	 * \brief The information related to the source VMs location. This includes the VMs
	 *        information as well as information about the Dove switch on which the source
	 *        VM resides.
	 */
	dps_endpoint_loc_reply_t src_vm_loc;
} dps_vm_migration_event_t;


/**
 * \brief Tunnel Registration structures. Dove Switch/External Gw/Vlan Gw all register
 *        their tunnel endpoints. The message type is DPS_EXT_GW_REG_DEREG/DPS_VLAN_GW_REG_DEREG 
 *        with a sub_type field set to DPS_REG/DPS_DEREG. In the case of ADD/DELETE there
 *        is only the ip address of the tunnel endpoint present. In the case of 
 *        MODIFY the old ip address and the new ip address must be present. The MODIFY
 *        can be used when any attribute of the tunnel endpoint changes. 
 *  
 */

typedef struct tunnel_endpoint_reg_dereg_s {
	/**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;
	/**
	 * \brief List of Tunnel IP Addresses. Should be the final member of
	 *        this structure
	 */
	dps_tunnel_list_t tunnel_info;
} dps_tunnel_reg_dereg_t;

/**
 * \brief Multicast Structures
 *
 */

/**
 * \brief When ip address is available set the mcast_addr_type to MCAST_ADDR_V4/MCAST_ADDR_V6
 *        and set the mcast_mac to the corresponding ip address mac.
 *        If only multicast mac address is available then set the type to MCAST_ADDR_MAC
 *        with the ip address field set  to 0.
 *        The MCAST_ADDR_V4_ICB are v4 mcast addresses in the range 224.0.1.0 - 238.255.255.255.
 *        These addresses have internetwork wide significance. When mcast_addr_type is set to
 *        the above type, it indicates that a dove switch wants to register as a receiver for the
 *        entire range. The mac address is not set when MCAST_ADDR_V4_ICB is specified
 */

#define MCAST_ADDR_MAC          0
#define MCAST_ADDR_V4           1 
#define MCAST_ADDR_V6           2 
#define MCAST_ADDR_V4_ICB_RANGE 3

typedef	struct {
		uint16_t mcast_addr_type;
		uint8_t  mcast_mac[6];
		union {
			uint32_t mcast_ip4;
			uint8_t  mcast_ip6[16];
		}u;
} dps_mcast_addr_t;

/**
 * \brief The structure is used by multicast receivers to signal the DPS that
 *        it needs to subscribe to the multicast address specified in mcast_addr.
 *        If the receiver wants to specify a particular src it needs to receive
 *        the mcast traffic from, then it can specify the srcs. Specifiying the source 
 *        of the multicast is for those switches that support source specific multicast. 
 *        If the num_of_srcs is set to 0 then the receiver does not care about the src of
 *        the mcast traffic. The sources should be all ipv6 or all ipv4 addresses, with
 *        family set to AF_INET or AF_INET6.
 */

typedef struct dps_mcast_group_record_s {
	/**
	 * \brief The Multicast Address: A multicast address of 01:00:5E:00:00:00
	 *        represents ALL multicast i.e. the receiver is unaware of the exact
	 *        Multicast MAC. In the VMware case, 01:00:5E:00:00:00 should be
	 *        used in the Multicast Lance/Hash case.
	 */
	dps_mcast_addr_t mcast_addr;
	/**
	 * \brief Support for Source Routing is not enabled in this phase for DOVE
	 */
	uint16_t num_of_srcs;
	/**
	 * \brief AF_INET or AF_INET6 when Source Specific Multicast is supported
	 */
	uint16_t family;
	/*
	 * \brief A List of Source Addresses when Source Specific Multicast is supported.
	 */
	uint32_t src_addr[1];
} dps_mcast_group_record_t;

/**
 * \brief The structure is used for multicast source registration and de-registration
 *        When the source starts sending a mcast stream the Dove Switch sends a 
 *        DPS_MCAST_SENDER_REGISTRATION msg to DPS. When a source stops sending a 
 *        DPS_MCAST_SENDER_DEREGISTRATION msg is sent to the DPS.
 */
typedef struct dps_mcast_sender_s {
	/**
	 * \brief The IP Address of the DOVE Switch hosting the source of the mcast stream
	 */
	ip_addr_t tunnel_endpoint;
	/**
	 * \brief Information about the src VM sending the mcast stream
	 */
	dps_endpoint_info_t mcast_src_vm;
	/**
	 * \brief The multicast address of the mcast stream
	 */
	dps_mcast_addr_t mcast_addr;
	/**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;
} dps_mcast_sender_t;


/**
 * \brief The structure is used for multicast join/leave.
 *        When a Dove Switch wants to receive multicast traffic, it sends a 
 *        DPS_MCAST_RECEIVER_JOIN msg to the DPS. On leave it sends a DPS_MCAST_RECEIVER_LEAVE msg.
 *        It contains the mcast address and tunnel endpoint.  The mcast address can be a 
 *        mac address or an ip address.
*/
typedef struct dps_mcast_receiver_s {
	/**
	 * \brief The IP Address of the DOVE Switch that wants to receive the mcast stream
	 */
	ip_addr_t tunnel_endpoint;
	/**
	 * \brief The DPS Client IP Address.
	 *        DPS Clients MUST NOT set this value i.e. family should be 0
	 */
	ip_addr_t dps_client_addr;
	/**
	 * \brief The mcast group that the receiver is subscribing to. The group record is a variable list of
	 *        sources that the receiver wants to receive traffic from for the particular mcast group.
	 *        The list of sources are for those switches that support source specific multicast and should
	 *        be of the same type either ipv4 or ipv6.
	 */
	dps_mcast_group_record_t mcast_group_rec;

} dps_mcast_receiver_t;

/**
 * \brief The structure is encoded in a msg of type DPS_MCAST_RECEIVER_DS_LIST
 *        and is sent by the DPS to Dove Switches on which senders for the
 *        mcast_addr reside. The  mcast_ds_list is a list of Dove Switches
 *        on which VMs which want to receive mcast traffic for the address
 *        specified in the mcast_addr reside.
 * \note This list is already pruned based on Policy.
 *
*/
typedef struct  dps_mcast_recvr_ds_list_s {
	/**
	 * \brief The mcast address stream
	 */
	dps_mcast_addr_t mcast_addr;
	/**
	 * \brief The number of receiver VNIDs
	 */
	uint32_t num_of_rec;
	/**
	 * \brief The list of tunnels which have joined the group
	 */
	dps_pkd_tunnel_list_t mcast_recvr_list[1];
} dps_mcast_receiver_ds_list_t;

/**
 * \brief The structure is used to invalidate a VMs forwarding entry and is
 *        sent by DCS to all DS in the domain. 
 *
*/
typedef struct dps_vm_invalidate_s {
	dps_epri_t epri;
} dps_vm_invalidate_t;

typedef struct dps_client_data_s {
	void *context;
	dps_client_hdr_t hdr;
	union {
		dps_endpoint_loc_req_t          endpoint_loc_req;
		dps_endpoint_loc_reply_t        endpoint_loc_reply;
		dps_policy_req_t                policy_req;
		dps_policy_reply_t              policy_reply;
		dps_endpoint_update_t           endpoint_update;
		dps_endpoint_update_reply_t     endpoint_update_reply;
		dps_internal_gw_req_t           internal_gw_req;
		dps_internal_gw_t               internal_gw_list;
		// dps_bulk_vnid_policy_t used by DPS_BULK_VNID_POLICY_XFER/DPS_VNID_POLICY_LIST_REPLY
		dps_bulk_vnid_policy_t          bulk_vnid_policy;
		// dps_gen_msg_req_t used by DPS_INTERNAL_GW_REQ/DPS_BCAST_LIST_REQ/DPS_EXTERNAL_GW_LIST_REQ
		// DPS_VLAN_GW_LIST_REQ/DPS_BULK_VNID_POLICY_REQ/DPS_MCAST_CTRL_GW_REQ
		dps_gen_msg_req_t               gen_msg_req;
		// dove_switch_list used by DPS_BCAST_LIST_REPLY/DPS_UNSOLICITED_BCAST_LIST_REPLY
		dps_pkd_tunnel_list_t           dove_switch_list;
		dps_vm_migration_event_t        vm_migration_event;
		dps_mcast_sender_t              mcast_sender;
		dps_mcast_receiver_t            mcast_receiver;
		dps_mcast_receiver_ds_list_t    mcast_receiver_ds_list;
		// Used by DPS_UNSOLICITED_EXTERNAL_GW_LIST/DPS_UNSOLICITED_VLAN_GW_LIST/DPS_MCAST_CTRL_GW_REPLY msgs
		dps_tunnel_list_t               tunnel_info;   
		// Used by DPS_TUNNEL_REGISTER/DPS_TUNNEL_DEREGISTER msgs
		dps_tunnel_reg_dereg_t          tunnel_reg_dereg;  
		// Used by DPS_ADDR_RESOLVE
		dps_endpoint_loc_req_t          address_resolve;
		// Used by DPS_UNSOLICITED_INVALIDATE_VM
		dps_vm_invalidate_t             vm_invalidate_msg;
	};
} dps_client_data_t;

#define MAX_DPS_SERVER_NODES            1
#define DPS_MAX_BUFF_SZ                32768

/**
 * \brief The Structure that store the Location of a DPS Node
 *        in the DPS Cluster.
 */
typedef struct dps_svr_addr_info_s {
	/**
	 * \brief AF_INET or AF_INET6
	 */
	uint16_t	family;
	/**
	 * \brief Source IPv4 Location (Network Order)
	 */
	struct		sockaddr_in svr_ip4;
	/**
	 * \brief Source IPv6 Location
	 */
	struct		sockaddr_in6 svr_ip6;
	/**
	 * \brief The Socket Descriptor
	 */
	int		sock_fd;
	/**
	 * \brief SOCK_DGRAM or SOCK_STREAM
	 */
	uint8_t		xport_type;
	/**
	 * \brief Whether the structure is in use
	 */
	uint8_t		inuse;
	/**
	 * \brief The Buffer used to receive data from the socket
	 */
	int8_t		buff[DPS_MAX_BUFF_SZ];

} dps_svr_addr_info_t;

#define VNID_NODE_MAPPING_HASH_BITS 6

typedef struct dps_vnid_node_mapping_table_s {
	/**
	 * \brief The VNID->Node table lock.
	 */
	sem_t lock;
	/**
	 * \brief Number of bits in the hash. MAX 16
	 */
	uint16_t hash_bits;
	/**
	 * \brief Size of the hash = 2 ^ hash_bits
	 */
	uint16_t hash_size;
	/**
	 * \brief Array of hash list heads.
	 */
	dps_hlist_head hash[1<<VNID_NODE_MAPPING_HASH_BITS];
} dps_vnid_node_mapping_table_t;

/**
 * \brief Structure representing a VNID->Node entry
 */
typedef struct dps_vnid_node_mapping_entry_s {
	/**
	 * \brief Linkage to the hash list
	 */
	dps_hlist_node hlist_entry;
	/**
	 * \brief The VNID
	 */
	uint32_t vn_id;
	/**
	 * \brief Pointer to a DPS node
	 */
	ip_addr_t *svr_node;
} dps_vnid_node_mapping_entry_t;

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
dps_return_status dps_protocol_client_send(dps_client_data_t *msg);

/*
 ******************************************************************************
 * dps_protocol_send_to_client                                            *//**
 *
 * \brief - This routine sends the Message to be Processed by the Server/Client
 *          side Data Handler
 *
 * \param[in] msg - Pointer to a message (Type dps_client_data_t)
 *
 ******************************************************************************
 */
void dps_protocol_send_to_client(dps_client_data_t *msg);

/*
 ******************************************************************************
 * dps_protocol_construct_reply_msg                                       *//**
 *
 * \brief - This routine constructs a reply from the given protocol message.
 *
 * \param[in] msg - Pointer to a message (Type dps_client_data_t)
 * \param[in] err_code - The Error Code to set in the Reply message
 ******************************************************************************
 */
void dps_protocol_construct_reply_msg(dps_client_data_t *msg,
                                      dps_resp_status_t err_code);

/*
 * Communication APIs
 */

/**
 * \brief The callback for the Poll Events. This callback will be
 *        invoked if the related file descriptor has been triggered
 *        during the poll wait.
 *  \param[in] fd The File Descriptor on which this event happened
 *  \param[in] context The context for the callback
 *
 *  \return If the callback realized that it had unfinished processing to
 *          perform but voluntarily gave up the CPU since it didn't want
 *          to hog the processing, the callback should return a non-zero
 *          value. The poll processing routine will continue processing
 *          till all callbacks return 0, before going back to sleep.
 */
typedef int (*poll_event_callback)(int fd, void *context);

/*
 ******************************************************************************
 * fd_process_add_fd --                                                   *//**
 *
 * \brief This routine adds a new file descriptor to the array of polled
 *        file descriptors
 *
 * \param[in] fd	The File Descriptor
 * \param[in] callback	The Callback to invoke when an event occurs on the fd
 * \param[in] context	The Context to invoke with the callback
 *
 * \return The Status of the Add
 *
 * \note This routine updates the Global fd_count_total to the number of
 *       FDs that can be polled
 *
 *****************************************************************************/

int fd_process_add_fd(int fd,
                      poll_event_callback callback,
                      void *context);

/*
 ******************************************************************************
 * fd_process_del_fd --                                                    *//**
 *
 * \brief This routine removes a file descriptor to the array of polled
 *        file descriptors
 *
 * \param[in] fd	The File Descriptor
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_INVALID_FD File Descriptor doesn't exist in Array
 *
 * \note This routine updates the Global fd_count_total to the number of
 *       FDs that can be polled
 *
 *****************************************************************************/

int fd_process_del_fd(int fd);

/** @} */

#endif
