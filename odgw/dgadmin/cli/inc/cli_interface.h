/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */

#ifndef _DGWY_CLI_INC_INTERFACE_
#define _DGWY_CLI_INC_INTERFACE_


/*
 *****************************************************************************
 * DOVE Gateway CLI Related Structures                                   *//**
 *
 * \addtogroup DOVEGateway
 * @{
 * \defgroup DOVEGatewayCLI DOVE Gateway CLI
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/gateway_cli/config.py
 *
 */

/**
 * \brief The MAJOR Command Code for CLI Requests sent to DPS Server
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                              ------------                  -----
#define CLI_MAJOR_CODES \
	CLI_MAJOR_CODE_AT(CLI_MAIN_MENU,                          0)\
	CLI_MAJOR_CODE_AT(CLI_SERVICE,                            1)\
	CLI_MAJOR_CODE_AT(CLI_DPS_PROTOCOL,                       2)\
	CLI_MAJOR_CODE_AT(CLI_WEB_SERVICES,                       3)\
	CLI_MAJOR_CODE_AT(CLI_MAX,                                4) // Must be FINAL element

#define CLI_MAJOR_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_MAJOR_CODES
}cli_major_code;
#undef CLI_MAJOR_CODE_AT

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_base_s{
	uint32_t	menu_code;
	// The REST of the structure
}cli_base_t;

/**
 * \brief The CLI Structure
 */
typedef struct cli_dove_gateway_s{
	uint32_t	major_code;
	union{
		cli_base_t	menu;
	}u;
}cli_dove_gateway_t;

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

#endif // _DGWY_CLI_INC_INTERFACE_
