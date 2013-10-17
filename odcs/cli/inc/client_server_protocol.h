/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      client_server_protocol.h
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

#ifndef _CLI_CLIENT_SERVER_PROTOCOL_
#define _CLI_CLIENT_SERVER_PROTOCOL_

/*
 *****************************************************************************
 * DPS Client Server Protocol Related Structures                         *//**
 *
 * \addtogroup DPSServerCLI
 * @{
 * \defgroup DPSServerCLIClientServerProtocol Client Server Protocol
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/dps_cli/client_server_protocol.py
 *
 */

/**
 * \brief The MINOR Command Code for DPS CLIENT SERVER PROTOCOL CLI Requests
 *        sent to DPS SERVER
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_CLIENT_SERVER_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_CLIENT_SERVER_PROTOCOL_CODES \
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_SERVER_DEV_LOG_LEVEL,        0)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_SERVER_CUST_LOG_LEVEL,       1)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_SERVER_STATISTICS_SHOW,      2)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_SERVER_STATISTICS_CLEAR,     3)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_RETRANSMIT_SHOW,             4)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_RETRANSMIT_LOG_LEVEL,        5)\
	CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(CLI_CLIENT_SERVER_MAX,                  6)\

#define CLI_CLIENT_SERVER_PROTOCOL_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_CLIENT_SERVER_PROTOCOL_CODES
}cli_client_server_protocol_code_t;
#undef CLI_CLIENT_SERVER_PROTOCOL_CODE_AT

/**
 * \brief The Structure for Changing Client Server Protocol Log Level
 */
typedef struct cli_cs_protocol_log_level_s{
	/**
	 * \brief log_level representing log level defined in log.h file
	 */
	uint32_t	level;
}cli_cs_protocol_log_level_t;

/**
 * \brief The Structure for fetching statistics.
 */
typedef struct cli_cs_protocol_stats_s{
	/**
	 * \brief Represents DPS Request/Reply Message defined in
	 *        the dps_client_req_type file
	 * \remarks A pkt_type of 0 represents Wildcard i.e. All
	 *          request/reply types
	 */
	uint32_t pkt_type;
}cli_cs_protocol_stats_t;

/**
 * \brief The CLI Base Structure
 */
typedef struct cli_client_server_protocol_s{
	/**
	 * \brief Enum of type cli_client_server_protocol_code_t
	 */
	uint32_t client_server_protocol_code;
	union{
		cli_cs_protocol_log_level_t log_level;
		cli_cs_protocol_stats_t stats_type;
	};
}cli_client_server_protocol_t;

/*
 ******************************************************************************
 * cli_client_server_protocol_callback                                    *//**
 *
 * \brief - The callback for CLI_CLIENT_SERVER_PROTOCOL
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_client_server_protocol_callback(void *cli);

/*
 ******************************************************************************
 * cli_client_server_protocol_init                                        *//**
 *
 * \brief - Initializes the CLI_CLIENT_SERVER_PROTOCOL. related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_client_server_protocol_init(void);

/** @} */
/** @} */

#endif // _CLI_CLIENT_SERVER_PROTOCOL_
