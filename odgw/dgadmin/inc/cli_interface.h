/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


/*
 *
 *  Header File:
 *      cli_interface.h
 *
 *  Abstract:
 *      The CLI related structure that are passed from the CLI to
 *      the DPS Server Infrastructure
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
 * dove_gateway_cli_callback                                              *//**
 *
 * \brief - The Main Callback function when a CLI is invoked
 *
 * \param cli: The CLI Context Structure
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dove_gateway_cli_callback(void *cli);

/*
 ******************************************************************************
 * dove_gateway_cli_init                                                    *//**
 *
 * \brief - Initializes the DOVE Gateway CLI
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dove_gateway_cli_init(void);

/** @} */

#endif // _CLI_INTERFACE_
