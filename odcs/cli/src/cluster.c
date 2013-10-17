/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      cluster.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DCS Server
 *
 */


#include "include.h"
#include "interface.h"
#include <time.h>


/*
 ******************************************************************************
 * DPS Cluster CLI Related Structures                                     *//**
 *
 * \ingroup DPSServerCLICluster
 * @{
 */

/**
 * \brief The callback function for CLI_CLUSTER code
 */

typedef dove_status (*cli_cluster_callback_func)(cli_cluster_t *);

/**
 * \brief An Array of Callbacks for every CLI_CLUSTER Code
 */
static cli_cluster_callback_func cli_callback_array[CLI_CLUSTER_MAX];


/*
 ******************************************************************************
 * log_level                                                              *//**
 *
 * \brief - Changes the Log Level for Cluster Configuration
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status log_level(cli_cluster_t *cli_cluster)
{
	PythonClusterDataLogLevel = (int32_t)cli_cluster->log_level.level;
	log_info(CliLogLevel, "DpsClusterLogLevel set to %d",
	         PythonClusterDataLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * local_activate                                                         *//**
 *
 * \brief - Activate/Deactivate local node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status local_activate(cli_cluster_t *cli_cluster)
{
	uint32_t factivate;
	dove_status status;

	if (cli_cluster->local_activate.factive)
	{
		factivate = 1;
	}
	else
	{
		factivate = 0;
	}
	status = dcs_set_service_role(factivate);
	return status;
}

/*
 ******************************************************************************
 * heavy_load_threshold_level                                             *//**
 *
 * \brief - Changes the Heavy Load Threshold Level for the cluster
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status heavy_load_threshold_level(cli_cluster_t *cli_cluster)
{
	dps_cluster_heavy_load_threshold_set(cli_cluster->heavy_load_threshold.level);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_node_add                                                           *//**
 *
 * \brief - This routine adds a dps node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_node_add(cli_cluster_t *cli_data)
{
	ip_addr_t ip;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	inet_ntop(cli_data->node_add.IP_type,
	          cli_data->node_add.IPv6,
	          str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel, "Enter IP %s: Port %d",
	           str, cli_data->node_add.port);

	do
	{
		if (!memcmp(dcs_local_ip.ip6, cli_data->node_add.IPv6, 16))
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		ip.family = cli_data->node_add.IP_type;
		memcpy(ip.ip6, cli_data->node_add.IPv6, 16);
		ip.xport_type = SOCK_DGRAM;
		ip.port = cli_data->node_add.port;
		status = dps_cluster_node_add(&ip);
		// exchange local domain mapping with all the other
		// nodes in the cluster
		dps_cluster_exchange_domain_mapping();
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit status %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * dps_node_delete                                                           *//**
 *
 * \brief - This routine deletes a dps node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_node_delete(cli_cluster_t *cli_data)
{
	ip_addr_t ip;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	inet_ntop(cli_data->node_delete.IP_type,
	          cli_data->node_delete.IPv6,
	          str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel, "Enter IP %s", str);

	do
	{
		if (!memcmp(dcs_local_ip.ip6, cli_data->node_delete.IPv6, 16))
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		ip.family = cli_data->node_delete.IP_type;
		memcpy(ip.ip6, cli_data->node_delete.IPv6, 16);
		ip.xport_type = SOCK_DGRAM;
		ip.port = 0;
		dps_cluster_node_delete(&ip);
		// exchange local domain mapping with all the other nodes
		dps_cluster_exchange_domain_mapping();
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit status %s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * dps_node_add_domain                                                    *//**
 *
 * \brief - This routine add a domain to a node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_node_add_domain(cli_cluster_t *cli_data)
{
	ip_addr_t ip;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");
	inet_ntop(cli_data->node_domain_add.IP_type,
	          cli_data->node_domain_add.IPv6,
	          str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel, "IP %s, Domain %d",
	         str, cli_data->node_domain_add.domain);

	do
	{
		if (!memcmp(dcs_local_ip.ip6, cli_data->node_domain_add.IPv6, 16))
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		ip.family = cli_data->node_domain_add.IP_type;
		memcpy(ip.ip6, cli_data->node_domain_add.IPv6, 16);
		ip.xport_type = SOCK_DGRAM;
		ip.port = cli_data->node_domain_add.port;
		dps_cluster_node_add_domain(&ip,
		                            cli_data->node_domain_add.domain,
		                            MIN_REPLICATION_FACTOR);
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit status %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * dps_node_delete_domain                                                 *//**
 *
 * \brief - This routine add a domain to a node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_node_delete_domain(cli_cluster_t *cli_data)
{
	ip_addr_t ip;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");
	inet_ntop(cli_data->node_domain_delete.IP_type,
	          cli_data->node_domain_delete.IPv6,
	          str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel, "IP %s, Domain %d",
	         str, cli_data->node_domain_delete.domain);

	do
	{
		if (!memcmp(dcs_local_ip.ip6, cli_data->node_domain_delete.IPv6, 16))
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		ip.family = cli_data->node_domain_delete.IP_type;
		memcpy(ip.ip6, cli_data->node_domain_delete.IPv6, 16);
		ip.xport_type = SOCK_DGRAM;
		ip.port = 0;
		dps_cluster_node_delete_domain(&ip, cli_data->node_domain_delete.domain);
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit status %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * dps_node_delete_all_domains                                            *//**
 *
 * \brief - This routine add a domain to a node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_node_delete_all_domains(cli_cluster_t *cli_data)
{
	ip_addr_t ip;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	inet_ntop(cli_data->node_domain_delete_all.IP_type,
	          cli_data->node_domain_delete_all.IPv6,
	          str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel, "Enter IP %s", str);

	do
	{
		if (!memcmp(dcs_local_ip.ip6, cli_data->node_domain_delete_all.IPv6, 16))
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		ip.family = cli_data->node_domain_delete_all.IP_type;
		memcpy(ip.ip6, cli_data->node_domain_delete_all.IPv6, 16);
		ip.xport_type = SOCK_DGRAM;
		ip.port = 0;
		dps_cluster_node_delete_all_domains(&ip);
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit status %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * show_cluster                                                           *//**
 *
 * \brief - This routine add a domain to a node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status show_cluster(cli_cluster_t *cli_data)
{
	dps_cluster_show();
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * domain_get_nodes                                                       *//**
 *
 * \brief - This routine displays all the nodes in a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_get_nodes(cli_cluster_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	ip_addr_t *ip_list;
	uint32_t list_size_needed, i;
	char str[INET6_ADDRSTRLEN];

	do
	{
		list_size_needed = sizeof(ip_addr_t)*cli_data->domain_nodes_get.max_nodes;
		ip_list = (ip_addr_t *)malloc(list_size_needed);
		if (ip_list == NULL)
		{
			status = DOVE_STATUS_EXCEEDS_CAP;
			break;
		}
		status = dps_get_domain_node_mapping(cli_data->domain_nodes_get.domain_id,
		                                     cli_data->domain_nodes_get.max_nodes,
		                                     ip_list,
		                                     &list_size_needed);
		if (status == DOVE_STATUS_OK)
		{
			for (i = 0; i < list_size_needed; i++)
			{
				if (ip_list[i].family == AF_INET)
				{
					inet_ntop(AF_INET, &ip_list[i].ip4, str, INET_ADDRSTRLEN);
					// IPv4
					show_print("DCS Node: IPv4 %s, Port %d",
					           str, ntohs(ip_list[i].port));
				}
				else
				{
					// IPv6
					inet_ntop(AF_INET6, &ip_list[i].ip6, str, INET6_ADDRSTRLEN);
					show_print("DCS Node: IPv6 %s, Port %d",
					           str, ntohs(ip_list[i].port));
				}
			}
		}
		else
		{
			show_print("#Nodes handling Domain %d, #Max. Nodes Given %d\n",
			           list_size_needed,
			        cli_data->domain_nodes_get.max_nodes);
		}
	} while(0);

	return status;
}

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
dove_status cli_cluster_callback(void *cli)
{
	cli_cluster_t *cli_cluster = (cli_cluster_t *)cli;
	cli_cluster_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter");

	if (cli_cluster->cluster_code < CLI_CLUSTER_MAX)
	{
		func = cli_callback_array[cli_cluster->cluster_code];
		if (func)
		{
			status = func(cli_cluster);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;

}

/*
 ******************************************************************************
 * cli_client_server_protocol_init                                        *//**
 *
 * \brief - Initializes the CLI_CLIENT_SERVER_PROTOCOL related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_cluster_init(void)
{

	log_debug(CliLogLevel, "Enter");

	dove_status status = DOVE_STATUS_OK;

	// Initialize the CLI_CLIENT_SERVER_PROTOCOL callbacks here

	cli_callback_array[CLI_CLUSTER_LOG_LEVEL] = log_level;
	cli_callback_array[CLI_CLUSTER_DPS_NODE_ADD] = dps_node_add;
	cli_callback_array[CLI_CLUSTER_DPS_NODE_DEL] = dps_node_delete;
	cli_callback_array[CLI_CLUSTER_DPS_NODE_ADD_DOMAIN] = dps_node_add_domain;
	cli_callback_array[CLI_CLUSTER_DPS_NODE_DEL_DOMAIN] = dps_node_delete_domain;
	cli_callback_array[CLI_CLUSTER_DPS_NODE_DEL_ALL_DOMAINS] = dps_node_delete_all_domains;
	cli_callback_array[CLI_CLUSTER_SHOW] = show_cluster;
	cli_callback_array[CLI_CLUSTER_DOMAIN_GET_NODES] = domain_get_nodes;
	cli_callback_array[CLI_CLUSTER_HEAVY_LOAD_THRESHOLD] = heavy_load_threshold_level;
	cli_callback_array[CLI_CLUSTER_LOCAL_NODE_ACTIVATE] = local_activate;

	log_debug(CliLogLevel, "Exit");

	return status;
}
/** @} */

