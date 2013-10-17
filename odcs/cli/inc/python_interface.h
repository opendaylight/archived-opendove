/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      python_interface.h
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

#ifndef _CLI_PYTHON_INTERFACE_
#define _CLI_PYTHON_INTERFACE_

/*
 *****************************************************************************
 * DPS Client Server Protocol Related Structures                         *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLIPythonInterface Python Interface
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/python_interface.py
 *
 */

/**
 * \brief The MINOR Command Code for DPS PYTHON INTERFACE CLI Requests
 *        sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_PY_INTERFACE_NONE MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_PYTHON_INTERFACE_CODES \
	CLI_PYTHON_INTERFACE_CODE_AT(CLI_PY_INTERFACE_LOG_LEVEL,        0)\
	CLI_PYTHON_INTERFACE_CODE_AT(CLI_PY_INTERFACE_NONE,             1)\

#define CLI_PYTHON_INTERFACE_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_PYTHON_INTERFACE_CODES
}cli_py_interface_code_t;
#undef CLI_PYTHON_INTERFACE_CODE_AT

/**
 * \brief The Structure for Changing Client Server Protocol Log Level
 */
typedef struct cli_py_interface_log_level_s{
	/**
	 * \brief log_level representing log level defined in log.h file
	 */
	uint32_t	log_level;
}cli_py_interface_log_level_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_base_s{
	/**
	 * \brief Enum of type cli_py_interface_code_t
	 */
	uint32_t	py_interface_code;
	union{
		cli_py_interface_log_level_t log_level;
	}u;
}cli_base_t;

/** @} */
/** @} */

#endif // _CLI_PYTHON_INTERFACE_
