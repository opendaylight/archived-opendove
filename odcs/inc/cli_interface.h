/*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      cli_interface.h
 *      The CLI related structures that are passed from the CLI to
 *      the DCS Server Infrastructure
 *
 *  Author:
 *      Open DOVE development team
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
