/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Interface between the Controller and the python code  
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
*  $Log: controller_interface.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

/**
 * \brief Variable indicating that Controller Interface has started
 */
static int started = 0;

/**
 * \brief Contains the IP address of the DPS Cluster Leader
 */
ip_addr_t controller_location;

/**
 * \brief Whether the controller location was configured by user
 */
int controller_location_set = 0;

/**
 * \brief Contains the IP address of DPS Cluster Leader in Readable
 *        string format
 */
char controller_location_ip_string_storage[INET6_ADDRSTRLEN];
char *controller_location_ip_string = controller_location_ip_string_storage;

/**
 * \brief The module location defines for the DPS Controller Message Handling API
 */
#define PYTHON_MODULE_FILE_CONTROLLER_HANDLER "controller_protocol_handler"

/**
 * \brief The PYTHON Class that handles the DPS Controller Message Requests
 */
#define PYTHON_MODULE_CLASS_CONTROLLER_HANDLER "DpsControllerHandler"

/**
 * \brief The PYTHON function that handles Domain Add for domains handled
 *        by Local Node.
 */
#define PYTHON_FUNC_DOMAIN_ADD "Domain_Add_Local"

/**
 * \brief The PYTHON function that handles Domain Delete
 */
#define PYTHON_FUNC_DOMAIN_DELETE "Domain_Delete"

/**
 * \brief The PYTHON function that handles Domain_Get_Ready_For_Transfer
 */
#define PYTHON_FUNC_DOMAIN_GET_READY_FOR_TRANSFER "Domain_Get_Ready_For_Transfer"

/**
 * \brief The PYTHON function that handles Domain_Start_Transfer_To
 */
#define PYTHON_FUNC_DOMAIN_START_TRANSFER_TO "Domain_Start_Transfer_To"

/**
 * \brief The PYTHON function that handles Domain_Activate
 */
#define PYTHON_FUNC_DOMAIN_ACTIVATE "Domain_Activate"

/**
 * \brief The PYTHON function that handles Domain_Deactivate
 */
#define PYTHON_FUNC_DOMAIN_DEACTIVATE "Domain_Deactivate"

/**
 * \brief The PYTHON function that handles Domain_Update
 */
#define PYTHON_FUNC_DOMAIN_UPDATE "Domain_Update"

/**
 * \brief The PYTHON function that handles Domain Recovery Sequence
 */
#define PYTHON_FUNC_DOMAIN_RECOVERY_START "Domain_Recovery_Thread_Start"

/**
 * \brief The PYTHON function that handles DVG Add
 */
#define PYTHON_FUNC_DVG_ADD "Dvg_Add"

/**
 * \brief The PYTHON function that handles Dvg_Add_Query
 */
#define PYTHON_FUNC_DVG_ADD_QUERY "Dvg_Add_Query"

/**
 * \brief The PYTHON function that handles DVG Delete
 */
#define PYTHON_FUNC_DVG_DELETE "Dvg_Delete"

/**
 * \brief The PYTHON function that handles DVG Validate
 */
#define PYTHON_FUNC_DVG_VALIDATE "Dvg_Validate"

/**
 * \brief The PYTHON function that handles DVG GetAllIds
 */
#define PYTHON_FUNC_DVG_GETALLIDS "Dvg_GetAllIds"

/**
 * \brief The PYTHON function that handles Policy Add
 */
#define PYTHON_FUNC_POLICY_ADD "Policy_Add"

/**
 * \brief The PYTHON function that handles Policy Delete
 */
#define PYTHON_FUNC_POLICY_DELETE "Policy_Delete"

/**
 * \brief The PYTHON function that handles Policy Get
 */
#define PYTHON_FUNC_POLICY_GET "Policy_Get"

/**
 * \brief The PYTHON function that handles Policy Get All IDs
 */
#define PYTHON_FUNC_POLICY_GETALLIDS "Policy_GetAllIds"

/**
 * \brief The PYTHON function that handles Domain Show
 */
#define PYTHON_FUNC_DOMAIN_SHOW "Domain_Show_Local"

/**
 * \brief The PYTHON function that handles Domain Validation
 */
#define PYTHON_FUNC_DOMAIN_VALIDATE "Domain_Validate_Local"

/**
 * \brief The PYTHON function that Get all Domain IDs
 */
#define PYTHON_FUNC_GET_ALL_DOMAIN_IDS "Domain_GetAllIds_Local"

/**
 * \brief The PYTHON function that Show Global Domain Mapping
 */
#define PYTHON_FUNC_DOMAIN_GLOBAL_SHOW_MAPPING "Domain_Show_Global_Mapping"

/**
 * \brief The PYTHON function that Show Domain's Multicast Details
 */
#define PYTHON_FUNC_MULTICAST_SHOW "Multicast_Show"

/**
 * \brief The PYTHON function that Show Domain's Address Resolution Details
 */
#define PYTHON_FUNC_DOMAIN_ADDRESS_RESOLUTION_SHOW "Domain_Show_Address_Resolution"

/**
 * \brief The PYTHON function that Adds an External Gateway
 */
#define PYTHON_FUNC_EXTERNAL_GATEWAY_ADD "External_Gateway_Add"

/**
 * \brief The PYTHON function that Deletes an External Gateway
 */
#define PYTHON_FUNC_EXTERNAL_GATEWAY_DELETE "External_Gateway_Delete"

/**
 * \brief The PYTHON function that Clears all External Gateways
 */
#define PYTHON_FUNC_EXTERNAL_GATEWAY_CLEAR "External_Gateway_Clear"

/**
 * \brief The PYTHON function that handles gateway Validate
 */
#define PYTHON_FUNC_EXTERNAL_GATEWAY_VALIDATE "External_Gateway_Validate"

/**
 * \brief The PYTHON function that Get all External Gateways
 */
#define PYTHON_FUNC_EXTERNAL_GATEWAY_GETALLIDS "External_Gateway_GetAllIds"

/**
 * \brief The PYTHON function to add the Controller Location
 */
#define PYTHON_FUNC_CONTROLLER_LOCATION_UPDATE "Controller_Location_Update"

/**
 * \brief The PYTHON function to get the Controller Location
 */
#define PYTHON_FUNC_CONTROLLER_LOCATION_GET "Controller_Location_Get"

/**
 * \brief The PYTHON function that handles IP Subnet Add
 */
#define PYTHON_FUNC_IP_SUBNET_ADD "IP_Subnet_Add"

/**
 * \brief The PYTHON function that handles IP Subnet Delete
 */
#define PYTHON_FUNC_IP_SUBNET_DELETE "IP_Subnet_Delete"

/**
 * \brief The PYTHON function that handles IP Subnet Get
 */
#define PYTHON_FUNC_IP_SUBNET_GET "IP_Subnet_Get"

/**
 * \brief The PYTHON function that handles IP Subnet List
 */
#define PYTHON_FUNC_IP_SUBNET_LIST "IP_Subnet_List"

/**
 * \brief The PYTHON function that handles IP Subnet Flush
 */
#define PYTHON_FUNC_IP_SUBNET_FLUSH "IP_Subnet_Flush"

/**
 * \brief The PYTHON function that handles IP Subnet GetAllIds
 */
#define PYTHON_FUNC_IP_SUBNET_GETALLIDS "IP_Subnet_GetAllIds"

/**
 * \brief The PYTHON function that handles DVG Show
 */
#define PYTHON_FUNC_DVG_SHOW "Dvg_Show"

/**
 * \brief The PYTHON function that deletes all Local Domains
 */
#define PYTHON_FUNC_DOMAIN_DELETE_ALL_LOCAL "Domain_Delete_All_Local"

/**
 * \brief The PYTHON function that Stops the Controller Handler
 */
#define PYTHON_FUNC_CONTROLLER_FUNCTION_STOP "Stop"

/**
 * \brief The PYTHON function that Starts the Controller Handler
 */
#define PYTHON_FUNC_CONTROLLER_FUNCTION_START "Start"

/**
 * \brief The PYTHON function for DPSClientsShow
 */
#define PYTHON_FUNC_DPS_CLIENTS_SHOW "DPSClientsShow"

/**
 * \brief The DPS controller handler function pointers data structure
 */

typedef struct python_dps_controller_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function for Domain_Add_Local
	 */
	PyObject *Domain_Add_Local;
	/*
	 * \brief The function for Domain_Delete
	 */
	PyObject *Domain_Delete;
	/**
	 * \brief The PYTHON function that handles Domain_Get_Ready_For_Transfer
	 */
	PyObject *Domain_Get_Ready_For_Transfer;
	/**
	 * \brief The PYTHON function that handles Domain_Start_Transfer_To
	 */
	PyObject *Domain_Start_Transfer_To;
	/**
	 * \brief The PYTHON function that handles Domain_Activate
	 */
	PyObject *Domain_Activate;
	/**
	 * \brief The PYTHON function that handles Domain_Deactivate
	 */
	PyObject *Domain_Deactivate;
	/**
	 * \brief The PYTHON function that handles Domain_Update
	 */
	PyObject *Domain_Update;
	/*
	 * \brief The function for Dvg_Add
	 */
	PyObject *Dvg_Add;
	/*
	 * \brief The function for Dvg_Add_Query
	 */
	PyObject *Dvg_Add_Query;
	/*
	 * \brief The function for Dvg_Delete
	 */
	PyObject *Dvg_Delete;
	/*
	 * \brief The function for Dvg_Validate
	 */
	PyObject *Dvg_Validate;
	/*
	 * \brief The function for Dvg_GetAllIds
	 */
	PyObject *Dvg_GetAllIds;
	/*
	 * \brief The function for Policy_Add
	 */
	PyObject *Policy_Add;
	/*
	 * \brief The function for Policy_Delete
	 */
	PyObject *Policy_Delete;
	/*
	 * \brief The function for Policy_Get
	 */
	PyObject *Policy_Get;
	/*
	 * \brief The function for Policy_GetAllIDs
	 */
	PyObject *Policy_GetAllIds;
	/*
	 * \brief The function for Domain_Show_Local
	 */
	PyObject *Domain_Show_Local;
	/**
	 * \brief The function for Domain_Show_Global_Mapping
	 */
	PyObject *Domain_Show_Global_Mapping;
	/*
	 * \brief The function for Domain_Validate_Local
	 */
	PyObject *Domain_Validate_Local;
	/*
	 * \brief The function for Domain_GetAllIds_Local
	 */
	PyObject *Domain_GetAllIds_Local;
	/**
	 * \brief The function for Domain_Recovery_Thread_Start
	 */
	PyObject *Domain_Recovery_Thread_Start;
	/*
	 * \brief The function for External_Gateway_Add
	 */
	PyObject *External_Gateway_Add;
	/*
	 * \brief The function for External_Gateway_Delete
	 */
	PyObject *External_Gateway_Delete;
	/*
	 * \brief The function for External_Gateway_Clear
	 */
	PyObject *External_Gateway_Clear;
	/*
	 * \brief The function for External_Gateway_Validate
	 */
	PyObject *External_Gateway_Validate;
	/*
	 * \brief The function for External_Gateway_GetAll
	 */
	PyObject *External_Gateway_GetAllIds;
	/*
	 * \brief The function for Implicit_Gateway_Add
	 */
	PyObject *Implicit_Gateway_Add;
	/*
	 * \brief The function for Implicit_Gateway_Delete
	 */
	PyObject *Implicit_Gateway_Delete;
	/*
	 * \brief The function for Implicit_Gateway_Clear
	 */
	PyObject *Implicit_Gateway_Clear;
	/*
	 * \brief The function for Implicit_Gateway_Validate
	 */
	PyObject *Implicit_Gateway_Validate;
	/*
	 * \brief The function for Implicit_Gateway_GetAll
	 */
	PyObject *Implicit_Gateway_GetAllIds;
	/**
	 * \brief The function to update Controller Location
	 */
	PyObject *Controller_Location_Update;
	/**
	 * \brief The function to show Controller Location
	 */
	PyObject *Controller_Location_Get;
	/*
	 * \brief The function for IP_Subnet_Add
	 */
	PyObject *IP_Subnet_Add;
	/*
	 * \brief The function for IP_Subnet_Delete
	 */
	PyObject *IP_Subnet_Delete;
	/*
	 * \brief The function for IP_Subnet_Get
	 */
	PyObject *IP_Subnet_Get;
	/*
	 * \brief The function for IP_Subnet_List
	 */
	PyObject *IP_Subnet_List;
	/*
	 * \brief The function for IP_Subnet_Flush
	 */
	PyObject *IP_Subnet_Flush;
	/*
	 * \brief The function for IP_Subnet_GetAllIds
	 */
	PyObject *IP_Subnet_GetAllIds;
	/*
	 * \brief The function for Dvg_Show
	 */
	PyObject *Dvg_Show;
	/**
	 * \brief Domain's Multicast Details
	 */
	PyObject *Multicast_Show;
	/**
	 * \brief Domain_Show_Address_Resolution
	 */
	PyObject *Domain_Show_Address_Resolution;
	/**
	 * \brief Domain_Delete_All_Local
	 */
	PyObject *Domain_Delete_All_Local;
	/**
	 * \brief Domain_Show_Address_Resolution
	 */
	PyObject *Stop;
	/**
	 * \brief Domain_Delete_All_Local
	 */
	PyObject *Start;
	/**
	 * \brief DPSClientsShow
	 */
	PyObject *DPSClientsShow;
}python_dps_controller_t;

/*
 * \brief The DPS Controller Message Handler PYTHON Interface (Embed)
 */
static python_dps_controller_t Controller_Interface;

/*
 * \brief Function Handler for each DPS Message Type
 */

typedef dove_status (*dps_controller_func_handler)(dps_controller_data_op_t *data);

/**
 * \brief An Array of Callback for every DPS Controller Object Operation Code
 */

static dps_controller_func_handler function_array[DPS_CONTROLLER_OP_MAX];

/*
 ******************************************************************************
 * DPS Object Interface                                                   *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSControllerInterface DPS Client Server Protocol Interface
 * @{
 * Handles Interaction between the DPS Client Server Protocol and DPS (PYTHON)
 * Objects
 ******************************************************************************
 */

/*
 ******************************************************************************
 * domain_add --                                                          *//**
 *
 * \brief This routine handles Domain Add
 *
 * \param data The Structure of the Message for Domain Add
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_add(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	uint32_t replication_factor;
	PyObject *strret, *strargs, *PyFunction;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel,
	         "Enter: Domain %d, Replication Factor %d",
	         data->domain_add.domain_id,
	         data->domain_add.replication_factor);

	PyFunction = NULL;

	do
	{
		replication_factor = (data->domain_add.replication_factor > MAX_REPLICATION_FACTOR) ?
		                     MAX_REPLICATION_FACTOR:
		                     data->domain_add.replication_factor;
		replication_factor = replication_factor < MIN_REPLICATION_FACTOR ?
		                     MIN_REPLICATION_FACTOR:
		                     replication_factor;
		if (data->type == DPS_CONTROLLER_DOMAIN_ADD)
		{
			log_info(PythonDataHandlerLogLevel, "Domain Add %d",
			         data->domain_add.domain_id);
			PyFunction = Controller_Interface.Domain_Add_Local;
		}
		else if(data->type == DPS_CONTROLLER_DOMAIN_ACTIVATE)
		{
			log_info(PythonDataHandlerLogLevel, "Domain Activate %d",
			         data->domain_add.domain_id);
			PyFunction = Controller_Interface.Domain_Activate;
		}
		if (PyFunction == NULL)
		{
			break;
		}

		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//def Domain_Add_Local(self, domain_id):
		strargs = Py_BuildValue("(iI)",
		                        data->domain_add.domain_id,
		                        replication_factor);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_add.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Add: No resources");
			break;
		}

		// Invoke the Domain_Add_Local call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject Domain_Add_Local : Domain %d, returns NULL",
			          data->domain_add.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Add: No resources");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);

		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		if (SHARED_ADDR_SPACE_DOMAIN_ID != data->domain_add.domain_id)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Domain %d: Added/Activated on local node, replication factor %d",
			           data->domain_add.domain_id, replication_factor);
		}
		else
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Domain [Shared]: Added/Activated on local node, replication factor %d",
			           replication_factor);
		}

		// Add to Cluster collection
		status = dps_cluster_node_add_domain(&dps_local_ip,
		                                     data->domain_add.domain_id,
		                                     replication_factor);
		if (status != DOVE_STATUS_OK)
		{
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Cannot add Domain %d to local DPS node in Cluster Database",
			                   data->domain_add.domain_id);
			break;
		}

		// Exchange local domain mapping with other nodes
		dps_cluster_exchange_domain_mapping();
	}while(0);

	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_delete --                                                       *//**
 *
 * \brief This routine handles Domain Delete
 *
 * \param data The Structure of the Message for Domain Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_delete(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs, *PyFunction;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_delete.domain_id);

	do
	{
		if (data->type == DPS_CONTROLLER_DOMAIN_DELETE)
		{
			PyFunction = Controller_Interface.Domain_Delete;
		}
		else if(data->type == DPS_CONTROLLER_DOMAIN_DEACTIVATE)
		{
			PyFunction = Controller_Interface.Domain_Deactivate;
		}
		else
		{
			PyFunction = NULL;
			break;
		}

		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//def Domain_Delete(self, domain_id):
		strargs = Py_BuildValue("(i)", data->domain_delete.domain_id);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_add.domain_id);
		}

		// Invoke the Domain_Delete call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject: Domain Delete/Deactivate Domain %d, returns NULL",
			          data->domain_add.domain_id);
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);

		if (status == DOVE_STATUS_OK)
		{
			if (SHARED_ADDR_SPACE_DOMAIN_ID != data->domain_delete.domain_id)
			{
				log_notice(PythonDataHandlerLogLevel,
				           "Domain %d deleted/deactivated",
				           data->domain_delete.domain_id);
			}
			// Delete from Cluster collection
			status = dps_cluster_node_delete_domain(&dps_local_ip,
			                                        data->domain_delete.domain_id);
			if (status != DOVE_STATUS_OK)
			{
				customer_log_warn(PythonDataHandlerLogLevel,
				                  "Cannot remove Domain %d from Local Node in Cluster Database",
				                  data->domain_delete.domain_id);
			}
			// Remove domain from the Cluster
			if (data->type == DPS_CONTROLLER_DOMAIN_DELETE)
			{
				dps_cluster_domain_delete(data->domain_delete.domain_id);
			}
			// Exchange local domain mapping with other nodes
			dps_cluster_exchange_domain_mapping();
		}
	}while(0);

	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domains_clear --                                                       *//**
 *
 * \brief This routine handles Domain Delete All Local
 *
 * \param data The Structure of the Message for Domain Delete All
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domains_clear(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Delete(self, domain_id):
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue: returns NULL");
		}
		// Invoke the Domain_Delete call
		PyEval_CallObject(Controller_Interface.Domain_Delete_All_Local, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_mass_transfer_get_ready --                                      *//**
 *
 * \brief This routine primes the local DPS Node to handle mass transfer of
 *        Domain Data from another node
 *
 * \param data The Structure of the Message for domain_mass_transfer_get_ready
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_mass_transfer_get_ready(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_mass_transfer_get_ready.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def def Domain_Get_Ready_For_Transfer(self, domain_id):
		strargs = Py_BuildValue("(i)", data->domain_mass_transfer_get_ready.domain_id);
		if (strargs == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_mass_transfer_get_ready.domain_id);
			break;
		}

		// Invoke the Domain_Add_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Get_Ready_For_Transfer,
		                           strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject Domain_Get_Ready_For_Transfer : Domain %d, returns NULL",
			          data->domain_mass_transfer_get_ready.domain_id);
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

	}while(0);
	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_mass_transfer_start --                                          *//**
 *
 * \brief This routine starts the mass transfer of Domain to another DPS Node
 *
 * \param data The Structure of the Message for dps_object_mass_transfer_start_t
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_mass_transfer_start(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_mass_transfer_start.domain_id);

	do
	{
		// Get the DPS Node IP
		status = dps_cluster_node_get_port(&data->domain_mass_transfer_start.dps_server);
		if (status != DOVE_STATUS_OK)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Domain %d, Mass Transfer Start, cannot get Node port",
			         data->domain_mass_transfer_start.domain_id);
			break;
		}
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//def def Domain_Get_Ready_For_Transfer(self, domain_id):
		strargs = Py_BuildValue("(iIz#H)",
		                        data->domain_mass_transfer_start.domain_id,
		                        data->domain_mass_transfer_start.dps_server.family,
		                        data->domain_mass_transfer_start.dps_server.ip6, 16,
		                        data->domain_mass_transfer_start.dps_server.port);
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_mass_transfer_start.domain_id);
			break;
		}

		// Invoke the Domain_Add_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Start_Transfer_To,
		                           strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject Domain_Start_Transfer_To : Domain %d, returns NULL",
			          data->domain_mass_transfer_start.domain_id);
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);

	}while(0);

	log_info(PythonDataHandlerLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_show --                                                         *//**
 *
 * \brief This routine handles Domain Show
 *
 * \param data The Structure of the Message for Domain Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_show.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Show_Local(self, domain_id):
		strargs = Py_BuildValue("(ii)",
		                        data->domain_show.domain_id,
		                        data->domain_show.fDetails);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Show_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Show_Local, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_Show_Local returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * multicast_show --                                               *//**
 *
 * \brief This routine handles Multicast Show for Domain or VNID
 *
 * \param data The Structure of the Message for Domain Multicast Show
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status multicast_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_add.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Show_Local(self, domain_id):
		strargs = Py_BuildValue("(ii)",
		                        data->multicast_show.associated_type,
		                        data->multicast_show.associated_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Show_Local call
		strret = PyEval_CallObject(Controller_Interface.Multicast_Show, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Multicast_Show returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_address_resolution_show --                                      *//**
 *
 * \brief This routine shows the Address Resolution waiters in the Domain
 *
 * \param data The Structure of the Message for Domain Address Resolution Show
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_address_resolution_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_add.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Show_Local(self, domain_id):
		strargs = Py_BuildValue("(i)", data->domain_add.domain_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Show_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Show_Address_Resolution, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_Show_Address_Resolution returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_global_show --                                                  *//**
 *
 * \brief This routine handles Domain Show
 *
 * \param data The Structure of the Message for Domain Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_global_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret;
	PyObject *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Show_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Show_Global_Mapping, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_Show_Global_Mapping returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_validate --                                                         *//**
 *
 * \brief This routine validate the Domain ID
 *
 * \param data The Structure of the Message for Domain to validate
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_validate(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->domain_validate.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		//def Domain_Validate_Local(self, domain_id):
		strargs = Py_BuildValue("(i)", data->domain_validate.domain_id);
		if (strargs == NULL)
		{
			break;
		}

		// Invoke the Domain_Validate_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Validate_Local, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_Validate_Local returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_update --                                                       *//**
 *
 * \brief This routine handles Domain Update
 *
 * \param data The Structure of the Message for Domain Update
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_update(dps_controller_data_op_t *data)
{
	dove_status status = DOVE_STATUS_NO_MEMORY;
	uint32_t replication_factor;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel,
	         "Enter: Domain %d, Replication Factor %d",
	         data->domain_add.domain_id,
	         data->domain_add.replication_factor);

	do
	{
		replication_factor = (data->domain_add.replication_factor > MAX_REPLICATION_FACTOR) ?
		                     MAX_REPLICATION_FACTOR:
		                     data->domain_add.replication_factor;
		replication_factor = replication_factor < MIN_REPLICATION_FACTOR ?
		                     MIN_REPLICATION_FACTOR:
		                     replication_factor;
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		//def Domain_Add_Local(self, domain_id):
		strargs = Py_BuildValue("(iI)",
		                        data->domain_add.domain_id,
		                        replication_factor
		                        );
		if (strargs == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_add.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Update: No resources");
			break;
		}

		// Invoke the Domain_Add_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Update, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			PyGILState_Release(gstate);
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject Update : Domain %d, returns NULL",
			          data->domain_add.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Update: No resources");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		PyGILState_Release(gstate);

		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		if (SHARED_ADDR_SPACE_DOMAIN_ID != data->domain_add.domain_id)
		{
			log_notice(PythonDataHandlerLogLevel,
			           "Domain %d updated on local node, replication factor %d",
			           data->domain_add.domain_id, replication_factor);
		}

		// Add to Cluster collection
		status = dps_cluster_node_add_domain(&dps_local_ip,
		                                     data->domain_add.domain_id,
		                                     replication_factor);
		if (status != DOVE_STATUS_OK)
		{
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Cannot update Domain %d to local DPS node in Cluster Database",
			                   data->domain_add.domain_id);
			break;
		}

		// Exchange local domain mapping with other nodes
		dps_cluster_exchange_domain_mapping();
	}while(0);

	log_info(PythonDataHandlerLogLevel,  "Exit: %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * domain_recovery_start --                                               *//**
 *
 * \brief This routine starts the Domain Recovery Thread
 *
 * \param data The Structure of the Message for Domain Update
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_recovery_start(dps_controller_data_op_t *data)
{
	dove_status status = DOVE_STATUS_NO_MEMORY;
	uint32_t replication_factor;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel,
	         "Enter: Domain %d, Replication Factor %d",
	         data->domain_recover.domain_id,
	         data->domain_recover.replication_factor);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		replication_factor = (data->domain_recover.replication_factor > MAX_REPLICATION_FACTOR) ?
		                     MAX_REPLICATION_FACTOR:
		                     data->domain_recover.replication_factor;
		replication_factor = replication_factor < MIN_REPLICATION_FACTOR ?
		                     MIN_REPLICATION_FACTOR:
		                     replication_factor;
		log_notice(PythonDataHandlerLogLevel,
		           "Recovering Domain %d: Replication %d",
		           data->domain_recover.domain_id,
		           replication_factor);
		//def Domain_Add_Local(self, domain_id):
		strargs = Py_BuildValue("(II)",
		                        data->domain_add.domain_id,
		                        replication_factor
		                        );
		if (strargs == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Py_BuildValue: Domain %d, returns NULL",
			          data->domain_recover.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Recover: No resources");
			break;
		}

		// Invoke the Domain_Add_Local call
		strret = PyEval_CallObject(Controller_Interface.Domain_Recovery_Thread_Start,
		                           strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "PyEval_CallObject Domain_Recovery_Thread_Start : Domain %d, returns NULL",
			          data->domain_recover.domain_id);
			customer_log_alert(PythonDataHandlerLogLevel,
			                   "Domain Recover: No resources");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel,  "Exit: %s", DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * domain_getallids --                                                         *//**
 *
 * \brief This routine get all Domain IDs
 *
 * \param data The Structure of dps_controller_data_op_t
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_getallids(dps_controller_data_op_t *data)
{
	char *return_val = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	dove_status status = DOVE_STATUS_NO_MEMORY;

	// Ensure the PYTHON Global Interpreter Lock

	log_info(PythonDataHandlerLogLevel, "Enter");

	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_GetAllIds_Local(self):
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_GetaAllIds
		strret = PyEval_CallObject(Controller_Interface.Domain_GetAllIds_Local, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_GetAllIds_Local returns NULL");
			break;
		}
		//@return: The string of all Domain IDs
		//@rtype: String
		PyArg_Parse(strret, "z", &return_val);
		data->return_val = strdup(return_val);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		status = DOVE_STATUS_OK;
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return status;
}
/*
 ******************************************************************************
 * dvg_add --                                                             *//**
 *
 * \brief This routine handles DVG Add
 *
 * \param data The Structure of the Message for DVG Add
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_add(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs, *PyFunction;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter Domain %d, DVG %d",
	         data->dvg_add.domain_id,
	         data->dvg_add.dvg_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		PyFunction = NULL;
		if (data->type == DPS_CONTROLLER_DVG_ADD)
		{
			log_info(PythonDataHandlerLogLevel, "DVG Add");
			PyFunction = Controller_Interface.Dvg_Add;
		}
		else if(data->type == DPS_CONTROLLER_DVG_ADD_QUERY)
		{
			log_info(PythonDataHandlerLogLevel, "DVG Add Query");
			PyFunction = Controller_Interface.Dvg_Add_Query;
		}
		if (PyFunction == NULL)
		{
			break;
		}
		//def Dvg_Add(self, domain_id, dvg_id):
		strargs = Py_BuildValue("(ii)",
		                        data->dvg_add.domain_id,
		                        data->dvg_add.dvg_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Dvg_Add call
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_Add/Query returns NULL");
			break;
		}
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dvg_delete --                                                          *//**
 *
 * \brief This routine handles DVG Delete
 *
 * \param data The Structure of the Message for DVG Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_delete(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter DVG %d", data->dvg_delete.dvg_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Dvg_Delete(self, domain_id, dvg_id):
		strargs = Py_BuildValue("(i)",
		                        data->dvg_delete.dvg_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Dvg_Delete call
		strret = PyEval_CallObject(Controller_Interface.Dvg_Delete, strargs);
		Py_DECREF(strargs);
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		if(strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_Delete returns NULL");
			break;
		}
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/******************************************************************************
 * dvg_validate --                          *//**
 *
 * \brief This routine validate a DVG ID
 *
 * \param data The Structure of the Message for DVG validate
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_validate(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter DVG %d", data->dvg_validate.dvg_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def DVG_Validate(self, domain_id, dvg_id):
		strargs = Py_BuildValue("(ii)",
		                        data->dvg_validate.domain_id,
		                        data->dvg_validate.dvg_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Dvg_Validate call
		strret = PyEval_CallObject(Controller_Interface.Dvg_Validate, strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_Validate returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}


/******************************************************************************
 * dvg_getallids --                                         *//**
 *
 * \brief This routine get all DVG IDs in specified domain
 *
 * \param data The Structure of the Message for DVG get all ids
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_getallids(dps_controller_data_op_t *data)
{
	dove_status status = DOVE_STATUS_NO_MEMORY;
	char *return_val = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter Domain %d",
	         data->dvg_getallids.domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		//def DVG_GetAllIds(self, domain_id):
		strargs = Py_BuildValue("(i)",data->dvg_getallids.domain_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the DVG_GetAllIds call
		strret = PyEval_CallObject(Controller_Interface.Dvg_GetAllIds, strargs);
		Py_DECREF(strargs);

		if(strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_GetAllIds returns NULL");
			break;
		}
		//@return: The string of all DVG IDs
		//@rtype: String
		PyArg_Parse(strret, "z", &return_val);
		data->return_val = strdup(return_val);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		status = DOVE_STATUS_OK;
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString(status));

	return status;
}


/*
 ******************************************************************************
 * show_vnid_info --                                                      *//**
 *
 * \brief This routine handles DVG Show
 *
 * \param VNID
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status show_vnid_info(int vnid)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: VNID %d",
	         vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Show_Local(self, domain_id):
		strargs = Py_BuildValue("(ii)",vnid,1);
		if(strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Dvg_Show call
		strret = PyEval_CallObject(Controller_Interface.Dvg_Show, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_Show returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}




/*
 ******************************************************************************
 * dvg_show --                                                         *//**
 *
 * \brief This routine handles DVG Show
 *
 * \param data The Structure of the Message for DVG Show
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: VNID %d",
	         data->vnid_show.vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Show_Local(self, domain_id):
		strargs = Py_BuildValue("(ii)",
		                        data->vnid_show.vnid,
		                        data->vnid_show.fDetails);
		if(strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Dvg_Show call
		strret = PyEval_CallObject(Controller_Interface.Dvg_Show, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Dvg_Show returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}


/*
 ******************************************************************************
 * dvg_query --                                                         *//**
 *
 * \brief This routine handles Query DVG(VNID) request made to Dove Controller
 *
 * \param data The Structure of the Message for Query DVG(VNID)
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dvg_query(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;

	log_info(PythonDataHandlerLogLevel, "Enter: VNID %d", data->query_vnid.vnid);
	dps_rest_client_query_dove_controller_vnid_info(data->query_vnid.vnid);
	log_info(PythonDataHandlerLogLevel, "Exit");
	return (dove_status)status;
}

/*
 ******************************************************************************
 * policy_add --                                                             *//**
 *
 * \brief This routine handles Policy Add
 *
 * \param data The Structure of the Message for Policy Add
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status policy_add(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Policy_Add(self, traffic_type, domain_id, type, src_dvg_id, dst_dvg_id, ttl, version, action):
		strargs = Py_BuildValue("(iiiiiiz#)",
		                        data->policy_add.traffic_type,
		                        data->policy_add.domain_id,
		                        data->policy_add.type,
		                        data->policy_add.src_dvg_id,
		                        data->policy_add.dst_dvg_id,
		                        data->policy_add.ttl,
		                        (char *)&data->policy_add.action,
		                        sizeof(data->policy_add.action));
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Policy_Add call
		strret = PyEval_CallObject(Controller_Interface.Policy_Add, strargs);
		Py_DECREF(strargs);

		if(strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Policy_Add returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * policy_delete --                                                          *//**
 *
 * \brief This routine handles Policy Delete
 *
 * \param data The Structure of the Message for Policy Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status policy_delete(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Policy_Delete(self, traffic_type, domain_id, src_dvg_id, dst_dvg_id):
		strargs = Py_BuildValue("(iiii)",
		                        data->policy_delete.traffic_type,
		                        data->policy_delete.domain_id,
		                        data->policy_delete.src_dvg_id,
		                        data->policy_delete.dst_dvg_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Policy_Delete call
		strret = PyEval_CallObject(Controller_Interface.Policy_Delete, strargs);
		Py_DECREF(strargs);
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer

		if(strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Policy_Delete returns NULL");
			break;
		}
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * policy_get --                                                             *//**
 *
 * \brief This routine handles Policy Get
 *
 * \param data Reuse the Structure of the Message for Policy Add
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status policy_get(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	char *pAction;
	int size;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Policy_Get(self, traffic_type, domain_id, src_dvg_id, dst_dvg_id):
		strargs = Py_BuildValue("(iiii)",
		                        data->policy_add.traffic_type,
		                        data->policy_add.domain_id,
		                        data->policy_add.src_dvg_id,
		                        data->policy_add.dst_dvg_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Policy_Get call
		strret = PyEval_CallObject(Controller_Interface.Policy_Get, strargs);
		Py_DECREF(strargs);

		//@return: (DOVEStatus.DOVE_STATUS_OK, policy_obj.type,
		//          policy_obj.ttl, policy_obj.action, policy_obj.version)
		//@rtype: Integer, Integer, Integer, ByteArray
		if (NULL == strret)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Policy_Get returns NULL");
			break;
		}
		PyArg_ParseTuple(strret, "iiiz#i",
		                 &status,
		                 &(data->policy_add.type),
		                 &(data->policy_add.ttl),
		                 &pAction,
		                 &size,
		                 &(data->policy_add.version));
		if(DOVE_STATUS_OK == status)
		{
			memcpy(&(data->policy_add.action), pAction, sizeof(data->policy_add.action));
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * policy_getallids --                                                         *//**
 *
 * \brief This routine get all Policy IDs of specified domain
 *
 * \param data The Structure of dps_controller_data_op_t
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status policy_getallids(dps_controller_data_op_t *data)
{
	char *return_val = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	dove_status status = DOVE_STATUS_NO_MEMORY;

	log_info(PythonDataHandlerLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Policy_GetAllIds(self, traffic_type, domain_id):
		strargs = Py_BuildValue("(ii)",
		                        data->policy_add.traffic_type,
		                        data->policy_add.domain_id);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_GetaAllIds
		strret = PyEval_CallObject(Controller_Interface.Policy_GetAllIds, strargs);
		Py_DECREF(strargs);

		if (NULL == strret)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Policy_GetAllIds returns NULL");
			break;
		}
		//@return: The string of all Domain IDs
		//@rtype: String
		PyArg_Parse(strret, "z", &return_val);
		data->return_val = strdup(return_val);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

		status = DOVE_STATUS_OK;
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString(status));

	return status;
}
/*
 ******************************************************************************
 * gateway_update --                                                      *//**
 *
 * \brief This routine handles updating Gateways
 *
 * \param data The Structure of the Message for Updating Gateways
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status gateway_update(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret = NULL;
	PyObject *strargs = NULL;
	PyObject *PyFunction = NULL;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: VNID %d",
	         data->gateway_update.vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// Invoke the Correct call
		switch(data->type)
		{
			case DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD:
				PyFunction = Controller_Interface.External_Gateway_Add;
				break;
			case DPS_CONTROLLER_EXTERNAL_GATEWAY_DEL:
				PyFunction = Controller_Interface.External_Gateway_Delete;
				break;
			default:
				break;
		}

		if (PyFunction == NULL)
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(data->gateway_update.IP_type, data->gateway_update.IPv6, str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "Gateway_Update: %s", str);
		}
#endif
		if (data->gateway_update.IP_type == AF_INET)
		{
			//def External_Gateway_Add(self, vnid, IP_type, IP_packed):
			strargs = Py_BuildValue("(III)",
			                        data->gateway_update.vnid,
			                        AF_INET,
			                        data->gateway_update.IPv4);
		}
		else
		{
			//def External_Gateway_Add(self, vnid, IP_type, IP_packed):
			strargs = Py_BuildValue("(IIz#)",
			                        data->gateway_update.vnid,
			                        AF_INET6,
			                        data->gateway_update.IPv6,
			                        16);
		}
		if(strargs == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Py_BuildValue External_Gateway_X returns NULL");
			break;
		}

		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if (NULL == strret)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject External_Gateway_X returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		Py_DECREF(strret);
	}while(0);
	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * gateway_clear --                                                      *//**
 *
 * \brief This routine handles clearing Gateways
 *
 * \param data The Structure of the Message for Clearing Gateways
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status gateway_clear(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret = NULL;
	PyObject *strargs = NULL;
	PyObject *PyFunction = NULL;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter: VNID %d",
	         data->gateway_clear.dvg_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		// Invoke the Correct call
		switch(data->type)
		{
			case DPS_CONTROLLER_EXTERNAL_GATEWAY_CLR:
				PyFunction = Controller_Interface.External_Gateway_Clear;
				break;
			default:
				break;
		}
		if (PyFunction == NULL)
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}

		//def External_Gateway_Clear(self, vnid):
		strargs = Py_BuildValue("(I)",data->gateway_update.vnid);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if (NULL == strret)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject External_Gateway_Clear returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		Py_DECREF(strret);

	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/******************************************************************************
 * gateway_validate --                          *//**
 *
 * \brief This routine validate a gateway ID
 *
 * \param data The Structure of the Message for gateway validate
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status gateway_validate(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret = NULL;
	PyObject *strargs = NULL;
	PyGILState_STATE gstate;
	PyObject *PyFunction = NULL;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->gateway_update.vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// Invoke the Correct call
		switch(data->type)
		{
			case DPS_CONTROLLER_EXTERNAL_GATEWAY_VALIDATE:
				PyFunction = Controller_Interface.External_Gateway_Validate;
				break;
			default:
				break;
		}
		if (PyFunction == NULL)
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}

		//def DVG_Validate(self, domain_id, dvg_id):
		strargs = Py_BuildValue("(III)",
		                        data->gateway_update.vnid,
		                        data->gateway_update.IP_type,
		                        data->gateway_update.IPv4);
		if(strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);
		if (NULL == strret)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject External_Gateway_Validate returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

static dove_status gateway_getallids(dps_controller_data_op_t *data)
{
	dove_status status = DOVE_STATUS_NO_MEMORY;
	char *return_val = NULL;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	PyObject *PyFunction = NULL;

	log_info(PythonDataHandlerLogLevel, "Enter: Domain %d",
	         data->gateway_update.vnid);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// Invoke the Correct call
		switch(data->type)
		{
			case DPS_CONTROLLER_EXTERNAL_GATEWAY_GETALLIDS:
				PyFunction = Controller_Interface.External_Gateway_GetAllIds;
				break;
			default:
				break;
		}
		if (PyFunction == NULL)
		{
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
		strargs = Py_BuildValue("(ii)",
		                        data->gateway_update.vnid,
		                        data->gateway_update.IP_type);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		strret = PyEval_CallObject(PyFunction, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "PyEval_CallObject External_Gateway_GetAllIds returns NULL");
			break;
		}
		//@return: The string of all Gateway IPs
		//@rtype: String
		PyArg_Parse(strret, "z", &return_val);
		data->return_val = strdup(return_val);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		status = DOVE_STATUS_OK;
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * controller_location_update                                             *//**
 *
 * \brief - This function is used to add the Controller's Location to the
 *          collection
 *
 * \param controller_ip        The IP Address and Port of the Controller
 *
 * \retval DOVE_STATUS_OK Controller IP was added
 * \retval DOVE_STATUS_NO_RESOURCES No resources to add Controller IP
 *
 ******************************************************************************
 */
static dove_status controller_location_update(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	ip_addr_t *controller_ip = &data->controller_location;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	//Copy into C structure
	memcpy(&controller_location, controller_ip, sizeof(ip_addr_t));
	inet_ntop(controller_ip->family, controller_ip->ip6,
	          controller_location_ip_string, INET6_ADDRSTRLEN);
	log_info(PythonDataHandlerLogLevel, "Controller IP %s, Port %d",
	         controller_location_ip_string, controller_location.port);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("(Iz#H)",
		                        controller_ip->family,
		                        controller_ip->ip6, 16,
		                        controller_ip->port_http);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		//def Controller_Location_Update(self, ip_type, ip_val, port):
		// Invoke the Controller_Location_Update call
		strret = PyEval_CallObject(Controller_Interface.Controller_Location_Update, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "PyEval_CallObject Controller_Location_Update returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		if (status == DOVE_STATUS_OK)
		{
			controller_location_set = 1;
		}
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * controller_location_delete                                             *//**
 *
 * \brief - This function is used to delete the Controller's Location from the
 *          collection. This routine sets the local node's IP as the controller
 *          IP.
 *
 * \param controller_ip        The IP Address and Port of the Controller
 *
 * \retval DOVE_STATUS_OK Controller IP was added
 * \retval DOVE_STATUS_NO_RESOURCES No resources to add Controller IP
 *
 ******************************************************************************
 */
static dove_status controller_location_delete(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	//Copy local IP into C structure
	memcpy(&controller_location, &dps_local_ip, sizeof(ip_addr_t));
	//Unset the controller location set flag
	controller_location_set = 0;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("(Iz#H)",
		                        controller_location.family,
		                        controller_location.ip6, 16,
		                        controller_location.port_http);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		//def Controller_Location_Update(self, ip_type, ip_val, port):
		// Invoke the Controller_Location_Update call
		strret = PyEval_CallObject(Controller_Interface.Controller_Location_Update, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "PyEval_CallObject Controller_Location_Update returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * controller_location_show --                                            *//**
 *
 * \brief This routines Domain Show
 *
 * \param data The Structure of the Message for Domain Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status controller_location_get(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char *IP_packed;
	int IP_packed_size;
	uint32_t port;

	log_debug(RESTHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(RESTHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Controller_Location_Get call
		strret = PyEval_CallObject(Controller_Interface.Controller_Location_Get, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "PyEval_CallObject Controller_Location_Get returns NULL");
			break;
		}

		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_ParseTuple(strret, "Iz#I", &status, &IP_packed, &IP_packed_size, &port);

		if (status == DOVE_STATUS_OK)
		{
			memcpy(&data->controller_location.ip6, IP_packed, IP_packed_size);
			if (IP_packed_size == 4)
			{
				data->controller_location.family = AF_INET;
			}
			else
			{
				data->controller_location.family = AF_INET6;
			}
			data->controller_location.port = port;
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_debug(RESTHandlerLogLevel, "Exit: %s", DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_load_balancing_get --                                            *//**
 *
 * \brief This routine handles Load Balancing Get
 *
 * \param data The Structure of the Message for load balancing
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_load_balancing_get(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;

	status = dps_statistics_domain_load_balancing_get(&data->load_balancing);
	return (dove_status)status;
}

/*
 ******************************************************************************
 * domain_general_statistics_get --                                       *//**
 *
 * \brief This routine handles General Statistics Get
 *
 * \param data The Structure of the Message for general statistics
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status domain_general_statistics_get(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;

	status = dps_statistics_domain_general_statistics_get(&data->general_statistics);
	return (dove_status)status;
}

/*
 ******************************************************************************
 * ip_subnet_add --                                                       *//**
 *
 * \brief This routine handles IP subnet add
 *
 * \param data The Structure of the Message for IP Subnet Add
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_add(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		if (data->ip_subnet_add.IP_type == AF_INET)
		{
			//def IP_Subnet_Add(self, associated_type, associated_id, IP_type, IP_value, mask_value, mode, gateway):
			strargs = Py_BuildValue("(IIIIIII)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_add.IP_type,
			                        data->ip_subnet_add.IPv4,
			                        data->ip_subnet_add.mask,
			                        data->ip_subnet_add.mode,
			                        data->ip_subnet_add.gateway_v4);
		}
		else
		{
			//def IP_Subnet_Add(self, associated_type, associated_id, IP_type, IP_value, mask_value, mode, gateway):
			strargs = Py_BuildValue("(IIIz#IIz#)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_add.IP_type,
			                        data->ip_subnet_add.IPv6, 16,
			                        data->ip_subnet_add.prefix_len,
			                        data->ip_subnet_add.mode,
			                        data->ip_subnet_add.gateway_v6, 16);
		}
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the IP_Subnet_Add call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_Add, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_Add returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}

/*
 ******************************************************************************
 * ip_subnet_delete --                                                    *//**
 *
 * \brief This routine handles IP subnet delete
 *
 * \param data The Structure of the Message for IP Subnet Delete
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_delete(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		if (data->ip_subnet_delete.IP_type == AF_INET)
		{
			//def IP_Subnet_Delete(self, associated_type, associated_id, IP_type, IP_value, mask_value):
			strargs = Py_BuildValue("(IIIII)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_delete.IP_type,
			                        data->ip_subnet_delete.IPv4,
			                        data->ip_subnet_delete.mask);
		}
		else
		{
			//def IP_Subnet_Delete(self, associated_type, associated_id, IP_type, IP_value, mask_value):
			strargs = Py_BuildValue("(IIIz#I)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_delete.IP_type,
			                        data->ip_subnet_delete.IPv6, 16,
			                        data->ip_subnet_delete.prefix_len);
		}

		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the IP_Subnet_Delete call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_Delete, strargs);
		Py_DECREF(strargs);

		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_Delete returns NULL");
			break;
		}
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}


/*
 ******************************************************************************
 * ip_subnet_get --                                                       *//**
 *
 * \brief This routine handles IP subnet get
 *
 * \param data The Structure of the Message for IP Subnet Get
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_get(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	uint32_t subnet_mask, subnet_mode;
	char *subnet_ip_packed, *subnet_gateway_packed;
	int subnet_ip_packed_size, subnet_gateway_packed_size;

	log_debug(PythonDataHandlerLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		if (data->ip_subnet_get.IP_type == AF_INET)
		{
			//def IP_Subnet_Get(self, associated_type, associated_id, IP_type, IP_value, mask_value):
			strargs = Py_BuildValue("(IIIII)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_get.IP_type,
			                        data->ip_subnet_get.IPv4,
			                        data->ip_subnet_get.mask);
		}
		else
		{
			//def IP_Subnet_Get(self, associated_type, associated_id, IP_type, IP_value, mask_value):
			strargs = Py_BuildValue("(IIIz#I)",
			                        data->ip_subnet_add.associated_type,
			                        data->ip_subnet_add.associated_id,
			                        data->ip_subnet_get.IP_type,
			                        data->ip_subnet_get.IPv6, 16,
			                        data->ip_subnet_get.prefix_len);
		}

		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the IP_Subnet_Get call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_Get, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_Get returns NULL");
			break;
		}
		//@return: status, subnet_ip_packed, subnet_mask, subnet_mode, subnet_gateway_packed
		//@rtype: Integer, ByteArray, Integer, Integer, ByteArray
		PyArg_ParseTuple(strret, "iz#IIz#",
		                 &status, &subnet_ip_packed, &subnet_ip_packed_size,
		                 &subnet_mask, &subnet_mode,
		                 &subnet_gateway_packed, &subnet_gateway_packed_size);

		if (subnet_ip_packed != NULL)
		{
			if (subnet_ip_packed_size == 4)
			{
				memcpy(&data->ip_subnet_get.IPv4, subnet_ip_packed, 4);
				data->ip_subnet_get.mask = subnet_mask;
				data->ip_subnet_get.mode = subnet_mode;
				memcpy(&data->ip_subnet_get.gateway_v4, subnet_gateway_packed, 4);
			}
			else
			{
				memcpy(data->ip_subnet_get.IPv6, subnet_ip_packed, 16);
				data->ip_subnet_get.mask = subnet_mask;
				data->ip_subnet_get.mode = subnet_mode;
				memcpy(data->ip_subnet_get.gateway_v6, subnet_gateway_packed, 16);
			}
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);
	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}

/*
 ******************************************************************************
 * ip_subnet_list --                                                      *//**
 *
 * \brief This routine handles IP subnets list
 *
 * \param data The Structure of the Message for IP Subnets List
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_list(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		//def IP_Subnet_List(self, associated_type, associated_id, IP_type):
		strargs = Py_BuildValue("(III)",
		                        data->ip_subnet_list.associated_type,
		                        data->ip_subnet_list.associated_id,
		                        data->ip_subnet_list.IP_type);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the IP_Subnet_List call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_List, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_List returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * ip_subnet_flush --                                                     *//**
 *
 * \brief This routine handles IP subnets flush
 *
 * \param data The Structure of the Message for IP Subnets FLush
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_flush(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		//def IP_Subnet_List(self, associated_type, associated_id, IP_type):
		strargs = Py_BuildValue("(III)",
		                        data->ip_subnet_flush.associated_type,
		                        data->ip_subnet_flush.associated_id,
		                        data->ip_subnet_flush.IP_type);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the IP_Subnet_Flush call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_Flush, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_Flush returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit: %s",
	         DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}

/*
 ******************************************************************************
 * ip_subnet_getallids --                                                     *//**
 *
 * \brief This routine return all Subnet IDs of the domain
 *
 * \param data The Structure of the Message for IP Subnets GetAllIds
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status ip_subnet_getallids(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_NO_MEMORY;
	long subnet_ids;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def IP_Subnet_GetAllIds(self, associated_type, associated_id, IP_type):
		strargs = Py_BuildValue("(III)",
		                        data->ip_subnet_getallids.associated_type,
		                        data->ip_subnet_getallids.associated_id,
		                        data->ip_subnet_getallids.IP_type);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the IP_Subnet_GetAllIds call
		strret = PyEval_CallObject(Controller_Interface.IP_Subnet_GetAllIds, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_GetAllIds returns NULL");
			break;
		}
		//@return: status, subnet_ids
		//@rtype: Integer, Long
		PyArg_ParseTuple(strret, "iL", &status, &subnet_ids);
		data->return_val = (void *)subnet_ids;

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_debug(PythonDataHandlerLogLevel, "Exit: %s",
	          DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_clients_show --                                                     *//**
 *
 * \brief This routine returns all the DPS Clients
 *
 * \param data The Structure of the Message for dps_clients_show
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status dps_clients_show(dps_controller_data_op_t *data)
{
	int status = DOVE_STATUS_OK;
	PyObject *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def IP_Subnet_GetAllIds(self, associated_type, associated_id, IP_type):
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the IP_Subnet_GetAllIds call
		PyEval_CallObject(Controller_Interface.DPSClientsShow, strargs);
		Py_DECREF(strargs);

	} while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_debug(PythonDataHandlerLogLevel, "Exit: %s",
	          DOVEStatusToString((dove_status)status));
	return (dove_status)status;
}

/*
 ******************************************************************************
 * functions_init --                                              *//**
 *
 * \brief This initializes the C function handler for the various request types
 *
 * \retval DOVE_STATUS_OK
 *
 *****************************************************************************/

static dove_status functions_init(void)
{
	function_array[DPS_CONTROLLER_DOMAIN_ADD] = domain_add;
	function_array[DPS_CONTROLLER_DOMAIN_DELETE] = domain_delete;
	function_array[DPS_CONTROLLER_DVG_ADD] = dvg_add;
	function_array[DPS_CONTROLLER_DVG_DELETE] = dvg_delete;
	function_array[DPS_CONTROLLER_POLICY_ADD] = policy_add;
	function_array[DPS_CONTROLLER_POLICY_DELETE] = policy_delete;
	function_array[DPS_CONTROLLER_DOMAIN_SHOW] = domain_show;
	function_array[DPS_CONTROLLER_DOMAIN_VALIDATE] = domain_validate;
	function_array[DPS_CONTROLLER_DOMAIN_GETALLIDS] = domain_getallids;
	function_array[DPS_CONTROLLER_DVG_VALIDATE] = dvg_validate;
	function_array[DPS_CONTROLLER_DVG_GETALLIDS] = dvg_getallids;
	function_array[DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD] = gateway_update;
	function_array[DPS_CONTROLLER_EXTERNAL_GATEWAY_DEL] = gateway_update;
	function_array[DPS_CONTROLLER_EXTERNAL_GATEWAY_CLR] = gateway_clear;
	function_array[DPS_CONTROLLER_EXTERNAL_GATEWAY_VALIDATE] = gateway_validate;	   	
	function_array[DPS_CONTROLLER_EXTERNAL_GATEWAY_GETALLIDS] = gateway_getallids;
	function_array[DPS_CONTROLLER_POLICY_GET] = policy_get;
	function_array[DPS_CONTROLLER_POLICY_GETALLIDS] = policy_getallids;
	function_array[DPS_CONTROLLER_LOCATION_SET] = controller_location_update;
	function_array[DPS_CONTROLLER_LOCATION_GET] = controller_location_get;
	function_array[DPS_CONTROLLER_LOAD_BALANCING_GET] = domain_load_balancing_get;
	function_array[DPS_CONTROLLER_GENERAL_STATISTICS_GET] = domain_general_statistics_get;
	function_array[DPS_CONTROLLER_IP_SUBNET_ADD] = ip_subnet_add;
	function_array[DPS_CONTROLLER_IP_SUBNET_DELETE] = ip_subnet_delete;
	function_array[DPS_CONTROLLER_IP_SUBNET_GET] = ip_subnet_get;
	function_array[DPS_CONTROLLER_IP_SUBNET_LIST] = ip_subnet_list;
	function_array[DPS_CONTROLLER_IP_SUBNET_FLUSH] = ip_subnet_flush;
	function_array[DPS_CONTROLLER_VNID_SHOW] = dvg_show;
	function_array[DPS_CONTROLLER_QUERY_VNID] = dvg_query;
	function_array[DPS_CONTROLLER_IP_SUBNET_GETALLIDS] = ip_subnet_getallids;
	function_array[DPS_CONTROLLER_DOMAIN_GLOBAL_SHOW] = domain_global_show;
	function_array[DPS_CONTROLLER_SERVICE_ROLE] = NULL;
	function_array[DPS_CONTROLLER_MULTICAST_SHOW] = multicast_show;
	function_array[DPS_CONTROLLER_DOMAIN_ADDRESS_RESOLUTION_SHOW] = domain_address_resolution_show;
	function_array[DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_GET_READY] = domain_mass_transfer_get_ready;
	function_array[DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_START] = domain_mass_transfer_start;
	function_array[DPS_CONTROLLER_DOMAIN_ACTIVATE] = domain_add;
	function_array[DPS_CONTROLLER_DOMAIN_DEACTIVATE] = domain_delete;
	function_array[DPS_CONTROLLER_DOMAIN_UPDATE] = domain_update;
	function_array[DPS_CONTROLLER_LOCATION_DELETE] = controller_location_delete;
	function_array[DPS_CONTROLLER_DOMAIN_DELETE_ALL_LOCAL] = domains_clear;
	function_array[DPS_CONTROLLER_DOMAIN_RECOVERY_START] = domain_recovery_start;
	function_array[DPS_CONTROLLER_DVG_ADD_QUERY] = dvg_add;
	function_array[DPS_CONTROLLER_DCS_CLIENTS_SHOW] = dps_clients_show;
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (DpsControllerHandler) that are needed for processing
 *        requests received from the Controller.
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/

static dove_status python_functions_init(char *pythonpath)
{
	dove_status status;
	PyObject *pyargs;

	log_info(PythonDataHandlerLogLevel, "Enter");

	memset(&Controller_Interface, 0, sizeof(python_dps_controller_t));
	do
	{
		// Get handle to an instance of DpsControllerHandler
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_CONTROLLER_HANDLER,
		                                 PYTHON_MODULE_CLASS_CONTROLLER_HANDLER,
		                                 pyargs,
		                                 &Controller_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Domain_Add_Local
		Controller_Interface.Domain_Add_Local =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_ADD);
		if (Controller_Interface.Domain_Add_Local == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Delete
		Controller_Interface.Domain_Delete =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_DELETE);
		if (Controller_Interface.Domain_Delete == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Delete_All_Local
		Controller_Interface.Domain_Delete_All_Local =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_DELETE_ALL_LOCAL);
		if (Controller_Interface.Domain_Delete_All_Local == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_DELETE_ALL_LOCAL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Get_Ready_For_Transfer
		Controller_Interface.Domain_Get_Ready_For_Transfer =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_GET_READY_FOR_TRANSFER);
		if (Controller_Interface.Domain_Get_Ready_For_Transfer == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_GET_READY_FOR_TRANSFER);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Start_Transfer_To
		Controller_Interface.Domain_Start_Transfer_To =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_START_TRANSFER_TO);
		if (Controller_Interface.Domain_Start_Transfer_To == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_START_TRANSFER_TO);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Activate
		Controller_Interface.Domain_Activate =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_ACTIVATE);
		if (Controller_Interface.Domain_Activate == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_ACTIVATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Deactivate
		Controller_Interface.Domain_Deactivate =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_DEACTIVATE);
		if (Controller_Interface.Domain_Deactivate == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_DEACTIVATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Update
		Controller_Interface.Domain_Update =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_UPDATE);
		if (Controller_Interface.Domain_Update == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_UPDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Recovery_Thread_Start
		Controller_Interface.Domain_Recovery_Thread_Start =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_RECOVERY_START);
		if (Controller_Interface.Domain_Recovery_Thread_Start == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_RECOVERY_START);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_Add
		Controller_Interface.Dvg_Add =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_ADD);
		if (Controller_Interface.Dvg_Add == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_Add_Query
		Controller_Interface.Dvg_Add_Query =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_ADD_QUERY);
		if (Controller_Interface.Dvg_Add_Query == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_ADD_QUERY);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_Delete
		Controller_Interface.Dvg_Delete =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_DELETE);
		if (Controller_Interface.Dvg_Delete == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_Validate
		Controller_Interface.Dvg_Validate =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_VALIDATE);
		if (Controller_Interface.Dvg_Validate == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_VALIDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_GetAllIds
		Controller_Interface.Dvg_GetAllIds =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_GETALLIDS);
		if (Controller_Interface.Dvg_GetAllIds == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_GETALLIDS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Add
		Controller_Interface.Policy_Add =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_POLICY_ADD);
		if (Controller_Interface.Policy_Add == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Delete
		Controller_Interface.Policy_Delete =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_POLICY_DELETE);
		if (Controller_Interface.Policy_Delete == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Get
		Controller_Interface.Policy_Get =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_POLICY_GET);
		if (Controller_Interface.Policy_Get == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Policy_Get
		Controller_Interface.Policy_GetAllIds =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_POLICY_GETALLIDS);
		if (Controller_Interface.Policy_GetAllIds == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_POLICY_GETALLIDS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Show_Local
		Controller_Interface.Domain_Show_Local =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_SHOW);
		if (Controller_Interface.Domain_Show_Local == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Validate_Local
		Controller_Interface.Domain_Validate_Local =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_VALIDATE);
		if (Controller_Interface.Domain_Validate_Local == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_VALIDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_GetAllIds_Local
		Controller_Interface.Domain_GetAllIds_Local =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_GET_ALL_DOMAIN_IDS);
		if (Controller_Interface.Domain_GetAllIds_Local == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_GET_ALL_DOMAIN_IDS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Show_Global_Mapping
		Controller_Interface.Domain_Show_Global_Mapping =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_GLOBAL_SHOW_MAPPING);
		if (Controller_Interface.Domain_Show_Global_Mapping == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_GLOBAL_SHOW_MAPPING);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function External_Gateway_Add
		Controller_Interface.External_Gateway_Add =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_EXTERNAL_GATEWAY_ADD);
		if (Controller_Interface.External_Gateway_Add == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_EXTERNAL_GATEWAY_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function External_Gateway_Delete
		Controller_Interface.External_Gateway_Delete =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_EXTERNAL_GATEWAY_DELETE);
		if (Controller_Interface.External_Gateway_Delete == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_EXTERNAL_GATEWAY_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function External_Gateway_Clear
		Controller_Interface.External_Gateway_Clear =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_EXTERNAL_GATEWAY_CLEAR);
		if (Controller_Interface.External_Gateway_Clear == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_EXTERNAL_GATEWAY_CLEAR);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function External_Gateway_Validate
		Controller_Interface.External_Gateway_Validate =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_EXTERNAL_GATEWAY_VALIDATE);
		if (Controller_Interface.External_Gateway_Validate == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_EXTERNAL_GATEWAY_VALIDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function External_Gateway_GetAll
		Controller_Interface.External_Gateway_GetAllIds =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_EXTERNAL_GATEWAY_GETALLIDS);
		if (Controller_Interface.External_Gateway_GetAllIds == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_EXTERNAL_GATEWAY_GETALLIDS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}


		// Get handle to function Controller_Location_Update
		Controller_Interface.Controller_Location_Update =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_LOCATION_UPDATE);
		if (Controller_Interface.Controller_Location_Update == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
				      "ERROR! PyObject_GetAttrString (%s) failed...\n",
				      PYTHON_FUNC_CONTROLLER_LOCATION_UPDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Controller_Location_Get
		Controller_Interface.Controller_Location_Get =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_LOCATION_GET);
		if (Controller_Interface.Controller_Location_Get == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CONTROLLER_LOCATION_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_Add
		Controller_Interface.IP_Subnet_Add =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_ADD);
		if (Controller_Interface.IP_Subnet_Add == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_Delete
		Controller_Interface.IP_Subnet_Delete =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_DELETE);
		if (Controller_Interface.IP_Subnet_Delete == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_Get
		Controller_Interface.IP_Subnet_Get =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_GET);
		if (Controller_Interface.IP_Subnet_Get == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_List
		Controller_Interface.IP_Subnet_List =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_LIST);
		if (Controller_Interface.IP_Subnet_List == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_LIST);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_Flush
		Controller_Interface.IP_Subnet_Flush =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_FLUSH);
		if (Controller_Interface.IP_Subnet_Flush == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_FLUSH);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function IP_Subnet_GetAllIds
		Controller_Interface.IP_Subnet_GetAllIds =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_IP_SUBNET_GETALLIDS);
		if (Controller_Interface.IP_Subnet_GetAllIds == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_IP_SUBNET_GETALLIDS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Dvg_Show
		Controller_Interface.Dvg_Show =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DVG_SHOW);
		if (Controller_Interface.Dvg_Show == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DVG_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Multicast_Show
		Controller_Interface.Multicast_Show =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_MULTICAST_SHOW);
		if (Controller_Interface.Multicast_Show == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_MULTICAST_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Show_Address_Resolution
		Controller_Interface.Domain_Show_Address_Resolution =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_ADDRESS_RESOLUTION_SHOW);
		if (Controller_Interface.Domain_Show_Address_Resolution == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_ADDRESS_RESOLUTION_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Stop
		Controller_Interface.Stop =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_FUNCTION_STOP);
		if (Controller_Interface.Stop == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CONTROLLER_FUNCTION_STOP);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Start
		Controller_Interface.Start =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_CONTROLLER_FUNCTION_START);
		if (Controller_Interface.Start == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CONTROLLER_FUNCTION_START);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function DPSClientsShow
		Controller_Interface.DPSClientsShow =
			PyObject_GetAttrString(Controller_Interface.instance,
			                       PYTHON_FUNC_DPS_CLIENTS_SHOW);
		if (Controller_Interface.DPSClientsShow == NULL)
		{
			log_emergency(PythonDataHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DPS_CLIENTS_SHOW);
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
 * python_init_dps_controller_interface --                                *//**
 *
 * \brief This routine initializes the DPS Controller Interface to PYTHON
 *        OBHECTS
 *
 * \param pythonpath - Pointer to the Python Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_dps_controller_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		status = python_functions_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		status = functions_init();
	} while (0);

	return status;
}

/*
 ******************************************************************************
 * dps_controller_interface_stop --                                       *//**
 *
 * \brief This routine stops the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_controller_interface_stop()
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
		PyEval_CallObject(Controller_Interface.Stop, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Controller Interface Stopped");
		started = 0;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_controller_interface_start --                                       *//**
 *
 * \brief This routine starts the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dps_controller_interface_start()
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
		PyEval_CallObject(Controller_Interface.Start, strargs);
		Py_DECREF(strargs);
		log_notice(PythonDataHandlerLogLevel, "DPS Controller Interface Started");
		started = 1;
		status = DOVE_STATUS_OK;
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonDataHandlerLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_controller_data_msg --                                             *//**
 *
 * \brief This routine initializes handles DPS Controller Data Object Messages
 *
 * \param data - Pointer to DPS Client Server Protocol Message
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status dps_controller_data_msg(dps_controller_data_op_t *data)
{
	uint32_t msg_type;
	dove_status status = DOVE_STATUS_NOT_FOUND;
	dps_controller_func_handler func;

	msg_type = (uint32_t)data->type;
	do
	{
		if (!started &&
		    (msg_type != DPS_CONTROLLER_LOCATION_SET) &&
		    (msg_type != DPS_CONTROLLER_LOCATION_GET))
		{
			status = DOVE_STATUS_INACTIVE;
			break;
		}
		if (msg_type >= DPS_CONTROLLER_OP_MAX)
		{
			break;
		}
		func = function_array[msg_type];
		if (func == NULL)
		{
			break;
		}
		status = func(data);
	}while(0);

	return status;
}

/** @} */
/** @} */
