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


#include "include.h"
#include "dgadmin_generic_api.h"
#include "cli_interface.h"


/*
 ******************************************************************************
 * DPS CLI Main Menu Functionality                                        *//**
 *
 * \ingroup DOVEGatewayCLIMainMenu
 * @{
 */

/**
 * \brief The callback function for CLI_MAIN_MENU Codes
 */

typedef dove_status (*cli_main_menu_callback_func)(cli_main_menu_t *);

/**
 * \brief An Array of Callbacks for every CLI_MAIN_MENU Code
 */
static cli_main_menu_callback_func cli_callback_array[CLI_MAIN_MENU_MAX];

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
static dove_status cli_log_console(cli_main_menu_t *cli_main_menu)
{
	log_console = cli_main_menu->cli_log_console.log_console;
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
static dove_status cli_log_level(cli_main_menu_t *cli_main_menu)
{
	CliLogLevel = (int)cli_main_menu->cli_log_level.log_level;
	log_info(CliLogLevel, "CliLogLevel set to %d", CliLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_log_level                                                          *//**
 *
 * \brief - Changes the Log Level of the CLI itself
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_log_level(cli_main_menu_t *cli_main_menu)
{
	DpsProtocolLogLevel = (int)cli_main_menu->cli_log_level.log_level;
	log_info(CliLogLevel, "DpsProtocolLogLevel set to %d", DpsProtocolLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_main_menu_callback                                                 *//**
 *
 * \brief - The callback for CLI_MAIN_MENU
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_main_menu_callback(void *cli)
{
	cli_main_menu_t *cli_main_menu = (cli_main_menu_t *)cli;
	cli_main_menu_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;
	dove_status dv_status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter");

	if (cli_main_menu->main_menu_code < CLI_MAIN_MENU_MAX)
	{
		func = cli_callback_array[cli_main_menu->main_menu_code];
		if (func)
		{
			status = func(cli_main_menu);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	if(status != DOVE_STATUS_OK)
	{
		dv_status = DOVE_STATUS_UNKNOWN;
	}
	return dv_status;

}

/*
 ******************************************************************************
 * cli_main_menu_init                                                     *//**
 *
 * \brief - Initializes the DOVE Gateway CLI_MAIN_MENU related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_main_menu_init(void)
{

	log_debug(CliLogLevel, "Enter");

	dove_status status = DOVE_STATUS_OK;
    dove_status dv_status = DOVE_STATUS_OK;

	// Initialize the CLI_MAIN_MENU callbacks here

	cli_callback_array[CLI_MAIN_MENU_LOGIN] = NULL;
	cli_callback_array[CLI_MAIN_MENU_ENTER] = NULL;
	cli_callback_array[CLI_MAIN_MENU_CLEAR_SCREEN] = NULL;
	cli_callback_array[CLI_MAIN_MENU_SHELL] = NULL;
	cli_callback_array[CLI_MAIN_MENU_LOG_CONSOLE] = cli_log_console;
	cli_callback_array[CLI_MAIN_MENU_LOG_CLI] = cli_log_level;
	cli_callback_array[CLI_MAIN_MENU_LOG_DPS] = dps_log_level;

	log_debug(CliLogLevel, "Exit");

	if (status != DOVE_STATUS_OK)
	{
		dv_status = DOVE_STATUS_UNKNOWN;
	}
	return dv_status;
}
/** @} */

