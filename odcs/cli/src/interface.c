/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      interface.c
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

int CliLogLevel = DPS_SERVER_LOGLEVEL_WARNING;

/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLI
 * @{
 */

/**
 * \brief An Array of Callbacks for every CLI DPS Code
 */
static cli_callback_func cli_callback_array[CLI_MAX];

/*
 ******************************************************************************
 * dps_server_cli_callback                                                *//**
 *
 * \brief - The Main Callback function when a CLI is invoked
 *
 * \param cli: The CLI Context Structure
 *
 * \return dove_status
 *
 ******************************************************************************
 */

dove_status dps_server_cli_callback(void *cli)
{
	cli_dps_t *cli_dps = (cli_dps_t *)cli;
	cli_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter: Major Code %d", cli_dps->major_code);

	if (cli_dps->major_code < CLI_MAX)
	{
		func = cli_callback_array[cli_dps->major_code];
		log_debug(CliLogLevel, "Function 0x%p", func);
		if (func)
		{
			status = func((void *)&cli_dps->u);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * dcs_server_cli_init                                                    *//**
 *
 * \brief - Initializes the DCS Server CLI
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dcs_server_cli_init(void)
{

	dove_status status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter");

	// Initialize the MAJOR TYPE CLI callbacks here

	do
	{
		cli_callback_array[CLI_CONFIG] = cli_config_callback;
		status = cli_config_init();
		if (status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"dps_server_cli_config_init returns %s",
			         DOVEStatusToString(status));
		}

		cli_callback_array[CLI_CLIENT_SERVER_PROTOCOL] = cli_client_server_protocol_callback;
		status = cli_client_server_protocol_init();
		if (status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_client_server_protocol_init returns %s",
			         DOVEStatusToString(status));
		}

		cli_callback_array[CLI_DATA_OBJECTS] = cli_data_object_callback;
		status = cli_data_object_init();
		if (status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_data_object_init returns %s",
			         DOVEStatusToString(status));
		}

		cli_callback_array[CLI_CLUSTER] = cli_cluster_callback;
		status = cli_cluster_init();
		if (status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_cluster_init returns %s",
			         DOVEStatusToString(status));
		}

		cli_callback_array[CLI_CORE_API] = NULL;
		cli_callback_array[CLI_CONTROLLER] = NULL;
		cli_callback_array[CLI_REST] = NULL;
		cli_callback_array[CLI_OSW] = NULL;
		cli_callback_array[CLI_WEB_SERVER] = NULL;
		cli_callback_array[CLI_SPIDERCAST] = NULL;

		cli_callback_array[CLI_DEBUG] = cli_debug_callback;
		status = cli_debug_init();
		if (status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_debug_init returns %s",
			         DOVEStatusToString(status));
		}

	}
	while (0);


	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;
}
/** @} */

