/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      debug.h
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

#ifndef _CLI_DATA_HANDLER_DEBUG_
#define _CLI_DATA_HANDLER_DEBUG_

/*
 *****************************************************************************
 * DPS Client Server Protocol Related Structures                         *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLIDebug Data Handler Debugging
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/debug.py
 *
 */

/**
 * \brief The MINOR Command Code for DPS DATA HANDLER DEBUG CLI Requests
 *        sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_CLIENT_SERVER_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_DEBUG_CODES \
	CLI_DEBUG_CODE_AT(CLI_DEBUG_LOG_LEVEL,                    0)\
	CLI_DEBUG_CODE_AT(CLI_DEBUG_GET_VNID_ENDPOINTS,           1)\
	CLI_DEBUG_CODE_AT(CLI_DEBUG_GET_VNID_DOVE_SWITCHES,       2)\
	CLI_DEBUG_CODE_AT(CLI_DEBUG_GET_VNID_GATEWAYS,            3)\
	CLI_DEBUG_CODE_AT(CLI_DEBUG_MAX,                          4)\

#define CLI_DEBUG_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_DEBUG_CODES
}cli_debug_code_t;
#undef CLI_DEBUG_CODE_AT

/**
 * \brief The Structure for changing log level
 */
typedef struct cli_debug_log_level_s{
	/**
	 * \brief log_level representing log level defined in log.h file
	 */
	uint32_t	level;
}cli_debug_log_level_t;

/**
 * \brief The Structure for getting VNID information
 */
typedef struct cli_debug_vnid_s{
	/**
	 * \brief The VNID
	 */
	uint32_t	vnid;
}cli_debug_vnid_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_debug_s{
	/**
	 * \brief Enum of type cli_debug_code_t
	 */
	uint32_t debug_code;
	union{
		cli_debug_log_level_t log_level;
		cli_debug_vnid_t vnid;
	};
}cli_debug_t;

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
dove_status cli_debug_callback(void *cli);

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
dove_status cli_debug_init(void);

/** @} */
/** @} */

#endif // _CLI_DATA_HANDLER_DEBUG_
