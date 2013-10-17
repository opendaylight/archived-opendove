/*
 *  Copyright (c) IBM, Inc. 2012 -
 *  All rights reserved
 *
 *  Header File:
 *      cli_interface.h
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

#ifndef _CLI_INTERFACE_
#define _CLI_INTERFACE_

/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLI
 * @{
 */

/*
 ******************************************************************************
 * dps_server_cli_callback                                                *//**
 *
 * \brief - The Main Callback function when a CLI is invoked
 *
 * \param cli: The CLI Context Structure
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_server_cli_callback(void *cli);

/*
 ******************************************************************************
 * dcs_server_cli_init                                                    *//**
 *
 * \brief - Initializes the DCS Server CLI
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dcs_server_cli_init(void);

/** @} */

#endif // _CLI_INTERFACE_
