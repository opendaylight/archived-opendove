/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      data_objects.h
 *
 *  Abstract:
 *      The CLI related structure that are passed from the CLI to
 *      the DCS Server Infrastructure for monitoring/testing/updating
 *      Data Objects
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _CLI_DATA_OBJECTS_
#define _CLI_DATA_OBJECTS_

/*
 *****************************************************************************
 * DPS Client Server Protocol Related Structures                         *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLIDataObjects Data Objects
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/data_objects.py
 *
 */

/**
 * \brief The MINOR Command Code for DPS DATA OBJECTS CLI Requests
 *        sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_DATA_OBJECTS_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
//CLI_DATA_OBJECTS_QUERY_VNID: DPS query Dove Controller the related info of a certain VNID(dvg)
#define CLI_DATA_OBJECTS_CODES \
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DEV_LOG_LEVEL,         0)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_ADD,            1)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_DELETE,         2)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DVG_ADD,               3)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DVG_DELETE,            4)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ENDPOINT_UPDATE,       5)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ENDPOINT_LOOKUP_MAC,   6)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ENDPOINT_LOOKUP_IP,    7)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_POLICY_LOOKUP_VMAC,    8)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_POLICY_LOOKUP_IP,      9)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_SHOW,           10)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_PERFORMANCE_TEST,      11)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_POLICY_ADD,            12)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_POLICY_DELETE,         13)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_ADD,  14)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_DEL,  15)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_CLR,  16)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_CREATE,      17)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_ADD_SUBNET,  18)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_DELETE_SUBNET, 19)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_LOOKUP,      20)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_DESTROY,     21)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_LIST,        22)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_IP_SUBNET_FLUSH,       23)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ENDPOINT_UPDATE_CONFLICT_TEST, 24)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_OUTSTANDING_CONTEXT_COUNT, 25)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_VNID_SHOW,             26)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_QUERY_VNID,            27)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_GLOBAL_SHOW,    28)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_TUNNEL_REGISTER,       29)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_TUNNEL_UNREGISTER,     30)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_RECEIVER_ADD, 31)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_RECEIVER_DEL, 32)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_SENDER_ADD,   33)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_SENDER_DEL,   34)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_SHOW,         35)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ADDRESS_RESOLUTION_SHOW, 36)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_GET,   37)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_VLAN_GATEWAY_GET,       38)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_HEARTBEAT_REPORT_INTERVAL_SET,  39)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_STATISTICS_REPORT_INTERVAL_SET, 40)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MASS_TRANSFER_GET_READY,        41)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MASS_TRANSFER_START,            42)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MASS_TRANSFER_DOMAIN_ACTIVATE,  43)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_DEACTIVATE,      44)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_LOG_LEVEL,    45)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_UPDATE,          46)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_LOAD_BALANCE_TEST,      47)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DOMAIN_DELETE_ALL_LOCAL,    48)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_ENDPOINT_MIGRATE_HINT,  49)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD,  50)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL,  51)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_GET,  52)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_DPS_CLIENTS_SHOW,       53)\
	CLI_DATA_OBJECTS_CODE_AT(CLI_DATA_OBJECTS_MAX,                    54)\


#define CLI_DATA_OBJECTS_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_DATA_OBJECTS_CODES
}cli_data_objects_code_t;
#undef CLI_DATA_OBJECTS_CODE_AT

/**
 * \brief The Structure for Changing Data Object Log Level
 */
typedef struct cli_data_object_log_level_s{
	/**
	 * \brief log_level representing log level defined in log.h file
	 */
	uint32_t	level;
}cli_data_object_log_level_t;

/**
 * \brief The Structure for Adding Domain
 */
typedef struct cli_data_object_domain_add_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	domain_id;
}cli_data_object_domain_add_t;

/**
 * \brief The Structure for Adding Domain
 */
typedef struct cli_data_object_domain_update_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	domain_id;
	/**
	 * \brief The Replication Factor
	 */
	uint32_t	replication_factor;
}cli_data_object_domain_update_t;

/**
 * \brief The Structure for Deleting Domain
 */
typedef struct cli_data_object_domain_delete_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	domain_id;
}cli_data_object_domain_delete_t;

/**
 * \brief The Structure for Adding DVG
 */
typedef struct cli_data_object_dvg_add_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	domain_id;
	/**
	 * \brief The DVG ID
	 */
	uint32_t	dvg_id;
}cli_data_object_dvg_add_t;

/**
 * \brief The Structure for Deleting DVG
 */
typedef struct cli_data_object_dvg_delete_s{
	/**
	 * \brief The DVG ID
	 */
	uint32_t	dvg_id;
}cli_data_object_dvg_delete_t;

/**
 *\brief The structure for policy action
 */
typedef struct cli_data_object_policy_action_s{
	/*
	 * \brief The connectivity or source routing
	 */
	uint8_t ver;
	uint8_t pad;
	union
	{
		uint16_t connectivity;
		struct
		{
			uint16_t address_count;
			uint8_t ip_list[0];
		} source_routing;
	};
} cli_data_object_policy_action_t;

/**
 * \brief The Structure for Adding Policy
 */
typedef struct cli_data_object_policy_add_s{
	/**
	 * \brief traffic type
	 */
	uint32_t traffic_type;
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The Policy Type
	 */
	uint32_t type;
	/*
	 * \brief The Source DVG ID
	 */
	uint32_t src_dvg_id;
	/*
	 * \brief The Destination DVG ID
	 */
	uint32_t dst_dvg_id;
	/*
	 * \brief Time-To-Live
	 */
	uint32_t ttl;
	/*
	 * \brief action
	 */
	cli_data_object_policy_action_t action;
}cli_data_object_policy_add_t;

/**
 * \brief The Structure for Deleting Policy
 */
typedef struct cli_data_object_policy_delete_s{
	/**
	 * \brief traffic type
	 */
	uint32_t traffic_type;
	/**
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The Source DVG ID
	 */
	uint32_t src_dvg_id;
	/*
	 * \brief The Destination DVG ID
	 */
	uint32_t dst_dvg_id;
}cli_data_object_policy_delete_t;

/**
 * \brief The Structure for Endpoint Update
 */
typedef struct cli_data_object_endpoint_update_s{
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/**
	 * \brief The Operation (dps_endpoint_update_option)
	 */
	uint32_t update_op;
	/**
	 * \brief The dps_client_id type
	 */
	uint32_t client_type;
	/*
	 * \brief The Physical IP type AF_INET or AF_INET6
	 */
	uint32_t pIP_type;
	/**
	 * \brief The Physical IP Address Value (Network Work Order)
	 */
	union {
		uint32_t pIPv4;
		char pIPv6[16];
	};
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t vIP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t vIPv4;
		char vIPv6[16];
	};
	/**
	 * \brief The Virtual MAC Address Value
	 */
	char vMac[6];
}cli_data_object_endpoint_update_t;

/**
 * \brief The Structure for Endpoint Lookup by vMAC
 */
typedef struct cli_data_object_endpoint_lookup_vMac_s{
	/**
	 * \brief The VNID ID
	 */
	uint32_t vnid;
	/**
	 * \brief The Virtual MAC Address Value
	 */
	char vMac[6];
}cli_data_object_endpoint_lookup_vMac_t;

/**
 * \brief The Structure for Endpoint Lookup by vIP
 */
typedef struct cli_data_object_endpoint_lookup_vIP_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t vnid;
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t vIP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t vIPv4;
		char vIPv6[16];
	};
}cli_data_object_endpoint_lookup_vIP_t;

/**
 * \brief The Structure for Policy Lookup by vMAC
 */
typedef struct cli_data_object_policy_lookup_vMac_s{
	/*
	 * \brief The Source VNID
	 */
	uint32_t src_vnid;
	/**
	 * \brief The Virtual MAC Address Value
	 */
	char vMac[6];
}cli_data_object_policy_lookup_vMac_t;

/**
 * \brief The Structure for Policy Lookup by vIP
 */
typedef struct cli_data_object_policy_lookup_vIP_s{
	/*
	 * \brief The Source VNID
	 */
	uint32_t src_vnid;
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t vIP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t vIPv4;
		char vIPv6[16];
	};
}cli_data_object_policy_lookup_vIP_t;

/**
 * \brief The Structure for Showing Domain Details
 */
typedef struct cli_data_object_domain_show_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	domain_id;
	/**
	 * \brief Whether to show the details
	 *        0 - Don't show details
	 *        1 - Do show details
	 */
	uint32_t	fDetails;
}cli_data_object_domain_show_t;

/**
 * \brief The Structure for Showing VNID Details
 */
typedef struct cli_data_object_vnid_show_s{
	/**
	 * \brief The VNID
	 */
	uint32_t	vnid;
	/**
	 * \brief Whether to show the details
	 *        0 - Don't show details
	 *        1 - Do show details
	 */
	uint32_t	fDetails;
}cli_data_object_vnid_show_t;

/**
 * \brief The Structure for Query VNID for Dove Controller
 */
typedef struct cli_data_object_query_vnid_s{
	/**
	 * \brief The VNID
	 */
	uint32_t	vnid;
}cli_data_object_query_vnid_t;

/**
 * \brief The Structure for adding a Gateway to a Domain
 */
typedef struct cli_data_object_gateway_s{
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/*
	 * \brief The IP type AF_INET or AF_INET6 of gateway
	 */
	uint32_t IP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_data_object_gateway_t;

/**
 * \brief The Structure for Hash Performance Test
 */
typedef struct cli_data_object_perf_test_s{
	/**
	 * \brief The count of number of times the test is repeated
	 */
	int	iterations;
	/**
	 * \brief the count of number of times the lookup is repeated per iteration
	 */
	int	lookup_iterations;
	/**
	 * \brief The number of distinct Endpoint updates per test
	 */
	int	endpoints;
	/**
	 * \brief The frequency of printing per test
	 */
	int	print_frequency;
	/**
	 * \brief The Number of Domains
	 */
	int	domains;
	/**
	 * \brief The Number of VNID per domain
	 */
	int	vnid_per_domain;
	/**
	 * \brief The Domain seed to start with, 0 means no seed
	 */
	int	domain_seed;
	/**
	 * \brief Whether to create domains and dvgs
	 */
	int	fcreate;
} cli_data_object_perf_test_t;

/**
 * \brief The Structure for Adding IP Subnet
 */
typedef struct cli_data_object_ip_subnet_add_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
	/**
	 * \brief The Mask Value or Prefix length (In network byte 
	 *        order)
	 */
	union {
		uint32_t mask;
		uint32_t prefix_len;
	};
	/**
	 * \brief The Mode Dedicated(0) or Shared(1)
	 */
	uint32_t mode;
	/**
	 * \brief The Gateway for the subnet (In network byte order)
	 */
	union {
		uint32_t gateway_v4;
		char gateway_v6[16];
	};
}cli_data_object_ip_subnet_add_t;

/**
 * \brief The Structure for Deleting IP Subnet
 */
typedef struct cli_data_object_ip_subnet_delete_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
	/**
	 * \brief The Mask Value or Prefix length (In network byte 
	 *        order)
	 */
	union {
		uint32_t mask;
		uint32_t prefix_len;
	};
}cli_data_object_ip_subnet_delete_t;

/**
 * \brief The Structure for Looking for a IP Subnet
 */
typedef struct cli_data_object_ip_subnet_lookup_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_data_object_ip_subnet_lookup_t;

/**
 * \brief The Structure for Listing a IP Subnet List
 */
typedef struct cli_data_object_ip_subnet_list_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
}cli_data_object_ip_subnet_list_t;

/**
 * \brief The Structure for Flushing a IP Subnet List
 */
typedef cli_data_object_ip_subnet_list_t cli_data_object_ip_subnet_flush_t;

/**
 * \brief The Structure for Registering a Tunnel IP Addresses
 */
typedef struct cli_data_object_tunnel_register_s{
	/**
	 * \brief The Client Type: dps_client_id
	 */
	uint32_t client_type;
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t num_tunnels;
	/**
	 * \brief The IP Address Value (Network Work Order)
	 */
	struct{
		/**
		 * \brief AF_INET or AF_INET6
		 */
		uint32_t IP_type;
		union {
			uint32_t pIPv4;
			char pIPv6[16];
		};
	}ip_list[4];
}cli_data_object_tunnel_register_t;

/**
 * \brief The Structure for (Un)Registering a Multicast Sender/Receiver
 */
typedef struct cli_data_object_multicast_register_s{
	/**
	 * \brief The Client Type: dps_client_id
	 */
	uint32_t client_type;
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/**
	 * \brief Multicast IP type 0, AF_INET, AF_INET6
	 *        type 0 indicates IP is not known
	 */
	uint32_t multicast_IP_type;
	/**
	 * \brief The MAC
	 */
	char mac[6];
	/**
	 * \brief The Multicast IP
	 */
	union {
		uint32_t multicast_IPv4;
		char multicast_IPv6[16];
	};
	/**
	 * \brief Tunnel Address AF_INET or AF_INET6
	 */
	uint32_t tunnel_IP_type;
	union {
		uint32_t tunnel_IPv4;
		char tunnel_IPv6[16];
	};
}cli_data_object_multicast_register_t;


/**
 * \brief The Structure for Setting Heartbeat Interval
 */
typedef struct cli_data_object_heartbeat_interval_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	interval;
}cli_data_object_heartbeat_interval_t;

/**
 * \brief The Structure for Setting Statistics Interval
 */
typedef struct cli_data_object_statistics_interval_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t	interval;
}cli_data_object_statistics_interval_t;

/**
 * \brief The Structure for Endpoint Lookup by vIP
 */
typedef struct cli_data_object_domain_transfer_s{
	/**
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6 of DCS Server to transfer to
	 */
	uint32_t IP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_data_object_domain_transfer_t;

/**
 * \brief The Structure for showing Multicast
 */
typedef struct cli_data_object_multicast_show_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
}cli_data_object_multicast_show_t;

typedef struct cli_data_object_endpoint_migrate_s{
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/*
	 * \brief The Physical IP type AF_INET or AF_INET6 of the Source
	 *        Tunnel Endpoint
	 */
	uint32_t src_pIP_type;
	/**
	 * \brief The Physical IP Address Value (Network Work Order)
	 */
	union {
		uint32_t src_pIPv4;
		char src_pIPv6[16];
	};
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t src_vIP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t src_vIPv4;
		char src_vIPv6[16];
	};
	/*
	 * \brief The Virtual IP type AF_INET or AF_INET6
	 */
	uint32_t dst_vIP_type;
	/**
	 * \brief The Virtual IP Address Value (Network Work Order)
	 */
	union {
		uint32_t dst_vIPv4;
		char dst_vIPv6[16];
	};
}cli_data_object_endpoint_migrate_t;

/**
 * \brief The Structure of the CLI for Data Objects
 */
typedef struct cli_data_object_s{
	/**
	 * \brief The Type of the CLI
	 */
	uint32_t	cli_data_object_code;
	union {
		cli_data_object_log_level_t log_level;
		cli_data_object_domain_add_t domain_add;
		cli_data_object_domain_update_t domain_update;
		cli_data_object_domain_delete_t domain_delete;
		cli_data_object_dvg_add_t dvg_add;
		cli_data_object_dvg_delete_t dvg_delete;
		cli_data_object_policy_add_t policy_add;
		cli_data_object_policy_delete_t policy_delete;
		cli_data_object_endpoint_update_t endpoint_update;
		cli_data_object_endpoint_lookup_vMac_t endpoint_lookup_vMac;
		cli_data_object_endpoint_lookup_vIP_t endpoint_lookup_vIP;
		cli_data_object_policy_lookup_vMac_t policy_lookup_vMac;
		cli_data_object_policy_lookup_vIP_t policy_lookup_vIP;
		cli_data_object_domain_show_t domain_show;
		cli_data_object_perf_test_t perf_test;
		cli_data_object_gateway_t gateway_update;
		cli_data_object_dvg_delete_t gateway_clear;
		cli_data_object_dvg_delete_t gateway_get;
		cli_data_object_ip_subnet_add_t ip_subnet_add;
		cli_data_object_ip_subnet_delete_t ip_subnet_delete;
		cli_data_object_ip_subnet_lookup_t ip_subnet_lookup;
		cli_data_object_ip_subnet_list_t ip_subnet_list;
		cli_data_object_ip_subnet_flush_t ip_subnet_flush;
		cli_data_object_vnid_show_t vnid_show;
		cli_data_object_query_vnid_t query_vnid;
		cli_data_object_tunnel_register_t tunnel_reg;
		cli_data_object_tunnel_register_t tunnel_dereg;
		cli_data_object_multicast_register_t multicast_register;
		cli_data_object_heartbeat_interval_t heartbeat_interval;
		cli_data_object_statistics_interval_t statistics_interval;
		cli_data_object_domain_transfer_t domain_transfer;
		cli_data_object_multicast_show_t multicast_show;
		cli_data_object_perf_test_t load_balance_test;
		cli_data_object_endpoint_migrate_t endpoint_migrate;
		cli_data_object_dvg_delete_t multicast_control_get;
	};
}cli_data_object_t;

/*
 ******************************************************************************
 * cli_data_object_callback                                               *//**
 *
 * \brief - The callback for CLI_DATA_OBJECTS
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_data_object_callback(void *cli);

/*
 ******************************************************************************
 * cli_data_object_init                                        *//**
 *
 * \brief - Initializes the CLI_DATA_OBJECTS related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_data_object_init(void);

/** @} */
/** @} */

#endif // _CLI_DATA_OBJECTS_
