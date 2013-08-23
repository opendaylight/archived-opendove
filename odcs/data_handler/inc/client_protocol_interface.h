/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   client_protocol_interface.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */

#include "include.h"

/**
 * \ingroup DPSClientProtocolInterface
 * @{
 */

#ifndef _PYTHON_DPS_CLIENT_PROTOCOL_H_
#define _PYTHON_DPS_CLIENT_PROTOCOL_H_

/**
 * \brief Variable that holds the logging variable for data handler
 */
extern int PythonDataHandlerLogLevel;

/**
 * \brief Variable that holds the logging variable for (multicast)data handler
 */
extern int PythonMulticastDataHandlerLogLevel;

/**
 * \brief The number of Outstanding Unsolicited Count
 */
extern int outstanding_unsolicited_msg_count;

/**
 * \brief The types of Gateways supported by DOVE
 */
typedef enum {
	/**
	 * \brief External Gateway for access to External World
	 */
	GATEWAY_TYPE_EXTERNAL = 1,
	/**
	 * \brief VLAN Gateways
	 */
	GATEWAY_TYPE_VLAN = 2,
	/**
	 * \brief Implicit Gateways part of Subnet.
	 */
	GATEWAY_TYPE_IMPLICIT = 3,
} data_handler_gateway_types;

/*
 ******************************************************************************
 * cli_hash_perf_reply                                                    *//**
 *
 * \brief This routine is called by the data handler when the CLI is doing
 *        Performance Testing.
 *
 * \param[in] msg - A pointer to a message that the client wants to send.
 *
 * \retval 0 DPS_SUCCESS
 * \retval 1 DPS_FAILURE
 *
 ******************************************************************************
 */
void cli_hash_perf_reply(dps_client_data_t *msg);

/*
 ******************************************************************************
 * send_all_connectivity_policies --                                      *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send a list
 *        of all 'Allow' connectivity policies to a DPS Client. Note that
 *        'Deny' connectivity policies are not sent
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *                  dcslib.send_all_connectivity_policies(
 *                      dps_client.location.ip_value_packed, #DPS Client Location IP
 *                      dps_client.location.port, #DPS Client Port
 *                      self.unique_id, #Domain ID
 *                      len(policy_list), #Number of Policies,
 *                      policy_list
 *                      )
 *
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_all_connectivity_policies(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_gateways --                                                       *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send a
 *        set of external/vlan gateways to a DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_gateways(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_gateways --                                                       *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send a
 *        set of multicast tunnels that can receive data on a vnid to a
 *        DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_multicast_tunnels(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_broadcast_table --                                                *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send the
 *        broadcast table to a DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_broadcast_table(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_address_resolution --                                             *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send the
 *        address resolution request to a DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_address_resolution(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_heartbeat --                                                      *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send the
 *        heartbeat (request) to a DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_heartbeat(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_vnid_deletion --                                                  *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send the
 *        VNID deletion to a DPS Client
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_vnid_deletion(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_endpoint_reply --                                                 *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        unsolicited endpoint reply to DPS Clients
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_endpoint_reply(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * send_all_vm_migration_update --                                                 *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        unsolicited endpoint invalidation reply to DPS Clients
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_all_vm_migration_update(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * mass_transfer_endpoint --                                              *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        endpoint mass transfer to a remote DPS Server
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *mass_transfer_endpoint(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * mass_transfer_tunnel --                                                *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        tunnel endpoint mass transfer to a remote DPS Server
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval dps_return_status
 *
 ******************************************************************************/
PyObject *mass_transfer_tunnel(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * mass_transfer_multicast_sender --                                      *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        multicast sender (mass transfer) to a remote DPS Server
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval dps_return_status
 *
 ******************************************************************************/
PyObject *mass_transfer_multicast_sender(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * mass_transfer_multicast_receiver --                                    *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to send
 *        multicast receiver (mass transfer) to a remote DPS Server
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval dps_return_status
 *
 ******************************************************************************/
PyObject *mass_transfer_multicast_receiver(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_vnids_replicate --                                                *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to replicate VNIDs
 *        to a remote host
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_vnids_replicate(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_policy_replicate --                                                *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to replicate policy
 *        configuration to other DPS nodes
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_policy_replicate(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_policy_bulk_replicate --                                           *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to replicate bulk
 *        policy configuration to other DPS nodes
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_policy_bulk_replicate(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_policy_replicate --                                                *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to replicate policy
 *        configuration to other DPS nodes
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_ipsubnet_replicate(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_ipsubnet_bulk_replicate --                                         *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to send a domains
 *        subnet list configuration to a remote node
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_ipsubnet_bulk_replicate(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * report_endpoint_conflict --                                            *//**
 *
 * \brief This is the routine that the PYTHON Scripts to report a Endpoint vIP
 *        conflict to the Controller
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *report_endpoint_conflict(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * vnid_query_send_to_controller --                                       *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to send a query
 *        to the controller regarding a particular vnid.
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *vnid_query_send_to_controller(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * vnid_query_subnets_from_controller --                                   *//**
 *
 * \brief This is the routine that the PYTHON routine can call to request
 *        the list of subnets from the DMC
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *vnid_query_subnets_from_controller(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * domain_query_from_controller --                                        *//**
 *
 * \brief This is the routine that the PYTHON routine can call to query domain
 *        setting from the DMC
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *domain_query_from_controller(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * domain_query_policy_from_controller --                                 *//**
 *
 * \brief This is the routine that the PYTHON routine can call to request
 *        a policy from DMC
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *domain_query_policy_from_controller(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * python_init_dps_protocol_interface --                                  *//**
 *
 * \brief This routine initializes the new DPS Message Handling PYTHON Interface
 *
 * \param pythonpath - Pointer to the Python Path
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 *****************************************************************************/

dove_status python_init_dps_protocol_interface(char *pythonpath);

/*
 ******************************************************************************
 * dps_protocol_send_to_server --                                         *//**
 *
 * \brief This routine sends a request to the PYTHON based DPS Server Data
 *        Handler
 *
 * \param client_data - Pointer to DPS Client Server Protocol Message
 *
 * \return dps_return_status
 *
 *****************************************************************************/

dps_return_status dps_protocol_send_to_server(dps_client_data_t *client_data);

/*
 ******************************************************************************
 * send_message_and_free --                                               *//**
 *
 * \brief This is the routine that the PYTHON Replication Scripts calls to
 *        send a reply to a DPS Client Request which was replicated by the DPS
 *        Server. The DPS Server C code i.e. this routine MUST free up the
 *        message buffer.
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *send_message_and_free(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_protocol_handler_stop --                                           *//**
 *
 * \brief This routine stops the DPS Client Server Protocol Handler
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_protocol_handler_stop();

/*
 ******************************************************************************
 * dps_protocol_handler_start --                                           *//**
 *
 * \brief This routine starts the DPS Client Server Protocol Handler
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_protocol_handler_start();


/** @} */

#endif // _PYTHON_DPS_CLIENT_PROTOCOL_H_
