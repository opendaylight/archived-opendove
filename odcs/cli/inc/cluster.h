/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      client_server_protocol.h
 *
 *  Abstract:
 *      The CLI related structure that are passed from the CLI to
 *      the DCS Server Infrastructure
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

#ifndef _CLI_CLIENT_CLUSTER_
#define _CLI_CLIENT_CLUSTER_

/*
 *****************************************************************************
 * DPS Cluster CLI Related Structures                                    *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLICluster Clustering Services
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/cluster.py
 *
 */

/**
 * \brief The MINOR Command Code for DPS CLUSTER CLI Requests sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_CLUSTER_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_CLUSTER_CODES \
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_LOG_LEVEL,                0)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DPS_NODE_ADD,             1)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DPS_NODE_DEL,             2)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DPS_NODE_ADD_DOMAIN,      3)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DPS_NODE_DEL_DOMAIN,      4)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DPS_NODE_DEL_ALL_DOMAINS, 5)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_SHOW,                     6)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_DOMAIN_GET_NODES,         7)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_HEAVY_LOAD_THRESHOLD,     8)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_LOCAL_NODE_ACTIVATE,      9)\
	CLI_CLUSTER_CODE_AT(CLI_CLUSTER_MAX,                      10)\

#define CLI_CLUSTER_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_CLUSTER_CODES
}cli_cluster_code_t;
#undef CLI_CLUSTER_CODE_AT

/**
 * \brief The Structure for Changing Cluster Log Level
 */
typedef struct cli_cluster_log_level_s{
	/**
	 * \brief log_level representing log level defined in log.h file
	 */
	uint32_t	level;
}cli_cluster_log_level_t;

/**
 * \brief The Structure adding a DPS Node to Cluster. Note that when using the CLI
 *        a DPS Node must be added via CLI to every other node in the Cluster.
 *        In other words the CLI does not propagate the added to other Nodes in
 *        the cluster.
 *        This is only used for testing purposes till the Spidercast mechanism
 *        for gossip protocol is ready.
 */
typedef struct cli_cluster_node_add_s{
	/**
	 * \brief Port
	 */
	uint16_t port;
	/**
	 * \brief Padding
	 */
	uint16_t pad;
	/**
	 * \brief The DPS Node IP Address Value
	 */
	uint32_t IP_type;
	/*
	 * \brief The DPS Node IP type AF_INET or AF_INET6 (Network Byte Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_cluster_node_add_t;

typedef struct cli_cluster_node_delete_s{
	/**
	 * \brief The DPS Node IP Address Value
	 */
	uint32_t IP_type;
	/*
	 * \brief The DPS Node IP type AF_INET or AF_INET6
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_cluster_node_delete_t;

typedef cli_cluster_node_delete_t cli_cluster_node_delete_all_domains_t;

/**
 * \brief The Structure adding a Domain to a DPS Node in the Cluster.
 *        Note this information is not propagated to other Nodes in
 *        the Cluster if done through the CLI.
 */
typedef struct cli_cluster_node_add_domain_s{
	/**
	 * \brief The Domain being added
	 */
	uint32_t domain;
	/**
	 * \brief Port
	 */
	uint16_t port;
	/**
	 * \brief Padding
	 */
	uint16_t pad;
	/*
	 * \brief The DPS Node IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The DPS Node IP Address Value (Network Byte Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_cluster_node_add_domain_t;

typedef struct cli_cluster_node_delete_domain_s{
	/**
	 * \brief The Domain being added
	 */
	uint32_t domain;
	/*
	 * \brief The DPS Node IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The DPS Node IP Address Value (Network Byte Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_cluster_node_delete_domain_t;

/**
 * \brief The Structure for printing the nodes associated with a domain
 */
typedef struct cli_cluster_domain_nodes_s{
	/**
	 * \brief Domain ID
	 */
	uint32_t domain_id;
	/**
	 * \brief A maximum number of nodes
	 */
	uint32_t max_nodes;
}cli_cluster_domain_nodes_t;

/**
 * \brief The Structure for printing the nodes associated with a domain
 */
typedef struct cli_cluster_local_activate_s{
	/**
	 * \brief Domain ID
	 */
	uint32_t factive;
}cli_cluster_local_activate_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_cluster_s{
	/**
	 * \brief Enum of type cli_client_server_protocol_code_t
	 */
	uint32_t cluster_code;
	union{
		cli_cluster_log_level_t log_level;
		cli_cluster_node_add_t node_add;
		cli_cluster_node_delete_t node_delete;
		cli_cluster_node_add_domain_t node_domain_add;
		cli_cluster_node_delete_domain_t node_domain_delete;
		cli_cluster_node_delete_all_domains_t node_domain_delete_all;
		cli_cluster_domain_nodes_t domain_nodes_get;
		cli_cluster_log_level_t heavy_load_threshold;
		cli_cluster_local_activate_t local_activate;
	};
}cli_cluster_t;

/*
 ******************************************************************************
 * cli_cluster_callback                                                   *//**
 *
 * \brief - The callback for CLI_CLUSTER
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_cluster_callback(void *cli);

/*
 ******************************************************************************
 * cli_cluster_init                                                       *//**
 *
 * \brief - Initializes the CLI_CLUSTER related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_cluster_init(void);

/** @} */
/** @} */

#endif // _CLI_CLIENT_CLUSTER_
