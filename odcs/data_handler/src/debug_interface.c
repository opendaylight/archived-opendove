/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  This file embedded the Python Module that handles
**                    Debugging Requests
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
*  $Log: debug_interface.c $
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
int PythonDebugHandlerLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief The module location defines for the DPS Debug Handler
 */
#define PYTHON_MODULE_FILE_DEBUG_HANDLER "debug_handler"

/**
 * \brief The PYTHON Class that handles the DPS Debug Handler
 */
#define PYTHON_MODULE_CLASS_DEBUG_HANDLER "DpsDebugHandler"

/**
 * \brief The PYTHON function for Get_VNID_Endpoints
 */
#define PYTHON_GET_VNID_ENDPOINTS "Get_VNID_Endpoints"

/**
 * \brief The PYTHON function for Get_VNID_DoveSwitches
 */
#define PYTHON_GET_VNID_DOVESWITCHES "Get_VNID_DoveSwitches"

/**
 * \brief The PYTHON function for Get_Domain_VNID_Mapping
 */
#define PYTHON_GET_DOMAIN_VNID_MAPPING "Get_Domain_VNID_Mapping"

/**
 * \brief The PYTHON function for Get_VNID_Allow_Policies
 */
#define PYTHON_GET_VNID_ALLOW_POLICIES "Get_VNID_Allow_Policies"

/**
 * \brief The PYTHON function for Get_VNID_Subnets
 */
#define PYTHON_GET_VNID_SUBNETS "Get_VNID_Subnets"

/**
 * \brief The DPS Debug handler function pointers data structure
 */
typedef struct python_dps_debug_s{
	/**
	 * \brief The DpsDebugHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function for Get_VNID_Endpoints
	 */
	PyObject *Get_VNID_Endpoints;
	/*
	 * \brief The function for Get_VNID_DoveSwitches
	 */
	PyObject *Get_VNID_DoveSwitches;
	/**
	 * \brief The function for Get_Domain_VNID_Mapping
	 */
	PyObject *Get_Domain_VNID_Mapping;
	/**
	 * \brief The function for Get_VNID_Allow_Policies
	 */
	PyObject *Get_VNID_Allow_Policies;
	/**
	 * \brief The PYTHON function for Get_VNID_Subnets
	 */
	PyObject *Get_VNID_Subnets;
}python_dps_debug_t;

/*
 * \brief The Debug Handler PYTHON Interface (Embed)
 */
static python_dps_debug_t Debug_Interface;

/**
 * \brief A buffer to store the Domain --> VNID Mapping
 */
#define DOMAIN_VNID_BUFFER_SIZE 2048
static char domain_vnid_buffer[DOMAIN_VNID_BUFFER_SIZE];

/*
 ******************************************************************************
 * DPS Debug Interface                                                   *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSDebugInterface DPS Debugging Interface
 * @{
 * Handles Debugging requests from REST interface and CLI interface
 */

/*
 ******************************************************************************
 * gdb_show_domain_info --                                                  *//**
 *
 * \brief This routine handles Domain Show. This routine can be invoked from GDB
 *        for debugging purposes
 *
 * \param domain The Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/

void gdb_show_domain_info(int domain)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	log_info(PythonDebugHandlerLogLevel, "Enter Domain %d", domain);
	data_op.type = DPS_CONTROLLER_DOMAIN_SHOW;
	data_op.domain_show.domain_id = domain;
	data_op.domain_show.fDetails = 1;
	status = dps_controller_data_msg(&data_op);
	if (status != DOVE_STATUS_OK)
	{
		show_print("Error - %s", DOVEStatusToString(status))
	}
	log_info(PythonDebugHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * gdb_show_domain_multicast --                                           *//**
 *
 * \brief This routine handles Multicast show for a Domain
 *
 * \param domain The Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/
void gdb_show_domain_multicast(int domain)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	log_info(PythonDebugHandlerLogLevel, "Enter Domain %d", domain);
	data_op.type = DPS_CONTROLLER_MULTICAST_SHOW;
	data_op.multicast_show.associated_type = 1; //Domain
	data_op.multicast_show.associated_id = domain;
	status = dps_controller_data_msg(&data_op);
	if (status != DOVE_STATUS_OK)
	{
		show_print("Error - %s", DOVEStatusToString(status))
	}
	log_info(PythonDebugHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * gdb_show_vnid_info --                                                  *//**
 *
 * \brief This routine handles DVG Show. This routine can be invoked from GDB
 *        for debugging purposes
 *
 * \param vnid The VNID
 *
 * \return dove_status
 *
 *****************************************************************************/

void gdb_show_vnid_info(int vnid)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);
	data_op.type = DPS_CONTROLLER_VNID_SHOW;
	data_op.vnid_show.vnid = vnid;
	data_op.vnid_show.fDetails = 1;
	status = dps_controller_data_msg(&data_op);
	if (status != DOVE_STATUS_OK)
	{
		show_print("Error - %s", DOVEStatusToString(status))
	}
	log_info(PythonDebugHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * gdb_show_vnid_multicast --                                             *//**
 *
 * \brief This routine handles Multicast show for a VNID
 *
 * \param vnid The VNID
 *
 * \return dove_status
 *
 *****************************************************************************/
void gdb_show_vnid_multicast(int vnid)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);
	data_op.type = DPS_CONTROLLER_MULTICAST_SHOW;
	data_op.multicast_show.associated_type = 2; //VNID
	data_op.multicast_show.associated_id = vnid;
	status = dps_controller_data_msg(&data_op);
	if (status != DOVE_STATUS_OK)
	{
		show_print("Error - %s", DOVEStatusToString(status))
	}
	log_info(PythonDebugHandlerLogLevel, "Exit status");
	return;
}

/*
 ******************************************************************************
 * gdb_show_cluster --                                                    *//**
 *
 * \brief This routine handles showing the Cluster
 *
 * \param vnid The VNID
 *
 * \return dove_status
 *
 *****************************************************************************/
void gdb_show_cluster()
{
	dps_cluster_show();
	return;
}

/*
 ******************************************************************************
 * gdb_show_vnid_mapping   --                                             *//**
 *
 * \brief This routine shows the VNID mapping.
 *
 * \param vnid The VNID
 *
 * \return dove_status
 *
 *****************************************************************************/
void gdb_show_vnid_mapping()
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_DOMAIN_GLOBAL_SHOW;
	dps_controller_data_msg(&data_op);
	return;
}


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

dove_status vnid_get_endpoints(int vnid)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pyEndpointList, *pyEndpointTuple;
	unsigned char *vIP_address, *pIP_address, *hostIP_address, *vMac;
	int i, vIP_address_size, pIP_address_size, hostIP_address_size, vMac_size;
	uint32_t log_console_value;
	PyGILState_STATE gstate;
	char vip_str[INET6_ADDRSTRLEN];
	char host_str[INET6_ADDRSTRLEN];
	char pip_str[INET6_ADDRSTRLEN];
	unsigned char vMac_str[MAC_MAX_LENGTH];

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);

	// Store the Log Console Value
	log_console_value = log_console;
	log_console = 1;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		memset(vMac_str, 0, MAC_MAX_LENGTH);

		//def Get_VNID_Endpoints(self, vnid):
		strargs = Py_BuildValue("(i)",vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_Endpoints call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_Endpoints, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_Endpoints returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iO", &status, &pyEndpointList);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		if(!PyList_Check(pyEndpointList))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not List Type!!!");
			Py_DECREF(strret);
			break;
		}

		for (i = 0; i < PyList_Size(pyEndpointList); i++)
		{
			pyEndpointTuple = PyList_GetItem(pyEndpointList, i);
			//(vMac, pIP, [vIP1, vIP2, vIP3])
			if (!PyArg_ParseTuple(pyEndpointTuple, "z#z#z#z#",
			                      &vMac, &vMac_size,
			                      &hostIP_address, &hostIP_address_size,
			                      &pIP_address, &pIP_address_size,
			                      &vIP_address, &vIP_address_size
			                     ))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Invalid Endpoint in element %d", i);
				continue;
			}
			if (pIP_address_size == 4)
			{
				inet_ntop(AF_INET, pIP_address, pip_str, INET6_ADDRSTRLEN);
			}
			else
			{
				inet_ntop(AF_INET6, pIP_address, pip_str, INET6_ADDRSTRLEN);
			}
			if (hostIP_address_size == 4)
			{
				inet_ntop(AF_INET, hostIP_address, host_str, INET6_ADDRSTRLEN);
			}
			else
			{
				inet_ntop(AF_INET6, hostIP_address, host_str, INET6_ADDRSTRLEN);
			}
			if (vIP_address_size == 4)
			{
				inet_ntop(AF_INET, vIP_address, vip_str, INET6_ADDRSTRLEN);
			}
			else
			{
				inet_ntop(AF_INET6, vIP_address, vip_str, INET6_ADDRSTRLEN);
			}
			/* Get the endpoint MAC and pIP str */
			sprintf((char *)vMac_str, MAC_FMT, MAC_OCTETS(vMac));

			log_notice(PythonDebugHandlerLogLevel,
			           "Endpoint [%d] vMac %s, Host %s, pIP %s, vIP %s",
			           i, vMac_str, host_str, pip_str, vip_str);

			/*Next endpoint, clear again */
			memset(vMac_str, 0, MAC_MAX_LENGTH);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.

		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);

	log_console = log_console_value;

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));
	return ((dove_status)status);
}

/*
URI:
/api/dove/dps/vns/<vn_id>/endpoints
Json body:
{
	"endpoints":
	[
		{
			"mac":"11:11:11:11:11:11",
			"virtual IPs":"192.168.1.11, 172.31.1.11"
			"physical IPs":"1.1.1.1, 2.2.2.2"
		},

		{
			"mac":"22:22:22:22:22:22",
			"virtual IPs":"192.168.1.23, 172.31.1.23"
			"physical IPs":"1.1.1.5, 2.2.2.5"
		}
	]
}
*/

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
json_t  *vnid_get_endpoints_json(int vnid)
{
	json_t *js_root = NULL;
	json_t *js_endpoints = NULL;
	json_t *js_endpoint = NULL;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pyEndpointList, *pyEndpointTuple;
	unsigned char *vIP_address, *pIP_address, *hostIP_address, *vMac;
	int i, vIP_address_size, pIP_address_size, hostIP_address_size, vMac_size;
	PyGILState_STATE gstate;
	char vip_str[INET6_ADDRSTRLEN];
	char host_str[INET6_ADDRSTRLEN];
	char pip_str[INET6_ADDRSTRLEN];
	unsigned char vMac_str[MAC_MAX_LENGTH];

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		memset(vMac_str, 0, MAC_MAX_LENGTH);

		js_endpoints = json_array();
		if (js_endpoints == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "json_array returns NULL");
			break;
		}

		//def Get_VNID_Endpoints(self, vnid):
		strargs = Py_BuildValue("(i)",vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_Endpoints call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_Endpoints, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_Endpoints returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iO", &status, &pyEndpointList);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		if(!PyList_Check(pyEndpointList))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not List Type!!!");
			Py_DECREF(strret);
			break;
		}
		
		for (i = 0; i < PyList_Size(pyEndpointList); i++)
		{
			pyEndpointTuple = PyList_GetItem(pyEndpointList, i);
			//(vMac, pIP, [vIP1, vIP2, vIP3])
			if (!PyArg_ParseTuple(pyEndpointTuple, "z#z#z#z#",
			                      &vMac, &vMac_size,
			                      &hostIP_address, &hostIP_address_size,
			                      &pIP_address, &pIP_address_size,
			                      &vIP_address, &vIP_address_size
			                     ))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Invalid Endpoint in element %d", i);
				continue;
			}
			if (pIP_address_size == 4)
			{
				inet_ntop(AF_INET, pIP_address, pip_str, INET6_ADDRSTRLEN);
			}
			else
			{
				inet_ntop(AF_INET6, pIP_address, pip_str, INET6_ADDRSTRLEN);
			}
			if (hostIP_address_size == 4)
			{
				inet_ntop(AF_INET, hostIP_address, host_str, INET6_ADDRSTRLEN);
			}
			else
			{
				inet_ntop(AF_INET6, hostIP_address, host_str, INET6_ADDRSTRLEN);
			}
			if (vIP_address_size == 4)
			{
				inet_ntop(AF_INET, vIP_address, vip_str, INET6_ADDRSTRLEN);
				if (!strcmp(vip_str, "0.0.0.0"))
				{
					memset(vip_str, 0, INET6_ADDRSTRLEN);
				}
			}
			else
			{
				inet_ntop(AF_INET6, vIP_address, vip_str, INET6_ADDRSTRLEN);
			}
			log_info(PythonDebugHandlerLogLevel,
			         "Endpoint [%d] vMac "MAC_FMT", Host %s, pIP %s, vIP %s",
			         i, MAC_OCTETS(vMac), host_str, pip_str, vip_str);

			/* Get the endpoint MAC and pIP str */
			sprintf((char *)vMac_str, MAC_FMT, MAC_OCTETS(vMac));

			js_endpoint = json_pack("{s:s,s:s,s:s,s:s}",
			                        "mac", vMac_str,
			                        "Host IP", host_str,
			                        "virtual IPs", vip_str,
			                        "physical IPs", pip_str);

			/*Next endpoint, clear again */
			memset(vMac_str, 0, MAC_MAX_LENGTH);
			json_array_append_new(js_endpoints, js_endpoint);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.

		// maybe it never be NULL, the js_endpoints may be just [], 
		//which means it has no endpoints here
		if(js_endpoints != NULL)
		{
			js_root = json_pack("{s:o}", "endpoints", js_endpoints);
		}

		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));

	return (status == DOVE_STATUS_OK? js_root:NULL);
}


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

dove_status vnid_get_dove_switches(int vnid, char *buff, size_t buff_size)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pyTunnelList, *pypIPListTuple, *pypIPList;
	int i, pIP_index, hostIP_address_size, hostIP_type;
	PyGILState_STATE gstate;
	char ip_str[INET6_ADDRSTRLEN];
	char host_str[INET6_ADDRSTRLEN];
	char *hostIP_address;

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Get_VNID_Endpoints(self, vnid):
		strargs = Py_BuildValue("(i)",vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_DoveSwitches call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_DoveSwitches, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_DoveSwitches returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iO", &status, &pyTunnelList);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		if(!PyList_Check(pyTunnelList))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not List Type!!!");
			Py_DECREF(strret);
			break;
		}
		// TODO: Copy into the buffer provided by calling routine.
		for (i = 0; i < PyList_Size(pyTunnelList); i++)
		{
			pypIPListTuple = PyList_GetItem(pyTunnelList, i);
			//@return: (ret_val, list_endpoints)
			if (!PyArg_ParseTuple(pypIPListTuple, "z#O",
			                      &hostIP_address, &hostIP_address_size, &pypIPList))
			{
				log_warn(PythonDebugHandlerLogLevel, "Bad Data in Element %d", i);
				continue;
			}
			if(!PyList_Check(pypIPList))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Invalid pIP List in DOVE Switch Element %d", i);
				continue;
			}
			if (hostIP_address_size == 4)
			{
				hostIP_type = AF_INET;
			}
			else
			{
				hostIP_type = AF_INET6;
			}
			inet_ntop(hostIP_type, hostIP_address, host_str, INET6_ADDRSTRLEN);
			for (pIP_index = 0; pIP_index < PyList_Size(pypIPList); pIP_index++)
			{
				PyObject *py_pIP;
				int ipv4;

				py_pIP = PyList_GetItem(pypIPList, pIP_index);
				if (PyInt_CheckExact(py_pIP))
				{
					//IPv4
					if (!PyArg_Parse(py_pIP, "I", &ipv4))
					{
						log_warn(PythonDebugHandlerLogLevel,
						         "Cannot get IPv4 address at pIP_List Index %d:%d",
						         i, pIP_index);
						continue;
					}
					inet_ntop(AF_INET, &ipv4, ip_str, INET6_ADDRSTRLEN);
				}
				else
				{
					char *ipv6;
					int ipv6_size;
					//Assume IPv6
					if (!PyArg_Parse(py_pIP, "z#", &ipv6, &ipv6_size))
					{
						log_warn(PythonDebugHandlerLogLevel,
						         "Cannot get IPv4 address at pIP_List Index %d:%d",
						         i, pIP_index);
						continue;
					}
					inet_ntop(AF_INET6, ipv6, ip_str, INET6_ADDRSTRLEN);
				}
				log_debug(PythonDebugHandlerLogLevel,
				          "[%d] Host IP%s, pIP[%d] %s",
				          i, host_str, pIP_index, ip_str);
			}
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));
	return ((dove_status)status);
}

/*

URI:
/api/dove/dps/vns/<vn_id>/tunnel-endpoints
Json body:
{
	"tunnel-endpoints":
	[
		{
			"IPs":"1.1.1.1, 2.2.2.2"
		},

		{
			"IPs":"1.1.1.5, 2.2.2.5"
		}
	]
}

*/

/* Return tunnel endpoints */
json_t *vnid_get_dove_switches_json(int vnid)
{

	json_t *js_root = NULL;
	json_t *js_tunnel_endpoints = NULL;
	json_t *js_tunnel_endpoint = NULL;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pyTunnelList, *pypIPListTuple, *pypIPList;
	int i, pIP_index, hostIP_address_size, hostIP_type;
	PyGILState_STATE gstate;
	char ip_str[INET6_ADDRSTRLEN];
	char pip_str[512];
	uint32_t pip_str_remaining_len = 512;
	char host_str[INET6_ADDRSTRLEN];
	char *hostIP_address;

	js_tunnel_endpoints = json_array();

	memset(pip_str,0,512);

	log_info(PythonDebugHandlerLogLevel, "Enter VNID %d", vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Get_VNID_Endpoints(self, vnid):
		strargs = Py_BuildValue("(i)",vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_DoveSwitches call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_DoveSwitches, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_DoveSwitches returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iO", &status, &pyTunnelList);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		if(!PyList_Check(pyTunnelList))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not List Type!!!");
			Py_DECREF(strret);
			break;
		}
		
		// TODO: Copy into the buffer provided by calling routine.
		for (i = 0; i < PyList_Size(pyTunnelList); i++)
		{
			pypIPListTuple = PyList_GetItem(pyTunnelList, i);
			//@return: (ret_val, list_endpoints)
			if (!PyArg_ParseTuple(pypIPListTuple, "z#O",
			                      &hostIP_address, &hostIP_address_size, &pypIPList))
			{
				log_warn(PythonDebugHandlerLogLevel, "Bad Data in Element %d", i);
				continue;
			}
			if(!PyList_Check(pypIPList))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Invalid pIP List in Tunnel Element %d", i);
				continue;
			}
			if (hostIP_address_size == 4)
			{
				hostIP_type = AF_INET;
			}
			else
			{
				hostIP_type = AF_INET6;
			}
			inet_ntop(hostIP_type, hostIP_address, host_str, INET6_ADDRSTRLEN);
			for (pIP_index = 0; pIP_index < PyList_Size(pypIPList); pIP_index++)
			{
				PyObject *py_pIP;
				int ipv4;

				py_pIP = PyList_GetItem(pypIPList, pIP_index);
				if (PyInt_CheckExact(py_pIP))
				{
					//IPv4
					if (!PyArg_Parse(py_pIP, "I", &ipv4))
					{
						log_warn(PythonDebugHandlerLogLevel,
						         "Cannot get IPv4 address at pIP_List Index %d:%d",
						         i, pIP_index);
						continue;
					}
					inet_ntop(AF_INET, &ipv4, ip_str, INET6_ADDRSTRLEN);
				}
				else
				{
					char *ipv6;
					int ipv6_size;
					//Assume IPv6
					if (!PyArg_Parse(py_pIP, "z#", &ipv6, &ipv6_size))
					{
						log_warn(PythonDebugHandlerLogLevel,
						         "Cannot get IPv4 address at pIP_List Index %d:%d",
						         i, pIP_index);
						continue;
					}
					inet_ntop(AF_INET6, ipv6, ip_str, INET6_ADDRSTRLEN);
				}
				
				/* append a "," to the prev string, don't use  pIP_index!=0 as the first element
				may be bypassed 
				*/
				if( strlen(pip_str) !=0 && pip_str_remaining_len-1 > 0)
				{
					strcat(pip_str,",");
					pip_str_remaining_len --;
				}

				log_info(PythonDebugHandlerLogLevel,
				         "[%d] Host IP%s, pIP[%d] %s]",
				         i, host_str, pIP_index, ip_str);

				if( (pip_str_remaining_len-1) > strlen(ip_str))
				{
					strncat(pip_str,ip_str,pip_str_remaining_len-1);
					pip_str_remaining_len -= strlen(ip_str);
				}else if(pip_str_remaining_len-1 > 0)
				{
					strncat(pip_str,ip_str,pip_str_remaining_len-1);
					pip_str_remaining_len = 0;
				}

			}
			/*	"IPs":"1.1.1.1, 2.2.2.2" */
			js_tunnel_endpoint = json_pack("{s:s,s:s}",
			                               "Host IP", host_str,
			                               "pIPs", pip_str);
			/*Next endpoint, clear again */
			memset(pip_str,0,512);
			pip_str_remaining_len = 512;
			json_array_append_new(js_tunnel_endpoints, js_tunnel_endpoint);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);

	// maybe it never be NULL, the js_tunnel_endpoints may be just [], 
	//which means it has no tunnel endpoints here

	if(js_tunnel_endpoints != NULL)
	{
		js_root = json_pack("{s:o}", "tunnel-endpoints", js_tunnel_endpoints);
	}

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));

//	return ((dove_status)status);
	if (status != DOVE_STATUS_OK)
		return NULL;

	return js_root;

}

json_t *vnid_get_tunnel_endpoints_json(int vnid)
{
	return vnid_get_dove_switches_json(vnid);
}

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
json_t *vnid_get_domain_mapping_json()
{

	json_t *js_root = NULL;
	json_t *js_domain_to_vnid = NULL;
	json_t *js_vnid_to_domain = NULL;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pyDomainDict, *pyVnidDict;
	PyObject *key, *value;
	PyGILState_STATE gstate;
	Py_ssize_t Dict_pos = 0;

	log_info(PythonDebugHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		js_domain_to_vnid = json_array();
		if (js_domain_to_vnid == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "json_array() for js_domain_to_vnid returns NULL");
			break;
		}
		js_vnid_to_domain = json_array();
		if (js_vnid_to_domain == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "json_array() for js_vnid_to_domain returns NULL");
			break;
		}
		//def Get_VNID_Endpoints(self, vnid):
		strargs = Py_BuildValue("()");
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_Domain_VNID_Mapping call
		strret = PyEval_CallObject(Debug_Interface.Get_Domain_VNID_Mapping, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_Domain_VNID_Mapping returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "OO", &pyDomainDict, &pyVnidDict);
		if(!PyDict_Check(pyDomainDict))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not Dict Type!!!");
			Py_DECREF(strret);
			break;
		}
		if(!PyDict_Check(pyVnidDict))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not Dict Type!!!");
			Py_DECREF(strret);
			break;
		}
		while (PyDict_Next(pyDomainDict, &Dict_pos, &key, &value))
		{
			PyObject *key_vnid, *value_vnid;
			Py_ssize_t DictVnid_pos = 0;
			uint32_t domain;
			json_t *js_vnids;
			int domain_vnid_buffer_len = 0;

			memset(domain_vnid_buffer, 0, DOMAIN_VNID_BUFFER_SIZE);

			//Each key is a Domain, value is another dictionary
			if(!PyArg_Parse(key, "I", &domain))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "pyDomainDict Key Cannot get Domain");
				continue;
			}
			if(!PyDict_Check(value))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "VNID Dict in Domain Dict %d not Dictionary type",
				         domain);
				continue;
			}
			while(PyDict_Next(value, &DictVnid_pos, &key_vnid, &value_vnid))
			{
				uint32_t vnid;
				char vnid_string[16];
				size_t vnid_string_len;

				if(!PyArg_Parse(key_vnid, "I", &vnid))
				{
					log_warn(PythonDebugHandlerLogLevel,
					         "VNID Dict in Domain Dict %d Cannot get VNID",
					         domain);
					continue;
				}
				memset(vnid_string, 0, 16);
				snprintf(vnid_string, 15, "%d ", vnid);
				vnid_string_len = strlen(vnid_string);
				//Check if domain_vnid_buffer can handle this
				if ((domain_vnid_buffer_len+vnid_string_len+1) > DOMAIN_VNID_BUFFER_SIZE)
				{
					log_notice(PythonDebugHandlerLogLevel,
					           "VNID Dict in Domain Dict %d, too big",
					           domain);
					break;
				}
				strncat(domain_vnid_buffer, vnid_string, 15);
				domain_vnid_buffer_len += vnid_string_len;
			}
			js_vnids = json_pack("{s:i, s:s}",
			                     "Domain", (unsigned int)domain,
			                     "Vnids", domain_vnid_buffer);
			if (js_vnids == NULL)
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Domain Dict %d cannot create JSON Object of VNIDs",
				         domain);
				continue;
			}
			json_array_append_new(js_domain_to_vnid, js_vnids);
		}
		Dict_pos = 0;
		while (PyDict_Next(pyVnidDict, &Dict_pos, &key, &value))
		{
			uint32_t domain, vnid;
			json_t *js_vnid_mapping;
			//Each key is a VNID, value is Domain
			if(!PyArg_Parse(key, "I", &vnid))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "pyVnidDict Key Cannot get VNID");
				continue;
			}

			if(!PyArg_Parse(value, "I", &domain))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "pyVnidDict Key Cannot get Domain for VNID %d",
				         vnid);
				continue;
			}
			js_vnid_mapping = json_pack("{s:i, s:i}",
			                            "Vnid", vnid,
			                            "Domain", domain);
			if (js_vnid_mapping == NULL)
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "VNID Dict %d cannot create JSON Object of VNID:Domain",
				         vnid);
				continue;
			}
			json_array_append_new(js_vnid_to_domain, js_vnid_mapping);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		// Create the main object
		js_root = json_pack("{s:o, s:o}",
		                    "Domain Mapping", js_domain_to_vnid,
		                    "Vnid Mapping", js_vnid_to_domain);
		if (js_root == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "json_pack (js_root) returns NULL");
			break;
		}
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);

	// maybe it never be NULL, the js_tunnel_endpoints may be just [],
	//which means it has no tunnel endpoints here

	if (status != DOVE_STATUS_OK)
	{
		if (js_domain_to_vnid)
		{
			json_decref(js_domain_to_vnid);
		}
		if (js_vnid_to_domain)
		{
			json_decref(js_vnid_to_domain);
		}
	}

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));

	return js_root;

}

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
json_t *vnid_get_allow_policies(int vnid)
{

	json_t *js_root = NULL;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	char *unicast_policy_string, *multicast_policy_string;
	PyGILState_STATE gstate;

	log_info(PythonDebugHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Get_VNID_Allow_Policies(self, vnid):
		strargs = Py_BuildValue("(i)", vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_DoveSwitches call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_Allow_Policies, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_Allow_Policies returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iss", &status,
		                                &unicast_policy_string,
		                                &multicast_policy_string);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		// Create the main object
		js_root = json_pack("{s:s, s:s}",
		                    "Unicast Policies", unicast_policy_string,
		                    "Multicast Policies", multicast_policy_string);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		if (js_root == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "json_pack (js_root) returns NULL");
			break;
		}
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));

	return js_root;

}

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
json_t *vnid_get_subnets(int vnid)
{

	json_t *js_root = NULL;
	json_t *js_subnets = NULL;
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *pySubnetList;
	PyGILState_STATE gstate;
	int i;

	log_info(PythonDebugHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		js_subnets = json_array();
		if (js_subnets == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "json_array() for js_subnets returns NULL");
			break;
		}
		//def Get_VNID_Allow_Policies(self, vnid):
		strargs = Py_BuildValue("(i)", vnid);
		if(strargs == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Get_VNID_Subnets call
		strret = PyEval_CallObject(Debug_Interface.Get_VNID_Subnets, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "PyEval_CallObject Get_VNID_Subnets returns NULL");
			break;
		}
		//@return: (ret_val, list_endpoints)
		PyArg_ParseTuple(strret, "iO", &status, &pySubnetList);
		if (status != DOVE_STATUS_OK)
		{
			Py_DECREF(strret);
			break;
		}
		if(!PyList_Check(pySubnetList))
		{
			log_warn(PythonDebugHandlerLogLevel, "Not List Type!!!");
			Py_DECREF(strret);
			break;
		}
		for (i = 0; i < PyList_Size(pySubnetList); i++)
		{
			PyObject *pySubnetTuple;
			char *mode;
			int ip, mask, gateway;
			char ip_str[INET6_ADDRSTRLEN];
			char mask_str[INET6_ADDRSTRLEN];
			char gateway_str[INET6_ADDRSTRLEN];
			json_t *js_subnet = NULL;

			pySubnetTuple = PyList_GetItem(pySubnetList, i);
			if (!PyArg_ParseTuple(pySubnetTuple, "sIII",
			                      &mode, &ip, &mask, &gateway))
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Invalid Subnet in element %d", i);
				continue;
			}
			inet_ntop(AF_INET, &ip, ip_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &mask, mask_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &gateway, gateway_str, INET6_ADDRSTRLEN);
			js_subnet = json_pack("{s:s, s:s, s:s, s:s}",
			                      "Mode", mode,
			                      "IP", ip_str,
			                      "Mask", mask_str,
			                      "Gateway", gateway_str);
			if (js_subnet == NULL)
			{
				log_warn(PythonDebugHandlerLogLevel,
				         "Cannot allocate JSON for Subnet %s",
				         ip_str);
				continue;
			}
			json_array_append_new(js_subnets, js_subnet);
		}

		// Create the main object
		js_root = json_pack("{s:o}", "Subnets", js_subnets);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		if (js_root == NULL)
		{
			log_warn(PythonDebugHandlerLogLevel,
			         "json_pack (js_root) returns NULL");
			break;
		}
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);

	if (status != DOVE_STATUS_OK)
	{
		if (js_subnets)
		{
			json_decref(js_subnets);
		}
	}

	log_info(PythonDebugHandlerLogLevel, "Exit status:%s",
	         DOVEStatusToString((dove_status)status));

	return js_root;

}

/*
 ******************************************************************************
 * python_debug_handler_init --                                           *//**
 *
 * \brief This routine gets references to all functions in the PYTHON debug
 *        handler code (DpsDebugHandler) that are needed for processing
 *        requests received from various entities that are looking for debugging
 *        information.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_debug_handler_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDebugHandlerLogLevel, "Enter");

	memset(&Debug_Interface, 0, sizeof(python_dps_debug_t));

	do
	{
		// Get handle to an instance of DpsDebugHandler
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_DEBUG_HANDLER,
		                                 PYTHON_MODULE_CLASS_DEBUG_HANDLER,
		                                 pyargs,
		                                 &Debug_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Get_VNID_Endpoints
		Debug_Interface.Get_VNID_Endpoints =
			PyObject_GetAttrString(Debug_Interface.instance,
			                       PYTHON_GET_VNID_ENDPOINTS);
		if (Debug_Interface.Get_VNID_Endpoints == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_GET_VNID_ENDPOINTS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Get_VNID_DoveSwitches
		Debug_Interface.Get_VNID_DoveSwitches =
			PyObject_GetAttrString(Debug_Interface.instance,
			                       PYTHON_GET_VNID_DOVESWITCHES);
		if (Debug_Interface.Get_VNID_DoveSwitches == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_GET_VNID_DOVESWITCHES);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Get_Domain_VNID_Mapping
		Debug_Interface.Get_Domain_VNID_Mapping =
			PyObject_GetAttrString(Debug_Interface.instance,
			                       PYTHON_GET_DOMAIN_VNID_MAPPING);
		if (Debug_Interface.Get_Domain_VNID_Mapping == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_GET_DOMAIN_VNID_MAPPING);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Get_VNID_Allow_Policies
		Debug_Interface.Get_VNID_Allow_Policies =
			PyObject_GetAttrString(Debug_Interface.instance,
			                       PYTHON_GET_VNID_ALLOW_POLICIES);
		if (Debug_Interface.Get_VNID_Allow_Policies == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_GET_VNID_ALLOW_POLICIES);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Get_VNID_Subnets
		Debug_Interface.Get_VNID_Subnets =
			PyObject_GetAttrString(Debug_Interface.instance,
			                       PYTHON_GET_VNID_SUBNETS);
		if (Debug_Interface.Get_VNID_Subnets == NULL)
		{
			log_emergency(PythonDebugHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_GET_VNID_SUBNETS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}
		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonDebugHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

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

dove_status python_init_debug_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonDebugHandlerLogLevel, "Enter");

	do
	{
		status = python_debug_handler_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

	} while (0);

	log_info(PythonDebugHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));
	return status;
}

/** @} */
/** @} */
