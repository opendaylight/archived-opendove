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
 * \brief The PYTHON function that handles Endpoint Update for -
 *        any combination of pIP and vIP
 */
#define PYTHON_FUNC_ENDPOINT_UPDATE "Endpoint_Update"

/**
 * \brief The PYTHON function that handles Endpoint Location Request for -
 *        vIP
 */
#define PYTHON_FUNC_ENDPOINT_REQUEST_vIP "Endpoint_Location_vIP"

/**
 * \brief The PYTHON function that handles Endpoint Location Request for -
 *        vMac
 */
#define PYTHON_FUNC_ENDPOINT_REQUEST_vMac "Endpoint_Location_vMac"

/**
 * \brief The PYTHON function that handles Policy Resolution 
 *        Request for vMac
 */
#define PYTHON_FUNC_POLICY_REQUEST_vMac "Policy_Resolution_vMac"

/**
 * \brief The PYTHON function that handles Policy Resolution 
 *        Request for vIP
 */
#define PYTHON_FUNC_POLICY_REQUEST_vIP "Policy_Resolution_vIP"

/**
 * \brief The PYTHON function that handles the Implicit Gateway List
 */
#define PYTHON_FUNC_IMPLICIT_GATEWAY_LIST "Implicit_Gateway_List"

/**
 * \brief The PYTHON function that handles the VLAN/External Gateway List
 */
#define PYTHON_FUNC_GATEWAY_LIST "Gateway_List"

/**
 * \brief The PYTHON function that handles Is_VNID_Handled_Locally
 */
#define PYTHON_FUNC_IS_VNID_LOCAL "Is_VNID_Handled_Locally"

/**
 * \brief The PYTHON function that handles Broadcast_List
 */
#define PYTHON_FUNC_BROADCAST_LIST "Broadcast_List"

/**
 * \brief The PYTHON function that determines the DPS Client Location
 */
#define PYTHON_PIP_GET_DPS_CLIENT "pIP_Get_DPS_Client"

/**
 * \brief The PYTHON function that determines the DVG of an endpoint
 */
#define PYTHON_VIP_GET_DVG "vIP_Get_DVG"

/**
 * \brief The PYTHON function that handles Handle_Unsolicited_Message_Reply
 */
#define PYTHON_UNSOLICTED_MESSAGE_REPLY "Handle_Unsolicited_Message_Reply"

/**
 * \brief The PYTHON function that handles Handle_Resolution_Reply
 */
#define PYTHON_RESOLUTION_MESSAGE_REPLY "Handle_Resolution_Reply"

/**
 * \brief The PYTHON function for the Tunnel_Register
 */
#define PYTHON_TUNNEL_REGISTER "Tunnel_Register"

/**
 * \brief The PYTHON function for the Tunnel_Unregister
 */
#define PYTHON_TUNNEL_UNREGISTER "Tunnel_Unregister"

/**
 * \brief The PYTHON function for Multicast_Sender_Register
 */
#define PYTHON_MULTICAST_SENDER_REGISTER "Multicast_Sender_Register"

/**
 * \brief The PYTHON function for Multicast_Sender_Unregister
 */
#define PYTHON_MULTICAST_SENDER_UNREGISTER "Multicast_Sender_Unregister"

/**
 * \brief The PYTHON function for Multicast_Receiver_Register
 */
#define PYTHON_MULTICAST_RECEIVER_REGISTER "Multicast_Receiver_Register"

/**
 * \brief The PYTHON function for Multicast_Receiver_Unregister
 */
#define PYTHON_MULTICAST_RECEIVER_UNREGISTER "Multicast_Receiver_Unregister"

/**
 * \brief The PYTHON function for Multicast_Global_Scope_Get
 */
#define PYTHON_MULTICAST_GLOBAL_SCOPE_GET "Multicast_Global_Scope_Get"

/**
 * \brief The PYTHON function for Policy_Updates_Send_To
 */
#define PYTHON_POLICY_UPDATES_SEND_TO "Policy_Updates_Send_To"

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
	/*
	 * \brief The function that Endpoint_Update
	 */
	PyObject *Endpoint_Update;
	/*
	 * \brief The function that Endpoint_Location_vIP
	 */
	PyObject *Endpoint_Location_vIP;
	/*
	 * \brief The function that Endpoint_Location_vMac
	 */
	PyObject *Endpoint_Location_vMac;
	/*
	 * \brief The function that Policy_Resolution_vIP
	 */
	PyObject *Policy_Resolution_vIP;
	/*
	 * \brief The function that Policy_Resolution_vMac
	 */
	PyObject *Policy_Resolution_vMac;
	/**
	 * \brief The Implicit Gateway Query
	 */
	PyObject *Implicit_Gateway_List;
	/**
	 * \brief The VLAN/External Gateway Query
	 */
	PyObject *Gateway_List;
	/**
	 * \brief PYTHON_FUNC_IS_VNID_LOCAL
	 */
	PyObject *Is_VNID_Handled_Locally;
	/**
	 * \brief PYTHON_FUNC_BROADCAST_LIST
	 */
	PyObject *Broadcast_List;
	/**
	 * \brief PYTHON_PIP_GET_DPS_CLIENT
	 */
	PyObject *pIP_Get_DPS_Client;
	/**
	 * \brief PYTHON_VIP_GET_DVG
	 */
	PyObject *vIP_Get_DVG;
	/**
	 * \brief Handle_Unsolicited_Message_Reply
	 */
	PyObject *Handle_Unsolicited_Message_Reply;
	/**
	 * \brief Handle_Resolution_Reply
	 */
	PyObject *Handle_Resolution_Reply;
	/**
	 * \brief Tunnel_Register
	 */
	PyObject *Tunnel_Register;
	/**
	 * \brief Tunnel_Unregister
	 */
	PyObject *Tunnel_Unregister;
	/**
	 * \brief Multicast_Sender_Register
	 */
	PyObject *Multicast_Sender_Register;
	/**
	 * \brief Multicast_Sender_Unregister
	 */
	PyObject *Multicast_Sender_Unregister;
	/**
	 * \brief Multicast_Receiver_Register
	 */
	PyObject *Multicast_Receiver_Register;
	/**
	 * \brief Multicast_Receiver_Unregister
	 */
	PyObject *Multicast_Receiver_Unregister;
	/**
	 * \brief Multicast_Global_Scope_Get
	 */
	PyObject *Multicast_Global_Scope_Get;
	/**
	 * \brief Policy_Updates_Send_To
	 */
	PyObject *Policy_Updates_Send_To;
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

/**
 * \brief The module location defines for the DPS Replication Handling API
 */
#define PYTHON_MODULE_FILE_REPLICATION_HANDLER "replication_handler"

/**
 * \brief The PYTHON Class that handles the DPS Protocol Message Requests
 */
#define PYTHON_MODULE_CLASS_REPLICATION_HANDLER "DPSReplicationTracker"

/**
 * \brief The PYTHON function to generate the Replication Query ID
 */
#define PYTHON_REPLICATION_QUERY_ID_GENERATE "ReplicationQueryIDGenerate"

/**
 * \brief The PYTHON function to process Replication
 */
#define PYTHON_REPLICATION_REPLY_PROCESS "ReplicationReplyProcess"

/**
 * \brief The Replication handler function pointers data structure
 */

typedef struct python_dps_replication_s{
	/**
	 * \brief The DpsReplicationHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The PYTHON function to process Replication
	 */
	PyObject *ReplicationQueryIDGenerate;
	/*
	 * \brief The PYTHON function to process Replication
	 */
	PyObject *ReplicationReplyProcess;
}python_dps_replication_t;

/*
 * \brief The Replication Handler PYTHON Interface (Embed)
 */
static python_dps_replication_t Replication_Interface;

/**
 * \brief The module location defines for the DPS Replication Handling API
 */
#define PYTHON_MODULE_FILE_MASS_TRANSFER "mass_transfer_handler"

/**
 * \brief The PYTHON Class that handles the DPS Protocol Message Requests
 */
#define PYTHON_MODULE_CLASS_MASS_TRANSFER "DPSMassTransfer"

/**
 * \brief The PYTHON function to generate the Process Mass Transfer ACKs
 */
#define PYTHON_MASS_TRANSFER_ACK "Transfer_Ack"

/**
 * \brief The Mass Transfer handler function pointers data structure
 */

typedef struct python_dps_mass_transfer_s{
	/**
	 * \brief The DPSMassTransfer Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The PYTHON function to Transfer_Ack
	 */
	PyObject *Transfer_Ack;
}python_dps_mass_transfer_t;

/**
 * \brief The Mass Transfer Handler PYTHON Interface (Embed)
 */
static python_dps_mass_transfer_t Mass_Transfer_Interface;

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
/*
 * \brief To be used by PYTHON Code calling C
 */
static uint8_t send_buff_py[SEND_BUFF_SIZE];
/**
 * \brief To be used by calls from the DPS Protocol Handler Thread
 */
static uint8_t send_buff[SEND_BUFF_SIZE];

/**
 * \brief Buffer to store Endpoint Update Addresses
 */
#define MAX_VIP_ADDRESS 32
static ip_addr_t vIP_address[MAX_VIP_ADDRESS];

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

/**
 * \brief The Hash Performance Test should set this Flag to 1
 */
int Hash_Perf_Test_CLI = 0;

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
PyObject *send_all_connectivity_policies(PyObject *self, PyObject *args)
{
	int num_items, dps_client_ip_size;
	uint32_t vnid, query_id, solicited;
	uint16_t dps_client_port;
	char *dps_client_ip;
	PyObject *ret_val;
	PyObject *pyList;
	uint32_t src_vnid, dst_vnid;
	Py_ssize_t i;
	PyObject *pyPolicy;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_bulk_vnid_policy_t *bulk_policy = &((dps_client_data_t *)send_buff_py)->bulk_vnid_policy;
	dps_return_status return_status = DPS_ERROR;
	unsolicited_msg_context_t *pcontext;

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIiO",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &solicited,
		                      &vnid,
		                      &query_id,
		                      &num_items,
		                      &pyList))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}

		log_info(PythonDataHandlerLogLevel,"VNID: %d", vnid);
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		if (solicited)
		{
			hdr->type = DPS_VNID_POLICY_LIST_REPLY;
		}
		else
		{
			hdr->type = DPS_UNSOLICITED_VNID_POLICY_LIST;
		}
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;

		bulk_policy->num_deny_rules = bulk_policy->num_permit_rules = 0;
		for (i = 0; i < PyList_Size(pyList); i++)
		{
			// Check if the current policy will cross the buffer size
			if (dps_offsetof(dps_client_data_t,
			                 bulk_vnid_policy.src_dst_vnid[bulk_policy->num_permit_rules+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of Policies %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList), SEND_BUFF_SIZE
				         );
				break;
			}
			log_info(PythonDataHandlerLogLevel, "Elements %d", i);
			pyPolicy = PyList_GetItem(pyList, i);
			if (!PyArg_ParseTuple(pyPolicy, "II", &src_vnid, &dst_vnid))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid Policy in element %d", i);
				continue;
			}
			log_info(PythonDataHandlerLogLevel,
			         "Policy [%d], src dvg %d, dst dvg %d",
			         i, src_vnid, dst_vnid);
			bulk_policy->src_dst_vnid[bulk_policy->num_permit_rules].dvnid = dst_vnid;
			bulk_policy->src_dst_vnid[bulk_policy->num_permit_rules].svnid = src_vnid;
			bulk_policy->num_permit_rules++;
		}
//		if (Hash_Perf_Test_CLI)
//		{
//			break;
//		}
		if (solicited)
		{
			client_data->context = NULL;
		}
		else
		{
			pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
			client_data->context = (void *)pcontext;
			log_debug(PythonDataHandlerLogLevel,
			          "Setting QueryID %d, Context %p",
			          client_data->hdr.query_id,
			          client_data->context);
			if (pcontext != NULL)
			{
				outstanding_unsolicited_msg_count++;
				pcontext->vnid_id = vnid;
				pcontext->original_msg_type = client_data->hdr.type;
				if (dps_client_ip_size == 4)
				{
					pcontext->dps_client_location.family = AF_INET;
				}
				else
				{
					pcontext->dps_client_location.family = AF_INET6;
				}
				pcontext->dps_client_location.port = dps_client_port;
				memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
			}
		}
		// Form the header
		if (dps_client_ip_size == 4)
		{
#if defined(NDEBUG)
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS

	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *send_gateways(PyObject *self, PyObject *args)
{
	int dps_client_ip_size, j;
	uint32_t vnid, gwy_vnid, query_id, gateway_type;
	uint16_t dps_client_port;
	char *dps_client_ip;
	PyObject *ret_val;
	PyObject *pyList_ipv4, *pyList_ipv6;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_tunnel_list_t *gw_data = &((dps_client_data_t *)send_buff_py)->tunnel_info;
	int ipv4, ipv6_size;
	char *ipv6;
	Py_ssize_t i;
	PyObject *pyIP;
	dps_return_status return_status = DPS_ERROR;
	unsolicited_msg_context_t *pcontext;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonMulticastDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIOO",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &gateway_type,
		                      &pyList_ipv4,
		                      &pyList_ipv6))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ipv4) || !PyList_Check(pyList_ipv6))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}

		switch (gateway_type)
		{
			case GATEWAY_TYPE_EXTERNAL:
				log_info(PythonMulticastDataHandlerLogLevel,"External Gateways");
				hdr->type = DPS_UNSOLICITED_EXTERNAL_GW_LIST;
				break;
			case GATEWAY_TYPE_VLAN:
				log_info(PythonMulticastDataHandlerLogLevel,"VLAN Gateways");
				hdr->type = DPS_UNSOLICITED_VLAN_GW_LIST;
				break;
			default:
				hdr->type = DPS_MAX_MSG_TYPE;
				break;
		}
		log_info(PythonMulticastDataHandlerLogLevel,
		         "Source VNID %d, Destination Gateway Numbers IPv4 %d, IPv6 %d",
		         vnid, PyList_Size(pyList_ipv4), PyList_Size(pyList_ipv6));
		if (hdr->type >= DPS_MAX_MSG_TYPE)
		{
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "Gateway Type %d not supported", gateway_type);
			break;
		}
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;
		gw_data->num_of_tunnels = 0;
	
		j = 0;
		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
			// Check if the current policy will cross the buffer size
			if (dps_offsetof(dps_client_data_t,
			                 tunnel_info.tunnel_list[j+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonMulticastDataHandlerLogLevel,
				          "ALERT!!! Number of Gateways %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_ParseTuple(pyIP, "II", &gwy_vnid, &ipv4))
			{
				log_warn(PythonMulticastDataHandlerLogLevel,
				         "Invalid IPv4 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "IPv4 [%d] VNID %d, Gateway %s", i, gwy_vnid, str);
			gw_data->tunnel_list[j].tunnel_type = TUNNEL_TYPE_VXLAN;
			gw_data->tunnel_list[j].port = 0;
			gw_data->tunnel_list[j].family = AF_INET;
			gw_data->tunnel_list[j].ip4 = ntohl(ipv4);
			gw_data->tunnel_list[j].vnid = gwy_vnid;
			j++;
			gw_data->num_of_tunnels++;
		}
		for (i = 0; i < PyList_Size(pyList_ipv6); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 tunnel_info.tunnel_list[j+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonMulticastDataHandlerLogLevel,
				          "ALERT!!! Number of Gateways %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_ParseTuple(pyIP, "Iz#", &gwy_vnid, &ipv6, &ipv6_size))
			{
				log_warn(PythonMulticastDataHandlerLogLevel,
				         "Invalid IPv6 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "IPv6 [%d], VNID %d, Gateway %s", i, gwy_vnid, str);
			gw_data->tunnel_list[j].tunnel_type = TUNNEL_TYPE_VXLAN;
			gw_data->tunnel_list[j].port = 0;
			gw_data->tunnel_list[j].family = AF_INET6;
			memcpy((uint8_t *)(&gw_data->tunnel_list[j]), ipv6, ipv6_size);
			gw_data->tunnel_list[j].vnid = gwy_vnid;
			gw_data->num_of_tunnels++;
			j ++;
		}
//		if (Hash_Perf_Test_CLI)
//		{
//			break;
//		}
		pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonMulticastDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
		}
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonMulticastDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonMulticastDataHandlerLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * send_multicast_tunnels --                                              *//**
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
PyObject *send_multicast_tunnels(PyObject *self, PyObject *args)
{
	uint32_t vnid, query_id, ipv4;
	uint16_t dps_client_port;
	char *dps_client_ip, *ipv6, *mcast_mac, *mcast_ip;
	PyObject *ret_val, *pyReceiverDict, *key, *value, *pyIP;
	Py_ssize_t ReceiverDict_pos = 0;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_mcast_receiver_ds_list_t *mcast_data = &((dps_client_data_t *)send_buff_py)->mcast_receiver_ds_list;
	dps_pkd_tunnel_list_t *mcast_receiver;
	char *mcast_receiver_pointer;
	size_t mcast_receiver_list_size;
	int inet_type, dps_client_ip_size, ipv6_size, mcast_macsize, macst_ipsize;
	dps_return_status return_status = DPS_ERROR;
	char str[INET_ADDRSTRLEN];
#if 0
	unsolicited_msg_context_t *pcontext;
#endif

	log_info(PythonMulticastDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIz#Iz#O",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &mcast_mac, &mcast_macsize,
		                      &inet_type,
		                      &mcast_ip, &macst_ipsize,
		                      &pyReceiverDict))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (mcast_macsize != 6)
		{
			log_notice(PythonMulticastDataHandlerLogLevel, "MAC Address not 6 bytes");
			break;
		}
		if(!PyDict_Check(pyReceiverDict))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Not Dictionary Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		log_info(PythonMulticastDataHandlerLogLevel,
		         "Source VNID %d, Number of Destination VNIDs %d",
		         vnid, PyDict_Size(pyReceiverDict));
		hdr->type = DPS_MCAST_RECEIVER_DS_LIST;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;

		//Fill in the MAC + IP Header
		mcast_data->mcast_addr.mcast_addr_type = inet_type;
		memcpy(mcast_data->mcast_addr.mcast_mac, mcast_mac, 6);
		memcpy(mcast_data->mcast_addr.u.mcast_ip6, mcast_ip, macst_ipsize);
		if (inet_type == AF_INET)
		{
			mcast_data->mcast_addr.u.mcast_ip4 = ntohl(mcast_data->mcast_addr.u.mcast_ip4);
		}

		mcast_receiver = &mcast_data->mcast_recvr_list[0];
		mcast_receiver_pointer = (char *)mcast_receiver;
		mcast_receiver_list_size = dps_offsetof(dps_client_data_t, mcast_receiver_ds_list.mcast_recvr_list[0]);
		mcast_data->num_of_rec = 0;
		while (PyDict_Next(pyReceiverDict, &ReceiverDict_pos, &key, &value))
		{
			//Each key is a VNID
			PyObject *pyTunnelList_v4, *pyTunnelList_v6;
			uint32_t rvnid;
			int i, tunnel_present;
			int switch_index = 0;

			tunnel_present = 0;
			if(!PyArg_Parse(key, "I", &rvnid))
			{
				log_warn(PythonMulticastDataHandlerLogLevel,
				         "pyReceiverDict Key Cannot get VNID");
				continue;
			}
			mcast_receiver->vnid = rvnid;
			mcast_receiver->num_v4_tunnels = 0;
			mcast_receiver->num_v6_tunnels = 0;
			//Value is a tuple(IPv4 Tunnels, IPv6 Tunnels)
			if(!PyArg_ParseTuple(value, "OO", &pyTunnelList_v4, &pyTunnelList_v6))
			{
				log_warn(PythonMulticastDataHandlerLogLevel,
				         "pyReceiverDict Value Cannot get Tunnels IPv4 and IPv6");
				continue;
			}

			for (i = 0; i < PyList_Size(pyTunnelList_v4); i++)
			{
				// Check if the current policy will cross the buffer size
				if ((mcast_receiver_list_size +
				     dps_offsetof(dps_pkd_tunnel_list_t,
				                  tunnel_list[switch_index+1])) >
				    SEND_BUFF_SIZE)
				{
					log_alert(PythonMulticastDataHandlerLogLevel,
					          "ALERT!!! Tunnel Index %d exceeds Send Buffer %d max size",
					          switch_index+1,
					          SEND_BUFF_SIZE
					         );
					break;
				}
				pyIP = PyList_GetItem(pyTunnelList_v4, i);
				if (!PyArg_Parse(pyIP, "I", &ipv4))
				{
					log_warn(PythonMulticastDataHandlerLogLevel,
					         "Invalid IPv4 in element %d", i);
					continue;
				}
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonMulticastDataHandlerLogLevel,
				         "R-VNID %d, IPv4 [%d] %s", rvnid, i, str);
				mcast_receiver->tunnel_list[switch_index] = ntohl(ipv4);
				mcast_receiver->num_v4_tunnels++;
				switch_index++;
				tunnel_present = 1;
			}
			//TODO: Right now ipv6 and ipv4 are mingled since there is no rvnid. FIX this
			for (i = 0; i < PyList_Size(pyTunnelList_v6); i++)
			{
				if ((mcast_receiver_list_size +
				     dps_offsetof(dps_pkd_tunnel_list_t,
				                  tunnel_list[switch_index+4])) >
				    SEND_BUFF_SIZE)
				{
					log_alert(PythonMulticastDataHandlerLogLevel,
					          "ALERT!!! Tunnel Index %d exceeds Send Buffer %d max size",
					          switch_index+1,
					          SEND_BUFF_SIZE
					         );
					break;
				}
				pyIP = PyList_GetItem(pyTunnelList_v6, i);
				if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonMulticastDataHandlerLogLevel,
					         "Invalid IPv6 in element %d", i);
					continue;
				}
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonMulticastDataHandlerLogLevel,
				         "R-VNID %d, IPv6 [%d] %s", rvnid, i, str);
				memcpy((uint8_t *)&mcast_receiver->tunnel_list[switch_index],
				       ipv6, ipv6_size);
				mcast_receiver->num_v6_tunnels++;
				switch_index += 4;
				tunnel_present = 1;
			}
			if (!tunnel_present)
			{
				continue;
			}
			mcast_data->num_of_rec++;
			mcast_receiver_list_size += dps_offsetof(dps_pkd_tunnel_list_t,
			                                         tunnel_list[mcast_receiver->num_v4_tunnels +
			                                                     (mcast_receiver->num_v6_tunnels << 2)]);
			mcast_receiver_pointer += dps_offsetof(dps_pkd_tunnel_list_t,
			                                       tunnel_list[mcast_receiver->num_v4_tunnels +
			                                                   (mcast_receiver->num_v6_tunnels << 2)]);
			mcast_receiver = (dps_pkd_tunnel_list_t *)mcast_receiver_pointer;
		}
		if (Hash_Perf_Test_CLI)
		{
			break;
		}
#if 0
		pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonMulticastDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
		}
#else
		client_data->context = NULL;
#endif
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		log_info(PythonMulticastDataHandlerLogLevel,
		         "Source VNID %d, Actual Number of Destination VNIDs %d",
		         vnid, mcast_data->num_of_rec);
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonMulticastDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
#if 0
			free(client_data->context);
#endif
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonMulticastDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *send_broadcast_table(PyObject *self, PyObject *args)
{
	int dps_client_ip_size, j;
	uint32_t vnid, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip;
	PyObject *ret_val;
	PyObject *pyList_ipv4, *pyList_ipv6;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_pkd_tunnel_list_t *switch_list = &((dps_client_data_t *)send_buff_py)->dove_switch_list;
	int ipv4, ipv6_size;
	char *ipv6;
	Py_ssize_t i;
	PyObject *pyIP;
	dps_return_status return_status = DPS_ERROR;
	unsolicited_msg_context_t *pcontext;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIOO",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &pyList_ipv4,
		                      &pyList_ipv6))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ipv4) || !PyList_Check(pyList_ipv6))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}

		log_info(PythonDataHandlerLogLevel,"VNID: %d, Broadcast Switches IPv4 %d, IPv6 %d",
		         vnid, PyList_Size(pyList_ipv4), PyList_Size(pyList_ipv6));
		hdr->type = DPS_UNSOLICITED_BCAST_LIST_REPLY;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;
		switch_list->num_v4_tunnels = 0;
		switch_list->num_v6_tunnels = 0;

		j = 0;
		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 dove_switch_list.tunnel_list[j+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_Parse(pyIP, "I", &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv4 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] Broadcast: DOVE Switch %s", i, str);
			switch_list->tunnel_list[j++] = ntohl(ipv4);
			switch_list->num_v4_tunnels++;
		}
		for (i = 0, j = 0; i < PyList_Size(pyList_ipv6); i++, j++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 dove_switch_list.tunnel_list[j+4]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv6 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] Broadcast: DOVE Switch %s",
			         switch_list->num_v4_tunnels+i, str);
			memcpy((uint8_t *)(&switch_list->tunnel_list[j]), ipv6, ipv6_size);
			switch_list->num_v6_tunnels++;
			j += 4;
		}
//		if (Hash_Perf_Test_CLI)
//		{
//			break;
//		}
#if 1
		pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
		}
#else
		client_data->context = NULL;
#endif
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_debug(PythonDataHandlerLogLevel, "Exit status %d", return_status);

	return ret_val;
}

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
PyObject *send_address_resolution(PyObject *self, PyObject *args)
{
	int dps_client_ip_size, vIP_address_size, vMac_size;
	uint32_t vnid, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip, *vIP_address, *vMac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_endpoint_loc_req_t *addr_resolve = &((dps_client_data_t *)send_buff_py)->address_resolve;
	dps_return_status return_status = DPS_ERROR;
	endpoint_resolution_context_t *pcontext;
	char str[INET6_ADDRSTRLEN];

	//Py_BEGIN_ALLOW_THREADS
	log_info(PythonDataHandlerLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIz#z#",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &vIP_address, &vIP_address_size,
		                      &vMac, &vMac_size))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		if (vIP_address == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual IP Address!!!");
			break;
		}
		hdr->type = DPS_ADDR_RESOLVE;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;
		memset(addr_resolve, 0, sizeof(dps_endpoint_loc_req_t));
		if (Hash_Perf_Test_CLI)
		{
			break;
		}

		pcontext = (endpoint_resolution_context_t *)malloc(sizeof(endpoint_resolution_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
			if (vIP_address_size == 4)
			{
				pcontext->vIP.family = AF_INET;
			}
			else
			{
				pcontext->vIP.family = AF_INET6;
			}
			memcpy(pcontext->vIP.ip6, vIP_address, vIP_address_size);
		}
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		addr_resolve->vnid = vnid;
		memcpy(addr_resolve->mac, vMac, vMac_size);
		if (vIP_address_size == 4)
		{
			inet_ntop(AF_INET, vIP_address, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel, "VNID %d, vMac "MAC_FMT", vIP Address %s",
			         vnid, MAC_OCTETS(addr_resolve->mac),str);
			addr_resolve->vm_ip_addr.family = AF_INET;
			addr_resolve->vm_ip_addr.ip4 = ntohl(*((uint32_t *)vIP_address));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, vIP_address, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "VNID %d, vMac "MAC_FMT", vIP Address %s",
			         vnid, MAC_OCTETS(addr_resolve->mac), str);
			addr_resolve->vm_ip_addr.family = AF_INET6;
			memcpy(addr_resolve->vm_ip_addr.ip6, vIP_address, vIP_address_size);
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *send_heartbeat(PyObject *self, PyObject *args)
{
	int dps_client_ip_size;
	uint32_t vnid, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip;
	PyObject *ret_val;
	dps_client_data_t dps_msg;
	dps_client_hdr_t *hdr = &dps_msg.hdr;
	dps_return_status return_status = DPS_ERROR;
	unsolicited_msg_context_t *pcontext;
	char str[INET6_ADDRSTRLEN];

	//Py_BEGIN_ALLOW_THREADS
	log_info(PythonDataHandlerLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "z#HII",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		if (vIP_address == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual IP Address!!!");
			break;
		}
		hdr->type = DPS_CTRL_PLANE_HB;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;

		pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
		dps_msg.context = (void *)pcontext;
		log_info(PythonDataHandlerLogLevel, "Setting QueryID %d, Context %p",
		         hdr->query_id, dps_msg.context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = hdr->type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
		}
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		return_status = dps_protocol_client_send(&dps_msg);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(dps_msg.context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *send_vnid_deletion(PyObject *self, PyObject *args)
{
	int dps_client_ip_size;
	uint32_t vnid, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_return_status return_status = DPS_ERROR;

	//Py_BEGIN_ALLOW_THREADS
	log_info(PythonDataHandlerLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "z#HII",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		hdr->type = DPS_UNSOLICITED_VNID_DEL_REQ;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;
		if (Hash_Perf_Test_CLI)
		{
			break;
		}
		client_data->context = NULL;
		log_debug(PythonDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (dps_client_ip_size == 4)
		{
#if defined(NDEBUG)
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *send_endpoint_reply(PyObject *self, PyObject *args)
{
	int dps_client_ip_size, vIP_address_size, vMac_size, i, j;
	uint32_t vnid, endpoint_vnid, endpoint_version, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip, *vIP_address, *vMac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_endpoint_loc_reply_t *endpoint_reply = &((dps_client_data_t *)send_buff_py)->endpoint_loc_reply;
	PyObject *pyList_ipv4, *pyList_ipv6, *pyIP;
	dps_tunnel_endpoint_t *tunnel;
	int ipv4, ipv6_size;
	char *ipv6;
	dps_return_status return_status = DPS_ERROR;
	endpoint_resolution_context_t *pcontext;

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIIOOz#z#",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &endpoint_vnid,
		                      &endpoint_version,
		                      &pyList_ipv4,
		                      &pyList_ipv6,
		                      &vMac, &vMac_size,
		                      &vIP_address, &vIP_address_size))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ipv4) || !PyList_Check(pyList_ipv6))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		if (vIP_address == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual IP Address!!!");
			break;
		}
		if ((vMac == NULL) || (vMac_size != 6))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual MAC Address!!!");
			break;
		}
		hdr->type = DPS_UNSOLICITED_ENDPOINT_LOC_REPLY;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;

//		if (Hash_Perf_Test_CLI)
//		{
//			break;
//		}

		pcontext = (endpoint_resolution_context_t *)malloc(sizeof(endpoint_resolution_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
			if (vIP_address_size == 4)
			{
				pcontext->vIP.family = AF_INET;
			}
			else
			{
				pcontext->vIP.family = AF_INET6;
			}
			memcpy(pcontext->vIP.ip6, vIP_address, vIP_address_size);
		}
		if (dps_client_ip_size == 4)
		{
#if defined(NDEBUG)
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d", str, dps_client_port);
#endif
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		if (vIP_address_size == 4)
		{
#if defined(NDEBUG)
			char str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, vIP_address, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel, "VNID %d, vIP Address: IPv4 %s", vnid, str);
#endif
			endpoint_reply->vm_ip_addr.family = AF_INET;
			endpoint_reply->vm_ip_addr.ip4 = ntohl(*((uint32_t *)vIP_address));
		}
		else
		{
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
			// IPv6
			inet_ntop(AF_INET6, vIP_address, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "VNID %d, vIP Address: IPv6 %s", vnid, str);
#endif
			endpoint_reply->vm_ip_addr.family = AF_INET6;
			memcpy(endpoint_reply->vm_ip_addr.ip6, vIP_address, vIP_address_size);
		}
		memcpy(endpoint_reply->mac, vMac, vMac_size);
		endpoint_reply->vnid = endpoint_vnid;
		endpoint_reply->tunnel_info.num_of_tunnels = 0;
		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
#if defined(NDEBUG)
			char str[INET_ADDRSTRLEN];
#endif
			if (dps_offsetof(dps_client_data_t,
			                 endpoint_loc_reply.tunnel_info.tunnel_list[endpoint_reply->tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_Parse(pyIP, "I", &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv4 in element %d", i);
				continue;
			}
#if defined(NDEBUG)
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s", i, str);
#endif
			tunnel = &endpoint_reply->tunnel_info.tunnel_list[endpoint_reply->tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET;
			tunnel->ip4 = ntohl(ipv4);
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			endpoint_reply->tunnel_info.num_of_tunnels++;
		}
		for (i = 0; i < PyList_Size(pyList_ipv6); i++, j++)
		{
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
#endif
			if (dps_offsetof(dps_client_data_t,
			                 endpoint_loc_reply.tunnel_info.tunnel_list[endpoint_reply->tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv6 in element %d", i);
				continue;
			}
#if defined(NDEBUG)
			inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s",
			         endpoint_reply->tunnel_info.num_of_tunnels, str);
#endif
			tunnel = &endpoint_reply->tunnel_info.tunnel_list[endpoint_reply->tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET6;
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			memcpy(tunnel->ip6, ipv6, ipv6_size);
			endpoint_reply->tunnel_info.num_of_tunnels++;
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * send_all_vm_migration_update --                                        *//**
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
PyObject *send_all_vm_migration_update(PyObject *self, PyObject *args)
{
	int dps_client_ip_size, vMac_size, i, j, pyList_size;
	uint32_t vnid, endpoint_vnid, endpoint_version, query_id;
	uint16_t dps_client_port;
	char *dps_client_ip, *vMac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_vm_invalidate_t *vm_invalidate = &((dps_client_data_t *)send_buff_py)->vm_invalidate_msg;
	PyObject *pyList_ipv4, *pyList_ipv6, *pyIP, *pyList_vip4, *pyList_vip6, *pyvIP;
	dps_tunnel_endpoint_t *tunnel;
	dps_ip4_info_t *ip4_addr;
	dps_ip6_info_t *ip6_addr;
	int ipv4, ipv6_size;
	char *ipv6;
	dps_return_status return_status = DPS_ERROR;
	unsolicited_msg_context_t *pcontext;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIIOOz#OO",
		                      &dps_client_ip, &dps_client_ip_size,
		                      &dps_client_port,
		                      &vnid,
		                      &query_id,
		                      &endpoint_vnid,
		                      &endpoint_version,
		                      &pyList_ipv4,
		                      &pyList_ipv6,
		                      &vMac, &vMac_size,
		                      &pyList_vip4, &pyList_vip6))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ipv4) || !PyList_Check(pyList_ipv6))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_client_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		if ((vMac == NULL) || (vMac_size != 6))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual MAC Address!!!");
			break;
		}
		// msg type will be unsolicited endpoint invalidate reply
		hdr->type = DPS_UNSOLICITED_VM_LOC_INFO;
		hdr->client_id = DPS_POLICY_SERVER_ID;
		hdr->transaction_type = DPS_TRANSACTION_NORMAL;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_client_port;

		pcontext = (unsolicited_msg_context_t *)malloc(sizeof(unsolicited_msg_context_t));
		client_data->context = (void *)pcontext;
		log_debug(PythonDataHandlerLogLevel,
		          "Setting QueryID %d, Context %p",
		          client_data->hdr.query_id,
		          client_data->context);
		if (pcontext != NULL)
		{
			outstanding_unsolicited_msg_count++;
			pcontext->vnid_id = vnid;
			pcontext->original_msg_type = client_data->hdr.type;
			if (dps_client_ip_size == 4)
			{
				pcontext->dps_client_location.family = AF_INET;
			}
			else
			{
				pcontext->dps_client_location.family = AF_INET6;
			}
			pcontext->dps_client_location.port = dps_client_port;
			memcpy(pcontext->dps_client_location.ip6, dps_client_ip, dps_client_ip_size);
		}
		if (dps_client_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_client_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DCS Client: IPv4 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_client_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_client_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DCS Client: pIPv6 %s, Port %d", str, dps_client_port);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_client_ip, dps_client_ip_size);
		}
		memcpy(vm_invalidate->epri.mac, vMac, vMac_size);
		vm_invalidate->epri.vnid = endpoint_vnid;
		vm_invalidate->epri.vm_ip4_addr.num_of_ips = 0;
		//fill vIP ipv4
		pyList_size = PyList_Size(pyList_vip4);
		for (i = 0; i < pyList_size; i++)
		{
			if(i >= MAX_VIP_ADDR) {
				log_alert(PythonDataHandlerLogLevel,
			        	  "ALERT!!! Number of vIPv4s %d exceed max vIPs",
				          pyList_size);
				break;
			}
			pyvIP = PyList_GetItem(pyList_vip4, i);
			if (!PyArg_Parse(pyvIP, "I", &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid vIP IPv4 in element %d", i);
				continue;
			}
			ip4_addr = &vm_invalidate->epri.vm_ip4_addr.ip_info[vm_invalidate->epri.vm_ip4_addr.num_of_ips];
			ip4_addr->flags = 0;
			ip4_addr->ip = ntohl(ipv4);
			ip4_addr->reserved = 0;
			vm_invalidate->epri.vm_ip4_addr.num_of_ips++;
		}

		vm_invalidate->epri.vm_ip6_addr.num_of_ips = 0;
		//fill vIP ipv6
		pyList_size = PyList_Size(pyList_vip6);
		for (i = 0; i < pyList_size; i++)
		{
			if(i >= MAX_VIP_ADDR) {
				log_alert(PythonDataHandlerLogLevel,
			        	  "ALERT!!! Number of vIPv6s %d exceed max vIPs",
				          pyList_size);
				break;
			}
			pyvIP = PyList_GetItem(pyList_vip6, i);
			if (!PyArg_Parse(pyvIP, "z#", &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid vIP IPv6 in element %d", i);
				continue;
			}
			ip6_addr = &vm_invalidate->epri.vm_ip6_addr.ip_info[vm_invalidate->epri.vm_ip6_addr.num_of_ips];
			ip6_addr->flags = 0;
			memcpy(ip6_addr->ip, ipv6, ipv6_size);
			ip6_addr->reserved = 0;
			vm_invalidate->epri.vm_ip6_addr.num_of_ips++;
		}

		vm_invalidate->epri.tunnel_info.num_of_tunnels = 0;
		// fill tunnel info (ipv4)

		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 vm_invalidate_msg.epri.tunnel_info.tunnel_list[vm_invalidate->epri.tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				          );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_Parse(pyIP, "I", &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid TUNNEL IPv4 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s", i, str);
			tunnel = &vm_invalidate->epri.tunnel_info.tunnel_list[vm_invalidate->epri.tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET;
			tunnel->ip4 = ntohl(ipv4);
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			vm_invalidate->epri.tunnel_info.num_of_tunnels++;
		}
		// fill tunnel info (ipv6)
		for (i = 0; i < PyList_Size(pyList_ipv6); i++, j++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 vm_invalidate_msg.epri.tunnel_info.tunnel_list[vm_invalidate->epri.tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				          );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid TUNNEL IPv6 in element %d", i);
				continue;
			}
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s",
			         vm_invalidate->epri.tunnel_info.num_of_tunnels, str);
			tunnel = &vm_invalidate->epri.tunnel_info.tunnel_list[vm_invalidate->epri.tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET6;
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			memcpy(tunnel->ip6, ipv6, ipv6_size);
			vm_invalidate->epri.tunnel_info.num_of_tunnels++;
		}

		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
			free(client_data->context);
			outstanding_unsolicited_msg_count--;
		}
	}while(0);
	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *mass_transfer_endpoint(PyObject *self, PyObject *args)
{
	int dps_server_ip_size, host_ip_size, vIP_address_size, vMac_size, i, j;
	uint32_t vnid, endpoint_vnid, endpoint_client_type, endpoint_update_type, endpoint_version, query_id;
	uint16_t dps_server_port, host_port;
	char *dps_server_ip, *host_ip, *vIP_address, *vMac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_endpoint_update_t *endpoint_update = &((dps_client_data_t *)send_buff_py)->endpoint_update;
	PyObject *pyList_ipv4, *pyList_ipv6, *pyIP;
	dps_tunnel_endpoint_t *tunnel;
	int ipv4, ipv6_size;
	char *ipv6;
	dps_return_status return_status = DPS_ERROR;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#Hz#HIIIIIIOOz#z#",
		                      &dps_server_ip, &dps_server_ip_size,
		                      &dps_server_port,
		                      &host_ip, &host_ip_size,
		                      &host_port,
		                      &vnid,
		                      &query_id,
		                      &endpoint_vnid,
		                      &endpoint_client_type,
		                      &endpoint_version,
		                      &endpoint_update_type,
		                      &pyList_ipv4,
		                      &pyList_ipv6,
		                      &vMac, &vMac_size,
		                      &vIP_address, &vIP_address_size))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ipv4) || !PyList_Check(pyList_ipv6))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_server_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		if (vIP_address == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual IP Address!!!");
			break;
		}
		if ((vMac == NULL) || (vMac_size != 6))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Virtual MAC Address!!!");
			break;
		}
		client_data->context = NULL;
		hdr->type = DPS_ENDPOINT_UPDATE;
		hdr->client_id = endpoint_client_type;
		hdr->transaction_type = DPS_TRANSACTION_MASS_COPY;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->sub_type = endpoint_update_type;
		hdr->reply_addr.port = dps_server_port;
		if (dps_server_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_server_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Server: IPv4 %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_server_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Server: pIPv6 %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_server_ip, dps_server_ip_size);
		}
		endpoint_update->dps_client_addr.port = host_port;
		if (host_ip_size == 4)
		{
			// IPv4
			inet_ntop(AF_INET, host_ip, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d, Query ID %d",
			         str, host_port, hdr->query_id);
			endpoint_update->dps_client_addr.family = AF_INET;
			endpoint_update->dps_client_addr.ip4 = ntohl(*((uint32_t *)host_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, host_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv6 %s, Port %d, Query ID %d",
			         str, host_port, hdr->query_id);
			endpoint_update->dps_client_addr.family = AF_INET6;
			memcpy(endpoint_update->dps_client_addr.ip6,
			       host_ip, host_ip_size);
		}
		endpoint_update->version = endpoint_version;
		memcpy(endpoint_update->mac, vMac, vMac_size);
		if (vIP_address_size == 4)
		{
			inet_ntop(AF_INET, vIP_address, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel, "VNID %d, vMac "MAC_FMT", vIP %s",
			         endpoint_vnid, MAC_OCTETS(endpoint_update->mac), str);
			endpoint_update->vm_ip_addr.family = AF_INET;
			endpoint_update->vm_ip_addr.ip4 = ntohl(*((uint32_t *)vIP_address));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, vIP_address, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "VNID %d, vMac "MAC_FMT", vIP %s",
			         endpoint_vnid, MAC_OCTETS(endpoint_update->mac), str);
			endpoint_update->vm_ip_addr.family = AF_INET6;
			memcpy(endpoint_update->vm_ip_addr.ip6, vIP_address, vIP_address_size);
		}
		endpoint_update->vnid = endpoint_vnid;
		endpoint_update->tunnel_info.num_of_tunnels = 0;
		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 endpoint_update.tunnel_info.tunnel_list[endpoint_update->tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_Parse(pyIP, "I", &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv4 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s", i, str);
			tunnel = &endpoint_update->tunnel_info.tunnel_list[endpoint_update->tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET;
			tunnel->ip4 = ntohl(ipv4);
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			endpoint_update->tunnel_info.num_of_tunnels++;
		}
		for (i = 0; i < PyList_Size(pyList_ipv6); i++, j++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 endpoint_update.tunnel_info.tunnel_list[endpoint_update->tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv6 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "[%d] DOVE Switch %s",
			         endpoint_update->tunnel_info.num_of_tunnels, str);
			tunnel = &endpoint_update->tunnel_info.tunnel_list[endpoint_update->tunnel_info.num_of_tunnels];
			tunnel->family = AF_INET6;
			tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
			tunnel->vnid = endpoint_vnid;
			memcpy(tunnel->ip6, ipv6, ipv6_size);
			endpoint_update->tunnel_info.num_of_tunnels++;
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *mass_transfer_tunnel(PyObject *self, PyObject *args)
{
	int dps_server_ip_size, host_ip_size, i, ipv4, ipv6_size;;
	uint32_t vnid, tunnel_vnid, tunnel_client_type, query_id, tunnel_reg_type;
	uint16_t dps_server_port, host_port;
	char *dps_server_ip, *host_ip, *ipv6;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_tunnel_reg_dereg_t *tunnel_reg = &((dps_client_data_t *)send_buff_py)->tunnel_reg_dereg;
	PyObject *pyList_ip, *pyIP;
	dps_tunnel_endpoint_t *tunnel;
	dps_return_status return_status = DPS_ERROR;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#Hz#HIIIIIO",
		                      &dps_server_ip, &dps_server_ip_size,
		                      &dps_server_port,
		                      &host_ip, &host_ip_size,
		                      &host_port,
		                      &vnid,
		                      &query_id,
		                      &tunnel_vnid,
		                      &tunnel_client_type,
		                      &tunnel_reg_type, //1: Register, 0: Unregister
		                      &pyList_ip))
		{
			log_warn(PythonDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyList_ip))
		{
			log_warn(PythonDataHandlerLogLevel, "Not List Type!!!");
			break;
		}
		if (dps_server_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		client_data->context = NULL;
		if (tunnel_reg_type)
		{
			hdr->type = DPS_TUNNEL_REGISTER;
			hdr->sub_type = DPS_TUNNEL_REGISTER;
		}
		else
		{
			hdr->type = DPS_TUNNEL_DEREGISTER;
			hdr->sub_type = DPS_TUNNEL_DEREGISTER;
		}
		hdr->client_id = tunnel_client_type;
		hdr->transaction_type = DPS_TRANSACTION_MASS_COPY;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_server_port;
		if (dps_server_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_server_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Server: IPv4 %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_server_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Server: pIPv6 %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_server_ip, dps_server_ip_size);
		}
		tunnel_reg->dps_client_addr.port = host_port;
		if (host_ip_size == 4)
		{
			inet_ntop(AF_INET, host_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: IPv4 %s, Port %d, Query ID %d",
			         str, host_port, hdr->query_id);
			tunnel_reg->dps_client_addr.family = AF_INET;
			tunnel_reg->dps_client_addr.ip4 = ntohl(*((uint32_t *)host_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, host_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "DPS Client: pIPv6 %s, Port %d, Query ID %d",
			         str, host_port, hdr->query_id);
			tunnel_reg->dps_client_addr.family = AF_INET6;
			memcpy(tunnel_reg->dps_client_addr.ip6, host_ip, host_ip_size);
		}
		tunnel_reg->tunnel_info.num_of_tunnels = 0;
		for (i = 0; i < PyList_Size(pyList_ip); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 tunnel_reg_dereg.tunnel_info.tunnel_list[tunnel_reg->tunnel_info.num_of_tunnels+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ip),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ip, i);
			if (PyInt_CheckExact(pyIP))
			{
				if (!PyArg_Parse(pyIP, "I", &ipv4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv4 in element %d", i);
					continue;
				}
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "[%d] DOVE Switch %s",
				         tunnel_reg->tunnel_info.num_of_tunnels, str);
				tunnel = &tunnel_reg->tunnel_info.tunnel_list[tunnel_reg->tunnel_info.num_of_tunnels];
				tunnel->family = AF_INET;
				tunnel->ip4 = ntohl(ipv4);
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->vnid = tunnel_vnid;
				tunnel_reg->tunnel_info.num_of_tunnels++;
			}
			else
			{
				if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv6 in element %d", i);
					continue;
				}
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "[%d] DOVE Switch %s",
				         tunnel_reg->tunnel_info.num_of_tunnels, str);
				tunnel = &tunnel_reg->tunnel_info.tunnel_list[tunnel_reg->tunnel_info.num_of_tunnels];
				tunnel->family = AF_INET6;
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->vnid = tunnel_vnid;
				memcpy(tunnel->ip6, ipv6, ipv6_size);
				tunnel_reg->tunnel_info.num_of_tunnels++;
			}
		}
		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *mass_transfer_multicast_sender(PyObject *self, PyObject *args)
{
	int dps_server_ip_size, multicast_mac_size, multicast_ip_size, tunnel_ip_size;
	uint32_t vnid, client_type, query_id, fregister;
	uint16_t dps_server_port, multicast_inet;
	char *dps_server_ip, *tunnel_ip, *multicast_ip, *multicast_mac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_mcast_sender_t *mcast_sender = &((dps_client_data_t *)send_buff_py)->mcast_sender;
	dps_return_status return_status = DPS_ERROR;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonMulticastDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIIz#Hz#z#",
		                      &dps_server_ip, &dps_server_ip_size,
		                      &dps_server_port,
		                      &vnid,
		                      &query_id,
		                      &fregister,
		                      &client_type,
		                      &multicast_mac, &multicast_mac_size,
		                      &multicast_inet,
		                      &multicast_ip, &multicast_ip_size,
		                      &tunnel_ip, &tunnel_ip_size))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (dps_server_ip == NULL)
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		client_data->context = NULL;
		if (fregister)
		{
			hdr->type = DPS_MCAST_SENDER_REGISTRATION;
			//hdr->sub_type = DPS_ENDPOINT_UPDATE_ADD;
		}
		else
		{
			hdr->type = DPS_MCAST_SENDER_DEREGISTRATION;
			//hdr->sub_type = DPS_ENDPOINT_UPDATE_DELETE;
		}
		hdr->client_id = client_type;
		hdr->transaction_type = DPS_TRANSACTION_MASS_COPY;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_server_port;
		if (dps_server_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_server_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Server: IP %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_server_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Server: IP %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_server_ip, dps_server_ip_size);
		}
		mcast_sender->dps_client_addr.family = 0;
		// No support for Source IP Multicast Routing.
		memset(&mcast_sender->mcast_src_vm, 0, sizeof(dps_endpoint_info_t));
		mcast_sender->mcast_src_vm.vnid = vnid;
		// Multicast Address
		memcpy(mcast_sender->mcast_addr.mcast_mac, multicast_mac, multicast_mac_size);
		mcast_sender->mcast_addr.mcast_addr_type = multicast_inet;
		log_info(PythonMulticastDataHandlerLogLevel, "Multicast INET %d", multicast_inet);
		if (multicast_inet == AF_INET)
		{
			inet_ntop(AF_INET, multicast_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel, "Multicast: IP %s", str);
			mcast_sender->mcast_addr.u.mcast_ip4 = ntohl(*((uint32_t *)multicast_ip));
			mcast_sender->mcast_addr.mcast_addr_type = MCAST_ADDR_V4;
		}
		else if (multicast_inet == AF_INET6)
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel, "Multicast: IP %s", str);
			memcpy(mcast_sender->mcast_addr.u.mcast_ip6,
			       multicast_ip, multicast_ip_size);
			mcast_sender->mcast_addr.mcast_addr_type = MCAST_ADDR_V6;
		}

		// Tunnel Address
		if (tunnel_ip_size == 4)
		{
			inet_ntop(AF_INET, tunnel_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel, "Tunnel: IP %s", str);
			mcast_sender->tunnel_endpoint.family = AF_INET;
			mcast_sender->tunnel_endpoint.ip4 = ntohl(*((uint32_t *)tunnel_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel, "Tunnel: IP %s", str);
			mcast_sender->tunnel_endpoint.family = AF_INET6;
			memcpy(mcast_sender->tunnel_endpoint.ip6, tunnel_ip, tunnel_ip_size);
		}

		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonMulticastDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonMulticastDataHandlerLogLevel, "Exit %d", return_status);

	return ret_val;
}

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
PyObject *mass_transfer_multicast_receiver(PyObject *self, PyObject *args)
{
	int dps_server_ip_size, multicast_mac_size, multicast_ip_size, tunnel_ip_size;
	uint32_t vnid, client_type, query_id, fregister, global_scope;
	uint16_t dps_server_port, multicast_inet;
	char *dps_server_ip, *tunnel_ip, *multicast_ip, *multicast_mac;
	PyObject *ret_val;
	dps_client_data_t *client_data = (dps_client_data_t *)send_buff_py;
	dps_client_hdr_t *hdr = &client_data->hdr;
	dps_mcast_receiver_t *mcast_receiver = &((dps_client_data_t *)send_buff_py)->mcast_receiver;
	dps_return_status return_status = DPS_ERROR;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonMulticastDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		if (!PyArg_ParseTuple(args, "z#HIIIIIz#Hz#z#",
		                      &dps_server_ip, &dps_server_ip_size,
		                      &dps_server_port,
		                      &vnid,
		                      &query_id,
		                      &fregister,
		                      &client_type,
		                      &global_scope,
		                      &multicast_mac, &multicast_mac_size,
		                      &multicast_inet,
		                      &multicast_ip, &multicast_ip_size,
		                      &tunnel_ip, &tunnel_ip_size))
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad Data!!!");
			break;
		}
		if (dps_server_ip == NULL)
		{
			log_warn(PythonMulticastDataHandlerLogLevel, "Bad DPS Client IP Address!!!");
			break;
		}
		client_data->context = NULL;
		if (fregister)
		{
			hdr->type = DPS_MCAST_RECEIVER_JOIN;
			//hdr->sub_type = DPS_ENDPOINT_UPDATE_ADD;
		}
		else
		{
			hdr->type = DPS_MCAST_RECEIVER_LEAVE;
			//hdr->sub_type = DPS_ENDPOINT_UPDATE_DELETE;
		}
		hdr->client_id = client_type;
		hdr->transaction_type = DPS_TRANSACTION_MASS_COPY;
		hdr->vnid = vnid;
		hdr->resp_status = DPS_NO_ERR;
		hdr->query_id = query_id;
		hdr->reply_addr.port = dps_server_port;
		if (dps_server_ip_size == 4)
		{
			inet_ntop(AF_INET, dps_server_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Server: IP %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET;
			hdr->reply_addr.ip4 = ntohl(*((uint32_t *)dps_server_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel,
			         "DPS Server: IP %s, Port %d, Query ID %d",
			         str, dps_server_port, hdr->query_id);
			hdr->reply_addr.family = AF_INET6;
			memcpy(hdr->reply_addr.ip6, dps_server_ip, dps_server_ip_size);
		}
		mcast_receiver->dps_client_addr.family = 0;
		// No support for Source IP Multicast Routing.
		mcast_receiver->mcast_group_rec.num_of_srcs = 0;
		mcast_receiver->mcast_group_rec.family = AF_INET;
		// Multicast Address
		memcpy(mcast_receiver->mcast_group_rec.mcast_addr.mcast_mac, multicast_mac, multicast_mac_size);
		mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = multicast_inet;
		if (multicast_inet == AF_INET)
		{
			inet_ntop(AF_INET, multicast_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel, "Multicast: IP %s", str);
			mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip4 = ntohl(*((uint32_t *)multicast_ip));
			mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V4;
		}
		else if (multicast_inet == AF_INET6)
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel, "Multicast: IP %s", str);
			memcpy(mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip6,
			       multicast_ip, multicast_ip_size);
			mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V6;
		}
		if (global_scope)
		{
			log_info(PythonMulticastDataHandlerLogLevel,
			         "Setting MCAST Receiver to GLOBAL SCOPE");
			mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V4_ICB_RANGE;
		}

		// Tunnel Address
		if (tunnel_ip_size == 4)
		{
			inet_ntop(AF_INET, tunnel_ip, str, INET_ADDRSTRLEN);
			// IPv4
			log_info(PythonMulticastDataHandlerLogLevel, "Tunnel: IP %s", str);
			mcast_receiver->tunnel_endpoint.family = AF_INET;
			mcast_receiver->tunnel_endpoint.ip4 = ntohl(*((uint32_t *)tunnel_ip));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, dps_server_ip, str, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel, "Tunnel: IP %s", str);
			mcast_receiver->tunnel_endpoint.family = AF_INET6;
			memcpy(mcast_receiver->tunnel_endpoint.ip6, tunnel_ip, tunnel_ip_size);
		}

		return_status = dps_protocol_client_send(client_data);
		if (return_status != DPS_SUCCESS)
		{
			log_info(PythonMulticastDataHandlerLogLevel,
			         "dps_protocol_client_send returns failure");
		}
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", return_status);
	log_info(PythonMulticastDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject *report_endpoint_conflict(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	unsigned char *mac1, *mac2;
	unsigned char mac1_str[6], mac2_str[6];

	char *vIP,  *pIP1, *pIP2, *dps_client1, *dps_client2;
	uint32_t domain, dvg1, dvg2, version;
	//uint32_t family;
	int vIP_size, mac1_size, mac2_size, pIP1_size, pIP2_size, dps_client1_size, dps_client2_size;
	char vIP_str[INET6_ADDRSTRLEN], pIP1_str[INET6_ADDRSTRLEN], pIP2_str[INET6_ADDRSTRLEN];
	char dps_client1_str[INET6_ADDRSTRLEN], dps_client2_str[INET6_ADDRSTRLEN];
#if 0
	dps_rest_client_to_dove_controller_endpoint_conflict_t endpoint_conflict_msg;
#endif

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	memset(mac1_str,0,6);
	memset(mac2_str,0,6);

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIIIz#z#z#z#z#z#z#",
		                      &domain, &dvg1, &dvg2, &version,
		                      &vIP, &vIP_size,
		                      &mac1, &mac1_size,
		                      &pIP1, &pIP1_size,
		                      &dps_client1, &dps_client1_size,
		                      &mac2, &mac2_size,
		                      &pIP2, &pIP2_size,
		                      &dps_client2, &dps_client2_size))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}

		if (vIP_size == 4)
		{
			inet_ntop(AF_INET, vIP, vIP_str, INET_ADDRSTRLEN);
			//family = AF_INET;
		}
		else
		{
			inet_ntop(AF_INET6, vIP, vIP_str, INET6_ADDRSTRLEN);
			//family = AF_INET6;
		}
		if (pIP1_size == 4)
		{
			inet_ntop(AF_INET, pIP1, pIP1_str, INET6_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, pIP1, pIP1_str, INET6_ADDRSTRLEN);
		}
		if (pIP2_size == 4)
		{
			inet_ntop(AF_INET, pIP2, pIP2_str, INET6_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, pIP2, pIP2_str, INET6_ADDRSTRLEN);
		}
		if (dps_client1_size == 4)
		{
			inet_ntop(AF_INET, dps_client1, dps_client1_str, INET6_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_client1, dps_client1_str, INET6_ADDRSTRLEN);
		}
		if (dps_client2_size == 4)
		{
			inet_ntop(AF_INET, dps_client2, dps_client2_str, INET6_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_client2, dps_client2_str, INET6_ADDRSTRLEN);
		}

		memcpy(mac1_str, mac1, 6);
		memcpy(mac2_str, mac2, 6);

		log_alert(PythonDataHandlerLogLevel,
		          "CONFLICT DETECTED: Sending to Controller vIP %s - "
		          "VNID %d, MAC " MAC_FMT ", pIP %s:%s AND "
		          "VNID %d, MAC " MAC_FMT ", pIP %s:%s",
		          vIP_str,
		          dvg1, MAC_OCTETS(mac1), pIP1_str, dps_client1_str,
		          dvg2, MAC_OCTETS(mac2), pIP2_str, dps_client2_str);
//#if 1
//		dps_rest_client_send_endpoint_conflict_to_dove_controller(domain, dvg1, dvg2, version,
//		                                                          mac1_str, mac2_str,
//		                                                          pIP1_str, pIP2_str,
//		                                                          vIP_str, family);
//#else
//		endpoint_conflict_msg.domain = domain;
//		endpoint_conflict_msg.dvg1 = dvg1;
//		endpoint_conflict_msg.dvg2 = dvg2;
//		endpoint_conflict_msg.version = version;
//		memcpy(&endpoint_conflict_msg.dps_client_addr, dps_client1_str, sizeof(ip_addr_t));
//		memcpy(&endpoint_conflict_msg.virtual_addr, vIP_str, sizeof(ip_addr_t));
//		memcpy(&endpoint_conflict_msg.vMac1, mac_str1, 6);
//		memcpy(&endpoint_conflict_msg.vMac2, mac_str2, 6);
//		memcpy(&endpoint_conflict_msg.physical_addr1, pIP1_str, sizeof(ip_addr_t));
//		memcpy(&endpoint_conflict_msg.physical_addr2, pIP2_str, sizeof(ip_addr_t));
//		dps_rest_client_send_endpoint_conflict_to_dove_controller2(&endpoint_conflict_msg);
//#endif

	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", 0);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
}

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
PyObject * dps_vnids_replicate(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	char *dps_node_ip;
	uint32_t domain, add;
	int dps_node_sz, status = 0;
	PyObject *pyList_vnid;
	char dps_node_str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIOz#",
		                      &add, &domain, &pyList_vnid,
		                      &dps_node_ip, &dps_node_sz))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}
		if (dps_node_sz == 4)
		{
			inet_ntop(AF_INET, dps_node_ip, dps_node_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_node_ip, dps_node_str, INET6_ADDRSTRLEN);
		}
		status = dps_cluster_vnid_replication(dps_node_str, add, domain, pyList_vnid);
		log_debug(PythonDataHandlerLogLevel,"Return status %d", status);
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	log_info(PythonDataHandlerLogLevel, "Exit status %d", status);

	return ret_val;
}

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
PyObject * dps_policy_replicate(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	char *dps_node_ip;
	uint32_t domain, add, traffic_type, type, sdvg, ddvg, ttl, action;
	int dps_node_sz, status = 0;
	char dps_node_str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIIIIIIIz#",
		                      &add, &domain, &traffic_type, &type, &sdvg, &ddvg, &ttl, &action,
		                      &dps_node_ip, &dps_node_sz))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}
		if (dps_node_sz == 4)
		{
			inet_ntop(AF_INET, dps_node_ip, dps_node_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_node_ip, dps_node_str, INET6_ADDRSTRLEN);
		}

		log_info(PythonDataHandlerLogLevel,
		         "Sending to Dps Node %s Policy: Domain %d Type %d Sdvg %d Ddvg %d ttl %d, action %d",
		         dps_node_str, domain, type, sdvg, ddvg, ttl, action);

		status = dps_cluster_policy_replication(dps_node_str, add, domain, traffic_type, type, sdvg, ddvg, ttl, action);
		log_debug(PythonDataHandlerLogLevel,"Return status %d", status);
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	log_info(PythonDataHandlerLogLevel, "Exit status %d", status);

	return ret_val;
}

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
PyObject * dps_policy_bulk_replicate(PyObject *self, PyObject *args)
{
	PyObject *ret_val, *pyPolicy_List;
	char *dps_node_ip;
	uint32_t domain, add;
	int dps_node_sz, status = 0;
	char dps_node_str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIOz#",
		                      &add, &domain, &pyPolicy_List,
		                      &dps_node_ip, &dps_node_sz))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}
		if (dps_node_sz == 4)
		{
			inet_ntop(AF_INET, dps_node_ip, dps_node_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_node_ip, dps_node_str, INET6_ADDRSTRLEN);
		}

		status = dps_cluster_bulk_policy_replication(dps_node_str, add,
		                                             domain, pyPolicy_List);
		log_debug(PythonDataHandlerLogLevel,"Return status %d", status);
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	log_info(PythonDataHandlerLogLevel, "Exit status %d", status);

	return ret_val;
}

/*
 ******************************************************************************
 * dps_ipsubnet_replicate --                                              *//**
 *
 * \brief This is the routine that the PYTHON Scripts calls to replicate subnet 
 *        configuration to other DPS nodes
 *
 * \param[in] self  PyObject
 * \param[in] args  The input must be the following:
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject * dps_ipsubnet_replicate(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	char *dps_node_ip, *subnet, *mask, *gw;
	uint32_t vnid, add, mode;
	int dps_node_sz,subnet_sz, mask_sz, gw_sz,status = 0;
	char dps_node_str[INET6_ADDRSTRLEN], subnet_str[INET6_ADDRSTRLEN],mask_str[INET6_ADDRSTRLEN],gw_str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIz#z#z#Iz#",
		                      &add, &vnid, &subnet, &subnet_sz, &mask, &mask_sz, &gw, &gw_sz, &mode,
		                      &dps_node_ip, &dps_node_sz))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}
		if (dps_node_sz == 4)
		{
			inet_ntop(AF_INET, dps_node_ip, dps_node_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_node_ip, dps_node_str, INET6_ADDRSTRLEN);
		}

		if (subnet_sz == 4)
		{
			inet_ntop(AF_INET, subnet, subnet_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, mask, mask_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, gw, gw_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, subnet, subnet_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, mask, mask_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, gw, gw_str, INET6_ADDRSTRLEN);
		}
		log_info(PythonDataHandlerLogLevel,
		         "Sending to Dps Node %s Vnid %d Crud %d Subnet %s Mask %s Nexthop %s Mode %d",
		         dps_node_str, vnid, add, subnet_str, mask_str, gw_str, mode);
		if (subnet_sz == 4)
		{
			status = dps_cluster_ipsubnet4_replication(dps_node_str, add, vnid,  subnet_str, mask_str, 
			                                           gw_str, mode);
		}
#if 0
		else
		{
			status = dps_cluster_ipsubnet6_replication(dps_node_str, add, vnid,  subnet_str, mask_str, 
			                                           gw_str, mode);
		}
#endif
		log_debug(PythonDataHandlerLogLevel,"Return status %d", status);
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	log_info(PythonDataHandlerLogLevel, "Exit status %d", status);

	return ret_val;
}

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
PyObject * dps_ipsubnet_bulk_replicate(PyObject *self, PyObject *args)
{
	PyObject *ret_val, *pySubnetList;
	char *dps_node_ip;
	uint32_t domain, add;
	int dps_node_sz, status = 0;
	char dps_node_str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	//Py_BEGIN_ALLOW_THREADS

	do
	{
		// Domain, DVG,
		if (!PyArg_ParseTuple(args, "IIOz#",
		                      &domain, &add, &pySubnetList,
		                      &dps_node_ip, &dps_node_sz))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Bulk Subnet Replicate: Cannot parse arguments");
			break;
		}
		if (dps_node_sz == 4)
		{
			inet_ntop(AF_INET, dps_node_ip, dps_node_str, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(AF_INET6, dps_node_ip, dps_node_str, INET6_ADDRSTRLEN);
		}

		status = dps_cluster_bulk_ip4subnet_replication(dps_node_str, add,
		                                                domain, pySubnetList);

		log_debug(PythonDataHandlerLogLevel,"Return status %d", status);
	}while(0);

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	log_info(PythonDataHandlerLogLevel, "Exit status %d", status);

	return ret_val;
}

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
PyObject *vnid_query_send_to_controller(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	uint32_t vnid;

	log_info(PythonDataHandlerLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "I", &vnid))
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Cannot parse arguments");
			break;
		}
		log_info(PythonDataHandlerLogLevel, "Query Controller for VNID %d", vnid);
		dps_rest_client_query_dove_controller_vnid_info(vnid);
	}while(0);
	ret_val = Py_BuildValue("i", 0);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
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
PyObject *domain_query_from_controller(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	uint32_t status = DOVE_STATUS_INVALID_PARAMETER;
	uint32_t domain_id;
	uint32_t replication_factor = 0;

	//Py_BEGIN_ALLOW_THREADS
	log_info(PythonDataHandlerLogLevel, "Enter");
	do
	{
		// Parameters are (vnid)
		if (!PyArg_ParseTuple(args, "I", &domain_id))
		{
			log_error(PythonDataHandlerLogLevel, "NO DATA! - NULL");
			break;
		}
		status = (uint32_t)dps_query_domain_config(domain_id, &replication_factor);
	} while(0);
	log_info(PythonDataHandlerLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("II", status, replication_factor);
	return ret_val;
}

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
PyObject *vnid_query_subnets_from_controller(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	int status = -1;
	uint32_t vnid;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonDataHandlerLogLevel, "Enter");
	do
	{
		// Parameters are (vnid)
		if (!PyArg_ParseTuple(args, "I", &vnid))
		{
			log_error(PythonDataHandlerLogLevel, "NO DATA! - NULL");
			break;
		}
		status = dps_query_dmc_vnid_subnet_config(vnid);
	} while(0);
	log_debug(PythonDataHandlerLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
	return ret_val;
}

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
PyObject *domain_query_policy_from_controller(PyObject *self, PyObject *args)
{
	PyObject *ret_val = NULL;
	int status = -1;
	uint32_t domain, src_vnid, dst_vnid, traffic_type;

	//Py_BEGIN_ALLOW_THREADS
	log_debug(PythonDataHandlerLogLevel, "Enter");
	do
	{
		// Parameters are (vnid)
		if (!PyArg_ParseTuple(args, "IIII", &domain, &src_vnid, &dst_vnid, &traffic_type))
		{
			log_error(PythonDataHandlerLogLevel, "NO DATA! - NULL");
			break;
		}
		status = dps_query_dmc_policy_info(domain, src_vnid, dst_vnid, traffic_type);
	} while(0);
	log_debug(PythonDataHandlerLogLevel, "Exit");

	//Py_END_ALLOW_THREADS
	ret_val = Py_BuildValue("i", status);
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
	dove_status status;

	log_debug(PythonDataHandlerLogLevel,
	          "Enter: Domain %d, Msg Type %d",
	          domain_id, dps_msg->hdr.type);

	do
	{

		// Find a remote node
		status = dps_get_domain_random_remote_node(domain_id,
		                                           &dps_msg->hdr.reply_addr);
		log_debug(PythonDataHandlerLogLevel,
		          "dps_get_domain_random_remote_node for Domain %d: status %d",
		          domain_id, status);

		if (status != DOVE_STATUS_OK)
		{
			break;
		}
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
static dps_return_status dps_form_endpoint_update_reply(dps_client_data_t *dps_msg_reply,
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
//#if defined (NDEBUG)
//	{
//		char str[INET6_ADDRSTRLEN];
//		ip_addr_t reply_addr;
//		memcpy(&reply_addr, &dps_msg_reply->hdr.reply_addr, sizeof(ip_addr_t));
//		if (reply_addr.family == AF_INET)
//		{
//			reply_addr.ip4 = htonl(reply_addr.ip4);
//		}
//		inet_ntop(reply_addr.family, reply_addr.ip6, str, INET6_ADDRSTRLEN);
//		log_debug(PythonDataHandlerLogLevel, "pMsg %p, Reply IP %s, Port %d, Transaction Type %d",
//		          dps_msg_reply, str, reply_addr.port, dps_msg_reply->hdr.transaction_type);
//	}
//#endif

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
static dps_return_status dps_form_tunnel_register_reply(dps_client_data_t *dps_msg_reply,
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
 * dps_msg_update_reply --                                                *//**
 *
 * \brief This routine handles Update Reply (from other DPS Servers) i.e.
 *        Endpoint and Tunnel Updates
 *
 * \param domain_id The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Update
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_update_reply(uint32_t domain_id,
                                              dps_client_data_t *dps_msg)
{
	char str[INET6_ADDRSTRLEN];
	PyObject *strret, *strargs, *pyFunction;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		if (dps_msg->hdr.reply_addr.family == AF_INET)
		{
			dps_msg->hdr.reply_addr.ip4 = htonl(dps_msg->hdr.reply_addr.ip4);
		}
		inet_ntop(dps_msg->hdr.reply_addr.family, dps_msg->hdr.reply_addr.ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel,
		         "Received [UPDATE_REPLY] for [qId %d, transaction type %d] from [%s:%d]",
		         dps_msg->hdr.query_id,
		         dps_msg->hdr.transaction_type,
		         str,
		         dps_msg->hdr.reply_addr.port);

		if (dps_msg->hdr.transaction_type == DPS_TRANSACTION_MASS_COPY)
		{
			//def Transfer_Ack(self, query_id, status):
			strargs = Py_BuildValue("(II)",
			                        dps_msg->hdr.query_id,
			                        dps_msg->hdr.resp_status);
			pyFunction = Mass_Transfer_Interface.Transfer_Ack;
		}
		else
		{
			//def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status):
			strargs = Py_BuildValue("(II)",
			                        dps_msg->hdr.query_id,
			                        dps_msg->hdr.resp_status);
			pyFunction = Replication_Interface.ReplicationReplyProcess;
		}

		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Endpoint_Update call
		strret = PyEval_CallObject(pyFunction, strargs);
		Py_DECREF(strargs);

		if (strret != NULL)
		{
			Py_DECREF(strret);
		}
		else
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject ReplicationReplyProcess returns NULL");
		}

	} while(0);

	PyGILState_Release(gstate);

	log_debug(PythonDataHandlerLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_update_replicate --                                                *//**
 *
 * \brief This routine handles the replication for Updates in general for e.g.
 *        Endpoint Updates and Tunnel Updates
 *
 * \param domain_id The Domain ID
 * \param update_msg The DPS Protocol message related to the Updates
 * \param update_reply A pre-formed reply message. This may be NULL in which
 *                     case the update doesn't need an ACK.
 * \param msg_size The size of Update Message
 * \param plocal_query_id The query id to be used for local endpoint update
 *                        is stored here. In case this value is set to 0 the
 *                        local node shouldn't store this endpoint request
 *
 * \return dps_resp_status_t
 *
 *****************************************************************************/

static dps_resp_status_t dps_update_replicate(uint32_t domain_id,
                                              dps_client_data_t *update_msg,
                                              dps_client_data_t *update_reply,
                                              size_t msg_size,
                                              uint32_t *plocal_query_id)
{
	dps_client_data_t update_replicated;
	dps_client_data_t *pupdate_replicated;
	int replicated_msg_from_heap = 0;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	dps_resp_status_t ret_code = DPS_NO_MEMORY;
	uint32_t dps_status;
	PyObject *py_update_reply, *py_ipaddress_list, *py_queryid_list;
	int i, list_size;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter: Msg_size %d", msg_size);

	*plocal_query_id = 0;
	py_ipaddress_list = py_queryid_list = py_update_reply = NULL;

	gstate = PyGILState_Ensure();

	do
	{
		if (msg_size > sizeof(dps_client_data_t))
		{
			pupdate_replicated = (dps_client_data_t *)malloc(msg_size);
			if (pupdate_replicated == NULL)
			{
				break;
			}
			log_debug(PythonDataHandlerLogLevel,
			          "Allocated pupdate_replicated %p, size %d",
			          pupdate_replicated, msg_size);
			replicated_msg_from_heap = 1;
		}
		else
		{
			pupdate_replicated = &update_replicated;
		}
		// Copy and form the base replication message
		memcpy(pupdate_replicated, update_msg, msg_size);

		// Indicate Replication
		pupdate_replicated->hdr.transaction_type = DPS_TRANSACTION_REPLICATION;
		pupdate_replicated->hdr.resp_status = DPS_NO_ERR;

		// Need to reset the context
		pupdate_replicated->context = NULL;

		py_update_reply = PyCObject_FromVoidPtr((void *)update_reply, NULL);
		if (py_update_reply == NULL)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "PyCObject_FromVoidPtr returns NULL");
			break;
		}
		log_debug(PythonDataHandlerLogLevel, "py_update_reply %p", py_update_reply);
		log_debug(PythonDataHandlerLogLevel, "update_reply: Query ID %d", update_reply->hdr.query_id);
		log_debug(PythonDataHandlerLogLevel, "update_reply: Message Type %d", update_reply->hdr.type);

		//def ReplicationQueryIDGenerate(self, msg_domain_id, transaction_type, reply_message):
		strargs = Py_BuildValue("(IIO)", domain_id,
		                                 update_msg->hdr.transaction_type,
		                                 py_update_reply);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Py_BuildValue returns NULL");
			break;
		}

		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Replication_Interface.ReplicationQueryIDGenerate,
		                           strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject ReplicationQueryIDGenerate returns NULL");
			break;
		}

		// Parse the return value (DOVEStatus, DPS Servers List, Query ID List)
		PyArg_ParseTuple(strret, "IOO", &dps_status, &py_ipaddress_list, &py_queryid_list);

		ret_code = (dps_resp_status_t)dps_status;
		if (ret_code != DPS_NO_ERR)
		{
			Py_DECREF(strret);
			log_notice(PythonDataHandlerLogLevel,
			           "ReplicationQueryIDGenerate returns dps_status %d",
			           ret_code);
			break;
		}

		// Parse the DPS Server List and the Query ID List
		list_size = PyList_Size(py_ipaddress_list);

		log_debug(PythonDataHandlerLogLevel,
		          "ReplicationQueryIDGenerate List Size %d",
		          list_size);

		// Iterate through the List of DPS Server Addresses to determine which
		// nodes to replicate to.
		for (i = 0; i < list_size; i++)
		{
			PyObject *py_ipaddress, *py_queryid;
			uint16_t family, port;
			int IP_packed_size;
			char *IP_packed;

			py_ipaddress = PyList_GetItem(py_ipaddress_list, i);
			if (py_ipaddress == NULL)
			{
				log_error(PythonDataHandlerLogLevel,
				          "No IP Address in element %d", i);
				continue;
			}
			if (!PyArg_ParseTuple(py_ipaddress, "Hz#H",
			                      &family,
			                      &IP_packed, &IP_packed_size,
			                      &port))
			{
				log_error(PythonDataHandlerLogLevel,
				         "Invalid IP Address in Replication element %d", i);
				continue;
			}
			py_queryid = PyList_GetItem(py_queryid_list, i);
			if (py_queryid == NULL)
			{
				log_error(PythonDataHandlerLogLevel,
				          "No QueryID in element %d", i);
				continue;
			}
			if (!PyArg_Parse(py_queryid, "I",
			                 &pupdate_replicated->hdr.query_id))
			{
				log_error(PythonDataHandlerLogLevel,
				         "Invalid QueryID in Replication element %d", i);
				continue;
			}
			inet_ntop(family, IP_packed, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "sending [REPLICATION] request [qId = %d] to DPS [%s:%d]",
			         pupdate_replicated->hdr.query_id, str, port);
			if (!memcmp(IP_packed, dps_local_ip.ip6, IP_packed_size))
			{
				log_info(PythonDataHandlerLogLevel,
				          "Local Node %s: Not replicating, local replication query Id %d",
				          str, pupdate_replicated->hdr.query_id);
				// Copy the query id to return parameter
				*plocal_query_id = pupdate_replicated->hdr.query_id;
				continue;
			}
			pupdate_replicated->hdr.reply_addr.family = family;
			pupdate_replicated->hdr.reply_addr.port = port;
			memcpy(pupdate_replicated->hdr.reply_addr.ip6,
			       IP_packed, 16);
			if(pupdate_replicated->hdr.reply_addr.family == AF_INET)
			{
				pupdate_replicated->hdr.reply_addr.ip4 =
					ntohl(pupdate_replicated->hdr.reply_addr.ip4);
			}

			dps_protocol_client_send(pupdate_replicated);

			//Handle the case of no-reply needed (RARE)
			if (update_reply == NULL)
			{
				//ABiswas: This is sort of hack but should work. The
				//         processing of update reply only cares about
				//         query id and response status. So we process
				//         this message itself as if it came as a reply.
				pupdate_replicated->hdr.resp_status = DPS_NO_ERR;
				dps_msg_update_reply(domain_id, pupdate_replicated);
			}
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);

	//Free the update replicated message if allocated from heap
	if (replicated_msg_from_heap)
	{
		log_debug(PythonDataHandlerLogLevel,
		          "Freeing pupdate_replicated %p",
		          pupdate_replicated);
		free(pupdate_replicated);
		pupdate_replicated = NULL;
	}

	//Lose the PYTHON container for the reply message in the failure case
	if (ret_code != DPS_NO_ERR)
	{
		if (py_update_reply != NULL)
		{
			Py_DECREF(py_update_reply);
			py_update_reply = NULL;
		}
	}

	log_debug(PythonDataHandlerLogLevel, "Exit: status %d", ret_code);

	return ret_code;
}

/*
 ******************************************************************************
 * dps_msg_endpoint_update --                                             *//**
 *
 * \brief This routine handles Endpoint Update
 *
 * \param domain_id The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Endpoint Update
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_endpoint_update(uint32_t domain_id,
                                                 dps_client_data_t *dps_msg)
{
	ip_addr_t dps_client_address, sender_client_address;
	dps_tunnel_endpoint_t pIP_address;
	dps_client_data_t *dps_msg_reply = NULL;
	dps_endpoint_update_t *endpoint_update_msg = NULL;
	PyObject *strret, *strargs;
	PyObject *py_vIP_List;
	PyGILState_STATE gstate;
	size_t dps_msg_size;
	uint32_t query_id = 0;
	int ret_code = DPS_NO_MEMORY; // dps_resp_status_t
	int shared_address = 0; // 0 Dedicated, 1 Shared
	int fFreeReplyMessage = 0;
	int i;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain_id);

	do
	{
		//Create the Reply Message
		dps_msg_reply = (dps_client_data_t *)malloc(sizeof(dps_client_data_t));
		if (dps_msg_reply == NULL)
		{
			break;
		}
		fFreeReplyMessage = 1;
		log_debug(PythonDataHandlerLogLevel, "Allocated Message %p:%d",
		          dps_msg_reply, sizeof(dps_client_data_t));
		dps_form_endpoint_update_reply(dps_msg_reply, dps_msg, DPS_NO_ERR);

		endpoint_update_msg = (dps_endpoint_update_t *)&dps_msg->endpoint_update;

		if ((endpoint_update_msg->dps_client_addr.family != AF_INET) &&
		    (endpoint_update_msg->dps_client_addr.family != AF_INET6))
		{
			ret_code = DPS_INVALID_SRC_IP;
			break;
		}
		if (endpoint_update_msg->tunnel_info.num_of_tunnels != 0)
		{
			if ((endpoint_update_msg->tunnel_info.tunnel_list[0].family != AF_INET) &&
			    (endpoint_update_msg->tunnel_info.tunnel_list[0].family != AF_INET6))
			{
				ret_code = DPS_INVALID_SRC_IP;
				break;
			}
		}
		if ((endpoint_update_msg->vm_ip_addr.family != AF_INET) &&
		    (endpoint_update_msg->vm_ip_addr.family != AF_INET6))
		{
			ret_code = DPS_INVALID_SRC_IP;
			break;
		}
		// Copy the Addresses so that they can be converted to Network Order
		memcpy(&sender_client_address, &dps_msg->hdr.reply_addr, sizeof(ip_addr_t));
		memcpy(&dps_client_address, &endpoint_update_msg->dps_client_addr, sizeof(ip_addr_t));
		memcpy(&pIP_address, &endpoint_update_msg->tunnel_info.tunnel_list[0], sizeof(dps_tunnel_endpoint_t));
		memcpy(&vIP_address[0], &endpoint_update_msg->vm_ip_addr, sizeof(ip_addr_t));

		if(sender_client_address.family == AF_INET)
		{
			sender_client_address.ip4 = htonl(sender_client_address.ip4);
		}
		if(dps_client_address.family == AF_INET)
		{
			dps_client_address.ip4 = htonl(dps_client_address.ip4);
		}
		if(pIP_address.family == AF_INET)
		{
			pIP_address.ip4 = htonl(pIP_address.ip4);
		}
		if(vIP_address[0].family == AF_INET)
		{
			vIP_address[0].ip4 = htonl(vIP_address[0].ip4);
		}
		{
			log_info(PythonDataHandlerLogLevel, "VNID %d", dps_msg->hdr.vnid);
			inet_ntop(sender_client_address.family, sender_client_address.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "Sender IP %s, Port %d",
			          str, sender_client_address.port);
			inet_ntop(dps_client_address.family, dps_client_address.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "DPS Client %s, Port %d",
			          str, dps_client_address.port);
			inet_ntop(pIP_address.family, pIP_address.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "DOVE Switch %s", str);
			inet_ntop(vIP_address[0].family, vIP_address[0].ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "Endpoint IP %s", str);
			log_info(PythonDataHandlerLogLevel, "Endpoint Mac " MAC_FMT,
			          MAC_OCTETS(dps_msg->endpoint_update.mac));
			log_info(PythonDataHandlerLogLevel, "Query ID %d", dps_msg->hdr.query_id);
			log_info(PythonDataHandlerLogLevel, "Sub Type %d", dps_msg->hdr.sub_type);
			log_info(PythonDataHandlerLogLevel, "Sender Type %d", dps_msg->hdr.client_id);
			log_info(PythonDataHandlerLogLevel, "Transaction Type %d", dps_msg->hdr.transaction_type);
			log_info(PythonDataHandlerLogLevel, "Version %d", endpoint_update_msg->version);
		}

		// Check if there are other nodes this request should be forwarded to.
		dps_msg_size = dps_offsetof(dps_client_data_t,
		                            endpoint_update.tunnel_info.tunnel_list[endpoint_update_msg->tunnel_info.num_of_tunnels]);
		ret_code = dps_update_replicate(domain_id,
		                                dps_msg,
		                                dps_msg_reply,
		                                dps_msg_size,
		                                &query_id);
		if (ret_code != DPS_NO_ERR)
		{
			break;
		}
		// Let the query handler take care of reply message
		fFreeReplyMessage = 0;
		if (query_id == 0)
		{
			break;
		}

		gstate = PyGILState_Ensure();
		// Update Local Nodes data structures
		//def Endpoint_Update(self, domain_id, dvg_id, vnid, client_type, transaction_type,
		//                    dps_client_IP_type, dps_client_IP_packed, dps_client_port,
		//                    pIP_type, pIP_packed,
		//                    vMac, vIP_type, vIP_packed,
		//                    operation, version):
		strargs = Py_BuildValue("(IIIIIIz#IIz#z#Iz#II)",
		                        domain_id,
		                        dps_msg->hdr.vnid,
		                        endpoint_update_msg->vnid,
		                        dps_msg->hdr.client_id,
		                        dps_msg->hdr.transaction_type,
		                        dps_client_address.family,
		                        dps_client_address.ip6, 16,
		                        dps_client_address.port,
		                        pIP_address.family,
		                        pIP_address.ip6, 16,
		                        (char *)endpoint_update_msg->mac, 6,
		                        vIP_address[0].family,
		                        vIP_address[0].ip6, 16,
		                        dps_msg->hdr.sub_type, endpoint_update_msg->version);
		if (strargs == NULL)
		{
			// Let the request timeout
			PyGILState_Release(gstate);
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			ret_code = DPS_NO_MEMORY;
			break;
		}

		// Invoke the Endpoint_Update call
		strret = PyEval_CallObject(Client_Protocol_Interface.Endpoint_Update, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			// Let the request timeout
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Endpoint_Update returns NULL");
			ret_code = DPS_NO_MEMORY;
			break;
		}

		// Parse the return value (ret_val, version)
		PyArg_ParseTuple(strret, "IIIO", &ret_code, &shared_address, &endpoint_update_msg->version, &py_vIP_List);

		log_info(PythonDataHandlerLogLevel,
		         "PYTHON Endpoint_Update: returns ret_code %d, shared_address %d, "
		         "version %d, vIP_List size %d",
		         ret_code, shared_address,
		         endpoint_update_msg->version, PyList_Size(py_vIP_List));

		// Copy the vIP_Address from the py_vIP_List into the Reply
		for (i = 0; i < PyList_Size(py_vIP_List); i++)
		{
			PyObject *py_vIP;
			if (i == MAX_VIP_ADDR)
			{
				break;
			}
			py_vIP = PyList_GetItem(py_vIP_List, i);
			if (PyInt_CheckExact(py_vIP))
			{
				//IPv4
				if (!PyArg_Parse(py_vIP, "I",
				                 &dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Cannot get IPv4 address at vIP_List Index %d",
					         i);
					continue;
				}
				dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].family = AF_INET;
				dps_msg_reply->endpoint_update_reply.num_of_vip++;
				inet_ntop(AF_INET, &dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip4, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Endpoint Update Reply [%d] vIP %s", i, str);
				dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip4 =
					ntohl(dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip4);
			}
			else
			{
				char *ipv6;
				int ipv6_size;
				//Assume IPv6
				//IPv4
				if (!PyArg_Parse(py_vIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Cannot get IPv6 address at vIP_List Index %d",
					         i);
					continue;
				}
				dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].family = AF_INET6;
				memcpy(dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip6,
				       ipv6, ipv6_size);
				dps_msg_reply->endpoint_update_reply.num_of_vip++;
				inet_ntop(AF_INET6, dps_msg_reply->endpoint_update_reply.vm_ip_addr[i].ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Endpoint Update Reply [%d] vIP %s", i, str);
			}

		}
		log_info(PythonDataHandlerLogLevel, "Tunnel Info: Family %d",
		         dps_msg_reply->endpoint_update_reply.tunnel_info.tunnel_list[0].family);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);

		// Handle Shared Address Space
		if ((ret_code == DPS_NO_ERR) &&
		    (dps_msg->hdr.transaction_type == DPS_TRANSACTION_NORMAL) &&
		    (shared_address))
		{
			// Send the original message to the node which handles vnid 0
			// Modify the header to indicate VNID 0 and use the local query
			// id
			dps_msg->hdr.vnid = SHARED_ADDR_SPACE_VNID;
			dps_msg->hdr.query_id = query_id;
			dps_msg->endpoint_update.version = 0;
			// Replace the Sender IP with local IP so that the reply
			// comes back here
			memcpy(&dps_msg->hdr.reply_addr, &dps_local_ip, sizeof(ip_addr_t));
			if (dps_msg->hdr.reply_addr.family == AF_INET)
			{
				dps_msg->hdr.reply_addr.ip4 = ntohl(dps_msg->hdr.reply_addr.ip4);
			}
			dps_msg_endpoint_update(0, dps_msg);
		}
		else
		{
			gstate = PyGILState_Ensure();
			//Process the local node replication
			//def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status)
			strargs = Py_BuildValue("(II)", query_id, ret_code);
			if (strargs == NULL)
			{
				// No need to reply - ReplicationReplyProcess
				// Let it timeout, unfortunately there is no better :(
				// If we send the message and clean out the structure
				// then the Python routine when it does timeout will
				// crash on the reply message
				PyGILState_Release(gstate);
				log_warn(PythonDataHandlerLogLevel,
				         "Cannot allocate arguments for local replication reply");
				break;
			}

			// Add the query ID to the dictionary to track the replies
			strret = PyEval_CallObject(Replication_Interface.ReplicationReplyProcess,
			                           strargs);

			Py_DECREF(strargs);

			if (strret == NULL)
			{
				PyGILState_Release(gstate);
				log_warn(PythonDataHandlerLogLevel,
				         "PyEval_CallObject ReplicationReplyProcess returns NULL");
				break;
			}
			// Lose the reference on all parameters and return arguments since they
			// are no longer needed.
			Py_DECREF(strret);
			PyGILState_Release(gstate);
			log_info(PythonDataHandlerLogLevel,
			         "Processed Local Query ID %d",
			         query_id);
		}

	}while(0);

	if ((fFreeReplyMessage) && (dps_msg_reply != NULL))
	{
		free(dps_msg_reply);
	}

	log_debug(PythonDataHandlerLogLevel, "Exit");

	return (ret_code == DPS_NO_ERR ? DPS_SUCCESS: DPS_ERROR);
}

/*
 ******************************************************************************
 * dps_msg_endpoint_request --                                            *//**
 *
 * \brief This routine handles Endpoint Request
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Endpoint Update
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_endpoint_request(uint32_t domain,
                                                  dps_client_data_t *dps_msg)
{
	dps_client_data_t dps_msg_reply, *pdps_msg_reply;
	dps_endpoint_loc_req_t *endpoint_loc_msg;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int ret_code, freplymsgallocated; // dps_resp_status_t
	dps_return_status return_status = DPS_ERROR;
	int vIP_type, vIPv4;
	uint32_t dvg, version;
	char *vMac, *vIPv6;
	char *vIP_ret_packed, *vMac_ret;
	PyObject *pyList_ipv4, *pyList_ipv6;
	int vIP_ret_packed_size, vMac_ret_size;
	ip_addr_t dps_client;
	unsigned int fGateway = 0;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);

	pdps_msg_reply = &dps_msg_reply;
	freplymsgallocated = 0;
	endpoint_loc_msg = (dps_endpoint_loc_req_t *)&dps_msg->endpoint_loc_req;

	//Determine the search criteria
	//TODO: The search criteria should be specified in the location request.
	//      When the DPS Protocol provides the criteria in the request, this
	//      routine will change.
	//      Right now this routine just determines based on the IPv4 value.
	//      If the IPv4 value is 0 (it means 1st 4 bytes of IPv6 is zero
	//      too), then search by vMac else search by IP.

	vIPv4 = htonl(endpoint_loc_msg->vm_ip_addr.ip4);
	vIPv6 = (char *)endpoint_loc_msg->vm_ip_addr.ip6;
	vMac = (char *)endpoint_loc_msg->mac;
	vIP_type = endpoint_loc_msg->vm_ip_addr.family;
	memcpy(&dps_client, &endpoint_loc_msg->dps_client_addr, sizeof(ip_addr_t));
	if (dps_client.family == AF_INET)
	{
		dps_client.ip4 = htonl(dps_client.ip4);
	}

	// Form the Endpoint Location Request reply message header
	pdps_msg_reply->context = NULL;
	pdps_msg_reply->hdr.type = DPS_ENDPOINT_LOC_REPLY;
	pdps_msg_reply->hdr.vnid = dps_msg->hdr.vnid;
	pdps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	pdps_msg_reply->hdr.client_id = DPS_POLICY_SERVER_ID;
	pdps_msg_reply->hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	pdps_msg_reply->hdr.resp_status = DPS_NO_MEMORY;
	memcpy(&pdps_msg_reply->hdr.reply_addr,
	       &dps_msg->endpoint_loc_req.dps_client_addr,
	       sizeof(ip_addr_t));

	log_debug(PythonDataHandlerLogLevel, "Endpoint_Request: domain %d, vnid %d",
	          domain, endpoint_loc_msg->vnid);

#if defined(NDEBUG)
	{
		char str_dps[INET6_ADDRSTRLEN];
		char str_sender[INET6_ADDRSTRLEN];
		char str_vIP[INET6_ADDRSTRLEN];
		ip_addr_t sender;

		inet_ntop(dps_client.family, dps_client.ip6, str_dps, INET6_ADDRSTRLEN);
		memcpy(&sender, &dps_msg->hdr.reply_addr, sizeof(ip_addr_t));
		if (sender.family == AF_INET)
		{
			sender.ip4 = htonl(sender.ip4);
		}
		inet_ntop(sender.family, sender.ip6, str_sender, INET6_ADDRSTRLEN);
		if (vIP_type == AF_INET)
		{
			inet_ntop(vIP_type, &vIPv4, str_vIP, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(vIP_type, vIPv6, str_vIP, INET6_ADDRSTRLEN);
		}
		log_info(PythonDataHandlerLogLevel,
		          "Endpoint_Request: Domain %d, VNID %d, DPS Client %s:%d, Sender %s:%d, vIP %s, vMac " MAC_FMT,
		          domain, endpoint_loc_msg->vnid, str_dps, dps_client.port,
		          str_sender, sender.port, str_vIP, MAC_OCTETS(vMac));
	}
#endif

	do{
		gstate = PyGILState_Ensure();
		if (vIPv4 == 0)
		{
			// Search by MAC
			// Endpoint_Location_vMac(self, domain_id, src_dvg, vMac):
			strargs = Py_BuildValue("(IIz#)",
			                        domain, endpoint_loc_msg->vnid, vMac, 6);
			if (strargs == NULL)
			{
				// Release the PYTHON Global Interpreter Lock
				PyGILState_Release(gstate);
				log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
				break;
			}
			// Ensure the PYTHON Global Interpreter Lock
			// Invoke the Endpoint_Location_vMac call
			strret = PyEval_CallObject(Client_Protocol_Interface.Endpoint_Location_vMac, strargs);
			Py_DECREF(strargs);
		}
		else
		{
			// Search by IP
			// def Endpoint_Location_vIP(self, domain_id, client_type, dps_client_IP_type, dps_client_IP_packed,
			//                           src_dvg_id, vIP_type, vIP_val):
			if (vIP_type == AF_INET)
			{
				strargs = Py_BuildValue("(IIIz#III)",
				                        domain, dps_msg->hdr.client_id,
				                        dps_client.family, dps_client.ip6, 16,
				                        endpoint_loc_msg->vnid,
				                        vIP_type, vIPv4);
			}
			else
			{
				strargs = Py_BuildValue("(IIIz#IIz#)",
				                        domain,
				                        domain, dps_msg->hdr.client_id,
				                        dps_client.family, dps_client.ip6, 16,
				                        endpoint_loc_msg->vnid,
				                        vIP_type, vIPv6, 16);
			}
			if (strargs == NULL)
			{
				// Release the PYTHON Global Interpreter Lock
				PyGILState_Release(gstate);
				log_warn(PythonDataHandlerLogLevel,
				         "Py_BuildValue for Endpoint_Location_vIP returns NULL");
				break;
			}
			// Invoke the Endpoint_Location_vMac call
			strret = PyEval_CallObject(Client_Protocol_Interface.Endpoint_Location_vIP, strargs);
			Py_DECREF(strargs);
		}

		if (strret == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Endpoint_Location_X returns NULL");
			break;
		}

		//@return: status, dvg_id, version, pIP_packed, vMac, vIP_packed
		//@rtype: Integer, Integer, Integer, ByteArray, ByteArray
		PyArg_ParseTuple(strret, "IIIOOz#z#I", &ret_code, &dvg, &version,
		                 &pyList_ipv4, &pyList_ipv6,
		                 &vMac_ret, &vMac_ret_size,
		                 &vIP_ret_packed, &vIP_ret_packed_size, &fGateway);

		if (fGateway)
		{
			pdps_msg_reply->hdr.sub_type = DPS_ENDPOINT_LOC_REPLY_GW;
		}
		else
		{
			pdps_msg_reply->hdr.sub_type = DPS_ENDPOINT_LOC_REPLY_VM;
		}
		pdps_msg_reply->hdr.resp_status = ret_code;
		pdps_msg_reply->endpoint_loc_reply.vnid = dvg;
#if defined(NDEBUG)
		log_info(PythonDataHandlerLogLevel,
		         "Endpoint_Request: status %d, vnid %d", ret_code, dvg);
#endif
		if (ret_code == DPS_NO_ERR)
		{
			Py_ssize_t i, j;
			int num_tunnelsv4, num_tunnelsv6;
			PyObject *pypIP;
			size_t dps_msg_reply_size;
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
#endif
			return_status = DPS_SUCCESS;
			pdps_msg_reply->endpoint_loc_reply.version =version;
			if (vMac_ret != NULL)
			{
#if defined(NDEBUG)
				log_info(PythonDataHandlerLogLevel,
				         "Endpoint_Request: vMac " MAC_FMT,
				         MAC_OCTETS(vMac_ret));
#endif
				memcpy(pdps_msg_reply->endpoint_loc_reply.mac,
				       vMac_ret,
				       6);
			}
			else
			{
				memset(pdps_msg_reply->endpoint_loc_reply.mac, 0, 6);
			}
			if (vIP_ret_packed != NULL)
			{
				if (vIP_ret_packed_size == 4)
				{
					// IPv4
#if defined(NDEBUG)
					inet_ntop(AF_INET, vIP_ret_packed, str, INET_ADDRSTRLEN);
					log_info(PythonDataHandlerLogLevel,
					          "Endpoint_Request: vIPv4 %s", str);
#endif
					pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.family = AF_INET;
					memcpy(&pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.ip4,
					       vIP_ret_packed,
					       4);
					pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.ip4 =
						ntohl(pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.ip4);
				}
				else
				{
					// IPv6
#if defined(NDEBUG)
					inet_ntop(AF_INET6, vIP_ret_packed, str, INET6_ADDRSTRLEN);
					log_info(PythonDataHandlerLogLevel,
					          "Endpoint_Request: vIPv6 %s", str);
#endif
					pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.family = AF_INET6;
					memcpy(pdps_msg_reply->endpoint_loc_reply.vm_ip_addr.ip6,
					       vIP_ret_packed,
					       16);
				}
			}
			// Determine if a bigger reply message size is needed
			num_tunnelsv4 = PyList_Size(pyList_ipv4);
			num_tunnelsv6 = PyList_Size(pyList_ipv6);
#if defined(NDEBUG)
			log_info(PythonDataHandlerLogLevel,
			          "Endpoint Request: Tunnels IPv4 %d, IPv6 %d",
			          num_tunnelsv4, num_tunnelsv6);
#endif
			dps_msg_reply_size = dps_offsetof(dps_client_data_t,
			                                  endpoint_loc_reply.tunnel_info.tunnel_list[num_tunnelsv4 + num_tunnelsv6]);

			if (dps_msg_reply_size > sizeof(dps_client_data_t))
			{
				pdps_msg_reply = (dps_client_data_t *)malloc(dps_msg_reply_size);
				if (pdps_msg_reply == NULL)
				{
					// Set back to old values
					pdps_msg_reply = &dps_msg_reply;
					if (PyList_Size(pyList_ipv4) > 0)
					{
						num_tunnelsv4 = 1;
						num_tunnelsv6 = 0;
					}
					else
					{
						num_tunnelsv4 = 0;
						num_tunnelsv6 = 1;
					}
				}
				else
				{
					freplymsgallocated = 1;
					log_debug(PythonDataHandlerLogLevel,
					          "Allocated Reply Message %p:%d sizeof dps_client_data_t %d",
					          pdps_msg_reply, dps_msg_reply_size, sizeof(dps_client_data_t));
					memcpy(pdps_msg_reply, &dps_msg_reply, sizeof(dps_client_data_t));
				}
			}
			j = 0;
			for (i = 0; i < num_tunnelsv4; i++)
			{
				int ipv4;
				pypIP = PyList_GetItem(pyList_ipv4, i);
				dps_tunnel_endpoint_t *tunnel;

				if (!PyArg_Parse(pypIP, "I", &ipv4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Endpoint_Request: Invalid IPv4 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Endpoint_Request: [%d] pIPv4 %s", i, str);
#endif
				tunnel = &pdps_msg_reply->endpoint_loc_reply.tunnel_info.tunnel_list[j];
				tunnel->family = AF_INET;
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->ip4 = ntohl(ipv4);
				tunnel->vnid = dvg;
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				j++;
			}
			for (i = 0; i < num_tunnelsv6; i++)
			{
				char *ipv6;
				int ipv6_size;
				dps_tunnel_endpoint_t *tunnel;

				pypIP = PyList_GetItem(pyList_ipv6, i);
				if (!PyArg_Parse(pypIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Endpoint_Request: Invalid IPv6 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				          "Endpoint_Request: [%d] pIPv6 %s", i, str);
#endif
				tunnel = &pdps_msg_reply->endpoint_loc_reply.tunnel_info.tunnel_list[j];
				tunnel->family = AF_INET6;
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->vnid = dvg;
				memcpy(tunnel->ip6, ipv6, ipv6_size);
				j++;
			}
			pdps_msg_reply->endpoint_loc_reply.tunnel_info.num_of_tunnels = j;
		}
		else
		{
			// Failure case
			// Just copy the destination endpoint location from the request message
			memset(&pdps_msg_reply->endpoint_loc_reply, 0, sizeof(dps_endpoint_loc_reply_t));
			memcpy(&pdps_msg_reply->endpoint_loc_reply.vm_ip_addr,
			       &endpoint_loc_msg->vm_ip_addr,
			       sizeof(ip_addr_t));
			pdps_msg_reply->endpoint_loc_reply.vnid = endpoint_loc_msg->vnid;
			memcpy(pdps_msg_reply->endpoint_loc_reply.mac,
			       endpoint_loc_msg->mac,
			       6);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		// Release the PYTHON Global Interpreter Lock
		PyGILState_Release(gstate);
	}while(0);

	dps_msg_send_inline(pdps_msg_reply);
	if (freplymsgallocated)
	{
		log_debug(PythonDataHandlerLogLevel, "Freeing Reply Message %p",
		          pdps_msg_reply);
		free(pdps_msg_reply);
		pdps_msg_reply = &dps_msg_reply;
	}

	log_debug(PythonDataHandlerLogLevel, "Exit");

	return return_status;

}

/*
 ******************************************************************************
 * dps_msg_policy_request --                                             *//**
 *
 * \brief This routine handles Policy Resolution Request
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Policy Resolution
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_policy_request(uint32_t domain,
                                                dps_client_data_t *dps_msg)
{
	dps_return_status return_status = DPS_ERROR;
	dps_client_data_t dps_msg_reply, *pdps_msg_reply;
	dps_policy_req_t *policy_req_msg;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int ret_code, freplymsgallocated; // dps_resp_status_t
	int vIP_type, vIPv4;
	uint32_t dvg, endpoint_version, policy_version, type, ttl;
	char *vMac, *vIPv6;
	PyObject *pyList_ipv4, *pyList_ipv6;
	char *vIP_ret_packed, *vMac_ret, *action_ret;
	int vIP_ret_packed_size, vMac_ret_size, action_ret_packed_size;
	ip_addr_t dps_client;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);

	pdps_msg_reply = &dps_msg_reply;
	freplymsgallocated = 0;
	policy_req_msg = (dps_policy_req_t *)&dps_msg->policy_req;
	dvg = policy_req_msg->src_endpoint.vnid;
	vIPv4 = htonl(policy_req_msg->dst_endpoint.vm_ip_addr.ip4);
	vIPv6 = (char *)policy_req_msg->dst_endpoint.vm_ip_addr.ip6;
	vMac = (char *)policy_req_msg->dst_endpoint.mac;
	vIP_type = policy_req_msg->dst_endpoint.vm_ip_addr.family;
	memcpy(&dps_client, &policy_req_msg->dps_client_addr, sizeof(ip_addr_t));
	if (dps_client.family == AF_INET)
	{
		dps_client.ip4 = htonl(dps_client.ip4);
	}

#if defined(NDEBUG)
	log_debug(PythonDataHandlerLogLevel, "Policy_Request: domain %d, source vnid %d",
	          domain, dvg);
	{
		char str_dps[INET6_ADDRSTRLEN];
		char str_sender[INET6_ADDRSTRLEN];
		char str_vIP[INET6_ADDRSTRLEN];
		ip_addr_t sender;
		inet_ntop(dps_client.family, dps_client.ip6, str_dps, INET6_ADDRSTRLEN);
		memcpy(&sender, &dps_msg->hdr.reply_addr, sizeof(ip_addr_t));
		if (sender.family == AF_INET)
		{
			sender.ip4 = htonl(sender.ip4);
		}
		inet_ntop(sender.family, sender.ip6, str_sender, INET6_ADDRSTRLEN);
		if (vIP_type == AF_INET)
		{
			inet_ntop(vIP_type, &vIPv4, str_vIP, INET_ADDRSTRLEN);
		}
		else
		{
			inet_ntop(vIP_type, vIPv6, str_vIP, INET6_ADDRSTRLEN);
		}
		log_info(PythonDataHandlerLogLevel,
		          "Policy_Request: Domain %d, Src VNID %d, DPS Client %s:%d, Sender %s:%d, vIP %s",
		          domain, dvg, str_dps, dps_client.port, str_sender, sender.port, str_vIP);
	}

#endif

	// Form the Policy Resolution Request reply message header
	pdps_msg_reply->context = NULL;
	pdps_msg_reply->hdr.type = DPS_POLICY_REPLY;
	pdps_msg_reply->hdr.vnid = dps_msg->hdr.vnid;
	pdps_msg_reply->hdr.query_id = dps_msg->hdr.query_id;
	pdps_msg_reply->hdr.client_id = DPS_POLICY_SERVER_ID;
	pdps_msg_reply->hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	pdps_msg_reply->hdr.resp_status = DPS_NO_MEMORY;
	memcpy(&pdps_msg_reply->hdr.reply_addr,
	       &dps_msg->policy_req.dps_client_addr,
	       sizeof(ip_addr_t));

	do
	{
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		if (vIPv4 == 0)
		{
			// Search by MAC
			// Policy_Resolution_vMac(self, domain_id, src_dvg_id, vMac):
			strargs = Py_BuildValue("(IIz#)", domain, dvg, vMac, 6);
			if (strargs == NULL)
			{
				// Release the PYTHON Global Interpreter Lock
				PyGILState_Release(gstate);
				log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
				break;
			}
			// Invoke the Policy_Resolution_vMac call
			strret = PyEval_CallObject(Client_Protocol_Interface.Policy_Resolution_vMac, strargs);
		}
		else
		{
			// Search by IP
			// def Policy_Resolution_vIP(self, domain_id, client_type, dps_client_IP_type, dps_client_IP_packed,
			//                           src_dvg_id, vIP_type, vIP_val):
			if (vIP_type == AF_INET)
			{
				strargs = Py_BuildValue("(IIIz#III)",
				                        domain, dps_msg->hdr.client_id,
				                        dps_client.family, dps_client.ip6, 16,
				                        dvg,
				                        vIP_type, vIPv4);
			}
			else
			{
				strargs = Py_BuildValue("(IIIz#IIz#)",
				                        domain, dps_msg->hdr.client_id,
				                        dps_client.family, dps_client.ip6, 16,
				                        dvg,
				                        vIP_type, vIPv6, 16);
			}
			if (strargs == NULL)
			{
				// Release the PYTHON Global Interpreter Lock
				PyGILState_Release(gstate);
				log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
				break;
			}
			// Invoke the Policy_Resolution_vIP call
			strret = PyEval_CallObject(Client_Protocol_Interface.Policy_Resolution_vIP, strargs);
		}
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Policy_Resolution_X returns NULL");
			break;
		}

		//@return: status, dvg_id, version, pIP_packed, vMac, vIP_packed, version, ttl, type, action
		//@rtype: Integer, Integer, Integer, ByteArray, ByteArray, ByteArray, Integer, Integer, Integer, ByteArray
		PyArg_ParseTuple(strret, "IIIOOz#z#IIIz#",
		                 &ret_code, &dvg, &endpoint_version,
		                 &pyList_ipv4, &pyList_ipv6,
		                 &vMac_ret, &vMac_ret_size,
		                 &vIP_ret_packed, &vIP_ret_packed_size,
		                 &policy_version, &ttl, &type,
		                 &action_ret, &action_ret_packed_size);

		// Form the remaining part of the Policy Resolution Request reply message header
		pdps_msg_reply->hdr.resp_status = ret_code;
#if defined(NDEBUG)
		log_info(PythonDataHandlerLogLevel,"Policy Lookup: SRC DVG %d, RET_CODE %d",
		         policy_req_msg->src_endpoint.vnid,
		         pdps_msg_reply->hdr.resp_status);
#endif
		pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vnid = dvg;

		if (ret_code == DPS_NO_ERR)
		{
			Py_ssize_t i, j;
			int num_tunnelsv4, num_tunnelsv6;
			PyObject *pypIP;
			size_t dps_msg_reply_size;
#if defined(NDEBUG)
			char str[INET6_ADDRSTRLEN];
#endif

			return_status = DPS_SUCCESS;
			pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vnid = dvg;
			pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.version = endpoint_version;
			if (vMac_ret != NULL)
			{
				log_debug(PythonDataHandlerLogLevel,
				          "Policy_Request: vMac " MAC_FMT,
				          MAC_OCTETS(vMac_ret));
				memcpy(pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.mac,
				       vMac_ret,
				       6);
			}
			else
			{
				memset(pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.mac, 0, 6);
			}
			if (vIP_ret_packed != NULL)
			{
				if (vIP_ret_packed_size == 4)
				{
					// IPv4
#if defined(NDEBUG)
					inet_ntop(AF_INET, vIP_ret_packed, str, INET_ADDRSTRLEN);
					log_info(PythonDataHandlerLogLevel,
					          "Policy_Request: vIPv4 %s", str);
#endif
					pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.family = AF_INET;
					memcpy(&pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.ip4,
					       vIP_ret_packed,
					       vIP_ret_packed_size);
					pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.ip4 =
						ntohl(pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.ip4);
				}
				else
				{
					// IPv6
#if defined(NDEBUG)
					inet_ntop(AF_INET6, vIP_ret_packed, str, INET6_ADDRSTRLEN);
					log_info(PythonDataHandlerLogLevel,
					          "Policy_Request: vIPv6 %s", str);
#endif
					pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.family = AF_INET6;
					memcpy(pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr.ip6,
					       vIP_ret_packed,
					       vIP_ret_packed_size);
				}
			}
			// Determine if a bigger reply message size is needed
			num_tunnelsv4 = PyList_Size(pyList_ipv4);
			num_tunnelsv6 = PyList_Size(pyList_ipv6);
			log_debug(PythonDataHandlerLogLevel,
			          "Endpoint Request: Tunnels IPv4 %d, IPv6 %d",
			          num_tunnelsv4, num_tunnelsv6);
			dps_msg_reply_size = dps_offsetof(dps_client_data_t,
			                                  policy_reply.dst_endpoint_loc_reply.tunnel_info.tunnel_list[num_tunnelsv4 + num_tunnelsv6]);
			if (dps_msg_reply_size > sizeof(dps_client_data_t))
			{
				pdps_msg_reply = (dps_client_data_t *)malloc(dps_msg_reply_size);
				if (pdps_msg_reply == NULL)
				{
					// Set back to old values
					pdps_msg_reply = &dps_msg_reply;
					if (PyList_Size(pyList_ipv4) > 0)
					{
						num_tunnelsv4 = 1;
						num_tunnelsv6 = 0;
					}
					else
					{
						num_tunnelsv4 = 0;
						num_tunnelsv6 = 1;
					}
				}
				else
				{
					freplymsgallocated = 1;
					memcpy(pdps_msg_reply, &dps_msg_reply, sizeof(dps_client_data_t));
				}
			}
			j = 0;
			for (i = 0; i < num_tunnelsv4; i++)
			{
				int ipv4;
				dps_tunnel_endpoint_t *tunnel;

				pypIP = PyList_GetItem(pyList_ipv4, i);
				if (!PyArg_Parse(pypIP, "I", &ipv4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Policy_Request: Invalid IPv4 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Policy_Request: [%d] pIPv4 %s", i, str);
#endif
				tunnel = &pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.tunnel_info.tunnel_list[j];
				tunnel->family = AF_INET;
				tunnel->ip4 = ntohl(ipv4);
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->vnid = dvg;
				j++;
			}
			for (i = 0; i < num_tunnelsv6; i++)
			{
				char *ipv6;
				int ipv6_size;
				dps_tunnel_endpoint_t *tunnel;

				pypIP = PyList_GetItem(pyList_ipv6, i);
				if (!PyArg_Parse(pypIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Policy_Request: Invalid IPv6 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				          "Policy_Request: [%d] pIPv6 %s", i, str);
#endif
				tunnel = &pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.tunnel_info.tunnel_list[j];
				tunnel->family = AF_INET6;
				memcpy(tunnel->ip6, ipv6, ipv6_size);
				tunnel->tunnel_type = TUNNEL_TYPE_VXLAN;
				tunnel->vnid = dvg;
				j++;
			}
			pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.tunnel_info.num_of_tunnels = j;
			pdps_msg_reply->policy_reply.dps_policy_info.version = policy_version;
			pdps_msg_reply->policy_reply.dps_policy_info.ttl = ttl;
			pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.policy_type = (uint8_t)type;
			if (type == DPS_POLICY_TYPE_CONNECTIVITY)
			{
				dps_object_policy_action_t *dps_policy_action = (dps_object_policy_action_t *)action_ret;
				if ((dps_policy_action != NULL) && (dps_policy_action->connectivity == DPS_CONNECTIVITY_ALLOW))
				{
					pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_deny_rules = 0;
					pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_permit_rules = 1;
				}
				else
				{
					pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_deny_rules = 1;
					pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.num_permit_rules = 0;
				}
				pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].dvnid = dvg;
				pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].svnid = (uint16_t)policy_req_msg->src_endpoint.vnid;
#if defined(NDEBUG)
				if ((dps_policy_action != NULL) && (dps_policy_action->connectivity == DPS_CONNECTIVITY_ALLOW))
				{
					log_info(PythonDataHandlerLogLevel,"Policy Action Connectivity: PERMIT between SRC %d, DST %d",
					         pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].svnid,
					         pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].dvnid);
				}
				else
				{
					log_info(PythonDataHandlerLogLevel,"Policy Action Connectivity: DENY between SRC %d, DST %d",
					         pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].svnid,
					         pdps_msg_reply->policy_reply.dps_policy_info.dps_policy.vnid_policy.src_dst_vnid[0].dvnid);
				}
#endif
			}
			else if (type == DPS_POLICY_TYPE_SOURCE_ROUTING)
			{
				log_warn(PythonDataHandlerLogLevel,
				         "DPS_POLICY_TYPE_SOURCE_ROUTING: not supported");
			}
		}
		else
		{
			// Failure case: Copy the destination from original message
			memset(&pdps_msg_reply->policy_reply, 0, sizeof(dps_policy_reply_t));
			memcpy(&pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.vm_ip_addr,
			       &policy_req_msg->dst_endpoint.vm_ip_addr,
			       sizeof(ip_addr_t));
			pdps_msg_reply->policy_reply.dst_endpoint_loc_reply.tunnel_info.num_of_tunnels = 0;
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		// Release the PYTHON Global Interpreter Lock
		PyGILState_Release(gstate);
	}while(0);

	dps_msg_send_inline(&dps_msg_reply);
	if (freplymsgallocated)
	{
		free(pdps_msg_reply);
		pdps_msg_reply = &dps_msg_reply;
	}

	log_debug(PythonDataHandlerLogLevel, "Exit: ret_status %d", return_status);

	return return_status;
}

/*
 ******************************************************************************
 * dps_msg_implicity_gateway_request --                                   *//**
 *
 * \brief This routine handles Implicit Gateway Requests from DPS Clients
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Implicit
 *                Gateway request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_implicit_gateway_request(uint32_t domain,
                                                          dps_client_data_t *dps_msg)
{
	dps_return_status return_status = DPS_SUCCESS;
	dps_client_hdr_t *hdr = &((dps_client_data_t *)send_buff)->hdr;
	dps_internal_gw_t *int_gw = &((dps_client_data_t *)send_buff)->internal_gw_list;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	uint32_t status;
	PyObject *pyList_ipv4, *pyList_ipv6;
	Py_ssize_t i, j;
	PyObject *pyIP;
	int ipv4, ipv6_size;
	char *ipv6;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);

#if defined(NDEBUG)
	if (dps_msg->internal_gw_req.dps_client_addr.family == AF_INET)
	{
		char str[INET_ADDRSTRLEN];
		int dps_client_ip4 = htonl(dps_msg->internal_gw_req.dps_client_addr.ip4);
		inet_ntop(AF_INET, &dps_client_ip4, str, INET_ADDRSTRLEN);
		// IPv4
		log_debug(PythonDataHandlerLogLevel,
		          "Gateway Request: VNID %d, DPS Client IPv4 %s, Port %d",
		          dps_msg->hdr.vnid, str,
		          dps_msg->internal_gw_req.dps_client_addr.port);
	}
	else
	{
		char str[INET6_ADDRSTRLEN];
		// IPv6
		inet_ntop(AF_INET6, dps_msg->internal_gw_req.dps_client_addr.ip6, str, INET6_ADDRSTRLEN);
		log_debug(PythonDataHandlerLogLevel,
		          "Gateway Request: VNID %d, DPS Client IPv6 %s, Port %d",
		          dps_msg->hdr.vnid, str,
		          dps_msg->internal_gw_req.dps_client_addr.port);
	}
#endif

	// Create the reply message header
	memcpy(hdr, &dps_msg->hdr, sizeof(dps_client_hdr_t));
	// Reply to the DPS Client
	memcpy(&hdr->reply_addr,
	       &dps_msg->internal_gw_req.dps_client_addr,
	       sizeof(dps_client_hdr_t));
	hdr->type = DPS_INTERNAL_GW_REPLY;
	hdr->client_id = DPS_POLICY_SERVER_ID;
	hdr->transaction_type = DPS_TRANSACTION_NORMAL;
	hdr->resp_status = DPS_NO_MEMORY;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("(II)", domain, hdr->vnid);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		strret = PyEval_CallObject(Client_Protocol_Interface.Implicit_Gateway_List, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Implicit_Gateway_List returns NULL");
			break;
		}

		//@return: (status, ListIPv4, ListIPv6)
		//@rtype: Integer, List, List
		PyArg_ParseTuple(strret, "IOO", &status, &pyList_ipv4, &pyList_ipv6);

		// Set the reply status
		hdr->resp_status = status;

		if (status == DPS_NO_ERR)
		{
			// Copy the implicit gateways
			int_gw->num_v4_gw = 0;
			int_gw->num_v6_gw = 0;

			j = 0;
			for (i = 0; i < PyList_Size(pyList_ipv4); i++)
			{
#if defined(NDEBUG)
				char str[INET_ADDRSTRLEN];
#endif
				pyIP = PyList_GetItem(pyList_ipv4, i);
				if (!PyArg_Parse(pyIP, "I", &ipv4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv4 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
					 "Implicit Gateway %s", str);
#endif
				int_gw->num_v4_gw++;
				int_gw->gw_list[j++] = ntohl(ipv4);
			}
			for (i = 0, j = 0; i < PyList_Size(pyList_ipv6); i++, j++)
			{
#if defined(NDEBUG)
				char str[INET6_ADDRSTRLEN];
#endif
				pyIP = PyList_GetItem(pyList_ipv6, i);
				if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv6 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Implicit Gateway %s", str);
#endif
				memcpy((uint8_t *)(&int_gw->gw_list[j]), ipv6, ipv6_size);
				int_gw->num_v6_gw++;
				j += 4;
			}
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	((dps_client_data_t *)send_buff)->context = NULL;
	dps_msg_send_inline((dps_client_data_t *)send_buff);

	log_debug(PythonDataHandlerLogLevel, "Exit: ret_status %d", return_status);

	return return_status;
}

/*
 ******************************************************************************
 * dps_msg_implicity_gateway_request --                                   *//**
 *
 * \brief This routine handles Implicit Gateway Requests from DPS Clients
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Implicit
 *                Gateway request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_gateway_request(uint32_t domain,
                                                 dps_client_data_t *dps_msg)
{
	dps_return_status return_status = DPS_SUCCESS;
	dps_client_hdr_t *hdr = &((dps_client_data_t *)send_buff)->hdr;
	dps_tunnel_list_t *gw_data = &((dps_client_data_t *)send_buff)->tunnel_info;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	uint32_t status;
	PyObject *pyList_ipv4, *pyList_ipv6;
	Py_ssize_t i, j;
	PyObject *pyIP;
	int gwy_vnid, ipv4, ipv6_size;
	char *ipv6;
	data_handler_gateway_types gateway_type;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);

	if (dps_msg->gen_msg_req.dps_client_addr.family == AF_INET)
	{
		int dps_client_ip4 = htonl(dps_msg->gen_msg_req.dps_client_addr.ip4);
		inet_ntop(AF_INET, &dps_client_ip4, str, INET_ADDRSTRLEN);
		// IPv4
		log_info(PythonDataHandlerLogLevel,
		          "Gateway Request: VNID %d, DPS Client IPv4 %s, Port %d",
		          dps_msg->hdr.vnid, str,
		          dps_msg->gen_msg_req.dps_client_addr.port);
	}
	else
	{
		// IPv6
		inet_ntop(AF_INET6, dps_msg->gen_msg_req.dps_client_addr.ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel,
		          "Gateway Request: VNID %d, DPS Client IPv6 %s, Port %d",
		          dps_msg->hdr.vnid, str,
		          dps_msg->gen_msg_req.dps_client_addr.port);
	}

	((dps_client_data_t *)send_buff)->context = NULL;
	// Create the reply message header
	memcpy(hdr, &dps_msg->hdr, sizeof(dps_client_hdr_t));
	// Reply to the DPS Client
	memcpy(&hdr->reply_addr,
	       &dps_msg->gen_msg_req.dps_client_addr,
	       sizeof(dps_client_hdr_t));
	if (dps_msg->hdr.type == DPS_EXTERNAL_GW_LIST_REQ)
	{
		hdr->type = DPS_EXTERNAL_GW_LIST_REPLY;
		//hdr->type = DPS_UNSOLICITED_EXTERNAL_GW_LIST;
		gateway_type = GATEWAY_TYPE_EXTERNAL;
	}
	else
	{
		hdr->type = DPS_VLAN_GW_LIST_REPLY;
		//hdr->type = DPS_UNSOLICITED_VLAN_GW_LIST;
		gateway_type = GATEWAY_TYPE_VLAN;
	}
	hdr->client_id = DPS_POLICY_SERVER_ID;
	hdr->transaction_type = DPS_TRANSACTION_NORMAL;
	hdr->resp_status = DPS_NO_MEMORY;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("(III)", domain, hdr->vnid, gateway_type);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		strret = PyEval_CallObject(Client_Protocol_Interface.Gateway_List, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Gateway_List returns NULL");
			break;
		}

		//@return: (status, ListIPv4, ListIPv6)
		//@rtype: Integer, List, List
		PyArg_ParseTuple(strret, "IOO", &status, &pyList_ipv4, &pyList_ipv6);

		// Set the reply status
		hdr->resp_status = status;
		if (status != DPS_NO_ERR)
		{
			Py_DECREF(strret);
			break;
		}

		gw_data->num_of_tunnels = 0;
		j = 0;
		for (i = 0; i < PyList_Size(pyList_ipv4); i++)
		{
			// Check if the current policy will cross the buffer size
			if (dps_offsetof(dps_client_data_t,
			                 tunnel_info.tunnel_list[j+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of Gateways %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv4, i);
			if (!PyArg_ParseTuple(pyIP, "II", &gwy_vnid, &ipv4))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv4 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "IPv4 [%d] VNID %d, Gateway %s", i, gwy_vnid, str);
			gw_data->tunnel_list[j].tunnel_type = TUNNEL_TYPE_VXLAN;
			gw_data->tunnel_list[j].port = 0;
			gw_data->tunnel_list[j].family = AF_INET;
			gw_data->tunnel_list[j].ip4 = ntohl(ipv4);
			gw_data->tunnel_list[j].vnid = gwy_vnid;
			j++;
			gw_data->num_of_tunnels++;
		}
		for (i = 0; i < PyList_Size(pyList_ipv6); i++)
		{
			if (dps_offsetof(dps_client_data_t,
			                 tunnel_info.tunnel_list[j+1]) > SEND_BUFF_SIZE)
			{
				log_alert(PythonDataHandlerLogLevel,
				          "ALERT!!! Number of Gateways %d exceeds Send Buffer %d max size",
				          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
				          SEND_BUFF_SIZE
				         );
				break;
			}
			pyIP = PyList_GetItem(pyList_ipv6, i);
			if (!PyArg_ParseTuple(pyIP, "Iz#", &gwy_vnid, &ipv6, &ipv6_size))
			{
				log_warn(PythonDataHandlerLogLevel,
				         "Invalid IPv6 in element %d", i);
				continue;
			}
			inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			          "IPv6 [%d], VNID %d, Gateway %s", i, gwy_vnid, str);
			gw_data->tunnel_list[j].tunnel_type = TUNNEL_TYPE_VXLAN;
			gw_data->tunnel_list[j].port = 0;
			gw_data->tunnel_list[j].family = AF_INET6;
			memcpy((uint8_t *)(&gw_data->tunnel_list[j]), ipv6, ipv6_size);
			gw_data->tunnel_list[j].vnid = gwy_vnid;
			gw_data->num_of_tunnels++;
			j++;
		}
		log_debug(PythonDataHandlerLogLevel, "Num Tunnels %d", gw_data->num_of_tunnels);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	((dps_client_data_t *)send_buff)->context = NULL;
	dps_msg_send_inline((dps_client_data_t *)send_buff);

	log_debug(PythonDataHandlerLogLevel, "Exit: ret_status %d", return_status);

	return return_status;
}

/*
 ******************************************************************************
 * dps_msg_broadcast_list_request --                                      *//**
 *
 * \brief This routine handles Implicit Gateway Requests from DPS Clients
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Implicit
 *                Gateway request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_broadcast_list_request(uint32_t domain,
                                                        dps_client_data_t *dps_msg)
{
	dps_return_status return_status = DPS_SUCCESS;
	dps_client_hdr_t *hdr = &((dps_client_data_t *)send_buff)->hdr;
	dps_pkd_tunnel_list_t *switch_list = &((dps_client_data_t *)send_buff)->dove_switch_list;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	uint32_t status;
	PyObject *pyList_ipv4, *pyList_ipv6;
	Py_ssize_t i, j;
	PyObject *pyIP;
	int ipv4, ipv6_size;
	char *ipv6;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain %d", domain);

#if defined(NDEBUG)
	if (dps_msg->gen_msg_req.dps_client_addr.family == AF_INET)
	{
		char str[INET_ADDRSTRLEN];
		int dps_client_ip4 = htonl(dps_msg->gen_msg_req.dps_client_addr.ip4);
		inet_ntop(AF_INET, &dps_client_ip4, str, INET_ADDRSTRLEN);
		// IPv4
		log_debug(PythonDataHandlerLogLevel,
		          "Broadcast Request: VNID %d, DPS Client IPv4 %s, Port %d",
		          dps_msg->hdr.vnid, str, dps_msg->gen_msg_req.dps_client_addr.port);
	}
	else
	{
		char str[INET6_ADDRSTRLEN];
		// IPv6
		inet_ntop(AF_INET6, dps_msg->gen_msg_req.dps_client_addr.ip6, str, INET6_ADDRSTRLEN);
		log_debug(PythonDataHandlerLogLevel,
		          "Broadcast Request: VNID %d, DPS Client IPv6 %s, Port %d",
		          dps_msg->hdr.vnid, str, dps_msg->gen_msg_req.dps_client_addr.port);
	}
#endif

	// Copy the header
	memcpy(hdr, &dps_msg->hdr, sizeof(dps_client_hdr_t));
	// Reply to the DPS Client
	memcpy(&hdr->reply_addr,
	       &dps_msg->gen_msg_req.dps_client_addr,
	       sizeof(dps_client_hdr_t));
	hdr->type = DPS_BCAST_LIST_REPLY;
	hdr->client_id = DPS_POLICY_SERVER_ID;
	hdr->transaction_type = DPS_TRANSACTION_NORMAL;
	hdr->resp_status = DPS_NO_MEMORY;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("(II)", domain, dps_msg->hdr.vnid);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		strret = PyEval_CallObject(Client_Protocol_Interface.Broadcast_List, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Broadcast_List returns NULL");
			break;
		}

		//@return: (status, ListIPv4, ListIPv6)
		//@rtype: Integer, List, List
		PyArg_ParseTuple(strret, "IOO", &status, &pyList_ipv4, &pyList_ipv6);
		// Set the return status
		hdr->resp_status = status;

		if (status == DPS_NO_ERR)
		{
			// Copy the DOVE Switches
			switch_list->num_v4_tunnels = 0;
			switch_list->num_v6_tunnels = 0;

			j = 0;
			for (i = 0; i < PyList_Size(pyList_ipv4); i++)
			{
	#if defined(NDEBUG)
				char str[INET_ADDRSTRLEN];
	#endif
				if (dps_offsetof(dps_client_data_t,
				                 dove_switch_list.tunnel_list[j+1]) > SEND_BUFF_SIZE)
				{
					log_alert(PythonDataHandlerLogLevel,
					          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
					          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
					          SEND_BUFF_SIZE
					          );
					break;
				}
				pyIP = PyList_GetItem(pyList_ipv4, i);
				if (!PyArg_Parse(pyIP, "I", &ipv4))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv4 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Broadcast: DOVE Switch %s", str);
#endif
				switch_list->tunnel_list[j++] = ntohl(ipv4);
				switch_list->num_v4_tunnels++;
			}
			for (i = 0, j = 0; i < PyList_Size(pyList_ipv6); i++, j++)
			{
#if defined(NDEBUG)
				char str[INET6_ADDRSTRLEN];
#endif
				if (dps_offsetof(dps_client_data_t,
						 dove_switch_list.tunnel_list[j+4]) > SEND_BUFF_SIZE)
				{
					log_alert(PythonDataHandlerLogLevel,
					          "ALERT!!! Number of DOVE Switches %d exceeds Send Buffer %d max size",
					          PyList_Size(pyList_ipv4) + PyList_Size(pyList_ipv6),
					          SEND_BUFF_SIZE
					          );
					break;
				}
				pyIP = PyList_GetItem(pyList_ipv6, i);
				if (!PyArg_Parse(pyIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonDataHandlerLogLevel,
					         "Invalid IPv6 in element %d", i);
					continue;
				}
#if defined(NDEBUG)
				inet_ntop(AF_INET6, ipv6, str, INET6_ADDRSTRLEN);
				log_info(PythonDataHandlerLogLevel,
				         "Broadcast: DOVE Switch %s", str);
#endif
				memcpy((uint8_t *)(&switch_list->tunnel_list[j]), ipv6, ipv6_size);
				switch_list->num_v6_tunnels++;
				j += 4;
			}
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	((dps_client_data_t *)send_buff)->context = NULL;
	dps_msg_send_inline((dps_client_data_t *)send_buff);

	log_debug(PythonDataHandlerLogLevel, "Exit: ret_status %d", return_status);

	return return_status;
}

/*
 ******************************************************************************
 * dps_msg_vm_migration_event --                                          *//**
 *
 * \brief This routine handles Migration Event. This message is "probably" sent
 *        by the (old) DOVE Switch which hosted the VM previously. This (old)
 *        DOVE Switch is still receiving traffic for the now migrated VM. The
 *        DOVE Switch determines the remote DOVE Switch sending the traffic
 *        and requests the DPS to send it the
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Implicit
 *                Gateway request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_vm_migration_event(uint32_t domain,
                                                    dps_client_data_t *dps_msg)
{
	dps_return_status return_status = DPS_SUCCESS;
	dps_client_data_t dps_msg_policy_request_data;
	PyObject *strret_dps = NULL;
	PyObject *strret_vm = NULL;
	PyObject *strargs_dps = NULL;
	PyObject *strargs_vm = NULL;
	PyGILState_STATE gstate;
	uint32_t status_dps = DPS_NO_MEMORY;
	uint32_t status_vm = DPS_NO_MEMORY;
	char *dps_client_packed = NULL;
	int dps_client_packed_size, dps_client_port, src_vm_dvg;
	ip_addr_t src_tunnel_addr;
	ip_addr_t src_vm_addr;
	ip_addr_t dst_vm_addr;
	int fInvokeMessage = 0;

	log_debug(PythonDataHandlerLogLevel, "Enter Domain %d", domain);

	// Copy to temporary location
	memcpy(src_tunnel_addr.ip6, &dps_msg->vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].ip6, 16);
	src_tunnel_addr.family = dps_msg->vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].family;
	memcpy(&src_vm_addr, &dps_msg->vm_migration_event.src_vm_loc.vm_ip_addr, sizeof(ip_addr_t));
	memcpy(&dst_vm_addr, &dps_msg->vm_migration_event.migrated_vm_info.vm_ip_addr, sizeof(ip_addr_t));
	// Flip all relevant addresses to network byte order
	if (src_tunnel_addr.family == AF_INET)
	{
		src_tunnel_addr.ip4 = htonl(src_tunnel_addr.ip4);
	}
	if (src_vm_addr.family == AF_INET)
	{
		src_vm_addr.ip4 = htonl(src_vm_addr.ip4);
	}
	if (dst_vm_addr.family == AF_INET)
	{
		dst_vm_addr.ip4 = htonl(dst_vm_addr.ip4);
	}

	{
		char str_vm_migrated[INET6_ADDRSTRLEN];
		char str_vm_source[INET6_ADDRSTRLEN];
		char str_pip_source[INET6_ADDRSTRLEN];
		inet_ntop(src_tunnel_addr.family,
		          src_tunnel_addr.ip6,
		          str_pip_source,
		          INET6_ADDRSTRLEN);
		inet_ntop(src_vm_addr.family,
		          src_vm_addr.ip6,
		          str_vm_source,
		          INET6_ADDRSTRLEN);
		inet_ntop(dst_vm_addr.family,
		          dst_vm_addr.ip6,
		          str_vm_migrated,
		          INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel,
		         "VM Migrate Hint: Domain %d, VNID %d, Source Tunnel %s, Source VM %s, Migrated VM %s",
		         domain, dps_msg->hdr.vnid, str_pip_source, str_vm_source, str_vm_migrated);
	}

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs_dps = Py_BuildValue("(IIz#)",
		                            domain,
		                            src_tunnel_addr.family,
		                            src_tunnel_addr.ip6,
		                            16);
		if (strargs_dps == NULL)
		{
			break;
		}
		strargs_vm = Py_BuildValue("(IIz#)",
		                           domain,
		                           src_vm_addr.family,
		                           src_vm_addr.ip6,
		                           16);
		if (strargs_vm == NULL)
		{
			Py_DECREF(strargs_dps);
			break;
		}

		// Determine the DPS Client Location for the Source Tunnel
		// def PIP_Get_DPS_Client(self, domain_id, pIP_type, pIP_packed):
		strret_dps = PyEval_CallObject(Client_Protocol_Interface.pIP_Get_DPS_Client,
		                               strargs_dps);
		Py_DECREF(strargs_dps);

		// Determine the Source VMs DVG
		// def vIP_Get_DVG(self, domain_id, vIP_type, vIP_packed):
		strret_vm = PyEval_CallObject(Client_Protocol_Interface.vIP_Get_DVG,
		                              strargs_vm);
		Py_DECREF(strargs_vm);

		if ((strret_dps == NULL) || (strret_vm == NULL))
		{
			if (strret_dps != NULL)
			{
				Py_DECREF(strret_dps);
			}
			else
			{
				log_warn(PythonDataHandlerLogLevel,
				         "PyEval_CallObject pIP_Get_DPS_Client returns NULL");
			}
			if (strret_vm != NULL)
			{
				Py_DECREF(strret_vm);
			}
			else
			{
				log_warn(PythonDataHandlerLogLevel,
				         "PyEval_CallObject vIP_Get_DVG returns NULL");
			}
			break;
		}
		//@return: status, dps_client_ip_packed, dps_client_port
		//@rtype: Integer, ByteArray, Integer
		PyArg_ParseTuple(strret_dps, "Iz#I",
		                 &status_dps, &dps_client_packed,
		                 &dps_client_packed_size, &dps_client_port);
		log_debug(PythonDataHandlerLogLevel, "status_dps %d, dps_client_packed_size %d",
		          status_dps, dps_client_packed_size);
		//@return: status, DVG
		//@rtype: Integer, Integer
		PyArg_ParseTuple(strret_vm, "II", &status_vm, &src_vm_dvg);
		log_debug(PythonDataHandlerLogLevel, "status_vm %d, src_vm_dvg %d",
		          status_vm, src_vm_dvg);

		// Create a Policy Request Header as if it originated from the DPS Client
		// Copy the DPS Client Address if valid
		if (status_dps == DPS_NO_ERR)
		{
			if (dps_client_packed_size == 4)
			{
				// IPv4
				dps_msg_policy_request_data.hdr.reply_addr.family = AF_INET;
				memcpy(dps_msg_policy_request_data.hdr.reply_addr.ip6, dps_client_packed, 4);
				dps_msg_policy_request_data.hdr.reply_addr.ip4 = ntohl(dps_msg_policy_request_data.hdr.reply_addr.ip4);
			}
			else if (dps_client_packed_size == 16)
			{
				// IPv6
				dps_msg_policy_request_data.hdr.reply_addr.family = AF_INET6;
				memcpy(dps_msg_policy_request_data.hdr.reply_addr.ip6, dps_client_packed, 16);
			}
			dps_msg_policy_request_data.hdr.reply_addr.port = dps_client_port;
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret_dps);
		Py_DECREF(strret_vm);

		if ((status_dps != DPS_NO_ERR) || (status_vm != DPS_NO_ERR))
		{
			break;
		}

		// Create a Policy Request Header as if it originated from the Source DPS Client
		dps_msg_policy_request_data.context = NULL;
		dps_msg_policy_request_data.hdr.type = DPS_POLICY_REQ;
		dps_msg_policy_request_data.hdr.vnid = src_vm_dvg;
		dps_msg_policy_request_data.hdr.client_id = DPS_SWITCH_AGENT_ID;
		dps_msg_policy_request_data.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
		dps_msg_policy_request_data.hdr.query_id = 0; //Must be zero

		//Policy Request Body
		// 1. DPS Client Location
		memcpy(&dps_msg_policy_request_data.policy_req.dps_client_addr,
		       &dps_msg_policy_request_data.hdr.reply_addr,
		       sizeof(ip_addr_t));
		// 2. Migrated (Destination) Endpoint Information
		memcpy(&dps_msg_policy_request_data.policy_req.dst_endpoint.vm_ip_addr,
		       &dst_vm_addr, sizeof(ip_addr_t));
		if (dps_msg_policy_request_data.policy_req.dst_endpoint.vm_ip_addr.family == AF_INET)
		{
			dps_msg_policy_request_data.policy_req.dst_endpoint.vm_ip_addr.ip4 =
				ntohl(dps_msg_policy_request_data.policy_req.dst_endpoint.vm_ip_addr.ip4);
		}
		memcpy(dps_msg_policy_request_data.policy_req.dst_endpoint.mac,
		       dps_msg->vm_migration_event.migrated_vm_info.mac, 6);
		// 3. Source Endpoint Information
		dps_msg_policy_request_data.policy_req.src_endpoint.vnid = src_vm_dvg;
		memcpy(dps_msg_policy_request_data.policy_req.src_endpoint.mac,
		       dps_msg->vm_migration_event.src_vm_loc.mac, 6);
		memcpy(&dps_msg_policy_request_data.policy_req.src_endpoint.vm_ip_addr,
		       &src_vm_addr, sizeof(ip_addr_t));
		if(dps_msg_policy_request_data.policy_req.src_endpoint.vm_ip_addr.family == AF_INET)
		{
			dps_msg_policy_request_data.policy_req.src_endpoint.vm_ip_addr.ip4 =
				ntohl(dps_msg_policy_request_data.policy_req.src_endpoint.vm_ip_addr.ip4);
		}

		fInvokeMessage = 1;

	} while(0);

	PyGILState_Release(gstate);

	if (fInvokeMessage)
	{
		// Invoke the Endpoint Location Request
		dps_msg_policy_request(domain, &dps_msg_policy_request_data);
	}

	log_debug(PythonDataHandlerLogLevel, "Exit: ret_status %d", return_status);

	return return_status;
}

/*
 ******************************************************************************
 * dps_msg_general_ack --                                                 *//**
 *
 * \brief This routine handles Migration Event. This message is "probably" sent
 *        by the (old) DOVE Switch which hosted the VM previously. This (old)
 *        DOVE Switch is still receiving traffic for the now migrated VM. The
 *        DOVE Switch determines the remote DOVE Switch sending the traffic
 *        and requests the DPS to send it the
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for Implicit
 *                Gateway request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_general_ack(uint32_t domain,
                                             dps_client_data_t *dps_msg)
{
	unsolicited_msg_context_t *pmsg_context;
	endpoint_resolution_context_t *resolution_context;
	PyObject *strargs = NULL;
	PyObject *pyFunction = NULL;
	PyGILState_STATE gstate;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		pmsg_context = (unsolicited_msg_context_t *)dps_msg->context;
		resolution_context = (endpoint_resolution_context_t *)dps_msg->context;
		if (pmsg_context == NULL)
		{
			log_debug(PythonDataHandlerLogLevel, "NULL context");
			break;
		}

		log_debug(PythonDataHandlerLogLevel, "Context %p", pmsg_context);

		inet_ntop(pmsg_context->dps_client_location.family,
		          pmsg_context->dps_client_location.ip6, str, INET6_ADDRSTRLEN);

		log_info(PythonDataHandlerLogLevel,
		         "Received [GEN_ACK] VNID %d, Type %d, QueryId %d from %s, Status %d",
		         pmsg_context->vnid_id, pmsg_context->original_msg_type,
		         dps_msg->hdr.query_id, str, dps_msg->hdr.resp_status);

		switch(pmsg_context->original_msg_type)
		{
			case DPS_UNSOLICITED_VNID_POLICY_LIST:
			case DPS_UNSOLICITED_BCAST_LIST_REPLY:
			case DPS_UNSOLICITED_EXTERNAL_GW_LIST:
			case DPS_UNSOLICITED_VLAN_GW_LIST:
			case DPS_CTRL_PLANE_HB:
				//def Handle_Unsolicited_Message_Reply(self, message_type,
				//                                     response_error, vnid_id,
				//                                     dps_client_IP_type,
				//                                     dps_client_ip_packed):
				strargs = Py_BuildValue("(IIIHz#)",
				                        pmsg_context->original_msg_type,
				                        dps_msg->hdr.resp_status,
				                        pmsg_context->vnid_id,
				                        pmsg_context->dps_client_location.family,
				                        pmsg_context->dps_client_location.ip6, 16);
				pyFunction = Client_Protocol_Interface.Handle_Unsolicited_Message_Reply;
				break;
			case DPS_ADDR_RESOLVE:
			case DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
				//def Handle_Resolution_Reply(self, message_type,
				//                            response_error, vnid_id,
				//                            dps_client_IP_type,
				//                            dps_client_ip_packed,
				//                            vIP_type,
				//                            vIP_packed):
				strargs = Py_BuildValue("(IIIHz#Hz#)",
				                        resolution_context->original_msg_type,
				                        dps_msg->hdr.resp_status,
				                        resolution_context->vnid_id,
				                        resolution_context->dps_client_location.family,
				                        resolution_context->dps_client_location.ip6, 16,
				                        resolution_context->vIP.family,
				                        resolution_context->vIP.ip6, 16
				                        );
				pyFunction = Client_Protocol_Interface.Handle_Resolution_Reply;
				break;
			default:
				strargs = NULL;
				break;
		}
		if (strargs == NULL)
		{
			log_debug(PythonDataHandlerLogLevel, "Freeing Context %p", pmsg_context);
			free(pmsg_context);
			outstanding_unsolicited_msg_count--;
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the appropriate function call
		PyEval_CallObject(pyFunction, strargs);
		Py_DECREF(strargs);
		log_debug(PythonDataHandlerLogLevel, "Freeing Context %p", pmsg_context);
		free(pmsg_context);
		outstanding_unsolicited_msg_count--;
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_debug(PythonDataHandlerLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_msg_tunnel_register --                                             *//**
 *
 * \brief This routine handles registration of tunnel endpoints on a single
 *        VNID
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Protocol Message containing the list of tunnel
 *                endpoints
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_tunnel_register(uint32_t domain,
                                                 dps_client_data_t *dps_msg)
{

	dps_client_data_t *dps_msg_reply = NULL;
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;
	dps_tunnel_reg_dereg_t *tunnel_reg;
	ip_addr_t dps_client_address;
	dps_tunnel_endpoint_t *tunnel_ip;
	size_t dps_msg_size;
	PyObject *pip_list = NULL;
	PyObject *pip_tuple = NULL;
	PyObject *PyFunction = NULL;
	uint32_t i, ret_code;
	dps_return_status ret_val = DPS_SUCCESS;
	uint32_t query_id = 0;
	int fReplyToRequest = 0;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonDataHandlerLogLevel, "Enter Domain Id %d", domain);
	do
	{
		tunnel_reg = &dps_msg->tunnel_reg_dereg;
		dps_msg_reply = (dps_client_data_t *)malloc(sizeof(dps_client_data_t));
		if (dps_msg_reply == NULL)
		{
			break;
		}
		log_debug(PythonDataHandlerLogLevel, "Allocated Message %p", dps_msg_reply);
		dps_form_tunnel_register_reply(dps_msg_reply, dps_msg, DPS_NO_ERR);
		//Set the Reply Status
		fReplyToRequest = 1;

		//Check the number of Tunnels
		if (tunnel_reg->tunnel_info.num_of_tunnels <= 0)
		{
			log_info(PythonDataHandlerLogLevel,
			         "No Tunnel IP Addresses Provided!");
			ret_code = DPS_INVALID_SRC_IP;
			break;
		}

		// Copy the Addresses so that they can be converted to Network Order
		memcpy(&dps_client_address, &tunnel_reg->dps_client_addr, sizeof(ip_addr_t));
		if (dps_client_address.family == AF_INET)
		{
			dps_client_address.ip4 = htonl(dps_client_address.ip4);
		}
		if (dps_msg->hdr.type == DPS_TUNNEL_REGISTER)
		{
			log_info(PythonDataHandlerLogLevel, "Tunnel Register");
		}
		else
		{
			log_info(PythonDataHandlerLogLevel, "Tunnel Unregister");
		}
		inet_ntop(dps_client_address.family, dps_client_address.ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel, "DPS Client %s", str);
		log_info(PythonDataHandlerLogLevel, "VNID %d", dps_msg->hdr.vnid);
		log_info(PythonDataHandlerLogLevel, "Query ID %d", dps_msg->hdr.query_id);
		log_info(PythonDataHandlerLogLevel, "Sender Type %d", dps_msg->hdr.client_id);
		log_info(PythonDataHandlerLogLevel, "Transaction Type %d", dps_msg->hdr.transaction_type);
		log_info(PythonDataHandlerLogLevel, "Number of Tunnels %d", tunnel_reg->tunnel_info.num_of_tunnels);
		// Check if there are other nodes this request should be forwarded to.
		dps_msg_size = dps_offsetof(dps_client_data_t,
		                            tunnel_reg_dereg.tunnel_info.tunnel_list[tunnel_reg->tunnel_info.num_of_tunnels]);
		ret_code = dps_update_replicate(domain,
		                                dps_msg,
		                                dps_msg_reply,
		                                dps_msg_size,
		                                &query_id);
		if (ret_code != DPS_NO_ERR)
		{
			break;
		}
		// Let the timeout take care of reply in case of failure
		fReplyToRequest = 0;
		if (query_id == 0)
		{
			break;
		}

		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		// Form the List of Physical IP Addresses
		pip_list = PyList_New(tunnel_reg->tunnel_info.num_of_tunnels);
		if (pip_list == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel, "Cannot allocate PyList_New");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}
		for (i = 0; i < tunnel_reg->tunnel_info.num_of_tunnels; i++)
		{
			tunnel_ip = &tunnel_reg->tunnel_info.tunnel_list[i];
			if (tunnel_ip->family == AF_INET)
			{
				uint32_t ip4 = htonl(tunnel_ip->ip4);
				pip_tuple = Py_BuildValue("(II)", AF_INET, ip4);
				inet_ntop(AF_INET, &ip4, str, INET6_ADDRSTRLEN);
			}
			else
			{
				pip_tuple = Py_BuildValue("(Iz#)", AF_INET6, tunnel_ip->ip6, 16);
				inet_ntop(AF_INET6, tunnel_ip->ip6, str, INET6_ADDRSTRLEN);
			}
			log_info(PythonDataHandlerLogLevel,"[%d] PIP %s", i, str);
			if (pip_tuple == NULL)
			{
				log_warn(PythonDataHandlerLogLevel, "Cannot allocate Py_BuildValue");
				ret_val = DPS_ERROR_NO_RESOURCES;
				break;
			}
			PyList_SetItem(pip_list, i, pip_tuple);
		}
		if (ret_val != DPS_SUCCESS)
		{
			Py_DECREF(pip_list);
			PyGILState_Release(gstate);
			break;
		}

		if (dps_msg->hdr.type == DPS_TUNNEL_REGISTER)
		{
			//def Tunnel_Register(self, domain_id, vnid, client_type, transaction_type,
			//                    dps_client_IP_type, dps_client_IP_packed, dps_client_port,
			//                    pip_tuple_list):
			strargs = Py_BuildValue("(IIIIHz#HO)",
			                        domain,
			                        dps_msg->hdr.vnid,
			                        dps_msg->hdr.client_id,
			                        dps_msg->hdr.transaction_type,
			                        dps_client_address.family,
			                        dps_client_address.ip6, 16,
			                        dps_client_address.port,
			                        pip_list);
			PyFunction = Client_Protocol_Interface.Tunnel_Register;
		}
		else
		{
			//def Tunnel_Unregister(self, domain_id, vnid, client_type, pip_tuple_list):
			strargs = Py_BuildValue("(IIIO)",
			                        domain,
			                        dps_msg->hdr.vnid,
			                        dps_msg->hdr.client_id,
			                        pip_list);
			PyFunction = Client_Protocol_Interface.Tunnel_Unregister;
		}
		if (strargs == NULL)
		{
			Py_DECREF(pip_list);
			PyGILState_Release(gstate);
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}
		// Invoke the Tunnel Register or Unregister call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);
		Py_DECREF(pip_list);

		if(strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Tunnel_(Un)Register returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}

		// Parse the return value (ret_code)
		PyArg_Parse(strret, "I", &ret_code);
		log_info(PythonDataHandlerLogLevel, "Tunnel_Register returns %d", ret_code);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

		//Process the local node replication
		//def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status)
		strargs = Py_BuildValue("(II)", query_id, ret_code);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "Cannot allocate arguments for local replication reply");
			break;
		}

		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Replication_Interface.ReplicationReplyProcess,
		                           strargs);

		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject ReplicationReplyProcess returns NULL");
			break;
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);
	}while(0);

	if ((fReplyToRequest) && (dps_msg_reply != NULL))
	{
		//dps_msg_reply->hdr.resp_status = ret_code;
		//dps_msg_send_inline(dps_msg_reply);
		free(dps_msg_reply);
	}

	log_debug(PythonDataHandlerLogLevel, "Exit");

	return ret_val;
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
static dps_return_status dps_form_multicast_sender_register_reply(dps_client_data_t *dps_msg_reply,
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
 * dps_msg_multicast_sender_register --                                   *//**
 *
 * \brief This routine handles registration of a sender in a Multicast Group
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Protocol Message containing the list of tunnel
 *                endpoints
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_multicast_sender_register(uint32_t domain,
                                                           dps_client_data_t *dps_msg)
{

	dps_client_data_t *dps_msg_reply = NULL;
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;
	dps_mcast_sender_t *mcast_sender;
	ip_addr_t tunnel_address, multicast_address;
	size_t dps_msg_size;
	PyObject *PyFunction = NULL;
	uint32_t ret_code;
	dps_return_status ret_val = DPS_SUCCESS;
	uint32_t query_id = 0;
	int fReplyToRequest = 0;
	char str_tunnel[INET6_ADDRSTRLEN];
	char str_multicast[INET6_ADDRSTRLEN];

	log_debug(PythonMulticastDataHandlerLogLevel, "Enter Domain Id %d", domain);
	do
	{
		mcast_sender = &dps_msg->mcast_sender;
		dps_msg_reply = (dps_client_data_t *)malloc(sizeof(dps_client_data_t));
		if (dps_msg_reply == NULL)
		{
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "Cannot allocated reply Message");
			break;
		}
		log_debug(PythonMulticastDataHandlerLogLevel, "Allocated Message %p", dps_msg_reply);
		dps_form_multicast_sender_register_reply(dps_msg_reply, dps_msg, DPS_NO_ERR);
		//Set the Reply Status
		fReplyToRequest = 1;

		// Copy the Tunnel Addresses so that they can be converted to Network Order
		memcpy(&tunnel_address, &mcast_sender->tunnel_endpoint, sizeof(ip_addr_t));
		if (tunnel_address.family == AF_INET)
		{
			tunnel_address.ip4 = htonl(tunnel_address.ip4);
		}
		// Copy the Multicast Address so that it can be converted to Network Order
		if (mcast_sender->mcast_addr.mcast_addr_type == MCAST_ADDR_V4)
		{
			multicast_address.ip4 = htonl(mcast_sender->mcast_addr.u.mcast_ip4);
			multicast_address.family = AF_INET;
		}
		else if (mcast_sender->mcast_addr.mcast_addr_type == MCAST_ADDR_V6)
		{
			memcpy(multicast_address.ip6, mcast_sender->mcast_addr.u.mcast_ip6, 16);
			multicast_address.family = AF_INET6;
		}
		else
		{
			multicast_address.family = 0;
		}
		{
			log_info(PythonMulticastDataHandlerLogLevel, "VNID %d", dps_msg->hdr.vnid);
			inet_ntop(tunnel_address.family, tunnel_address.ip6, str_tunnel, INET6_ADDRSTRLEN);
			log_info(PythonMulticastDataHandlerLogLevel, "Tunnel %s", str_tunnel);
			if (multicast_address.family != 0)
			{
				inet_ntop(multicast_address.family, multicast_address.ip6, str_multicast, INET6_ADDRSTRLEN);
				log_debug(PythonMulticastDataHandlerLogLevel, "Multicast IP %s", str_multicast);
			}
			log_info(PythonMulticastDataHandlerLogLevel, "Multicast Mac " MAC_FMT,
			          MAC_OCTETS(mcast_sender->mcast_addr.mcast_mac));
			log_info(PythonMulticastDataHandlerLogLevel, "Query ID %d", dps_msg->hdr.query_id);
			log_info(PythonMulticastDataHandlerLogLevel, "Sender Type %d", dps_msg->hdr.client_id);
			log_info(PythonMulticastDataHandlerLogLevel, "Transaction Type %d", dps_msg->hdr.transaction_type);
		}
		// Check if there are other nodes this request should be forwarded to.
		dps_msg_size = sizeof(dps_client_data_t);
		ret_code = dps_update_replicate(domain,
		                                dps_msg,
		                                dps_msg_reply, //dps_msg_reply
		                                dps_msg_size,
		                                &query_id);
		if (ret_code != DPS_NO_ERR)
		{
			break;
		}
		// Let the timeout take care of reply in case of failure
		fReplyToRequest = 0;
		if (query_id == 0)
		{
			break;
		}

		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//
		//  def Multicast_Sender_Register(self, domain_id, vnid, client_type, transaction_type,
		//                                multicast_mac, multicast_ip_family, multicast_ip_packed,
		//                                tunnel_ip_family, tunnel_ip_packed):
		strargs = Py_BuildValue("(IIIIz#Hz#Hz#)",
		                        domain,
		                        dps_msg->hdr.vnid,
		                        dps_msg->hdr.client_id,
		                        dps_msg->hdr.transaction_type,
		                        mcast_sender->mcast_addr.mcast_mac, 6,
		                        multicast_address.family,
		                        multicast_address.ip6, 16,
		                        tunnel_address.family,
		                        tunnel_address.ip6, 16);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_notice(PythonMulticastDataHandlerLogLevel, "Py_BuildValue returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}

		if (dps_msg->hdr.type == DPS_MCAST_SENDER_REGISTRATION)
		{
			log_debug(PythonMulticastDataHandlerLogLevel, "DPS_MCAST_SENDER_REGISTRATION");
			PyFunction = Client_Protocol_Interface.Multicast_Sender_Register;
		}
		else
		{
			log_debug(PythonMulticastDataHandlerLogLevel, "DPS_MCAST_SENDER_DEREGISTRATION");
			PyFunction = Client_Protocol_Interface.Multicast_Sender_Unregister;
		}
		// Invoke the Multicast Sender Register/Unregister call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if(strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "PyEval_CallObject Multicast_Sender_(Un)Register returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}

		// Parse the return value (ret_code)
		PyArg_Parse(strret, "I", &ret_code);
		log_info(PythonMulticastDataHandlerLogLevel,
		         "Multicast_Sender_Register returns %d", ret_code);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

		//Process the local node replication
		//def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status)
		strargs = Py_BuildValue("(II)", query_id, ret_code);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "Cannot allocate arguments for local replication reply");
			break;
		}

		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Replication_Interface.ReplicationReplyProcess,
		                           strargs);

		Py_DECREF(strargs);
		// Release the PYTHON Global Interpreter Lock

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "PyEval_CallObject ReplicationReplyProcess returns NULL");
			break;
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);
	}while(0);

	if ((fReplyToRequest) && (dps_msg_reply != NULL))
	{
		//dps_msg_reply->hdr.resp_status = ret_code;
		//dps_msg_send_inline(dps_msg_reply);
		free(dps_msg_reply);
	}

	log_debug(PythonMulticastDataHandlerLogLevel, "Exit");

	return ret_val;
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
static dps_return_status dps_form_multicast_receiver_register_reply(dps_client_data_t *dps_msg_reply,
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
 * dps_msg_multicast_receiver_register --                                 *//**
 *
 * \brief This routine handles registration of a sender in a Multicast Group
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Protocol Message containing the list of tunnel
 *                endpoints
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_multicast_receiver_register(uint32_t domain,
                                                             dps_client_data_t *dps_msg)
{

	dps_client_data_t *dps_msg_reply = NULL;
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;
	dps_mcast_receiver_t *mcast_receiver;
	ip_addr_t tunnel_address, multicast_address;
	size_t dps_msg_size;
	PyObject *PyFunction = NULL;
	uint32_t ret_code;
	dps_return_status ret_val = DPS_SUCCESS;
	uint32_t query_id = 0;
	int fReplyToRequest = 0;
	int fGlobalScope = 0;
	char str_tunnel[INET6_ADDRSTRLEN];
	char str_multicast[INET6_ADDRSTRLEN];

	log_debug(PythonMulticastDataHandlerLogLevel, "Enter Domain Id %d", domain);
	do
	{
		mcast_receiver = &dps_msg->mcast_receiver;
		dps_msg_reply = (dps_client_data_t *)malloc(sizeof(dps_client_data_t));
		if (dps_msg_reply == NULL)
		{
			break;
		}
		log_debug(PythonMulticastDataHandlerLogLevel, "Allocated Message %p", dps_msg_reply);
		dps_form_multicast_receiver_register_reply(dps_msg_reply, dps_msg, DPS_NO_ERR);
		//Set the Reply Status
		fReplyToRequest = 1;

		// Copy the Tunnel Addresses so that they can be converted to Network Order
		memcpy(&tunnel_address, &mcast_receiver->tunnel_endpoint, sizeof(ip_addr_t));
		if (tunnel_address.family == AF_INET)
		{
			tunnel_address.ip4 = htonl(tunnel_address.ip4);
		}
		// Copy the Multicast Address so that it can be converted to Network Order
		if (mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type == MCAST_ADDR_V4)
		{
			multicast_address.ip4 = htonl(mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip4);
			multicast_address.family = AF_INET;
		}
		else if (mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type == MCAST_ADDR_V6)
		{
			memcpy(multicast_address.ip6, mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip6, 16);
			multicast_address.family = AF_INET6;
		}
		else
		{
			multicast_address.family = 0;
		}
		// Check Global Scope
		if (mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type == MCAST_ADDR_V4_ICB_RANGE)
		{
			fGlobalScope = 1;
		}

		log_info(PythonMulticastDataHandlerLogLevel, "VNID %d", dps_msg->hdr.vnid);
		inet_ntop(tunnel_address.family, tunnel_address.ip6, str_tunnel, INET6_ADDRSTRLEN);
		log_info(PythonMulticastDataHandlerLogLevel, "Tunnel %s", str_tunnel);
		if (multicast_address.family != 0)
		{
			inet_ntop(multicast_address.family, multicast_address.ip6, str_multicast, INET6_ADDRSTRLEN);
			log_debug(PythonMulticastDataHandlerLogLevel, "Multicast IP %s", str_multicast);
		}
		log_info(PythonMulticastDataHandlerLogLevel, "Multicast Mac " MAC_FMT,
		         MAC_OCTETS(mcast_receiver->mcast_group_rec.mcast_addr.mcast_mac));
		log_info(PythonMulticastDataHandlerLogLevel, "Query ID %d", dps_msg->hdr.query_id);
		log_info(PythonMulticastDataHandlerLogLevel, "Sender Type %d", dps_msg->hdr.client_id);
		log_info(PythonMulticastDataHandlerLogLevel, "Transaction Type %d", dps_msg->hdr.transaction_type);
		log_info(PythonMulticastDataHandlerLogLevel, "Global Scope %d", fGlobalScope);
		// Check if there are other nodes this request should be forwarded to.
		dps_msg_size = sizeof(dps_client_data_t);
		ret_code = dps_update_replicate(domain,
		                                dps_msg,
		                                dps_msg_reply, //dps_msg_reply
		                                dps_msg_size,
		                                &query_id);
		if (ret_code != DPS_NO_ERR)
		{
			break;
		}
		// Let the timeout take care of reply in case of failure
		fReplyToRequest = 0;
		if (query_id == 0)
		{
			break;
		}

		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//    def Multicast_Receiver_Register(self, domain_id, vnid, client_type, transaction_type, global_scope
		//                                    multicast_mac, multicast_ip_family, multicast_ip_packed,
		//                                    tunnel_ip_family, tunnel_ip_packed):
		strargs = Py_BuildValue("(IIIIIz#Hz#Hz#)",
		                        domain,
		                        dps_msg->hdr.vnid,
		                        dps_msg->hdr.client_id,
		                        dps_msg->hdr.transaction_type,
		                        fGlobalScope,
		                        mcast_receiver->mcast_group_rec.mcast_addr.mcast_mac, 6,
		                        multicast_address.family,
		                        multicast_address.ip6, 16,
		                        tunnel_address.family,
		                        tunnel_address.ip6, 16);
		if (strargs == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_notice(PythonMulticastDataHandlerLogLevel, "Py_BuildValue returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}

		if (dps_msg->hdr.type == DPS_MCAST_RECEIVER_JOIN)
		{
			PyFunction = Client_Protocol_Interface.Multicast_Receiver_Register;
			log_debug(PythonMulticastDataHandlerLogLevel, "DPS_MCAST_RECEIVER_JOIN");
		}
		else
		{
			PyFunction = Client_Protocol_Interface.Multicast_Receiver_Unregister;
			log_debug(PythonMulticastDataHandlerLogLevel, "DPS_MCAST_RECEIVER_LEAVE");
		}
		// Invoke the Multicast Receiver Register/Unregister call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if(strret == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_alert(PythonMulticastDataHandlerLogLevel, "PyEval_CallObject returns NULL");
			ret_val = DPS_ERROR_NO_RESOURCES;
			break;
		}

		// Parse the return value (ret_code)
		PyArg_Parse(strret, "I", &ret_code);
		log_info(PythonMulticastDataHandlerLogLevel,
		         "Multicast_Receiver_Register returns %d", ret_code);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

		//Process the local node replication
		//def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status)
		strargs = Py_BuildValue("(II)", query_id, ret_code);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "Cannot allocate arguments for local replication reply");
			break;
		}

		// Add the query ID to the dictionary to track the replies
		strret = PyEval_CallObject(Replication_Interface.ReplicationReplyProcess,
		                           strargs);

		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonMulticastDataHandlerLogLevel,
			         "PyEval_CallObject ReplicationReplyProcess returns NULL");
			break;
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);
	}while(0);

	if ((fReplyToRequest) && (dps_msg_reply != NULL))
	{
		dps_msg_reply->hdr.resp_status = ret_code;
		dps_msg_send_inline(dps_msg_reply);
		free(dps_msg_reply);
	}

	log_debug(PythonMulticastDataHandlerLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_msg_vnid_policy_request --                                         *//**
 *
 * \brief This routine handles a bulk policy request for a VNID.
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for VNID Policy
 *                Request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_vnid_policy_request(uint32_t domain,
                                                     dps_client_data_t *dps_msg)
{
	PyObject *strargs = NULL;
	PyGILState_STATE gstate;
	ip_addr_t dps_client;
	char str_dps[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter Domain Id %d, VNID %d",
	          domain, dps_msg->hdr.vnid);

	memcpy(&dps_client, &dps_msg->gen_msg_req.dps_client_addr, sizeof(ip_addr_t));
	if (dps_client.family == AF_INET)
	{
		dps_client.ip4 = htonl(dps_client.ip4);
	}

	inet_ntop(dps_client.family, dps_client.ip6, str_dps, INET6_ADDRSTRLEN);
	log_info(PythonDataHandlerLogLevel,
	         "VNID Policy Request: DPS Client %s:%d, VNID %d",
	         str_dps, dps_client.port, dps_msg->hdr.vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// def Policy_Updates_Send_To(self, vnid_id, query_id, dps_client_IP_type, dps_client_ip_packed):
		strargs = Py_BuildValue("(IIIz#)",
		                        dps_msg->hdr.vnid,
		                        dps_msg->hdr.query_id,
		                        dps_client.family,
		                        dps_client.ip6, 16);
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Policy_Updates_Send_To call
		PyEval_CallObject(Client_Protocol_Interface.Policy_Updates_Send_To, strargs);
		Py_DECREF(strargs);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit");

	return DPS_SUCCESS;
}

/*
 ******************************************************************************
 * dps_msg_multicast_global_scope_request --                              *//**
 *
 * \brief This routine handles a bulk policy request for a VNID.
 *
 * \param domain The Domain ID
 * \param dps_msg The DPS Client Server Protocol Message for VNID Policy
 *                Request
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dps_return_status dps_msg_multicast_global_scope_request(uint32_t domain,
                                                                dps_client_data_t *dps_msg)
{
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;
	ip_addr_t dps_client;
	dps_client_data_t dps_msg_reply;
	int pIP_address_size;
	char *pIP_address;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter Domain Id %d, VNID %d",
	          domain, dps_msg->hdr.vnid);

	memcpy(&dps_client, &dps_msg->gen_msg_req.dps_client_addr, sizeof(ip_addr_t));
	if (dps_client.family == AF_INET)
	{
		dps_client.ip4 = htonl(dps_client.ip4);
	}

	inet_ntop(dps_client.family, dps_client.ip6, str, INET6_ADDRSTRLEN);
	log_info(PythonDataHandlerLogLevel,
	         "Multicast Global Scope Request: DPS Client %s:%d, VNID %d",
	         str, dps_client.port, dps_msg->hdr.vnid);

	//Create the reply message, use the request message itself
	dps_msg_reply.context = NULL;
	memcpy(&dps_msg_reply.hdr, &dps_msg->hdr, sizeof(dps_client_hdr_t));
	memcpy(&dps_msg_reply.hdr.reply_addr, &dps_msg->gen_msg_req.dps_client_addr, sizeof(ip_addr_t));
	dps_msg_reply.hdr.type = DPS_MCAST_CTRL_GW_REPLY;
	dps_msg_reply.hdr.resp_status = DPS_NO_MEMORY;
	dps_msg_reply.tunnel_info.num_of_tunnels = 0;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// def Multicast_Global_Scope_Get(self, domain_id):
		strargs = Py_BuildValue("(I)", domain);
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Policy_Updates_Send_To call
		strret = PyEval_CallObject(Client_Protocol_Interface.Multicast_Global_Scope_Get, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject: Multicast_Global_Scope_Get returns NULL");
			break;
		}

		// Parse the return value @rtype: Integer, Integer, ByteArray
		if (!PyArg_ParseTuple(strret, "IIz#",
		                      &dps_msg_reply.hdr.resp_status,
		                      &dps_msg_reply.tunnel_info.tunnel_list[0].vnid,
		                      &pIP_address, &pIP_address_size))
		{
			Py_DECREF(strret);
			log_warn(PythonDataHandlerLogLevel,
			         "PyArg_ParseTuple: Multicast_Global_Scope_Get returns False");
			break;
		}

		if (dps_msg_reply.hdr.resp_status != DPS_NO_ERR)
		{
			Py_DECREF(strret);
			log_info(PythonDataHandlerLogLevel,
			         "Multicast_Global_Scope_Get returns Error %d",
			         dps_msg_reply.hdr.resp_status);
			break;
		}
		if (pIP_address_size == 4)
		{
			// IPv4
			inet_ntop(AF_INET, pIP_address, str, INET_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "Multicast Control Tunnel: IPv4 %s",
			         str);
			dps_msg_reply.tunnel_info.tunnel_list[0].family = AF_INET;
			dps_msg_reply.tunnel_info.tunnel_list[0].ip4 = ntohl(*((uint32_t *)pIP_address));
		}
		else
		{
			// IPv6
			inet_ntop(AF_INET6, pIP_address, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel,
			         "Multicast Control Tunnel: IPv6 %s",
			         str);
			dps_msg_reply.tunnel_info.tunnel_list[0].family = AF_INET6;
			memcpy(dps_msg_reply.tunnel_info.tunnel_list[0].ip6, pIP_address, pIP_address_size);
		}
		Py_DECREF(strret);

		dps_msg_reply.tunnel_info.tunnel_list[0].flags = TUNNEL_IS_DESIGNATED_MCAST_GW;

	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	dps_msg_send_inline(&dps_msg_reply);

	log_info(PythonDataHandlerLogLevel, "Exit");

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

	dps_msg_function_array[0] = NULL;
	dps_msg_function_array[DPS_ENDPOINT_LOC_REQ] = dps_msg_endpoint_request;
	dps_msg_function_array[DPS_ENDPOINT_LOC_REPLY] = NULL;
	dps_msg_function_array[DPS_POLICY_REQ] = dps_msg_policy_request;
	dps_msg_function_array[DPS_POLICY_REPLY] = NULL;
	dps_msg_function_array[DPS_POLICY_INVALIDATE] = NULL;
	dps_msg_function_array[DPS_ENDPOINT_UPDATE] = dps_msg_endpoint_update;
	dps_msg_function_array[DPS_ENDPOINT_UPDATE_REPLY] = dps_msg_update_reply;
	dps_msg_function_array[DPS_ADDR_RESOLVE] = NULL;
	dps_msg_function_array[DPS_ADDR_REPLY] = NULL;
	dps_msg_function_array[DPS_INTERNAL_GW_REQ] = dps_msg_implicit_gateway_request;
	dps_msg_function_array[DPS_INTERNAL_GW_REPLY] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_VNID_POLICY_LIST] = NULL;
	dps_msg_function_array[DPS_BCAST_LIST_REQ] = dps_msg_broadcast_list_request;
	dps_msg_function_array[DPS_BCAST_LIST_REPLY] = NULL;
	dps_msg_function_array[DPS_VM_MIGRATION_EVENT] = dps_msg_vm_migration_event;
	dps_msg_function_array[DPS_MCAST_SENDER_REGISTRATION] = dps_msg_multicast_sender_register;
	dps_msg_function_array[DPS_MCAST_SENDER_DEREGISTRATION] = dps_msg_multicast_sender_register;
	dps_msg_function_array[DPS_MCAST_RECEIVER_JOIN] = dps_msg_multicast_receiver_register;
	dps_msg_function_array[DPS_MCAST_RECEIVER_LEAVE] = dps_msg_multicast_receiver_register;
	dps_msg_function_array[DPS_MCAST_RECEIVER_DS_LIST] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_BCAST_LIST_REPLY] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_INTERNAL_GW_REPLY] = NULL;
	dps_msg_function_array[DPS_GENERAL_ACK] = dps_msg_general_ack;
	dps_msg_function_array[DPS_UNSOLICITED_EXTERNAL_GW_LIST] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_VLAN_GW_LIST] = NULL;
	dps_msg_function_array[DPS_TUNNEL_REGISTER] = dps_msg_tunnel_register;
	dps_msg_function_array[DPS_TUNNEL_DEREGISTER] = dps_msg_tunnel_register;
	dps_msg_function_array[DPS_REG_DEREGISTER_ACK] = dps_msg_update_reply;
	dps_msg_function_array[DPS_EXTERNAL_GW_LIST_REQ] = dps_msg_gateway_request;
	dps_msg_function_array[DPS_EXTERNAL_GW_LIST_REPLY] = NULL;
	dps_msg_function_array[DPS_VLAN_GW_LIST_REQ] = dps_msg_gateway_request;
	dps_msg_function_array[DPS_VLAN_GW_LIST_REPLY] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_ENDPOINT_LOC_REPLY] = NULL;
	dps_msg_function_array[DPS_VNID_POLICY_LIST_REQ] = dps_msg_vnid_policy_request;
	dps_msg_function_array[DPS_VNID_POLICY_LIST_REPLY] = NULL;
	dps_msg_function_array[DPS_MCAST_CTRL_GW_REQ] = dps_msg_multicast_global_scope_request;
	dps_msg_function_array[DPS_MCAST_CTRL_GW_REPLY] = NULL;
	dps_msg_function_array[DPS_UNSOLICITED_VNID_DEL_REQ] = NULL;
	dps_msg_function_array[DPS_CTRL_PLANE_HB] = NULL;
	dps_msg_function_array[DPS_GET_DCS_NODE] = NULL;


	dps_msg_forward_remote[0] = 0;
	dps_msg_forward_remote[DPS_ENDPOINT_LOC_REQ] = 1;
	dps_msg_forward_remote[DPS_ENDPOINT_LOC_REPLY] = 0;
	dps_msg_forward_remote[DPS_POLICY_REQ] = 1;
	dps_msg_forward_remote[DPS_POLICY_REPLY] = 0;
	dps_msg_forward_remote[DPS_POLICY_INVALIDATE] = 1;
	dps_msg_forward_remote[DPS_ENDPOINT_UPDATE] = 1;
	dps_msg_forward_remote[DPS_ENDPOINT_UPDATE_REPLY] = 0;
	dps_msg_forward_remote[DPS_ADDR_RESOLVE] = 0;
	dps_msg_forward_remote[DPS_ADDR_REPLY] = 0;
	dps_msg_forward_remote[DPS_INTERNAL_GW_REQ] = 1;
	dps_msg_forward_remote[DPS_INTERNAL_GW_REPLY] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_VNID_POLICY_LIST] = 0;
	dps_msg_forward_remote[DPS_BCAST_LIST_REQ] = 1;
	dps_msg_forward_remote[DPS_BCAST_LIST_REPLY] = 0;
	dps_msg_forward_remote[DPS_VM_MIGRATION_EVENT] = 1;
	dps_msg_forward_remote[DPS_MCAST_SENDER_REGISTRATION] = 1;
	dps_msg_forward_remote[DPS_MCAST_SENDER_DEREGISTRATION] = 1;
	dps_msg_forward_remote[DPS_MCAST_RECEIVER_JOIN] = 1;
	dps_msg_forward_remote[DPS_MCAST_RECEIVER_LEAVE] = 1;
	dps_msg_forward_remote[DPS_MCAST_RECEIVER_DS_LIST] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_BCAST_LIST_REPLY] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_INTERNAL_GW_REPLY] = 0;
	dps_msg_forward_remote[DPS_GENERAL_ACK] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_EXTERNAL_GW_LIST] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_VLAN_GW_LIST] = 0;
	dps_msg_forward_remote[DPS_TUNNEL_REGISTER] = 1;
	dps_msg_forward_remote[DPS_TUNNEL_DEREGISTER] = 1;
	dps_msg_forward_remote[DPS_REG_DEREGISTER_ACK] = 0;
	dps_msg_forward_remote[DPS_EXTERNAL_GW_LIST_REQ] = 1;
	dps_msg_forward_remote[DPS_EXTERNAL_GW_LIST_REPLY] = 0;
	dps_msg_forward_remote[DPS_VLAN_GW_LIST_REQ] = 1;
	dps_msg_forward_remote[DPS_VLAN_GW_LIST_REPLY] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_ENDPOINT_LOC_REPLY] = 0;
	dps_msg_forward_remote[DPS_VNID_POLICY_LIST_REQ] = 1;
	dps_msg_forward_remote[DPS_VNID_POLICY_LIST_REPLY] = 0;
	dps_msg_forward_remote[DPS_MCAST_CTRL_GW_REQ] = 1;
	dps_msg_forward_remote[DPS_MCAST_CTRL_GW_REPLY] = 0;
	dps_msg_forward_remote[DPS_UNSOLICITED_VNID_DEL_REQ] = 0;
	dps_msg_forward_remote[DPS_CTRL_PLANE_HB] = 0;
	dps_msg_forward_remote[DPS_GET_DCS_NODE] = 0;

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

		// Get handle to function Endpoint_Update
		Client_Protocol_Interface.Endpoint_Update =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_ENDPOINT_UPDATE);
		if (Client_Protocol_Interface.Endpoint_Update == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_ENDPOINT_UPDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Endpoint_Location_vMac
		Client_Protocol_Interface.Endpoint_Location_vMac =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_ENDPOINT_REQUEST_vMac);
		if (Client_Protocol_Interface.Endpoint_Location_vMac == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_ENDPOINT_REQUEST_vMac);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Endpoint_Location_vIP
		Client_Protocol_Interface.Endpoint_Location_vIP =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_ENDPOINT_REQUEST_vIP);
		if (Client_Protocol_Interface.Endpoint_Location_vIP == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_ENDPOINT_REQUEST_vIP);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Resolution_vMac
		Client_Protocol_Interface.Policy_Resolution_vMac =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_POLICY_REQUEST_vMac);
		if (Client_Protocol_Interface.Policy_Resolution_vMac == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_REQUEST_vMac);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Resolution_vIP
		Client_Protocol_Interface.Policy_Resolution_vIP =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_POLICY_REQUEST_vIP);
		if (Client_Protocol_Interface.Policy_Resolution_vIP == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_REQUEST_vIP);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Implicit_Gateway_List
		Client_Protocol_Interface.Implicit_Gateway_List =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_IMPLICIT_GATEWAY_LIST);
		if (Client_Protocol_Interface.Implicit_Gateway_List == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IMPLICIT_GATEWAY_LIST);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Gateway_List
		Client_Protocol_Interface.Gateway_List =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_GATEWAY_LIST);
		if (Client_Protocol_Interface.Gateway_List == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_GATEWAY_LIST);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Is_VNID_Handled_Locally
		Client_Protocol_Interface.Is_VNID_Handled_Locally =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_IS_VNID_LOCAL);
		if (Client_Protocol_Interface.Is_VNID_Handled_Locally == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IS_VNID_LOCAL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Broadcast_List
		Client_Protocol_Interface.Broadcast_List =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_FUNC_BROADCAST_LIST);
		if (Client_Protocol_Interface.Broadcast_List == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_BROADCAST_LIST);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function pIP_Get_DPS_Client
		Client_Protocol_Interface.pIP_Get_DPS_Client =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_PIP_GET_DPS_CLIENT);
		if (Client_Protocol_Interface.pIP_Get_DPS_Client == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_PIP_GET_DPS_CLIENT);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function vIP_Get_DVG
		Client_Protocol_Interface.vIP_Get_DVG =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_VIP_GET_DVG);
		if (Client_Protocol_Interface.vIP_Get_DVG == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_VIP_GET_DVG);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Handle_Unsolicited_Message_Reply
		Client_Protocol_Interface.Handle_Unsolicited_Message_Reply =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_UNSOLICTED_MESSAGE_REPLY);
		if (Client_Protocol_Interface.Handle_Unsolicited_Message_Reply == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_UNSOLICTED_MESSAGE_REPLY);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Handle_Resolution_Reply
		Client_Protocol_Interface.Handle_Resolution_Reply =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_RESOLUTION_MESSAGE_REPLY);
		if (Client_Protocol_Interface.Handle_Resolution_Reply == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_RESOLUTION_MESSAGE_REPLY);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Tunnel_Register
		Client_Protocol_Interface.Tunnel_Register =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_TUNNEL_REGISTER);
		if (Client_Protocol_Interface.Tunnel_Register == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_TUNNEL_REGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Tunnel_Unregister
		Client_Protocol_Interface.Tunnel_Unregister =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_TUNNEL_UNREGISTER);
		if (Client_Protocol_Interface.Tunnel_Unregister == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_TUNNEL_UNREGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Sender_Register
		Client_Protocol_Interface.Multicast_Sender_Register =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_MULTICAST_SENDER_REGISTER);
		if (Client_Protocol_Interface.Multicast_Sender_Register == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MULTICAST_SENDER_REGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Sender_Unregister
		Client_Protocol_Interface.Multicast_Sender_Unregister =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_MULTICAST_SENDER_UNREGISTER);
		if (Client_Protocol_Interface.Multicast_Sender_Unregister == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MULTICAST_SENDER_UNREGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Receiver_Register
		Client_Protocol_Interface.Multicast_Receiver_Register =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_MULTICAST_RECEIVER_REGISTER);
		if (Client_Protocol_Interface.Multicast_Receiver_Register == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MULTICAST_RECEIVER_REGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Receiver_Unregister
		Client_Protocol_Interface.Multicast_Receiver_Unregister =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_MULTICAST_RECEIVER_UNREGISTER);
		if (Client_Protocol_Interface.Multicast_Receiver_Unregister == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MULTICAST_RECEIVER_UNREGISTER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Global_Scope_Get
		Client_Protocol_Interface.Multicast_Global_Scope_Get =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_MULTICAST_GLOBAL_SCOPE_GET);
		if (Client_Protocol_Interface.Multicast_Global_Scope_Get == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MULTICAST_GLOBAL_SCOPE_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Updates_Send_To
		Client_Protocol_Interface.Policy_Updates_Send_To =
			PyObject_GetAttrString(Client_Protocol_Interface.instance,
			                       PYTHON_POLICY_UPDATES_SEND_TO);
		if (Client_Protocol_Interface.Policy_Updates_Send_To == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_POLICY_UPDATES_SEND_TO);
			status = DOVE_STATUS_NOT_FOUND;
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
 * python_replication_handler_init --                                     *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DPSReplicationTracker) that are needed for processing
 *        replication replies received from DPS Server and Client.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_replication_handler_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDataHandlerLogLevel, "Enter");

	memset(&Replication_Interface, 0, sizeof(python_dps_replication_t));

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
		                                 PYTHON_MODULE_FILE_REPLICATION_HANDLER,
		                                 PYTHON_MODULE_CLASS_REPLICATION_HANDLER,
		                                 pyargs,
		                                 &Replication_Interface.instance);

		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function ReplicationQueryIDGenerate
		Replication_Interface.ReplicationQueryIDGenerate =
			PyObject_GetAttrString(Replication_Interface.instance,
			                       PYTHON_REPLICATION_QUERY_ID_GENERATE);
		if (Replication_Interface.ReplicationQueryIDGenerate == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_REPLICATION_QUERY_ID_GENERATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function ReplicationReplyProcess
		Replication_Interface.ReplicationReplyProcess =
			PyObject_GetAttrString(Replication_Interface.instance,
			                       PYTHON_REPLICATION_REPLY_PROCESS);
		if (Replication_Interface.ReplicationReplyProcess == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_REPLICATION_REPLY_PROCESS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	return status;
}

/*
 ******************************************************************************
 * python_mass_transfer_handler_init --                                   *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DPSMassTransfer) that are needed for processing
 *        mass transfer replies received from DPS Servers.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_mass_transfer_handler_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDataHandlerLogLevel, "Enter");

	memset(&Mass_Transfer_Interface, 0, sizeof(python_dps_mass_transfer_t));

	do
	{
		// Get handle to an instance of DPSMassTransfer
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_MASS_TRANSFER,
		                                 PYTHON_MODULE_CLASS_MASS_TRANSFER,
		                                 pyargs,
		                                 &Mass_Transfer_Interface.instance);

		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function ReplicationQueryIDGenerate
		Mass_Transfer_Interface.Transfer_Ack =
			PyObject_GetAttrString(Mass_Transfer_Interface.instance,
			                       PYTHON_MASS_TRANSFER_ACK);
		if (Mass_Transfer_Interface.Transfer_Ack == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_MASS_TRANSFER_ACK);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

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

		status = python_replication_handler_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		status = python_mass_transfer_handler_init(pythonpath);
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
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel,
	          "Enter Msg Type %d, VNID %d, client id %d, transaction type %d, query id %d",
	          client_data->hdr.type,
	          client_data->hdr.vnid,
	          client_data->hdr.client_id,
	          client_data->hdr.transaction_type,
	          client_data->hdr.query_id);

	do
	{
		if (!started)
		{
			status = DPS_ERROR;
			break;
		}
		msg_type = (uint32_t)client_data->hdr.type;
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		strargs = Py_BuildValue("(II)",
		                        client_data->hdr.vnid,
		                        client_data->hdr.transaction_type);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Is_VNID_Handled_Locally call
		strret = PyEval_CallObject(Client_Protocol_Interface.Is_VNID_Handled_Locally, strargs);
		Py_DECREF(strargs);

		// Parse the return value @return: 0 = False, 1 = True
		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "Msg Type %d, VNID %d, PyEval_CallObject returns NULL",
			         client_data->hdr.type, client_data->hdr.vnid);
			break;
		}
		PyArg_ParseTuple(strret, "III", &dps_status, &fLocalDomain, &domain_id);
		Py_DECREF(strret);
		PyGILState_Release(gstate);

		log_debug(PythonDataHandlerLogLevel,
		          "Is_VNID_Handled_Locally returns dps_status %s, "
		          "fLocalDomain %d, Domain %d",
		          DOVEStatusToString(dps_status), fLocalDomain, domain_id);
		if (dps_status != DOVE_STATUS_OK)
		{
			// Let the client retry
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
