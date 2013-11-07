/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 *  Header File:
 *      interface.h
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

#ifndef _CLI_INC_INTERFACE_
#define _CLI_INC_INTERFACE_

#include "config.h"
#include "client_server_protocol.h"
#include "python_interface.h"
#include "data_objects.h"
#include "cluster.h"
#include "debug.h"

/*
 *****************************************************************************
 * DPS CLI Related Structures                                            *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup DPSServerCLI DCS Server CLI
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
 * \brief The MAJOR Command Code for CLI Requests sent to DCS Server
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_MAJOR_CODES \
	CLI_MAJOR_CODE_AT(CLI_CONFIG,                             0)\
	CLI_MAJOR_CODE_AT(CLI_CLIENT_SERVER_PROTOCOL,             1)\
	CLI_MAJOR_CODE_AT(CLI_DATA_OBJECTS,                       2)\
	CLI_MAJOR_CODE_AT(CLI_CLUSTER,                            3)\
	CLI_MAJOR_CODE_AT(CLI_CORE_API,                           4)\
	CLI_MAJOR_CODE_AT(CLI_CONTROLLER,                         5)\
	CLI_MAJOR_CODE_AT(CLI_REST,                               6)\
	CLI_MAJOR_CODE_AT(CLI_OSW,                                7)\
	CLI_MAJOR_CODE_AT(CLI_WEB_SERVER,                         8)\
	CLI_MAJOR_CODE_AT(CLI_SPIDERCAST,                         9)\
	CLI_MAJOR_CODE_AT(CLI_DEBUG,                              10)\
	CLI_MAJOR_CODE_AT(CLI_MAX,                                11) // Must be FINAL element

#define CLI_MAJOR_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_MAJOR_CODES
}cli_major_code;
#undef CLI_MAJOR_CODE_AT

/**
 * \brief The CLI Structure
 */
typedef struct cli_dps_s{
	uint32_t	major_code;
	union{
		cli_config_t	config;
	}u;
}cli_dps_t;

/**
 * \brief The callback function for minor DPS CLI codes
 */

typedef dove_status (*cli_callback_func)(void *cli);

/**
 * \brief The Variable for the CLI Log Level
 */
extern int CliLogLevel;

/** @} */
/** @} */

#endif // _CLI_INC_INTERFACE_
