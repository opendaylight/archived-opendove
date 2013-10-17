/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      debug.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DCS Server for Debugging Details
 *
 */


#include "include.h"
#include "interface.h"
#include <time.h>


/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLIDebug
 * @{
 */

/**
 * \brief The callback function for CLI_DEBUG code
 */

typedef dove_status (*cli_debug_callback_func)(cli_debug_t *);

/**
 * \brief An Array of Callbacks for every CLI_DEBUG_CODES Code
 */
static cli_debug_callback_func cli_callback_array[CLI_DEBUG_MAX];

/*
 ******************************************************************************
 * log_level                                                          *//**
 *
 * \brief - Changes the Log Level Debugging
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status log_level(cli_debug_t *cli_debug)
{
	PythonDebugHandlerLogLevel = (int32_t)cli_debug->log_level.level;
	log_info(CliLogLevel, "PythonDebugHandlerLogLevel set to %d", PythonDebugHandlerLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * get_vnid_endpoints                                                     *//**
 *
 * \brief - Gets the Endpoints in a VNID
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status vnid_endpoints(cli_debug_t *cli_debug)
{
	return vnid_get_endpoints(cli_debug->vnid.vnid);
}

/*
 ******************************************************************************
 * get_vnid_dove_switches                                                 *//**
 *
 * \brief - Gets the DOVE Switches in a VNID
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status vnid_dove_switches(cli_debug_t *cli_debug)
{
	return vnid_get_dove_switches(cli_debug->vnid.vnid, NULL, 0);
}

/*
 ******************************************************************************
 * cli_debug_callback                                                     *//**
 *
 * \brief - The callback for CLI_DEBUG
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_debug_callback(void *cli)
{
	cli_debug_t *cli_debug = (cli_debug_t *)cli;
	cli_debug_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter");

	if (cli_debug->debug_code < CLI_DEBUG_MAX)
	{
		func = cli_callback_array[cli_debug->debug_code];
		if (func)
		{
			status = func(cli_debug);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;

}

/*
 ******************************************************************************
 * cli_debug_init                                                         *//**
 *
 * \brief - Initializes the CLI_DEBUG related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_debug_init(void)
{

	log_debug(CliLogLevel, "Enter");

	dove_status status = DOVE_STATUS_OK;
	cli_callback_array[CLI_DEBUG_LOG_LEVEL] = log_level;
	cli_callback_array[CLI_DEBUG_GET_VNID_ENDPOINTS] = vnid_endpoints;
	cli_callback_array[CLI_DEBUG_GET_VNID_DOVE_SWITCHES] = vnid_dove_switches;
	cli_callback_array[CLI_DEBUG_GET_VNID_GATEWAYS] = NULL;

	log_debug(CliLogLevel, "Exit");

	return status;
}
/** @} */

