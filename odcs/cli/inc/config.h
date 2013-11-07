/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      config.h
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

#ifndef _CLI_CONFIG_
#define _CLI_CONFIG_

/*
 *****************************************************************************
 * CLI (config) Configuration Handling                                   *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLIConfig Base CLI Configuration
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/config.py
 *
 */

/**
 * \brief The MINOR Command Code for CONFIG CLI Requests sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_CONFIG_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_CONFIG_CODES \
	CLI_CONFIG_CODE_AT(CLI_CONFIG_LOGIN,                         0)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_ENTER,                         1)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_CLEAR_SCREEN,                  2)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_SHELL,                         3)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_LOG_CONSOLE,                   4)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_LOG_CLI,                       5)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_NODE_LOCATION,                 6)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_CONTROLLER_LOCATION_SET,       7)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_CONTROLLER_LOCATION_GET,       8)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_TEST_IP_SUBNET,                9)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_REST_API_LOG_LEVEL,            10)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_STATISTICS_THREAD_ACTION,      11)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_QUERY_DC_CLUSTER_NODE_INFO,    12)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_SHOW_UUID,                     13)\
	CLI_CONFIG_CODE_AT(CLI_CONFIG_MAX,                           14)

#define CLI_CONFIG_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_CONFIG_CODES
}cli_config_code;
#undef CLI_CONFIG_CODE_AT

/**
 * \brief The Structure for Logging to Console
 */
typedef struct cli_config_log_console_s{
	uint32_t	log_console;
}cli_config_log_console_t;

/**
 * \brief The Structure for Changing Log Level
 */
typedef struct cli_config_log_level_s{
	uint32_t	log_level;
}cli_config_log_level_t;

/**
 * \brief The Structure adding a Location (IP Address + Port)
 */
typedef struct cli_config_location_s{
	/**
	 * \brief Port (Host Order)
	 */
	uint16_t port;
	/**
	 * \brief Padding
	 */
	uint16_t pad;
	/**
	 * \brief The DPS Node IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/*
	 * \brief The DPS Node IP Value (Network Byte Order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}cli_config_location_t;

/**
 * \brief The Structure for Thread Action
 */
typedef struct cli_config_thread_action_s{
	uint32_t	action;
}cli_config_thread_action_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_config_s{
	uint32_t	config_code;
	union{
		cli_config_log_console_t cli_log_console;
		cli_config_log_level_t cli_log_level;
		cli_config_location_t cli_controller_location;
		cli_config_thread_action_t cli_thread_action;
	};
}cli_config_t;

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
dove_status cli_config_callback(void *cli);

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
dove_status cli_config_init(void);

/** @} */
/** @} */

#endif // _CLI_CONFIG_
