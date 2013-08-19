/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  This file embedded the Python Module that handles the
**                    DPS Client Server Protocol
**/
/*
{
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*
*  HISTORY
*
*  $Log: client_protocol_interface.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

/**
 * \brief Variable that holds the logging variable for data handler
 */
int PythonDataHandlerLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief Variable indicating that Protocol Handler has started
 */
static int started = 0;

/**
 * \brief Variable that holds the logging variable for (multicast)data handler
 *        This value is "minimum" (i.e. max in terms of value) of the
 *        PythonDataHandlerLogLevel and the value set by the CLI.
 *        This will allow us to debug Multicast code independent of the
 *        unicast code.
 */
int PythonMulticastDataHandlerLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief The module location defines for the DPS Message Handling API
 */
#define PYTHON_MODULE_FILE_CLIENT_PROTOCOL "client_protocol_handler"

/**
 * \brief The PYTHON Class that handles the DPS Protocol Message Requests
 */
#define PYTHON_MODULE_CLASS_CLIENT_PROTOCOL "DpsClientHandler"

/**
 * \brief The PYTHON function for Stopping the Protocol Interface
 */
#define PYTHON_PROTOCOL_HANDLER_STOP "Stop"

/**
 * \brief The PYTHON function for Starting the Protocol Interface
 */
#define PYTHON_PROTOCOL_HANDLER_START "Start"

/**
 * \brief The PYTHON function for Policy_Updates_Send_To
 */
/**
 * \brief The DPS Client Protocol handler function pointers data structure
 */
typedef struct python_dps_client_protocol_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/**
	 * \brief The PYTHON function for Stopping the Protocol Interface
	 */
	PyObject *Stop;
	/**
	 * \brief The PYTHON function for Starting the Protocol Interface
	 */
	PyObject *Start;
}python_dps_client_protocol_t;

/*
 * \brief The DPS Protocol Message Handler PYTHON Interface (Embed)
 */
static python_dps_client_protocol_t Client_Protocol_Interface;

/*
 * \brief Function Handler for each DPS Message Type
 */

typedef dps_return_status (*dps_msg_func_handler)(uint32_t domain, dps_client_data_t *dps_msg);

/**
 * \brief An Array of Callback for every Client Protocol Request Code
 */

static dps_msg_func_handler dps_msg_function_array[DPS_MAX_MSG_TYPE];

/**
 * \brief An Array that determines if a msg type needs to be forwarded to
 *        a remote node
 */
static int dps_msg_forward_remote[DPS_MAX_MSG_TYPE];

/**
 * \brief The elements of the BASE CONTEXT
 */
#define CONTEXT_BASE /** \
	 * \brief The VNID ID \
	 */ \
	uint32_t vnid_id; \
	/** \
	 * \brief The Original Message Type \
	 */ \
	uint32_t original_msg_type; \
	/** \
	 * \brief The DPS Client IP Address \
	 */ \
	ip_addr_t dps_client_location;

/**
 * \brief The context associated with unsolicited messages
 */
typedef struct unsolicited_msg_context_s{
	CONTEXT_BASE
} unsolicited_msg_context_t;

/**
 * \brief The context associated with Address Resolution
 */
typedef struct endpoint_resolution_context_s{
	CONTEXT_BASE
	/**
	 * \brief The Virtual IP Address
	 */
	ip_addr_t vIP;
}endpoint_resolution_context_t;

/**
 * \brief The number of Outstanding Unsolicited Count
 */
int outstanding_unsolicited_msg_count = 0;

/**
 * \brief Buffer to send data
 */
#define SEND_BUFF_SIZE 32768

#define MAX_REPLICATION_COUNT 8

/*
 ******************************************************************************
 * DPS Object Interface                                                   *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSClientProtocolInterface DPS Client Server Protocol Interface
 * @{
 * Handles Interaction between the DPS Client Server Protocol and DPS (PYTHON)
 * Objects
 */

/*
 ******************************************************************************
 * dps_msg_send_inline --                                                 *//**
 *
 * \brief Inline function to send a DPS Client Server Protocol message using
 *
 * \param[in] dps_data A well formed DPS Client Server Protocol Message
 *
 ******************************************************************************/

static inline void dps_msg_send_inline(dps_client_data_t *dps_data)
{
	do
	{
		//Don't reply for retries
		if (dps_data->hdr.resp_status == DPS_ERROR_RETRY)
		{
			log_info(PythonDataHandlerLogLevel,
			         "Dropping Reply Packet, Message Type %d, Query ID %d with RETRY",
			         dps_data->hdr.type, dps_data->hdr.query_id);
			break;
		}
		dps_protocol_client_send(dps_data);
	}while(0);
	return;
}

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
PyObject *send_message_and_free(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	PyObject *py_message;
	dps_client_data_t *client_data;
	uint32_t resp_err;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonDataHandlerLogLevel, "Enter");
	do
	{
		// Parameters are (PyObject, dps_resp_status_t)
		if (!PyArg_ParseTuple(args, "OI", &py_message, &resp_err))
		{
			log_error(PythonDataHandlerLogLevel, "NO DATA! - NULL");
			break;
		}
		client_data = (dps_client_data_t *)PyCObject_AsVoidPtr(py_message);
		if (client_data == NULL)
		{
			break;
		}
		client_data->hdr.resp_status = resp_err;
		log_info(PythonDataHandlerLogLevel,
		          "Sending Message Type %d, Query ID %d, Status %d",
		          client_data->hdr.type, client_data->hdr.query_id, client_data->hdr.resp_status);
		dps_msg_send_inline(client_data);
		log_debug(PythonDataHandlerLogLevel,
		          "Freeing Message %p, PyObject %p, Ref Count %d",
		          client_data, py_message, py_message->ob_refcnt);
		free(client_data);
		Py_DECREF(py_message);
	} while(0);
	log_debug(PythonDataHandlerLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", 0);
	return ret_val;
}

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
dove_status dps_protocol_handler_stop()
{
	PyObject *strargs;
	PyGILState_STATE gstate;
	int status = DOVE_STATUS_NO_RESOURCES;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{

		//def Stop():
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Activate call
		PyEval_CallObject(Client_Protocol_Interface.Stop, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Protocol Handler Stopped");
		started = 0;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

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
dove_status dps_protocol_handler_start()
{
	PyObject *strargs;
	PyGILState_STATE gstate;
	int status = DOVE_STATUS_NO_RESOURCES;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{

		//def Start():
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Activate call
		PyEval_CallObject(Client_Protocol_Interface.Start, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Protocol Handler Started");
		started = 1;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_msg_forward_if_necessary --                                        *//**
 *
 * \brief This routine determines if a Client Server Protocol message needs
 *        to be forwarded to a remote node. The routine assumes that the
 *        message is self contained and ready for forwarding
 *
 * \param dps_msg Any DPS Client Server Protocol Message.
 * \param pdomain_id The Domain ID will be filled in at this location
 *
 * \retval DOVE_STATUS_OK Message was forwarded to a remote Node
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_EMPTY No DPS Node handles this domain
 *
 *****************************************************************************/

static dove_status dps_msg_forward_if_necessary(dps_client_data_t *dps_msg,
                                                uint32_t domain_id)
{
	dove_status status = DOVE_STATUS_NO_MEMORY;

	log_debug(PythonDataHandlerLogLevel,
	          "Enter: Domain %d, Msg Type %d",
	          domain_id, dps_msg->hdr.type);

	do
	{

#if defined (NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(dps_msg->hdr.reply_addr.family, dps_msg->hdr.reply_addr.ip6, str, INET6_ADDRSTRLEN);
			log_debug(PythonDataHandlerLogLevel,
			          "Domain %d: Forwarding message to DPS Node %s",
			          domain_id, str);
		}
#endif
		// Forward to the remote node
		if (dps_msg->hdr.reply_addr.family == AF_INET)
		{
			dps_msg->hdr.reply_addr.ip4 = ntohl(dps_msg->hdr.reply_addr.ip4);
		}
		dps_msg->context = NULL;
		dps_msg_send_inline(dps_msg);
		status = DOVE_STATUS_OK;
	}while (0);

	log_debug(PythonDataHandlerLogLevel, "Exit %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * dps_form_endpoint_update_reply --                                      *//**
 *
 * \brief This routine forms an Endpoint Update Reply Message based on the
 *        request
 *
 * \param dps_msg_reply   A caller allocated Reply Message Buffer
 * \param dps_msg         The original Endpoint Update Message
 * \param ret_code        The return code for the Reply
 *
 * \retval DPS_SUCCESS
 *
 *****************************************************************************/
/*static*/ dps_return_status dps_form_endpoint_update_reply(dps_client_data_t *dps_msg_reply,
                                                        dps_client_data_t *dps_msg,
                                                        int ret_code)
{
	// TODO: Check if this can be direct copy from the Endpoint Update
	dps_msg_reply->context = NULL;
	dps_msg_reply->hdr.type = DPS_ENDPOINT_UPDATE_REPLY;
	dps_msg_reply->hdr.sub_type = dps_msg->hdr.sub_type;
	//Set the VNID in the Reply header to the Inner VNID i.e. Endpoint VNID
	//This allows the DPS Node actually doing the replication (for shared address)
	//to process the reply correctly
	dps_msg_reply->hdr.vnid = dps_msg->endpoint_update.vnid;
	dps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	dps_msg_reply->hdr.client_id = dps_msg->hdr.client_id;
	dps_msg_reply->hdr.transaction_type = dps_msg->hdr.transaction_type;
	dps_msg_reply->hdr.resp_status = ret_code;
	if ((dps_msg->hdr.transaction_type == DPS_TRANSACTION_NORMAL) &&
	    (dps_msg->hdr.vnid != 0))
	{
		// Message originated from a DPS Client (forwarded)
		// Reply to DPS Client
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->endpoint_update.dps_client_addr,
		       sizeof(ip_addr_t));
	}
	else
	{
		// Message originated from a DPS Server (direct message).
		// Reply to DPS Server
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->hdr.reply_addr,
		       sizeof(ip_addr_t));
	}

	dps_msg_reply->endpoint_update_reply.vnid = dps_msg->endpoint_update.vnid;
	dps_msg_reply->endpoint_update_reply.version = dps_msg->endpoint_update.version;
	dps_msg_reply->endpoint_update_reply.tunnel_info.num_of_tunnels = dps_msg->endpoint_update.tunnel_info.num_of_tunnels;
	memcpy(&dps_msg_reply->endpoint_update_reply.tunnel_info.tunnel_list[0],
	       &dps_msg->endpoint_update.tunnel_info.tunnel_list[0],
	       sizeof(ip_addr_t));
	dps_msg_reply->endpoint_update_reply.num_of_vip = 0;
	memcpy(dps_msg_reply->endpoint_update_reply.mac,
	       &dps_msg->endpoint_update.mac,
	       6);
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_form_tunnel_register_reply --                                      *//**
 *
 * \brief This routine forms an Tunnel Register Update Reply Message based on the
 *        request
 *
 * \param dps_msg_reply   A caller allocated Reply Message Buffer
 * \param dps_msg         The original Endpoint Update Message
 * \param ret_code        The return code for the Reply
 *
 * \retval DPS_SUCCESS
 *
 *****************************************************************************/
/*static*/ dps_return_status dps_form_tunnel_register_reply(dps_client_data_t *dps_msg_reply,
                                                        dps_client_data_t *dps_msg,
                                                        int ret_code)
{
	dps_msg_reply->context = NULL;
	dps_msg_reply->hdr.type = DPS_REG_DEREGISTER_ACK;
	if (dps_msg->hdr.type == DPS_TUNNEL_REGISTER)
	{
		dps_msg_reply->hdr.sub_type = DPS_TUNNEL_REG;
	}
	else
	{
		dps_msg_reply->hdr.sub_type = DPS_TUNNEL_DEREG;
	}
	dps_msg_reply->hdr.vnid = dps_msg->hdr.vnid;
	dps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	dps_msg_reply->hdr.client_id = dps_msg->hdr.client_id; //DPS_POLICY_SERVER_ID;
	dps_msg_reply->hdr.transaction_type = dps_msg->hdr.transaction_type;
	dps_msg_reply->hdr.resp_status = ret_code;
	if (dps_msg->hdr.transaction_type == DPS_TRANSACTION_NORMAL)
	{
		// Message originated from a DPS Client (forwarded)
		// Reply to DPS Client
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->tunnel_reg_dereg.dps_client_addr,
		       sizeof(ip_addr_t));
	}
	else
	{
		// Message originated from a DPS Server (direct message).
		// Reply to DPS Server
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->hdr.reply_addr,
		       sizeof(ip_addr_t));
	}
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_form_multicast_sender_register_reply --                            *//**
 *
 * \brief This routine forms an Multicast Send Register Update Reply Message
 *        based on the request
 *
 * \param dps_msg_reply   A caller allocated Reply Message Buffer
 * \param dps_msg         The original Endpoint Update Message
 * \param ret_code        The return code for the Reply
 *
 * \retval DPS_SUCCESS
 *
 *****************************************************************************/
/*static*/ dps_return_status dps_form_multicast_sender_register_reply(dps_client_data_t *dps_msg_reply,
                                                                  dps_client_data_t *dps_msg,
                                                                  int ret_code)
{
	dps_msg_reply->context = NULL;
	dps_msg_reply->hdr.type = DPS_REG_DEREGISTER_ACK;
	dps_msg_reply->hdr.vnid = dps_msg->hdr.vnid;
	dps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	dps_msg_reply->hdr.client_id = dps_msg->hdr.client_id; //DPS_POLICY_SERVER_ID;
	dps_msg_reply->hdr.transaction_type = dps_msg->hdr.transaction_type;
	dps_msg_reply->hdr.resp_status = ret_code;
	if (dps_msg->hdr.transaction_type == DPS_TRANSACTION_NORMAL)
	{
		// Message originated from a DPS Client (forwarded)
		// Reply to DPS Client
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->mcast_sender.dps_client_addr,
		       sizeof(ip_addr_t));
	}
	else
	{
		// Message originated from a DPS Server (direct message).
		// Reply to DPS Server
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->hdr.reply_addr,
		       sizeof(ip_addr_t));
	}
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_form_multicast_receiver_register_reply --                          *//**
 *
 * \brief This routine forms an Multicast Send Register Update Reply Message
 *        based on the request
 *
 * \param dps_msg_reply   A caller allocated Reply Message Buffer
 * \param dps_msg         The original Endpoint Update Message
 * \param ret_code        The return code for the Reply
 *
 * \retval DPS_SUCCESS
 *
 *****************************************************************************/
/*static*/ dps_return_status dps_form_multicast_receiver_register_reply(dps_client_data_t *dps_msg_reply,
                                                                    dps_client_data_t *dps_msg,
                                                                    int ret_code)
{
	dps_msg_reply->context = NULL;
	dps_msg_reply->hdr.type = DPS_REG_DEREGISTER_ACK;
	dps_msg_reply->hdr.vnid = dps_msg->hdr.vnid;
	dps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	dps_msg_reply->hdr.client_id = dps_msg->hdr.client_id; //DPS_POLICY_SERVER_ID;
	dps_msg_reply->hdr.transaction_type = dps_msg->hdr.transaction_type;
	dps_msg_reply->hdr.resp_status = ret_code;
	if (dps_msg->hdr.transaction_type == DPS_TRANSACTION_NORMAL)
	{
		// Message originated from a DPS Client (forwarded)
		// Reply to DPS Client
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->mcast_receiver.dps_client_addr,
		       sizeof(ip_addr_t));
	}
	else
	{
		// Message originated from a DPS Server (direct message).
		// Reply to DPS Server
		memcpy(&dps_msg_reply->hdr.reply_addr, &dps_msg->hdr.reply_addr,
		       sizeof(ip_addr_t));
	}
	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_msg_functions_init --                                              *//**
 *
 * \brief This initializes the C function handler for the various request types
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dove_status dps_msg_functions_init(void)
{
	int i;
	for (i = 0; i < DPS_MAX_MSG_TYPE; i++)
	{
		dps_msg_function_array[i] = NULL;
		dps_msg_forward_remote[i] = 0;
	}
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * python_client_protocol_handler_init --                                 *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DPSClientHandler) that are needed for processing
 *        requests received from DPS Clients.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_client_protocol_handler_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDataHandlerLogLevel, "Enter");

	memset(&Client_Protocol_Interface, 0, sizeof(python_dps_client_protocol_t));

	do
	{
		// Get handle to an instance of DpsClientHandler
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_CLIENT_PROTOCOL,
		                                 PYTHON_MODULE_CLASS_CLIENT_PROTOCOL,
		                                 pyargs,
		                                 &Client_Protocol_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Stop
		Client_Protocol_Interface.Stop =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_PROTOCOL_HANDLER_STOP);
		if (Client_Protocol_Interface.Stop == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_PROTOCOL_HANDLER_STOP);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Start
		Client_Protocol_Interface.Start =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_PROTOCOL_HANDLER_START);
		if (Client_Protocol_Interface.Start == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_PROTOCOL_HANDLER_START);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * python_init_dps_protocol_interface --                                  *//**
 *
 * \brief This routine initializes the functions needed to handle the DPS
 *        Client Protocol Messages.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_dps_protocol_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		status = python_client_protocol_handler_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		status = dps_msg_functions_init();
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

	} while (0);

	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));
	return status;
}

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

dps_return_status dps_protocol_send_to_server(dps_client_data_t *client_data)
{
	uint32_t msg_type;
	uint32_t domain_id = 0;
	int fLocalDomain = 0;
	dove_status dps_status;
	dps_return_status status = DPS_ERROR;
	dps_msg_func_handler func;

	log_debug(PythonDataHandlerLogLevel,
	          "Enter Msg Type %d, VNID %d, client id %d, transaction type %d, query id %d",
	          client_data->hdr.type,
	          client_data->hdr.vnid,
	          client_data->hdr.client_id,
	          client_data->hdr.transaction_type,
	          client_data->hdr.query_id);

	do
	{
		msg_type = client_data->hdr.type;
		if (!started)
		{
			status = DPS_ERROR;
			break;
		}
		// If can be handled by Local Node, then handle it
		if (fLocalDomain)
		{
			if (msg_type >= DPS_MAX_MSG_TYPE)
			{
				break;
			}
			func = dps_msg_function_array[msg_type];
			if (func == NULL)
			{
				break;
			}
			log_debug(PythonDataHandlerLogLevel, "Domain %d", domain_id);
			status = func(domain_id, client_data);
			break;
		}
		// Only forward those requests that originated as Normal Transactions
		// In other words we don't forward Replication/Mass Transfer requests
		// AND we don't forward replies for Updates.
		else if ((client_data->hdr.transaction_type == DPS_TRANSACTION_NORMAL) &&
		         (dps_msg_forward_remote[msg_type]))
		{
			dps_status = dps_msg_forward_if_necessary(client_data,
			                                          domain_id);
			if (dps_status == DOVE_STATUS_OK)
			{
				// Assume Forwarded
				status = DPS_SUCCESS;
				break;
			}
		}
	}while(0);

	log_debug(PythonDataHandlerLogLevel, "Exit, status %d", status);

	return status;
}

/** @} */
/** @} */
