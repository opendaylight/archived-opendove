/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      client_server_protocol.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DCS Server
 *
 */


#include "include.h"
#include "interface.h"
#include <time.h>


/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLIClientServerProtocol
 * @{
 */

/**
 * \brief The callback function for CLI_CLIENT_SERVER_PROTOCOL code
 */

typedef dove_status (*cli_cs_protocol_callback_func)(cli_client_server_protocol_t *);

/**
 * \brief An Array of Callbacks for every CLI_CLIENT_SERVER_PROTOCOL Code
 */
static cli_cs_protocol_callback_func cli_callback_array[CLI_CLIENT_SERVER_MAX];

/*
 ******************************************************************************
 * dev_log_level                                                          *//**
 *
 * \brief - Changes the Log Level for Developent Code of the DPS Client Server
 *          Protocol. Customer log level not affected.
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dev_log_level(cli_client_server_protocol_t *cli_prot)
{
	DpsProtocolLogLevel = (int32_t)cli_prot->log_level.level;
	log_info(CliLogLevel, "DpsProtocolLogLevel set to %d", DpsProtocolLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * customer_log_level                                                     *//**
 *
 * \brief - Changes the Log Level for Developent Code of the DPS Client Server
 *          Protocol. Customer log level not affected.
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status customer_log_level(cli_client_server_protocol_t *cli_prot)
{
	DpsProtocolCustomerLogLevel = (int32_t)cli_prot->log_level.level;
	log_info(CliLogLevel, "DpsProtocolCustomerLogLevel set to %d",
	         DpsProtocolCustomerLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * stastistics_show                                                       *//**
 *
 * \brief - Show statistics
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status stastistics_show(cli_client_server_protocol_t *cli_prot)
{
	log_info(CliLogLevel,
	         "Client Server Protocol, Packet Type %d",
	         cli_prot->stats_type.pkt_type);
	return dps_packet_stats_show(cli_prot->stats_type.pkt_type);
}

/*
 ******************************************************************************
 * stastistics_clear                                                       *//**
 *
 * \brief - Clear statistics
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status stastistics_clear(cli_client_server_protocol_t *cli_prot)
{
	log_info(CliLogLevel,
	         "Client Server Protocol, Packet Type %d",
	         cli_prot->stats_type.pkt_type);
	return dps_packet_stats_clear(cli_prot->stats_type.pkt_type);
}

/*
 ******************************************************************************
 * stastistics_clear                                                       *//**
 *
 * \brief - Clear statistics
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status retransmit_show(cli_client_server_protocol_t *cli_prot)
{
	retransmit_timer_show();
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * retransmit_log_level                                                     *//**
 *
 * \brief - Changes the Log Level for Retransmit Code
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status retransmit_log_level(cli_client_server_protocol_t *cli_prot)
{
	PythonRetransmitLogLevel = (int32_t)cli_prot->log_level.level;
	return DOVE_STATUS_OK;
}

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
dove_status cli_client_server_protocol_callback(void *cli)
{
	cli_client_server_protocol_t *cli_prot = (cli_client_server_protocol_t *)cli;
	cli_cs_protocol_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter");

	if (cli_prot->client_server_protocol_code < CLI_CLIENT_SERVER_MAX)
	{
		func = cli_callback_array[cli_prot->client_server_protocol_code];
		if (func)
		{
			status = func(cli_prot);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;

}

/*
 ******************************************************************************
 * cli_client_server_protocol_init                                        *//**
 *
 * \brief - Initializes the CLI_CLIENT_SERVER_PROTOCOL related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_client_server_protocol_init(void)
{

	log_debug(CliLogLevel, "Enter");

	dove_status status = DOVE_STATUS_OK;

	// Initialize the CLI_CLIENT_SERVER_PROTOCOL callbacks here

	cli_callback_array[CLI_CLIENT_SERVER_DEV_LOG_LEVEL] = dev_log_level;
	cli_callback_array[CLI_CLIENT_SERVER_CUST_LOG_LEVEL] = customer_log_level;
	cli_callback_array[CLI_CLIENT_SERVER_STATISTICS_SHOW] = stastistics_show;
	cli_callback_array[CLI_CLIENT_SERVER_STATISTICS_CLEAR] = stastistics_clear;
	cli_callback_array[CLI_CLIENT_RETRANSMIT_SHOW] = retransmit_show;
	cli_callback_array[CLI_CLIENT_RETRANSMIT_LOG_LEVEL] = retransmit_log_level;

	log_debug(CliLogLevel, "Exit");

	return status;
}
/** @} */

