/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      config.c
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
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLIConfig
 * @{
 */

/**
 * \brief The callback function for CLI_CONFIG Codes
 */

typedef dove_status (*cli_config_callback_func)(cli_config_t *);

/**
 * \brief An Array of Callbacks for every CLI_CONFIG Code
 */
static cli_config_callback_func cli_callback_array[CLI_CONFIG_MAX];

/*
 ******************************************************************************
 * cli_log_console                                                        *//**
 *
 * \brief - Changes the Log to Console Variable.
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_log_console(cli_config_t *cli_config)
{
	log_console = cli_config->cli_log_console.log_console;
	log_info(CliLogLevel, "log_console set to %d", log_console);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_log_level                                                          *//**
 *
 * \brief - Changes the Log Level of the CLI itself
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_log_level(cli_config_t *cli_config)
{
	CliLogLevel = (int)cli_config->cli_log_level.log_level;
	log_info(CliLogLevel, "CliLogLevel set to %d", CliLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_node_location                                                      *//**
 *
 * \brief - Gets the IP Address and Port location of the DCS Server Node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_node_location(cli_config_t *cli_config)
{

	char buf[INET6_ADDRSTRLEN];

	inet_ntop(dcs_local_ip.family, dcs_local_ip.ip6, buf, INET6_ADDRSTRLEN);
	show_print("IP Address <%s>, Port <%d>", buf, dcs_local_ip.port);

	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_controller_set                                                     *//**
 *
 * \brief - This routine sets the Controller Location
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_controller_set(cli_config_t *cli_config)
{
	dove_status status = DOVE_STATUS_OK;
	char buf[INET6_ADDRSTRLEN];
	dps_controller_data_op_t data;

	inet_ntop(cli_config->cli_controller_location.IP_type,
	          cli_config->cli_controller_location.IPv6,
	          buf, INET6_ADDRSTRLEN);
	show_print("Setting Controller IP Address <%s>, Port <%d>",
	           buf, cli_config->cli_controller_location.port);

	data.type = DPS_CONTROLLER_LOCATION_SET;
	data.controller_location.family = cli_config->cli_controller_location.IP_type;
	if (data.controller_location.family == AF_INET)
	{
		data.controller_location.ip4 =
			cli_config->cli_controller_location.IPv4;
	}
	else
	{
		memcpy(data.controller_location.ip6,
		       cli_config->cli_controller_location.IPv6, 16);
	}
	data.controller_location.port_http = cli_config->cli_controller_location.port;
	status = dps_controller_data_msg(&data);

	// send dps (dcs) appliance registration message to controller (dmc)
	if (DOVE_STATUS_OK == status)
	{
		//Redo the Set Service Role
		dcs_set_service_role(dcs_role_assigned);
	}
	return status;
}

/*
 ******************************************************************************
 * cli_controller_set                                                     *//**
 *
 * \brief - This routine gets the Controller Location
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_controller_get(cli_config_t *cli_config)
{
	dove_status status = DOVE_STATUS_OK;
	char buf[INET6_ADDRSTRLEN];
	dps_controller_data_op_t data;

	data.type = DPS_CONTROLLER_LOCATION_GET;
	status = dps_controller_data_msg(&data);
	if (status == DOVE_STATUS_OK)
	{
		inet_ntop(data.controller_location.family,
		          data.controller_location.ip6,
		          buf, INET6_ADDRSTRLEN);
		show_print("Controller IP Address <%s>, Port <%d>",
		           buf, data.controller_location.port);
	}
	return status;
}

/*
 ******************************************************************************
 * cli_show_uuid                                                      *//**
 *
 * \brief - Show UUID
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_show_uuid(cli_config_t *cli_config)
{

	show_uuid();
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_log_level_rest                                                     *//**
 *
 * \brief - Changes the Log Level of the REST Handler
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_log_level_rest(cli_config_t *cli_config)
{
	RESTHandlerLogLevel = (int)cli_config->cli_log_level.log_level;
	log_info(CliLogLevel, "RESTHandlerLogLevel set to %d", RESTHandlerLogLevel);
	return DOVE_STATUS_OK;
}


/*
 ******************************************************************************
 * cli_query_dc_cluster_info                                                     *//**
 *
 * \brief - DPS Send a query to Dove Controller for Cluster node info
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_query_dc_cluster_info(cli_config_t *cli_config)
{
	//RESTHandlerLogLevel = (int)cli_config->cli_log_level.log_level;
	log_info(CliLogLevel, "Send a Query to Dove Controller to get the DPS Cluster node info");
	dps_rest_client_query_dove_controller_cluster_info();
	return DOVE_STATUS_OK;
}


/*
 ******************************************************************************
 * cli_statistics_thread_action                                          *//**
 *
 * \brief - Start/Stop Statistics Thread
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status cli_statistics_thread_action(cli_config_t *cli_config)
{
	dove_status status = DOVE_STATUS_OK;
	unsigned int action = cli_config->cli_thread_action.action;

	switch(action)
	{
	case 1:
		status = dps_statistics_start();
		if (status != DOVE_STATUS_OK)
		{
			log_alert(CliLogLevel, "Fail to start statistics thread\r\n");
		}
		break;
	case 0:
		status = dps_statistics_stop();
		if (status != DOVE_STATUS_OK)
		{
			log_alert(CliLogLevel, "Fail to stop statistics thread\r\n");
		}
		break;
	}

	return status;
}

/*
 ******************************************************************************
 * cli_config_callback                                                    *//**
 *
 * \brief - The callback for CLI_CONFIG
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_config_callback(void *cli)
{
	cli_config_t *cli_config = (cli_config_t *)cli;
	cli_config_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter");

	if (cli_config->config_code < CLI_CONFIG_MAX)
	{
		func = cli_callback_array[cli_config->config_code];
		if (func)
		{
			status = func(cli_config);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;

}

/*
 ******************************************************************************
 * cli_config_init                                                        *//**
 *
 * \brief - Initializes the DCS Server CLI_CONFIG related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_config_init(void)
{

	log_debug(CliLogLevel, "Enter");

	dove_status status = DOVE_STATUS_OK;

	// Initialize the CLI_CONFIG callbacks here

	cli_callback_array[CLI_CONFIG_LOGIN] = NULL;
	cli_callback_array[CLI_CONFIG_ENTER] = NULL;
	cli_callback_array[CLI_CONFIG_CLEAR_SCREEN] = NULL;
	cli_callback_array[CLI_CONFIG_SHELL] = NULL;
	cli_callback_array[CLI_CONFIG_LOG_CONSOLE] = cli_log_console;
	cli_callback_array[CLI_CONFIG_LOG_CLI] = cli_log_level;
	cli_callback_array[CLI_CONFIG_NODE_LOCATION] = cli_node_location;
	cli_callback_array[CLI_CONFIG_CONTROLLER_LOCATION_SET] = cli_controller_set;
	cli_callback_array[CLI_CONFIG_CONTROLLER_LOCATION_GET] = cli_controller_get;
	cli_callback_array[CLI_CONFIG_TEST_IP_SUBNET] = NULL;
	cli_callback_array[CLI_CONFIG_REST_API_LOG_LEVEL] = cli_log_level_rest;
	cli_callback_array[CLI_CONFIG_STATISTICS_THREAD_ACTION] = cli_statistics_thread_action;
	cli_callback_array[CLI_CONFIG_QUERY_DC_CLUSTER_NODE_INFO] = cli_query_dc_cluster_info;
	
	cli_callback_array[CLI_CONFIG_SHOW_UUID] = cli_show_uuid;

	log_debug(CliLogLevel, "Exit");

	return status;
}
/** @} */

