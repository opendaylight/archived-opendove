/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   debug_interface.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */


#include "include.h"

/**
 * \ingroup DPSDebugInterface
 * @{
 */

#ifndef _PYTHON_DPS_DEBUG_H_
#define _PYTHON_DPS_DEBUG_H_

/**
 * \brief Variable that holds the logging variable for debugging
 */
extern int PythonDebugHandlerLogLevel;

#define MAC_MAX_LENGTH 32

/*
 ******************************************************************************
 * gdb_show_domain_info --                                                  *//**
 *
 * \brief This routine handles Domain Show. This routine can be invoked from GDB
 *        for debugging purposes
 *
 * \param VNID
 *
 * \return dove_status
 *
 *****************************************************************************/

void gdb_show_domain_info(int domain);

/*
 ******************************************************************************
 * gdb_show_vnid_info --                                                  *//**
 *
 * \brief This routine handles VNID Show. This routine can be invoked from GDB
 *        for debugging purposes
 *
 * \param VNID
 *
 * \return dove_status
 *
 *****************************************************************************/

void gdb_show_vnid_info(int vnid);

/*
 ******************************************************************************
 * vnid_get_endpoints --                                                  *//**
 *
 * \brief This routine gets all endpoints in the VNID. This information can
 *        be copied into the buffer and returned to calling routine.
 *
 * \param vnid The VNID
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status vnid_get_endpoints(int vnid);

/*
 ******************************************************************************
 * vnid_get_dove_switches --                                              *//**
 *
 * \brief This routine gets all DOVE Switches in the VNID. This information can
 *        be copied into the buffer and returned to calling routine.
 *
 * \note At this point there is no provision for buffer. John He will add the
 *       buffer logic in.
 *
 * \param vnid The VNID
 * \param buff The Buffer to copy the data into. This buffer must be provided
 *             by calling routine
 * \param buff_size The Size of the buffer provided by the calling routine
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status vnid_get_dove_switches(int vnid, char *buff, size_t buff_size);


/*
 ******************************************************************************
 * vnid_get_endpoints --                                              *//**
 *
 * \brief This routine gets all endpoints(VMs) in the VNID. This information can
 *        be copied into the buffer and returned to calling routine.
 *
 * \param vnid The VNID
 *
 * \return json_t *, which describes the json string, and if an error occured,
 *                    return NULL
 *
 *****************************************************************************/
json_t *vnid_get_endpoints_json(int vnid);

/*
 ******************************************************************************
 * vnid_get_tunnel_endpoints --                                              *//**
 *
 * \brief This routine gets all tunnel endpoints(DOVE Switches) in the VNID. This information can
 *        be copied into the buffer and returned to calling routine.
 *
 * \note At this point there is no provision for buffer. John He will add the
 *       buffer logic in.
 *
 * \param vnid The VNID
 *
 * \return json_t *, which describes the json string, and if an error occured, return NULL
 *
 *****************************************************************************/
json_t *vnid_get_tunnel_endpoints_json(int vnid);

/*
 ******************************************************************************
 * vnid_get_domain_mapping_json --                                        *//**
 *
 * \brief This rooutine returns the DOMAIN to VNID and VNID to DOMAIN mapping
 *        as a JSON object
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
json_t *vnid_get_domain_mapping_json();

/*
 ******************************************************************************
 * vnid_get_allow_policies --                                             *//**
 *
 * \brief This rooutine returns the Allow Policies of a VNID as a JSON object
 *
 * \param[in] vnid The VNID
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
json_t *vnid_get_allow_policies(int vnid);

/*
 ******************************************************************************
 * vnid_get_subnets --                                                    *//**
 *
 * \brief This rooutine returns the subnets in this VNID as a JSON Object
 *
 * \param[in] vnid The VNID
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
json_t *vnid_get_subnets(int vnid);

/*
 ******************************************************************************
 * python_init_debug_interface --                                         *//**
 *
 * \brief This routine initializes the functions needed to handle the DPS
 *        Data Handler Debugging Routine
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_debug_interface(char *pythonpath);

/** @} */

#endif // _PYTHON_DPS_DEBUG_H_
