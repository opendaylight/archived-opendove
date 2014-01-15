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
 *      interface.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DPS Server
 *
 */


#include "dgadmin_generic_api.h"
#include "cli_interface.h"
#include <time.h>

int CliLogLevel = DPS_SERVER_LOGLEVEL_INFO;

/*
 ******************************************************************************
 * DOVE Gateway CLI Functionality                                         *//**
 *
 * \ingroup DOVEGatewayCLI
 * @{
 */

/**
 * \brief An Array of Callbacks for every CLI DPS Code
 */
static cli_callback_func cli_callback_array[CLI_MAX];

/*
 ******************************************************************************
 * dove_gateway_cli_callback                                              *//**
 *
 * \brief - The Main Callback function when a CLI is invoked
 *
 * \param cli: The CLI Context Structure
 *
 * \return dove_status
 *
 ******************************************************************************
 */

dove_status  dove_gateway_cli_callback(void *cli)
{
	cli_dove_gateway_t *cli_dove_gateway = (cli_dove_gateway_t *)cli;
	cli_callback_func func;
	dove_status dv_status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter: Major Code %d", cli_dove_gateway->major_code);

	if (cli_dove_gateway->major_code < CLI_MAX)
	{
		func = cli_callback_array[cli_dove_gateway->major_code];
		log_debug(CliLogLevel, "Function 0x%p", func);
		if (func)
		{
			dv_status = func((void *)&cli_dove_gateway->u);
		    log_debug(CliLogLevel, "dv_status %u", dv_status);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(dv_status));

	return dv_status;
}

/*
 ******************************************************************************
 * dove_gateway_cli_init                                                  *//**
 *
 * \brief - Initializes the DOVE Gateway CLI
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dove_gateway_cli_init(void)
{

    dove_status dv_status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter");

	// Initialize the MAJOR TYPE CLI callbacks here

	do
	{
		cli_callback_array[CLI_MAIN_MENU] = cli_main_menu_callback;
		dv_status = cli_main_menu_init();
		if (dv_status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_main_menu_init returns %s",
			         DOVEStatusToString(dv_status));
		}

		cli_callback_array[CLI_SERVICE] = cli_service_callback;
		dv_status = cli_service_init();
		if (dv_status != DOVE_STATUS_OK)
		{
			log_warn(CliLogLevel,"cli_service_init returns %s",
			         DOVEStatusToString(dv_status));
		}

		cli_callback_array[CLI_DPS_PROTOCOL] = NULL;
		cli_callback_array[CLI_WEB_SERVICES] = NULL;
	}
	while (0);


	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(dv_status));

	return dv_status;
}
/** @} */

