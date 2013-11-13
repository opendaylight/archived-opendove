/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#ifndef _CLI_MAIN_MENU_
#define _CLI_MAIN_MENU_

/*
 *****************************************************************************
 * CLI (Main Menu) Configuration Handling                                *//**
 *
 * \addtogroup DOVEGatewayCLI
 * @{
 * \defgroup DOVEGatewayCLIMainMenu Main Menu for CLI configuration
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/gateway_cli/main_menu.py
 *
 */

/**
 * \brief The MINOR Command Code for MAIN MENU CLI Requests sent to DOVE
 *        GATEWAY
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_MAIN_MENU_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_MAIN_MENU_CODES \
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_LOGIN,                         0)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_ENTER,                         1)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_CLEAR_SCREEN,                  2)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_SHELL,                         3)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_LOG_CONSOLE,                   4)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_LOG_CLI,                       5)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_LOG_DPS,                       6)\
	CLI_MAIN_MENU_CODE_AT(CLI_MAIN_MENU_MAX,                           7)

#define CLI_MAIN_MENU_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_MAIN_MENU_CODES
}cli_main_menu_code;
#undef CLI_MAIN_MENU_CODE_AT

/**
 * \brief The Structure for Logging to Console
 */
typedef struct cli_main_menu_log_console_s{
	uint32_t	log_console;
}cli_main_menu_log_console_t;

/**
 * \brief The Structure for Changing Log Level
 */
typedef struct cli_main_menu_log_level_s{
	uint32_t	log_level;
}cli_main_menu_log_level_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_main_menu_s{
	uint32_t	main_menu_code;
	union{
		cli_main_menu_log_console_t cli_log_console;
		cli_main_menu_log_level_t cli_log_level;
	};
}cli_main_menu_t;

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
dove_status cli_main_menu_callback(void *cli);

/*
 ******************************************************************************
 * cli_main_menu_init                                                     *//**
 *
 * \brief - Initializes the DPS Server CLI_MAIN_MENU related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_main_menu_init(void);

/** @} */
/** @} */

#endif // _CLI_MAIN_MENU_
