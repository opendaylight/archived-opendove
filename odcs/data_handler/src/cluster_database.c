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
*  $Log: cluster_database.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}  COPYRIGHT / HISTORY (end)
*/

#include "include.h"

int PythonClusterDataLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

/**
 * \brief Contains the IP address of the DPS Cluster Leader
 */
ip_addr_t dps_cluster_leader;

/**
 * \brief Contains the IP address of DPS Cluster Leader in Readable
 *        string format
 */
char dps_cluster_leader_ip_string_storage[INET6_ADDRSTRLEN+2];
char *dps_cluster_leader_ip_string = dps_cluster_leader_ip_string_storage;

char dps_local_ip_string_storage[INET6_ADDRSTRLEN+2];
char *dps_local_ip_string = dps_local_ip_string_storage;

/**
 * \brief Flag to tell if the local Node is the Leader or NOT
 */
static int fLocalLeader = 0;

/**
 * \brief Flag to tell if the current Node is Active.
 */
static int fLocalActive = 1;

/**
 * \brief The module location defines for the DPS Cluster Database Handling API
 */
#define PYTHON_MODULE_FILE_CLUSTER_DATABASE "cluster_database"

/**
 * \brief The PYTHON Class that handles the DPS Cluster Database Requests
 */
#define PYTHON_MODULE_CLASS_CLUSTER_DATABASE "ClusterDatabase"

/**
 * \brief The PYTHON function that handles (Remote) DPS Node Add
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_ADD "Node_Add"

/*
 * \brief The PYTHON function that handles Local DPS Node Add
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_UPDATE_LOCAL "Node_Local_Update"

/*
 * \brief The PYTHON function that handles Node_Local_Activate
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_ACTIVATE_LOCAL "Node_Local_Activate"

/**
 * \brief The PYTHON function that handles Node_Get_Port (to Cluster)
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_GET_PORT "Node_Get_Port"

/**
 * \brief The PYTHON function that handles DPS Node Add (from Cluster)
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE "Node_Delete"

/**
 * \brief The PYTHON function that handles Domain Add to a DPS Node in Cluster
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_ADD_DOMAIN "Node_Add_Domain"

/**
 * \brief The PYTHON function that handles Domain Delete from a DPS Node in Cluster
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_DOMAIN "Node_Delete_Domain"

/**
 * \brief The PYTHON function that handles Deletion of all Domain for a DPS Node
 */
#define PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_ALL_DOMAINS "Node_Delete_All_Domains"

/**
 * \brief The PYTHON function that handles Deletion of all Domain for a DPS Node
 */
#define PYTHON_FUNC_CLUSTER_SHOW "Show"

/**
 * \brief The PYTHON function that retrieves the list of node that handles a
 *        domain
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_GET_NODES "Domain_Get_Nodes"

/**
 * \brief The PYTHON function that retrieves a random node handling a Domain
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_GET_RANDOM_REMOTE_NODE "Domain_Get_Random_Remote_Node"

/**
 * \brief The PYTHON function that retrieves the list of domains handled by a
 *        node
 */
#define PYTHON_FUNC_CLUSTER_NODE_GET_DOMAINS "Node_Get_Domains"

/**
 * \brief The PYTHON function that retrieves the list of all nodes in the
 *        cluster
 */
#define PYTHON_FUNC_CLUSTER_GET_ALL_NODES "Cluster_Get_All_Nodes"

/**
 * \brief The PYTHON function that sets the domain mapping for a node
 */
#define PYTHON_FUNC_NODE_SET_DOMAIN_MAPPING "Node_Set_Domain_Mapping"

/**
 * \brief The PYTHON function that gets domain ID which a VN is 
 *        associated with
 */
#define PYTHON_FUNC_GET_DOMAINID_FROM_VNID "Get_DomainID_From_VNID"

/**
 * \brief The PYTHON function that should be called when the controller
 *        sends over the entire set of DCS nodes in the cluster
 */
#define PYTHON_FUNC_CLUSTER_NODES_UPDATE "Nodes_Update"

/**
 * \brief The PYTHON function that should be called when the controller
 *        sends over the entire set of DCS nodes in the cluster
 */
#define PYTHON_FUNC_CLUSTER_NODE_TOUCH "Node_Touch"

/**
 * \brief The PYTHON for Validating an IP Address
 */
#define PYTHON_FUNC_CLUSTER_NODE_VALIDATE "Node_Validate_Remote_DCS"

/*
 * \brief The PYTHON function that should be called when a Heartbeat
 *        message is received from a remote node
 */
#define PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT "Node_Heartbeat"

/**
 * \brief The PYTHON function that should be called when a Heartbeat
 *        Request message is received from a remote node
 */
#define PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_REQUEST "Node_Heartbeat_Request"

/**
 * \brief The PYTHON function that should be called when a Heartbeat
 *        Request message is received from a remote node
 */
#define PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_PROCESS "Node_Heartbeat_Process"

/**
 * \brief The PYTHON function that should be called when the Leader
 *        sends the status of nodes in response to a heartbeat
 */
#define PYTHON_FUNC_CLUSTER_NODES_STATUS "Nodes_Status"

/**
 * \brief The PYTHON function to exchange domain mapping with all nodes
 *        in the cluster
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_MAPPING_EXCHANGE "Domain_Mapping_Exchange"

/**
 * \brief The PYTHON function to exchange domain mapping synchronously with
 *        all other nodes in the cluster
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_ACTIVATE_MAPPING_EXCHANGE "Domain_Activate_Mapping_Exchange_Synchronous"

/**
 * \brief The PYTHON function to update a Node's Domain Specific Statistics
 */
#define PYTHON_FUNC_CLUSTER_NODE_UPDATE_DOMAIN_STATISTICS "Node_Update_Domain_Statistics"

/**
 * \brief The PYTHON function to show a Nodes' Load
 */
#define PYTHON_FUNC_CLUSTER_NODE_SHOW_LOAD "Node_Show_Load"

/**
 * \brief The PYTHON function to exchange domain mapping with all nodes
 *        in the cluster
 */
#define PYTHON_FUNC_CLUSTER_AVAILABLE_NODES "Get_Lowest_Loaded_Available_Nodes"

/**
 * \brief The PYTHON function to get all the nodes which are least loaded and do
 * 		  not handle a domain.
 */
#define PYTHON_FUNC_CLUSTER_AVAILABLE_NODES_NOT_HOSTING "Get_Lowest_Loaded_Available_Nodes_Not_Hosting"

/**
 * \brief The PYTHON function to get the current replication factor for a domain
 */
#define PYTHON_FUNC_DOMAIN_GET_REPLICATION_FACTOR "Domain_Get_Replication_Factor"

/**
 * \brief The PYTHON function to get all the nodes which are are heavily loaded
 * 	  and host a domain.
 */
#define PYTHON_FUNC_CLUSTER_GET_HIGHEST_LOADED_NODES_HOSTING "Get_Highest_Loaded_Nodes_Hosting"

/*
 * \brief The PYTHON function to send domain delete to all nodes
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_DELETE_SEND_TO_ALL "Domain_Delete_Send_To_All"

/*
 * \brief The PYTHON function to send VNID Add/Delete to All DCS Nodes
 */
#define PYTHON_FUNC_CLUSTER_VNID_EXISTENCE_SEND_TO_ALL "VNID_Existence_Send_To_All"

/*
 * \brief The PYTHON function to delete Domain from Cluster
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_DELETE "Domain_Delete"

/*
 * \brief The PYTHON function to check existence of Domain in Cluster
 */
#define PYTHON_FUNC_CLUSTER_DOMAIN_EXISTS "Domain_Exists"

/*
 * \brief The PYTHON function to set Heavy Load Threshold value
 */
#define PYTHON_FUNC_CLUSTER_HEAVY_LOAD_THRESHOLD "Set_Heavy_Load_Value"


/**
 * \brief The DPS controller handler function pointers data structure
 */
typedef struct python_dps_cluster_db_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function for Node_Add
	 */
	PyObject *Node_Add;
	/*
	 * \brief The function for Node_Local_Update
	 */
	PyObject *Node_Local_Update;
	/*
	 * \brief The PYTHON function that handles Node_Local_Activate
	 */
	PyObject *Node_Local_Activate;
	/**
	 * \brief The function for Node_Get_Port
	 */
	PyObject *Node_Get_Port;
	/*
	 * \brief The function for Node_Delete
	 */
	PyObject *Node_Delete;
	/*
	 * \brief The function for Node_Add_Domain
	 */
	PyObject *Node_Add_Domain;
	/*
	 * \brief The function for Node_Delete_Domain
	 */
	PyObject *Node_Delete_Domain;
	/*
	 * \brief The function for Node_Delete_All_Domains
	 */
	PyObject *Node_Delete_All_Domains;
	/**
	 * \brief Show Cluster Information
	 */
	PyObject *Show;
	/**
	 * \brief Domain Get Nodes
	 */
	PyObject *Domain_Get_Nodes;
	/**
	 * \brief Domain Get Random Node
	 */
	PyObject *Domain_Get_Random_Remote_Node;
	/**
	 * \brief Node Get Domains
	 */
	PyObject *Node_Get_Domains;
	/**
	 * \brief Cluster Get All Nodes
	 */
	PyObject *Cluster_Get_All_Nodes;
	/**
	 * \brief Node Set Domain Mapping
	 */
	PyObject *Node_Set_Domain_Mapping;
	/**
	 * \brief The function for Get_DomainID_From_VNID
	 */
	PyObject *Get_DomainID_From_VNID;
	/**
	 * \brief The function for Nodes_Update
	 */
	PyObject *Nodes_Update;
	/**
	 * \brief The function for Node_Touch
	 */
	PyObject *Node_Touch;
	/**
	 * \brief The function for Node_Validate_Remote_DCS
	 */
	PyObject *Node_Validate_Remote_DCS;
	/**
	 * \brief The function for Node_Heartbeat
	 */
	PyObject *Node_Heartbeat;
	/**
	 * \brief The function for Node_Heartbeat_Request
	 */
	PyObject *Node_Heartbeat_Request;
	/**
	 * \brief The function for Node_Heartbeat_Process
	 */
	PyObject *Node_Heartbeat_Process;
	/**
	 * \brief The function for Nodes_Status
	 */
	PyObject *Nodes_Status;
	/**
	 * \brief The function for Domain_Mapping_Exchange
	 */
	PyObject *Domain_Mapping_Exchange;
	/**
	 * \brief The PYTHON function to exchange domain mapping synchronously with
	 *        all other nodes in the cluster
	 */
	PyObject *Domain_Activate_Mapping_Exchange_Synchronous;
	/**
	 * \brief The function for Node_Update_Domain_Statistics
	 */
	PyObject *Node_Update_Domain_Statistics;
	/**
	 * \brief The function for Get_Lowest_Loaded_Available_Nodes
	 */
	PyObject *Get_Lowest_Loaded_Available_Nodes;
	/**
	 * \brief The function for Node_Show_Load
	 */
	PyObject *Node_Show_Load;
	/**
	 * \brief The function for Get_Lowest_Loaded_Available_Nodes_Not_Hosting
	 */
	PyObject *Get_Lowest_Loaded_Available_Nodes_Not_Hosting;
	/**
	 * \brief The function for Domain_Get_Replication_Factor
	 */
	PyObject *Domain_Get_Replication_Factor;
	/**
	 * \brief The function for Domain_Get_Replication_Factor
	 */
	PyObject *Get_Highest_Loaded_Nodes_Hosting;
	/*
	 * \brief The PYTHON function to send domain delete to all nodes
	 */
	PyObject *Domain_Delete_Send_To_All;
	/*
	 * \brief The PYTHON function to send VNID Add/Delete to All DCS Nodes
	 */
	PyObject *VNID_Existence_Send_To_All;
	/**
	 * \brief The PYTHON function to delete a Domain
	 */
	PyObject *Domain_Delete;
	/**
	 * \brief The PYTHON function to check existence of Domain
	 */
	PyObject *Domain_Exists;
	/**
	 * \brief The PYTHON function to set Heavy Load Threshold value
	 */
	PyObject *Set_Heavy_Load_Value;
}python_dps_cluster_db_t;

/*
 * \brief The Cluster Database Handler PYTHON Interface (Embed)
 */
static python_dps_cluster_db_t Cluster_DB_Interface;

/*
 ******************************************************************************
 * DPS Cluster Database Interface                                         *//**
 *
 * \addtogroup PythonInterface
 * @{
 * \defgroup DPSClusterDBInterface DPS Cluster Database Interface
 * @{
 * Handles Interaction between the DPS Server and DPS Cluster Database Objects.
 * The Gossip Protocol can use this Cluster Database Handler to store the Node
 * Attribute for each node in the DPS Cluster.
 */

/*
 ******************************************************************************
 * dps_cluster_node_add                                                   *//**
 *
 * \brief - This function can be used to add a Remote Node to Cluster.
 *          Determine if this function needs to exist with Spidercast or
 *          will Spidercast implement this implicitly.
 *
 * \param node_ip The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_add(ip_addr_t *node_ip)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel, "Enter");

#if defined(NDEBUG)
	{
		char str[INET6_ADDRSTRLEN];
		inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel,
		          "Node IP %s, Port %d", str, node_ip->port);
	}
#endif

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		//def Node_Add(self, ip_type, ip_val, port):
		strargs = Py_BuildValue("(Iz#H)",
		                        node_ip->family, node_ip->ip6, 16, node_ip->port);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Add call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Add, strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Add returns NULL");
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
	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_local_update                                          *//**
 *
 * \brief - This function can be used to update Local Node to cluster database
 *
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_local_update(ip_addr_t *node_ip)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel, "Enter");

	inet_ntop(node_ip->family, node_ip->ip6, dps_local_ip_string, INET6_ADDRSTRLEN);
	log_notice(PythonClusterDataLogLevel,
	           "Node IP %s, Port %d", dps_local_ip_string, node_ip->port);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		//def Node_Add(self, ip_type, ip_val, port):
		strargs = Py_BuildValue("(Iz#H)",
		                        node_ip->family, node_ip->ip6, 16, node_ip->port);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Update call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Local_Update, strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Local_Update returns NULL");
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
	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_local_activate                                        *//**
 *
 * \brief - This function can be used to update set if the local node is active
 *
 * \param factive: Whether Local Node is active DCS node in cluster
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_local_activate(int factive)
{
	PyObject *strargs, *strret;
	PyGILState_STATE gstate;
	int status;

	log_info(PythonClusterDataLogLevel, "Enter: factive %d", factive);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		//def Node_Local_Activate(self, factive):
		strargs = Py_BuildValue("(i)", factive);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Local_Activate call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Local_Activate, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "PyEval_CallObject: Node_Local_Activate returns NULL");
			break;
		}
		PyArg_Parse(strret, "I", &status);
		Py_DECREF(strret);
		if (status == DOVE_STATUS_OK)
		{
			// Update C variable
			fLocalActive = factive;
			if (fLocalActive == 0)
			{
				//Inactive node cannot be leader
				fLocalLeader = 0;
			}
		}
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_get_port                                              *//**
 *
 * \brief - This function is used to get the DPS Client Protocol port for a
 *          DPS Node.
 *
 * \param node_ip   The IP Address of the Node. Note that the port must
 *                  be set to zero if the caller doesn't know the port.
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_get_port(ip_addr_t *node_ip)
{
	int status = DOVE_STATUS_NO_MEMORY;
	uint16_t port;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");

	inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel,
	          "Node IP %s, Port %d", str, node_ip->port);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		//def Node_Get_Port(self, ip_type, ip_packed):
		strargs = Py_BuildValue("(Iz#)",
		                        node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Add call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Get_Port, strargs);
		Py_DECREF(strargs);
		if(strret == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "PyEval_CallObject Node_Get_Port returns NULL");
			break;
		}

		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_ParseTuple(strret, "IH", &status, &port);
		if (status == DOVE_STATUS_OK)
		{
			if ((node_ip->port != 0) && (node_ip->port != port))
			{
				log_warn(PythonClusterDataLogLevel,
				         "WARNING: Mismatch in DSP Server Ports Given %d, In Local Database %d",
				         node_ip->port, port);
			}
			if(node_ip->port == 0)
			{
				node_ip->port = port;
			}
		}
		else
		{
			log_warn(PythonClusterDataLogLevel,
			         "DCS Node %s not found in Cluster Database", str);
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);

	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_delete                                                *//**
 *
 * \brief - This function can be used to delete a Node from the Cluster.
 *          Determine if this function needs to exist with Spidercast or will
 *          Spidercast implement this implicitly.
 *
 * \param node_ip        The IP Address of the Node
 * *
 ******************************************************************************
 */
void dps_cluster_node_delete(ip_addr_t *node_ip)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel, "Enter");

#if defined(NDEBUG)
	{
		char str[INET6_ADDRSTRLEN];
		inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel,
		         "Node IP %s, Port %d", str, node_ip->port);
	}
#endif

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();

	do
	{
		// Node_Delete(self, ip_type, ip_packed):
		strargs = Py_BuildValue("(Iz#)", node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Delete, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Delete returns NULL");
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
	log_info(PythonClusterDataLogLevel, "Exit");

	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_add_domain                                            *//**
 *
 * \brief - This function is used to indicate a dps node handles a domain.
 *
 * \param node_ip            The IP Address of the Node
 * \param domain_id          The Domain ID
 * \param replication_factor The Replication Factor of the Domain
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_add_domain(ip_addr_t *node_ip,
                                        uint32_t domain_id,
                                        uint32_t replication_factor)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");

	inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel,
	         "Node IP %s, RAW Port %d", str, node_ip->port);
	log_info(PythonClusterDataLogLevel,
	         "Domain ID %d, Replication Factor %d", domain_id, replication_factor);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{

		//Node_Add_Domain(self, ip_type, ip_packed, port, domain_id):
		strargs = Py_BuildValue("(Iz#HII)",
		                        node_ip->family, node_ip->ip6, 16,
		                        node_ip->port, domain_id, replication_factor);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Add_Domain call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Add_Domain, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Add_Domain returns NULL");
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

	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_delete_domain                                         *//**
 *
 * \brief - This function is used to indicate a dps node no longer handles a
 *          domain.
 *
 * \param node_ip        The IP Address of the Node
 * \param domain_id      The Domain ID
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_delete_domain(ip_addr_t *node_ip,
                                           uint32_t domain_id)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");

	inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
	log_info(PythonClusterDataLogLevel,
	         "Domain %d, Node IP %s, Port %d", domain_id, str, node_ip->port);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//Node_Delete_Domain(self, ip_type, ip_packed, domain_id):
		strargs = Py_BuildValue("(Iz#I)", node_ip->family, node_ip->ip6, 16, domain_id);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_Domain call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Delete_Domain, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Delete_Domain returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_set_node_domain_mapping                                        *//**
 *
 * \brief - This function is used to indicate a dps node no longer handles a
 *
 * \param node_ip_str    The IP Address of the Node in string format
 * \param domain_list    List of Domains (in string format) handled by the node.
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */

dove_status dps_cluster_set_node_domain_mapping(char *node_ip_str,
                                                unsigned int port, char *domain_list)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char *ptr = NULL;
	ip_addr_t node;
	int version = 100;
	int ret_val;

	log_info(PythonClusterDataLogLevel,
	          "Enter: IP %s, Port %d",
	          node_ip_str, port);

	log_debug(PythonClusterDataLogLevel, "domain_list %s", domain_list);

	memset(&node,0,sizeof(ip_addr_t));

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		ptr = strchr(node_ip_str,':');
		if (NULL == ptr) { // IPv4
			node.family = AF_INET;
			ret_val = inet_pton(AF_INET,node_ip_str,node.ip6);
		}
		else {	// IPv6
			node.family = AF_INET6;
			ret_val = inet_pton(AF_INET6,node_ip_str,node.ip6);
		}
		if (!ret_val)
		{
			log_error(PythonClusterDataLogLevel,
			          "inet_pton for %s returns 0", node_ip_str);
			status = DOVE_STATUS_BAD_ADDRESS;
			break;
		}
		strargs = Py_BuildValue("(Iz#IIz#)", node.family, node.ip6, 16,
		                        port,version,
		                        domain_list, strlen(domain_list));
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Set_Domain_Mapping, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Set_Domain_Mapping returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_info(PythonClusterDataLogLevel,
	         "Exit: Status %s", DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_node_delete_all_domains                                    *//**
 *
 * \brief - This function is used to clear the domains handled by the node.
 *
 * \param node_ip        The IP Address of the Node
 *
 * \retval DOVE_STATUS_OK Cluster Node was added to collection
 * \retval DOVE_STATUS_NO_RESOURCES Cluster Node could not be added
 *
 ******************************************************************************
 */
dove_status dps_cluster_node_delete_all_domains(ip_addr_t *node_ip)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_notice(PythonClusterDataLogLevel, "Enter");

#if defined(NDEBUG)
	{
		char str[INET6_ADDRSTRLEN];
		inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
		log_notice(PythonClusterDataLogLevel,
		           "Node IP %s, Port %d", str, node_ip->port);
	}
#endif

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Node_Delete_All_Domains(self, ip_type, ip_packed):
		strargs = Py_BuildValue("(Iz#)", node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Delete_All_Domains, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Delete_All_Domains returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_notice(PythonClusterDataLogLevel, "Exit: Status %s",
	           DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_show                                                       *//**
 *
 * \brief - This function is used to show cluster details
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_EMPTY No Cluster - Error
 *
 ******************************************************************************
 */
dove_status dps_cluster_show()
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonClusterDataLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Show(self, ip_val):
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Show, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Show returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "i", &status);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_get_domain_node_mapping                                            *//**
 *
 * \brief - This function will provide a list of DPS nodes handling a particular
 *          DOVE Domain. This routine will be called by modules such as the
 *          Replicator.
 *
 * \param domain_id        DOVE Domain ID
 * \param node_list_count  The number of allocated elements in the node_list
 * \param node_list        The list of IP Address to be filled by the Cluster
 *                         module. The calling module must allocate this
 *                         structure
 * \param node_list_needed This will be number of IP Address/Node that handle
 *                         that domain. This will be filled in by the Cluster
 *                         Module
 *
 * \retval DOVE_STATUS_OK node_list filled with node_list_needed entries
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_RETRY Caller should retry with node_list_needed
 *                                 entries in node_list
 *
 ******************************************************************************
 */

dove_status dps_get_domain_node_mapping(uint32_t domain_id,
                                        uint32_t node_list_count,
                                        ip_addr_t *node_list,
                                        uint32_t *node_list_needed)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	PyObject *pyList, *pyIPAddress;
	uint32_t i;
	int IP_packed_size;
	char *IP_packed;
	ip_addr_t *node_list_elem;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter: Domain %d", domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		*node_list_needed = 0;

		//def Domain_Get_Nodes(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Domain_Get_Nodes, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Domain_Get_Nodes returns NULL");
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "O", &pyList);

		if(!PyList_Check(pyList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			status = DOVE_STATUS_UNKNOWN;
			break;
		}
		*node_list_needed = PyList_Size(pyList);

		for (i = 0; i < *node_list_needed; i++)
		{
			if (i >= node_list_count)
			{
				Py_DECREF(strret);
				status = DOVE_STATUS_RETRY;
				break;
			}
			node_list_elem = &node_list[i];
			pyIPAddress = PyList_GetItem(pyList, i);
			if (!PyArg_ParseTuple(pyIPAddress, "Hz#H",
			                      &node_list_elem->family,
			                      &IP_packed, &IP_packed_size,
			                      &node_list_elem->port
			                      ))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid Address in element %d", i);
				continue;
			}
			memcpy(node_list_elem->ip6, IP_packed, IP_packed_size);
			inet_ntop(node_list_elem->family, node_list_elem->ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "DCS Node[%d]: IP %s, Port %d",
			         i, str, node_list_elem->port);
		}
		Py_DECREF(strret);

	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_get_domain_random_remote_node                                      *//**
 *
 * \brief - This function will return a random node that handles a particular
 *          domain
 *
 * \param domain_id        DOVE Domain ID
 * \param node             A DPS Node that handles that domain. Caller
 *                         allocated structure
 *
 * \retval DOVE_STATUS_OK node will contain a valid entry
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_EMPTY No DPS Node handles this domain
 *
 ******************************************************************************
 */

dove_status dps_get_domain_random_remote_node(uint32_t domain_id,
                                              ip_addr_t *node)
{
	int status = DOVE_STATUS_NO_MEMORY;;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int IP_packed_size;
	char *IP_packed;

	log_info(PythonClusterDataLogLevel, "Enter: Domain %d", domain_id);

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Get_Random_Remote_Node(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Domain_Get_Random_Remote_Node, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Domain_Get_Random_Remote_Node returns NULL");
			break;
		}

		//@return: Tuple of the form (status, ip_type, ip_packed, port)
		PyArg_ParseTuple(strret, "IHz#H",
		                         &status,
		                         &node->family,
		                         &IP_packed, &IP_packed_size,
		                         &node->port);

		if (status == DOVE_STATUS_OK)
		{
			memcpy(node->ip6, IP_packed, IP_packed_size);
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(node->family, node->ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel,
				         "DCS Node IP %s, Port %d",
				         str, node->port);
			}
#endif
		}
		else
		{
			log_info(PythonClusterDataLogLevel,
			         "Domain_Get_Random_Remote_Node returns %s",
			         DOVEStatusToString((dove_status)status));
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit");

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_get_all_cluster_nodes                                            *//**
 *
 * \brief - This function will provide a list of all DPS nodes in the cluster.
 *          This function will be used to communicate local domain mapping to
 *          all the nodes in the cluster.
 *
 * \param node_list_count  The number of allocated elements in the node_list
 * \param node_list        The list of IP Address to be filled by the Cluster
 *                         module. The calling module must allocate this
 *                         structure
 * \param node_list_needed This will be number of IP Address/Node that handle
 *                         that domain. This will be filled in by the Cluster
 *                         Module
 *
 * \retval DOVE_STATUS_OK node_list filled with node_list_needed entries
 * \retval DOVE_STATUS_INVALID_DOMAIN Invalid Domain
 * \retval DOVE_STATUS_RETRY Caller should retry with node_list_needed
 *                                 entries in node_list
 *
 ******************************************************************************
 */

dove_status dps_get_all_cluster_nodes(uint32_t node_list_count,
                                      ip_addr_t *node_list,
                                      uint32_t *node_list_needed)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret;
	PyObject *strargs;
	PyGILState_STATE gstate;
	PyObject *pyList, *pyIPAddress;
	uint32_t i;
	int IP_packed_size;
	char *IP_packed;
	ip_addr_t *node_list_elem;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter: node_list_count %d", node_list_count);
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Py_BuildValue returns NULL");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		// Invoke the Cluster_Get_All_Nodes call
		strret = PyEval_CallObject(Cluster_DB_Interface.Cluster_Get_All_Nodes, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Cluster_Get_All_Nodes returns NULL");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		//@return: The status of the operation
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "O", &pyList);

		if(!PyList_Check(pyList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			status = DOVE_STATUS_UNKNOWN;
			break;
		}
		*node_list_needed = PyList_Size(pyList);
		if (*node_list_needed > node_list_count)
		{
			Py_DECREF(strret);
			log_info(PythonClusterDataLogLevel,
			         "node_list_needed %d > node_list_count %d",
			         *node_list_needed, node_list_count);
			status = DOVE_STATUS_RETRY;
			break;
		}
		for (i = 0; i < *node_list_needed; i++)
		{
			node_list_elem = &node_list[i];
			pyIPAddress = PyList_GetItem(pyList, i);
			if (!PyArg_ParseTuple(pyIPAddress, "Hz#",
			                      &node_list_elem->family,
			                      &IP_packed, &IP_packed_size
			                      ))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid Address in element %d", i);
				continue;
			}
			memcpy(node_list_elem->ip6, IP_packed, IP_packed_size);
			node_list_elem->port = 0;
			inet_ntop(node_list_elem->family, node_list_elem->ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "DCS Node[%d]: IP %s, Port %d",
			         i, str, node_list_elem->port);
		}
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_cluster_update_statistics                                            *//**
 *
 * \brief - This function allow updating of statistics of a node
 *
 * \param domain_id    The Domain ID
 * \param dps_node     The IP Address of the DPS Node
 *
 * \retval None
 *
 ******************************************************************************
 */
void dps_cluster_update_statistics(uint32_t domain_id,
                                   ip_addr_t *dps_node,
                                   uint32_t endpoints,
                                   uint32_t tunnels,
                                   uint32_t endpoint_updates,
                                   uint32_t endpoint_lookups,
                                   uint32_t policy_lookups)
{
	PyGILState_STATE gstate;
	PyObject *strargs, *strret;
#if defined(NDEBUG)
	char str[INET6_ADDRSTRLEN];
#endif

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();

	do {
#if defined(NDEBUG)
		inet_ntop(dps_node->family, dps_node->ip6, str, INET6_ADDRSTRLEN);
		log_debug(PythonClusterDataLogLevel, "DPS Node IP %s", str);
#endif
		//def Node_Update_Domain_Statistics(self, ip_type, ip_packed, domain_id,
		//                                  endpoints, tunnels,
		//                                  endpoint_updates, endpoint_lookups, policy_lookups):
		strargs = Py_BuildValue("(Iz#IIIIII)", dps_node->family,
		                                       dps_node->ip6, 16,
		                                       domain_id,
		                                       endpoints,
		                                       tunnels,
		                                       endpoint_updates,
		                                       endpoint_lookups,
		                                       policy_lookups);
		log_debug(PythonClusterDataLogLevel, "strargs %p", strargs);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Node_Update_Domain_Statistics");
			break;
		}
		// Invoke the Node_Update_Domain_Statistics call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Update_Domain_Statistics, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject: Node_Update_Domain_Statistics returns NULL");
			break;
		}
		Py_DECREF(strret);
	} while (0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_get_lowest_loaded_nodes                                            *//**
 *
 * \brief - This function will provide a list of all DPS nodes in the cluster.
 *          This function will be used to communicate local domain mapping to
 *          all the nodes in the cluster.
 *
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_lowest_loaded_nodes(uint32_t num_nodes,
                                        ip_addr_t *nodes,
                                        uint32_t *num_nodes_returned)
{
	int i, returned_nodes;
	dove_status status = DOVE_STATUS_OK;
	PyObject *strargs, *strret;
	PyObject *pyNodeIPList;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel, "Enter");

	returned_nodes = 0;

	gstate = PyGILState_Ensure();
	do {
		//def Get_Lowest_Loaded_Available_Nodes(self, desired_number):
		strargs = Py_BuildValue("(I)",num_nodes);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Get_Lowest_Loaded_Available_Nodes");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		// Invoke the Get_Lowest_Loaded_Available_Nodes call
		strret = PyEval_CallObject(Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Get_Lowest_Loaded_Available_Nodes returns NULL");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		if (!PyArg_Parse(strret, "O", &pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			break;
		}
		for (i = 0; i < PyList_Size(pyNodeIPList); i++)
		{
			PyObject *pyIPAddress;
			char *IP_packed;
			int IP_packed_size;

			pyIPAddress = PyList_GetItem(pyNodeIPList, i);
			if (!PyArg_Parse(pyIPAddress, "z#", &IP_packed, &IP_packed_size))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid Address in element %d", i);
				continue;
			}
			nodes->family = (IP_packed_size == 4) ? AF_INET : AF_INET6;
			memcpy(nodes->ip6, IP_packed, IP_packed_size);
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(nodes->family, nodes->ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel, "DPS Node[%d]: IP %s", i, str);
			}
#endif
			nodes++;
			returned_nodes++;
		}
		Py_DECREF(strret);

	} while (0);
	PyGILState_Release(gstate);

	*num_nodes_returned = returned_nodes;

	log_info(PythonClusterDataLogLevel,
	         "Exit: Status %s, Num Nodes Returned %d",
	         DOVEStatusToString(status), *num_nodes_returned
	         );

	return status;
}

/*
 ******************************************************************************
 * dps_get_lowest_loaded_nodes_not_handling                                            *//**
 *
 * \brief - This function will return a list of least loaded nodes not handling
 * 			a particular domain.
 *
 * \param domain	 Domain Id.
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_lowest_loaded_nodes_not_handling(uint32_t domain,
                                                     uint32_t num_nodes,
                                                     ip_addr_t *nodes,
                                                     uint32_t *num_nodes_returned)
{
	int i, returned_nodes;
	dove_status status = DOVE_STATUS_OK;
	PyObject *strargs, *strret;
	PyObject *pyNodeIPList;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel,
	         "Enter: domain id %d, num nodes %d",
	         domain, num_nodes);

	returned_nodes = 0;

	gstate = PyGILState_Ensure();
	do {
		//def Get_Lowest_Loaded_Available_Nodes(self, desired_number):
		strargs = Py_BuildValue("(II)",num_nodes,domain);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Get_Lowest_Loaded_Available_Nodes_Not_Hosting");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		// Invoke the Get_Lowest_Loaded_Available_Nodes_Not_Hosting call
		strret = PyEval_CallObject(Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes_Not_Hosting,
		                           strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Get_Lowest_Loaded_Available_Nodes_Not_Hosting returns NULL");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		if (!PyArg_Parse(strret, "O", &pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			break;
		}
		for (i = 0; i < PyList_Size(pyNodeIPList); i++)
		{
			PyObject *pyIPAddress;
			char *IP_packed;
			int IP_packed_size;

			pyIPAddress = PyList_GetItem(pyNodeIPList, i);
			if (!PyArg_Parse(pyIPAddress, "z#", &IP_packed, &IP_packed_size))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid Address in element %d", i);
				continue;
			}
			nodes->family = (IP_packed_size == 4) ? AF_INET : AF_INET6;
			memcpy(nodes->ip6, IP_packed, IP_packed_size);
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(nodes->family, nodes->ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel, "DPS Node[%d]: IP %s", i, str);
			}
#endif
			nodes++;
			returned_nodes++;
		}
		Py_DECREF(strret);

	} while (0);
	PyGILState_Release(gstate);

	*num_nodes_returned = returned_nodes;

	log_info(PythonClusterDataLogLevel,
	         "Exit: Status %s, Num Nodes Returned %d",
	         DOVEStatusToString(status), *num_nodes_returned
	         );

	return status;
}

/*
 ******************************************************************************
 * dps_get_highest_loaded_nodes_hosting                                            *//**
 *
 * \brief - This function will return a list of heavily loaded nodes handling
 *     	    a particular domain.
 *
 * \param domain	 Domain Id.
 * \param num_nodes  The number of lowest loaded nodes needed
 * \param nodes      The list of IP Address to be filled by the this routine.
 *                   The calling module must allocate this structure with at
 *                   least num_nodes entries
 * \param num_nodes_returned The number of nodes actually returned
 *
 * \retval DOVE_STATUS_OK
 * \retval DOVE_STATUS_EXCEEDS_CAP number of nodes actually returned is less
 *                                 than number of nodes needed.
 * \retval DOVE_STATUS_NO_MEMORY No memory
 *
 ******************************************************************************
 */
dove_status dps_get_highest_loaded_nodes_hosting(uint32_t domain,
                                                     uint32_t num_nodes,
                                                     ip_addr_t *nodes,
                                                     uint32_t *num_nodes_returned)
{
	int i, returned_nodes;
	dove_status status = DOVE_STATUS_OK;
	PyObject *strargs, *strret;
	PyObject *pyNodeIPList;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel,
	         "Enter: domain id %d, num nodes %d",
	         domain, num_nodes);

	returned_nodes = 0;

	gstate = PyGILState_Ensure();
	do {
		//def Get_Highest_Loaded_Nodes_Hosting(self, desired_number, domain_id):
		strargs = Py_BuildValue("(II)",num_nodes,domain);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Get_Highest_Loaded_Nodes_Hosting");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		// Invoke the Get_Highest_Loaded_Nodes_Hosting call
		strret = PyEval_CallObject(Cluster_DB_Interface.Get_Highest_Loaded_Nodes_Hosting,
		                           strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Get_Highest_Loaded_Nodes_Hosting returns NULL");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		if (!PyArg_Parse(strret, "O", &pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyNodeIPList))
		{
			Py_DECREF(strret);
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			break;
		}
		for (i = 0; i < PyList_Size(pyNodeIPList); i++)
		{
			PyObject *pyIPAddress;
			char *IP_packed;
			int IP_packed_size;

			pyIPAddress = PyList_GetItem(pyNodeIPList, i);
			if (!PyArg_Parse(pyIPAddress, "z#", &IP_packed, &IP_packed_size))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid Address in element %d", i);
				continue;
			}
			nodes->family = (IP_packed_size == 4) ? AF_INET : AF_INET6;
			memcpy(nodes->ip6, IP_packed, IP_packed_size);
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(nodes->family, nodes->ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel, "DPS Node[%d]: IP %s", i, str);
			}
#endif
			nodes++;
			returned_nodes++;
		}
		Py_DECREF(strret);

	} while (0);
	PyGILState_Release(gstate);

	*num_nodes_returned = returned_nodes;

	log_info(PythonClusterDataLogLevel,
	         "Exit: Status %s, Num Nodes Returned %d",
	         DOVEStatusToString(status), *num_nodes_returned
	         );

	return status;
}

dove_status dps_node_get_domains(ip_addr_t node_ip,char ** domains)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int domain_str_size;
	char *domain_str;

	log_info(PythonClusterDataLogLevel, "Enter");

#if defined(NDEBUG)
	{
		char str[INET6_ADDRSTRLEN];
		inet_ntop(node_ip.family, node_ip.ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel,
		         "Node IP %s, Port %d", str, node_ip.port);
	}
#endif

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Node_Get_Domains(self, ip_type, ip_packed):
		strargs = Py_BuildValue("(Iz#)", node_ip.family, node_ip.ip6, 16);
		if (strargs == NULL)
		{
			status = DOVE_STATUS_NO_MEMORY;
		}

		// Invoke the Node_Delete_All_Domains call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Get_Domains, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Get_Domains returns NULL");
			break;
		}
		//@return: Tuple of the form (status,domain_str)
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_ParseTuple(strret, "Iz#", &status, &domain_str, &domain_str_size);

		if (status == DOVE_STATUS_OK)
		{
			*domains = (char *)malloc(domain_str_size+1);
			if(*domains == NULL) {
				log_error(PythonClusterDataLogLevel,
				          "Could not allocate memory for local domain string");
				status = DOVE_STATUS_NO_MEMORY;
				break;
			}
			memset(*domains,0,domain_str_size+1);
			memcpy(*domains,domain_str,domain_str_size);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}

/*
 ******************************************************************************
 * dps_domain_get_replication_factor --                                  *//**
 *
 * \brief This routine returns the number of nodes handling a domain.
 *
 * \param domain_id[in]		The Domain ID
 * \param rep_factor[out]	Current Replication Factor
 *
 * \retval DOVE_STATUS_OK, rep_factor
 *
 *****************************************************************************/
dove_status dps_domain_get_replication_factor(unsigned int domain_id,
                                              unsigned int *rep_factor)
{
	int status = DOVE_STATUS_OK;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_info(PythonClusterDataLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Get_Replication_Factor(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			status = DOVE_STATUS_NO_MEMORY;
		}


		strret = PyEval_CallObject(Cluster_DB_Interface.Domain_Get_Replication_Factor,
		                           strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Domain_Get_Replication_Factor returns NULL");
			break;
		}
		//@return: Tuple of the form (status,domain_str)
		//@rtype: dove_status (defined in include/status.h) Integer
		PyArg_Parse(strret, "I", rep_factor);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	} while(0);

	PyGILState_Release(gstate);
	log_info(PythonClusterDataLogLevel, "Exit: Status %s",
	         DOVEStatusToString((dove_status)status));

	return (dove_status)status;
}
/*
 ******************************************************************************
 * dps_cluster_is_local_node_leader                                       *//**
 *
 * \brief - This routine check if the current node is the cluster leader
 *
 * \retval 1 Yes
 * \retval 0 No
 *
 ******************************************************************************
 */
int dps_cluster_is_local_node_leader(void)
{
	return fLocalLeader;
}

/*
 ******************************************************************************
 * dps_cluster_is_local_node_active                                       *//**
 *
 * \brief - This routine check if the current node is the cluster leader
 *
 * \retval 1 Yes
 * \retval 0 No
 *
 ******************************************************************************
 */
int dps_cluster_is_local_node_active(void)
{
	return fLocalActive;
}

/*
 ******************************************************************************
 * dps_cluster_get_domainid_from_vnid --                                  *//**
 *
 * \brief This routine returns a domain ID which the virtual network (VNID)
 *        is associated with.
 *
 * \param vnid[in]		The Virtual Network ID
 * \param domain_id[out]	The Domain ID
 *
 * \retval DOVE_STATUS_OK, domain_id
 * \retval DOVE_STATUS_INVALID_DVG, 0
 *
 *****************************************************************************/

dove_status dps_cluster_get_domainid_from_vnid(int vnid, int *domain_id)
{
	int status = DOVE_STATUS_NO_MEMORY;
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;

	log_debug(PythonDataHandlerLogLevel, "Enter");

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Get_DomainID_From_VNID(self, vn_id)
		strargs = Py_BuildValue("(I)", vnid);
		if (strargs == NULL)
		{
			log_notice(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Get_DomainID_From_VNID call
		strret = PyEval_CallObject(Cluster_DB_Interface.Get_DomainID_From_VNID, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject IP_Subnet_GetAllIds returns NULL");
			break;
		}
		//@return: status, domain_id
		//@rtype: Integer, Integer
		PyArg_ParseTuple(strret, "iI", &status, domain_id);

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
 * dps_cluster_write_log --                                               *//**
 *
 * \brief This routine writes logs sent by the Python Cluster Handler to the
 *        Logs
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_write_log(PyObject *self, PyObject *args)
{
	int log_level;
	char *log_string;
	PyObject *ret_val;

	do
	{
		if (!PyArg_ParseTuple(args, "is", &log_level, &log_string))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		switch(log_level)
		{
			case DPS_SERVER_LOGLEVEL_INFO:
				log_info(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_NOTICE:
				log_notice(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_WARNING:
				log_warn(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_ERROR:
				log_error(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_CRITICAL:
				log_critical(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_ALERT:
				log_critical(PythonClusterDataLogLevel, "%s", log_string);
				break;
			case DPS_SERVER_LOGLEVEL_EMERGENCY:
				log_emergency(PythonClusterDataLogLevel, "%s", log_string);
				break;
			default:
				break;
		}

	}while(0);

	ret_val = Py_BuildValue("i", 0);

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_send_local_mapping_to --                                   *//**
 *
 * \brief This routine sends local domain mapping to specific nodes in the
 *        DCS Cluster
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_local_mapping_to(PyObject *self, PyObject *args)
{
	PyObject *pyNodeList, *pyNodeIP;
	ip_addr_t *remote_nodes = NULL;
	void *remote_nodes_memory = NULL;
	char *local_domain_str, *ipv6;
	int i, ipv4, local_domain_str_size, ipv6_size, num_remote_nodes;
	PyObject *ret_val;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "Oz#",
		                      &pyNodeList,
		                      &local_domain_str,
		                      &local_domain_str_size))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyNodeList))
		{
			log_warn(PythonClusterDataLogLevel, "Node List NOT List Type!!!");
			break;
		}
		remote_nodes_memory = malloc(sizeof(ip_addr_t)*PyList_Size(pyNodeList));
		if (remote_nodes_memory == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Cannot allocate memory for remote nodes!!!");
			break;
		}
		remote_nodes = (ip_addr_t *)remote_nodes_memory;
		num_remote_nodes = 0;
		for (i = 0; i < PyList_Size(pyNodeList); i++)
		{
			pyNodeIP = PyList_GetItem(pyNodeList, i);
			if (PyInt_CheckExact(pyNodeIP))
			{
				if (!PyArg_Parse(pyNodeIP, "I", &ipv4))
				{
					log_warn(PythonClusterDataLogLevel,
					         "Invalid IPv4 in element %d", i);
					continue;
				}
				inet_ntop(AF_INET, &ipv4, str, INET_ADDRSTRLEN);
				remote_nodes->family = AF_INET;
				remote_nodes->ip4 = ipv4;
			}
			else
			{
				if (!PyArg_Parse(pyNodeIP, "z#", &ipv6, &ipv6_size))
				{
					log_warn(PythonClusterDataLogLevel,
					         "Invalid IPv6 in element %d", i);
					continue;
				}
				remote_nodes->family = AF_INET6;
				memcpy(remote_nodes->ip6, ipv6, ipv6_size);
			}
			log_info(PythonClusterDataLogLevel,
			         " [%d] Mapping Exchange with Remote DPS Node %s",
			         i, str);
			remote_nodes->port_http = DPS_REST_HTTPD_PORT;
			remote_nodes++;
			num_remote_nodes++;
		}
		// Send Local Domain mapping to the nodes via REST
		dps_cluster_send_local_domains((ip_addr_t *)remote_nodes_memory,
		                               num_remote_nodes,
		                               local_domain_str);

	}while(0);

	if (remote_nodes_memory)
	{
		free(remote_nodes_memory);
		remote_nodes_memory = NULL;
		remote_nodes = NULL;
	}

	ret_val = Py_BuildValue("i", 0);
	log_info(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_send_heartbeat_to --                                       *//**
 *
 * \brief This routine sends heartbeat message to the remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_heartbeat_to(PyObject *self, PyObject *args)
{
	ip_addr_t remote_node;
	PyObject *ret_val;
	int factive;
	long long config_version;
	char *remote_ip;
	int remote_ip_size;

	//log_debug(PythonClusterDataLogLevel, "Enter");
	do
	{
		if (!PyArg_ParseTuple(args, "iLz#",
		                      &factive,
		                      &config_version,
		                      &remote_ip,
		                      &remote_ip_size))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (remote_ip == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Remote IP Not set");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
//#if defined(NDEBUG)
//		{
//			char str[INET6_ADDRSTRLEN];
//			inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
//			log_info(PythonClusterDataLogLevel,
//			         "Remote Node IP Address %s", str);
//		}
//#endif

		dps_rest_heartbeat_send_to_dps_node(&remote_node,
		                                    factive,
		                                    config_version);

	}while(0);
	ret_val = Py_BuildValue("i", 0);
	//log_debug(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_send_heartbeat_request_to --                               *//**
 *
 * \brief This routine sends heartbeat request message to the remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_heartbeat_request_to(PyObject *self, PyObject *args)
{
	ip_addr_t remote_node;
	char *remote_ip;
	int remote_ip_size;
	PyObject *ret_val;

	//log_info(PythonClusterDataLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "z#", &remote_ip, &remote_ip_size))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (remote_ip == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Remote IP Not set");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
//#if defined(NDEBUG)
//		{
//			char str[INET6_ADDRSTRLEN];
//			inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
//			log_info(PythonClusterDataLogLevel,
//			         "Remote Node IP Address %s", str);
//		}
//#endif

		dps_rest_heartbeat_request_send_to_dps_node(&remote_node);

	}while(0);

	ret_val = Py_BuildValue("i", 0);
	//log_info(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_set_leader --                                              *//**
 *
 * \brief This routine sets the cluster_config_version variable
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_set_cluster_config_version(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	do
	{
		if (!PyArg_ParseTuple(args, "L", &cluster_config_version))
		{
			log_warn(PythonClusterDataLogLevel, "Cannot set cluster config version");
			break;
		}
		log_info(PythonClusterDataLogLevel,
		         "Cluster Config Version %ld",
		         cluster_config_version);
	}while(0);
	ret_val = Py_BuildValue("i", 0);
	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_set_leader --                                              *//**
 *
 * \brief This routine sets the Leader in the C code
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_set_leader(PyObject *self, PyObject *args)
{
	char *leader_ip;
	int leader_ip_size, local_leader;
	uint16_t leader_port;
	PyObject *ret_val;

	log_info(PythonClusterDataLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "Iz#H",
		                      &local_leader,
		                      &leader_ip,
		                      &leader_ip_size,
		                      &leader_port))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		fLocalLeader = local_leader;
		if (leader_ip == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Remote IP Not set");
			break;
		}
		if (leader_ip_size == 4)
		{
			dps_cluster_leader.family = AF_INET;
		}
		else
		{
			dps_cluster_leader.family = AF_INET6;
		}
		memcpy(dps_cluster_leader.ip6, leader_ip, leader_ip_size);
		dps_cluster_leader.port = leader_port;
		inet_ntop(dps_cluster_leader.family,
		          dps_cluster_leader.ip6,
		          dps_cluster_leader_ip_string,
		          INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel, "CLUSTER LEADER == >> [%s]",
		         dps_cluster_leader_ip_string);
		//if ((fLocalLeader) && (fLocalActive))
		//{
		//	dps_cluster_leader_reporting();
		//}
	}while(0);

	ret_val = Py_BuildValue("i", 0);
	log_info(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_send_nodes_status_to --                                    *//**
 *
 * \brief This routine sends node list to the specific remote node
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_send_nodes_status_to(PyObject *self, PyObject *args)
{
	PyObject *pyNodeList, *pyNodeTuple;
	PyObject *ret_val;
	ip_addr_t remote_node;
	ip_addr_t *nodes_status = NULL;
	long long config_version;
	void *nodes_status_memory = NULL;
	char *remote_ip, *node_ip;
	int i, remote_ip_size, node_ip_size, num_nodes;
//#if defined(NDEBUG)
//	char str[INET6_ADDRSTRLEN];
//#endif

	//log_debug(PythonClusterDataLogLevel, "Enter");

	do
	{
		if (!PyArg_ParseTuple(args, "Lz#O",
		                      &config_version,
		                      &remote_ip,
		                      &remote_ip_size,
		                      &pyNodeList))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if(!PyList_Check(pyNodeList))
		{
			log_warn(PythonClusterDataLogLevel,
			         "Node List NOT List Type!!!");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
//#if defined(NDEBUG)
//		inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
//		log_debug(PythonClusterDataLogLevel,
//		          "Sending Nodes Status to DPS Node %s", str);
//#endif
		nodes_status_memory = malloc(sizeof(ip_addr_t)*PyList_Size(pyNodeList));
		if (nodes_status_memory == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot allocate memory for remote nodes!!!");
			break;
		}
		nodes_status = (ip_addr_t *)nodes_status_memory;
		num_nodes = 0;
		for (i = 0; i < PyList_Size(pyNodeList); i++)
		{
			pyNodeTuple = PyList_GetItem(pyNodeList, i);
			if (!PyArg_ParseTuple(pyNodeTuple, "z#I",
			                      &node_ip, &node_ip_size,
			                      &nodes_status->status))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid IP in element %d", i);
				continue;
			}
			if (node_ip_size == 4)
			{
				nodes_status->family = AF_INET;
			}
			else
			{
				nodes_status->family = AF_INET6;
			}
			memcpy(nodes_status->ip6, node_ip, node_ip_size);
//#if defined(NDEBUG)
//			inet_ntop(nodes_status->family, nodes_status->ip6, str, INET6_ADDRSTRLEN);
//			log_info(PythonClusterDataLogLevel,
//			         "[%d] DPS Node %s, Status %d",
//			         num_nodes, str, nodes_status->status);
//#endif
			nodes_status++;
			num_nodes++;
		}
		dps_rest_nodes_status_send_to(&remote_node,
		                              config_version,
		                              (ip_addr_t*) nodes_status_memory,
		                              num_nodes);

	}while(0);

	if (nodes_status_memory)
	{
		free(nodes_status_memory);
		nodes_status_memory = NULL;
		nodes_status = NULL;
	}

	ret_val = Py_BuildValue("i", 0);
//	log_debug(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_domain_activate_on_node --                                    *//**
 *
 * \brief This routine activates a domain on a node
 * 	  Typically used after Mass transfer is complete
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_activate_on_node(PyObject *self, PyObject *args)
{
	char *remote_ip;
	int remote_ip_size, domain_id;
	uint32_t replication_factor;
	ip_addr_t remote_node;
	int ret = DOVE_STATUS_NO_RESOURCES;
	PyObject *ret_val;

	log_info(PythonClusterDataLogLevel, "Enter");

	do {
		if (!PyArg_ParseTuple(args, "z#II",&remote_ip,&remote_ip_size,
		                      &domain_id, &replication_factor))
		{
			ret = DOVE_STATUS_INVALID_PARAMETER;
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (remote_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Bad Remote DPS IP Address!!!");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
		// Check if this is local node
		if (!memcmp(remote_node.ip6, dps_local_ip.ip6, remote_ip_size))
		{
			dps_controller_data_op_t data_op;
			data_op.type = DPS_CONTROLLER_DOMAIN_ACTIVATE;
			data_op.domain_add.domain_id = domain_id;
			data_op.domain_add.replication_factor = replication_factor;
			ret = dps_controller_data_msg(&data_op);
			if (ret == DOVE_STATUS_OK)
			{
				dps_cluster_exchange_domain_mapping_activate(&dps_local_ip);
			}
		}
		else
		{
			ret = dps_node_domain_activate(&remote_node, domain_id, replication_factor);
		}
	} while (0);

	log_info(PythonClusterDataLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)ret));
	ret_val = Py_BuildValue("i", ret);

	return ret_val;
}

/*
 ******************************************************************************
 * dps_domain_recover_on_node --                                          *//**
 *
 * \brief This routine asks a node to recover a domain.
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_recover_on_node(PyObject *self, PyObject *args)
{
	char *remote_ip;
	int remote_ip_size, domain_id;
	uint32_t replication_factor;
	ip_addr_t remote_node;
	int ret = DOVE_STATUS_NO_RESOURCES;
	PyObject *ret_val;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");

	do {
		if (!PyArg_ParseTuple(args, "z#II",&remote_ip,&remote_ip_size,
		                      &domain_id, &replication_factor))
		{
			ret = DOVE_STATUS_INVALID_PARAMETER;
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (remote_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Bad Remote DPS IP Address!!!");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
		inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
		log_notice(PythonClusterDataLogLevel,
		           "Requesting DCS Node %s to recreate Domain %d",
		           str, domain_id);
		// Check if this is local node
		if (!memcmp(remote_node.ip6, dps_local_ip.ip6, remote_ip_size))
		{
			dps_controller_data_op_t data_op;
			data_op.type = DPS_CONTROLLER_DOMAIN_RECOVERY_START;
			data_op.domain_recover.domain_id = domain_id;
			data_op.domain_recover.replication_factor = replication_factor;
			ret = dps_controller_data_msg(&data_op);
		}
		else
		{
			ret = dps_node_domain_recover(&remote_node, domain_id, replication_factor);
		}
	} while (0);

	log_info(PythonClusterDataLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)ret));
	ret_val = Py_BuildValue("i", ret);

	return ret_val;
}

/*
 ******************************************************************************
 * dps_domain_deactivate_on_node --                                    *//**
 *
 * \brief This routine deactivates a domain on a node. Typically used after
 *        Mass transfer is complete
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_domain_deactivate_on_node(PyObject *self, PyObject *args)
{
	char *remote_ip;
	int remote_ip_size, domain_id;
	ip_addr_t remote_node;
	int ret = DOVE_STATUS_NO_RESOURCES;
	PyObject *ret_val;

	log_info(PythonClusterDataLogLevel, "Enter");

	do {
		if (!PyArg_ParseTuple(args, "z#I",&remote_ip,&remote_ip_size,
		                      &domain_id))
		{
			ret = DOVE_STATUS_INVALID_PARAMETER;
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (remote_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Bad Remote DPS IP Address!!!");
			break;
		}
		if (remote_ip_size == 4)
		{
			remote_node.family = AF_INET;
		}
		else
		{
			remote_node.family = AF_INET6;
		}
		memcpy(remote_node.ip6, remote_ip, remote_ip_size);
		remote_node.port_http = DPS_REST_HTTPD_PORT;
		// Check if this is local node
		if (!memcmp(remote_node.ip6, dps_local_ip.ip6, remote_ip_size))
		{
			//Do it on local node
			dps_controller_data_op_t data_op;
			data_op.type = DPS_CONTROLLER_DOMAIN_DEACTIVATE;
			data_op.domain_delete.domain_id = domain_id;
			ret = dps_controller_data_msg(&data_op);
		}
		else
		{
			//Send request to remote node
			ret = dps_node_domain_deactivate(&remote_node, domain_id);
		}
	} while (0);

	log_info(PythonClusterDataLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)ret));
	ret_val = Py_BuildValue("i", ret);

	return ret_val;
}


/*
 ******************************************************************************
 * dps_cluster_initiate_mass_transfer --                                   *//**
 *
 * \brief This route is used to initiate domain mass transfer from Python code.
 *
 * \return PyObject
 *
 *****************************************************************************/

PyObject *dps_cluster_initiate_mass_transfer(PyObject *self, PyObject *args)
{
	char *src_ip, *dst_ip;
	int src_ip_size,dst_ip_size, domain_id;
	int ret = DOVE_STATUS_NO_RESOURCES;
	PyObject *ret_val;
	ip_addr_t src_node,dst_node;

	log_info(PythonClusterDataLogLevel, "Enter");

	do {
		if (!PyArg_ParseTuple(args, "z#z#I",
		                      &src_ip,&src_ip_size,
		                      &dst_ip,&dst_ip_size,
		                      &domain_id))
		{
			ret = DOVE_STATUS_INVALID_PARAMETER;
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		if (src_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Bad SRC DPS IP Address!!!");
			break;
		}
		if (dst_ip == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,
			         "Bad DST DPS IP Address!!!");
			break;
		}

		if(src_ip_size < 0)
		{
			log_debug(PythonClusterDataLogLevel,"ERROR: src ip size ==> %d",src_ip_size);
			break;
		}
		else if(src_ip_size == 4) {
			memcpy(&src_node.ip6,src_ip,src_ip_size);
			src_node.family = AF_INET;
		}
		else
		{
			memcpy(&src_node.ip6,src_ip,src_ip_size);
			src_node.family = AF_INET6;
		}

		if(dst_ip_size < 0)
		{
			log_error(PythonClusterDataLogLevel,
			          "ERROR: dst ip size ==> %d",dst_ip_size);
			break;
		}
		else if(dst_ip_size == 4) {
			memcpy(&dst_node.ip6,dst_ip,dst_ip_size);
			dst_node.family = AF_INET;
		}
		else
		{
			memcpy(&dst_node.ip6,dst_ip,dst_ip_size);
			dst_node.family = AF_INET6;
		}
		ret = dps_cluster_leader_initiate_domain_move(domain_id,&src_node,&dst_node);

		if (ret != DOVE_STATUS_OK)
		{
			log_error(PythonClusterDataLogLevel,
			          "ERROR: dps_cluster_leader_initiate_domain_move returned [%d]",ret);
		}
	} while (0);

	log_info(PythonClusterDataLogLevel, "Exit status %s",
	         DOVEStatusToString((dove_status)ret));
	ret_val = Py_BuildValue("i", ret);

	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_exchange_domain_mapping --                                 *//**
 *
 * \brief This routine sends local domain mapping to all the nodes the cluster
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_exchange_domain_mapping()
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Mapping_Exchange()
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel,
			           "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Mapping_Exchange call
		PyEval_CallObject(Cluster_DB_Interface.Domain_Mapping_Exchange, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return;
}

/*
 ******************************************************************************
 * dps_cluster_exchange_domain_mapping_activate --                        *//**
 *
 * \brief This routine sends local domain mapping to all the nodes the cluster
 *        after domain activate. The remote node that activated the local node
 *        is sent last.
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_exchange_domain_mapping_activate(ip_addr_t *remote_node)
{
	PyGILState_STATE gstate;
	PyObject *strargs;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonClusterDataLogLevel, "Enter");
	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		inet_ntop(remote_node->family, remote_node->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel,
		         "Domain Activate: Local Mapping Exchange, Initiator %s",
		         str);

		//def Domain_Mapping_Exchange()
		strargs = Py_BuildValue("(Iz#)",
		                        remote_node->family,
		                        remote_node->ip6, 16);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel,
			           "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_Mapping_Exchange call
		PyEval_CallObject(Cluster_DB_Interface.Domain_Activate_Mapping_Exchange_Synchronous,
		                  strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_touch --                                              *//**
 *
 * \brief This routine should be called whenever a REST request/reply is
 *        received from in the REST interface. This routine will invoke the
 *        Python callback that deals with cluster interaction and mark the
 *        sending node as "up" if it belongs in the cluster.
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_touch(ip_addr_t *node_ip)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_debug(PythonClusterDataLogLevel, "Node IP %s", str);
		}
#endif
		//def def Node_Touch(self, node_list):
		strargs = Py_BuildValue("(Iz#)", node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Node_Touch call
		PyEval_CallObject(Cluster_DB_Interface.Node_Touch, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_validate --                                           *//**
 *
 * \brief This routine should be called to valid an IP address as a DCS Node
 *        IP Address
 *
 * \param node_ip The IP Address of the Node
 *
 * \return 1 if Valid, 0 if not valid
 *
 * \return void
 *
 *****************************************************************************/
int dps_cluster_node_validate_remote(char *node_ip_str)
{
	PyGILState_STATE gstate;
	PyObject *strargs, *strret;
	int valid = 0;
	char *ptr;
	ip_addr_t node;

	log_info(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		log_info(PythonClusterDataLogLevel, "Node IP %s", node_ip_str);
		ptr = strchr(node_ip_str,':');
		if (NULL == ptr) { // IPv4
			node.family = AF_INET;
			inet_pton(AF_INET,node_ip_str,node.ip6);
		}
		else {	// IPv6
			node.family = AF_INET6;
			inet_pton(AF_INET6,node_ip_str,node.ip6);
		}
		//def def Node_Touch(self, node_list):
		strargs = Py_BuildValue("(Iz#)", node.family, node.ip6, 16);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Node_Touch call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Validate_Remote_DCS, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Validate_Remote_DCS returns NULL");
			break;
		}
		// @rtype: (Integer, ByteArray)
		PyArg_Parse(strret, "i", &valid);
		Py_DECREF(strret);
	}while(0);
	PyGILState_Release(gstate);

	log_info(PythonClusterDataLogLevel, "Exit: Valid %d", valid);
	return valid;
}

/*
 ******************************************************************************
 * dps_cluster_node_show_load --                                          *//**
 *
 * \brief This routine will show the load on a dps node in the cluster
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_show_load(ip_addr_t *node_ip)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_debug(PythonClusterDataLogLevel, "Node IP %s", str);
		}
#endif
		//def def Node_Touch(self, node_list):
		strargs = Py_BuildValue("(Iz#)", node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Node_Show_Load");
			break;
		}
		// Invoke the Node_Show_Load call
		PyEval_CallObject(Cluster_DB_Interface.Node_Show_Load, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat --                                          *//**
 *
 * \brief This routine should be called whenever a Heartbeat message is
 *        received from a remote node
 *
 * \param node_ip The IP Address of the Node
 * \param factive Whether the node is active
 * \param dmc_config_version The DMC Version Number of the node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat(ip_addr_t *node_ip, int factive,
                                long long dmc_config_version)
{
	PyGILState_STATE gstate;
	PyObject *strret, *strargs;
	int status;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		{
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel, "Node IP %s", str);
		}
		//def def Node_Heartbeat(self, node_list):
		strargs = Py_BuildValue("(Iz#Li)",
		                        node_ip->family, node_ip->ip6, 16,
		                        dmc_config_version, factive);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Node_Heartbeat call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Heartbeat, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Heartbeat returns NULL");
			break;
		}
		// @rtype: (Integer, ByteArray)
		PyArg_Parse(strret, "I", &status);
		if (status != DOVE_STATUS_OK)
		{
			log_notice(PythonClusterDataLogLevel,
			           "Node_Heartbeat Status %s",
			           DOVEStatusToString((dove_status)status));
		}
		Py_DECREF(strret);
	}while(0);
	PyGILState_Release(gstate);
	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat_request --                                  *//**
 *
 * \brief This routine should be called whenever a Heartbeat Request message is
 *        received from a remote node
 *
 * \param node_ip The IP Address of the Node
 *
 * \return void
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat_request(ip_addr_t *node_ip)
{
	PyGILState_STATE gstate;
	PyObject *strret, *strargs;
	int status;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		{
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel, "Node IP %s", str);
		}
		//def def Node_Heartbeat_Request(self, node_list):
		strargs = Py_BuildValue("(Iz#)", node_ip->family, node_ip->ip6, 16);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Node_Heartbeat_Request call
		strret = PyEval_CallObject(Cluster_DB_Interface.Node_Heartbeat_Request, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Node_Heartbeat_Request returns NULL");
			break;
		}
		// @rtype: (Integer, ByteArray)
		PyArg_Parse(strret, "I", &status);
		if (status != DOVE_STATUS_OK)
		{
			log_notice(PythonClusterDataLogLevel,
			           "Node_Heartbeat_Request Status %s",
			           DOVEStatusToString((dove_status)status));
		}
		Py_DECREF(strret);
	}while(0);
	PyGILState_Release(gstate);
	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_node_heartbeat_process --                                  *//**
 *
 * \brief Whether to process heartbeat messages
 *
 * \param dps_heartbeat_process - 1/Process, 0/Don't Process
 *
 * \return None
 *
 *****************************************************************************/
void dps_cluster_node_heartbeat_process(uint32_t dps_heartbeat_process)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		//def Node_Heartbeat_Process(self, fProcess):
		strargs = Py_BuildValue("(I)", dps_heartbeat_process);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Node_Heartbeat_Process");
			break;
		}
		// Invoke the Node_Heartbeat_Process call
		PyEval_CallObject(Cluster_DB_Interface.Node_Heartbeat_Process, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_create_shared_domain                                       *//**
 *
 * \brief This routine create Domain 0 and VNID 0
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_create_shared_domain(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	dove_status status;

	log_info(PythonClusterDataLogLevel, "Enter");
	do
	{
		// Create domain-0 vnid-0
		status = dps_leader_create_domain(SHARED_ADDR_SPACE_DOMAIN_ID,
		                                  (char *)SHARED_ADDR_SPACE_DOMAIN_NAME);
		if (status != DOVE_STATUS_OK) {
			log_error(PythonClusterDataLogLevel,
			          "Domain [%d] creation FAILED...",
			          SHARED_ADDR_SPACE_DOMAIN_ID);
			break;
		}

		status = dps_leader_create_vn(SHARED_ADDR_SPACE_DOMAIN_ID,
		                              SHARED_ADDR_SPACE_VNID,
		                              (char *)SHARED_ADDR_SPACE_VN_NAME);
		if (status != DOVE_STATUS_OK) {
			log_error(PythonClusterDataLogLevel,
			          "VNID [%d] creation FAILED...",
			          SHARED_ADDR_SPACE_VNID);
			break;
		}
	}while(0);
	log_info(PythonClusterDataLogLevel, "Exit");
	ret_val = Py_BuildValue("i", 0);
	return ret_val;
}

/*
 ******************************************************************************
 * dps_cluster_nodes_from_dmc_update --                                   *//**
 *
 * \brief This routine updates the (remote) nodes of the cluster with the
 *        list provided in this list. The Local node may be included in this
 *        list too. But if the Local node is not included it doesn't mean
 *        that the local node is deleted.
 *
 * \param nodes - An array of ip addresse
 * \return void
 *
 *****************************************************************************/

void dps_cluster_nodes_from_dmc_update(ip_addr_t *nodes, int num_nodes)
{
	PyObject *pyNodeList = NULL, *pyNodeTuple = NULL;
	PyGILState_STATE gstate;
	PyObject *strargs;
	ip_addr_t *node_ip;
	int i;
	dove_status status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	log_info(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		pyNodeList = PyList_New(num_nodes);
		if (pyNodeList == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Cannot allocate PyList_New");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		for (i = 0; i < num_nodes; i++)
		{
			node_ip = &nodes[i];
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_notice(PythonClusterDataLogLevel,"[%d] DPS Node IP %s", i, str);
			//@param node_list: List consisting of (ip_type, ip_packed, port) tuples
			//@type node_list: [] of (integer, bytearray, port)
			pyNodeTuple = Py_BuildValue("(Iz#I)",
			                            node_ip->family,
			                            node_ip->ip6, 16,
			                            node_ip->port);
			if (pyNodeTuple == NULL)
			{
				log_warn(PythonClusterDataLogLevel, "Cannot build Py_BuildValue");
				status = DOVE_STATUS_NO_MEMORY;
				break;
			}
			PyList_SetItem(pyNodeList, i, pyNodeTuple);
		}
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		do
		{
			//def Nodes_Update(self, node_list):
			strargs = Py_BuildValue("(O)", pyNodeList);
			if (strargs == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "Cannot build Py_BuildValue for Nodes_Update");
				status = DOVE_STATUS_NO_MEMORY;
				break;
			}
			// Invoke the Nodes_Update call
			PyEval_CallObject(Cluster_DB_Interface.Nodes_Update, strargs);
			Py_DECREF(strargs);
		}while(0);
	}while(0);

	// Free allocated List and Items
	if (pyNodeList != NULL)
	{
		Py_DECREF(pyNodeList);
	}
	PyGILState_Release(gstate);

	log_info(PythonClusterDataLogLevel, "Exit");

	return;
}

/*
 ******************************************************************************
 * dps_cluster_nodes_status_from_leader --                                *//**
 *
 * \brief This routine should be called when the Leader send the List of Nodes
 *        and their status to the other nodes.
 *
 * \param leader The IP Address of the node sending this information.
 * \param config_version: The configuration version as seen by this node
 * \param nodes - An array of ip addresses of the nodes. The status is
 *                indicated by setting the ip_addr_t.status field according to
 *                the following specification.
 *                ip_addr_t.status = 1 Indicates node is up
 *                ip_addr_t.status = 0 Indicates node is down
 * \param num_nodes - The number of nodes in nodes (array)
 *
 * \return void
 *
 *****************************************************************************/

void dps_cluster_nodes_status_from_leader(ip_addr_t *leader,
                                          long long config_version,
                                          ip_addr_t *nodes,
                                          int num_nodes)
{
	PyObject *pyNodeList, *pyNodeTuple;
	PyGILState_STATE gstate;
	PyObject *strret, *strargs;
	ip_addr_t *node_ip;
	int i;
	int status = DOVE_STATUS_OK;
	char str[INET6_ADDRSTRLEN];

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		inet_ntop(leader->family, leader->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonClusterDataLogLevel,
		         "Leader Node IP %s, config version %ld",
		         str, config_version);

		pyNodeList = PyList_New(num_nodes);
		if (pyNodeList == NULL)
		{
			log_warn(PythonClusterDataLogLevel, "Cannot allocate PyList_New");
			break;
		}
		for (i = 0; i < num_nodes; i++)
		{
			node_ip = &nodes[i];
			inet_ntop(node_ip->family, node_ip->ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			          "[%d] DPS Node IP %s, status %d",
			          i, str, node_ip->status);
			//@param dps_nodes: A Collection of [Node IP Type, Node IP Packed, Up/Down (1/0)]
			//type dps_nodes: [] of (Integer, ByteArray, Integer)
			pyNodeTuple = Py_BuildValue("(Iz#I)",
			                            node_ip->family,
			                            node_ip->ip6, 16,
			                            node_ip->status);
			if (pyNodeTuple == NULL)
			{
				status = DOVE_STATUS_NO_MEMORY;
				log_warn(PythonClusterDataLogLevel, "Cannot build Py_BuildValue");
				break;
			}
			PyList_SetItem(pyNodeList, i, pyNodeTuple);
		}
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		do
		{
			//def Nodes_Status(self, leader_ip_type, leader_ip_packed, config_version, dps_nodes):
			strargs = Py_BuildValue("(Iz#LO)",
			                        leader->family,
			                        leader->ip6, 16,
			                        config_version,
			                        pyNodeList);
			if (strargs == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "Cannot build Py_BuildValue for Nodes_Update");
				status = DOVE_STATUS_NO_MEMORY;
				break;
			}
			// Invoke the Nodes_Status call
			strret = PyEval_CallObject(Cluster_DB_Interface.Nodes_Status, strargs);
			Py_DECREF(strargs);
			if (strret == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "PyEval_CallObject Nodes_Status returns NULL");
				status = DOVE_STATUS_NO_RESOURCES;
				break;
			}
			//@return: Whether the calling routine should run the leader election
			//         algorithm
			//@rtype: Integer
			PyArg_Parse(strret, "I", &status);
			Py_DECREF(strret);
			if(status != DOVE_STATUS_OK)
			{
				log_notice(PythonClusterDataLogLevel,
				           "ERROR: Nodes_Status returns %s",
				           DOVEStatusToString((dove_status)status));
			}
		}while(0);
	}while(0);

	// Free allocated List and Items
	if (pyNodeList != NULL)
	{
		Py_DECREF(pyNodeList);
	}

	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_send_domain_delete_to_all_nodes --                         *//**
 *
 * \brief This routine sends the domain delete REST message to all nodes in
 *        the cluster
 *
 * \param domain_id - Domain ID
 *
 * \return None
 *
 *****************************************************************************/
void dps_cluster_send_domain_delete_to_all_nodes(uint32_t domain_id)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter: %d", domain_id);
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Delete_Send_To_All(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Domain_Delete_Send_To_All");
			break;
		}
		// Invoke the Domain_Delete_Send_To_All call
		PyEval_CallObject(Cluster_DB_Interface.Domain_Delete_Send_To_All, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_send_vnid_existence_to_all_nodes --                        *//**
 *
 * \brief This routine sends the vnid add/ delete REST message to all nodes in
 *        the cluster
 *
 * \param vnid - The VNI
 * \param fAdd - Whether it's add (1) or delete (0)
 *
 * \return None
 *
 *****************************************************************************/
void dps_cluster_send_vnid_existence_to_all_nodes(uint32_t vnid, int fAdd)
{
	PyGILState_STATE gstate;
	PyObject *strargs;
	int domain_id;
	dove_status status;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		status = dps_cluster_get_domainid_from_vnid((int)vnid, &domain_id);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		//def VNID_Existence_Send_To_All(self, domain_id, vnid, fAdd):
		strargs = Py_BuildValue("(III)", domain_id, vnid, fAdd);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for VNID_Existence_Send_To_All");
			break;
		}
		// Invoke the VNID_Existence_Send_To_All call
		PyEval_CallObject(Cluster_DB_Interface.VNID_Existence_Send_To_All, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_domain_delete --                                           *//**
 *
 * \brief This routine deletes the Domain from the cluster
 *
 * \param domain_id - Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/
void dps_cluster_domain_delete(uint32_t domain_id)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Delete(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Domain_Delete_Send_To_All call
		PyEval_CallObject(Cluster_DB_Interface.Domain_Delete, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_domain_exists --                                           *//**
 *
 * \brief This routine checks existence of domain
 *
 * \param domain_id - Domain ID
 *
 * \retval 0 Domain doesn't exist
 * \retval 1 Domain exists
 *
 *****************************************************************************/
int dps_cluster_domain_exists(uint32_t domain_id)
{
	PyGILState_STATE gstate;
	PyObject *strargs, *strret;
	int domain_exists = 0;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Exists(self, domain_id):
		strargs = Py_BuildValue("(I)", domain_id);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Domain_Exists");
			break;
		}
		// Invoke the Domain_Delete_Send_To_All call
		strret = PyEval_CallObject(Cluster_DB_Interface.Domain_Exists, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_notice(PythonClusterDataLogLevel,
			           "PyEval_CallObject for Domain_Exists returns NULL");
			break;
		}
		PyArg_Parse(strret, "I", &domain_exists);
		Py_DECREF(strret);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return domain_exists;
}

/*
 ******************************************************************************
 * dps_cluster_heavy_load_threshold --                                    *//**
 *
 * \brief This routine sets the heavy load threshold
 *
 * \param domain_id - Domain ID
 *
 * \return dove_status
 *
 *****************************************************************************/
void dps_cluster_heavy_load_threshold_set(uint32_t threshold)
{
	PyGILState_STATE gstate;
	PyObject *strargs;

	log_debug(PythonClusterDataLogLevel, "Enter");
	gstate = PyGILState_Ensure();
	do
	{
		//def Domain_Delete(self, domain_id):
		strargs = Py_BuildValue("(I)", threshold);
		if (strargs == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Cannot build Py_BuildValue for Nodes_Update");
			break;
		}
		// Invoke the Domain_Delete_Send_To_All call
		PyEval_CallObject(Cluster_DB_Interface.Set_Heavy_Load_Value, strargs);
		Py_DECREF(strargs);
	}while(0);
	PyGILState_Release(gstate);

	log_debug(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_cluster_reregister_endpoints --                                    *//**
 *
 * \brief This routine asks the DMC to request re-registration of all endpoints
 *        in a VNID.
 *
 * \return PyObject
 *
 *****************************************************************************/
PyObject *dps_cluster_reregister_endpoints(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	uint32_t domain, vnid;
	int ret = -1;

	log_info(PythonClusterDataLogLevel, "Enter");
	do
	{
		if (!PyArg_ParseTuple(args, "II", &domain, &vnid))
		{
			log_warn(PythonClusterDataLogLevel, "Bad Data!!!");
			break;
		}
		ret = dps_dmc_register_domain_endpoints(domain, vnid);
	}while(0);
	ret_val = Py_BuildValue("i", ret);
	log_info(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code (ClusterDatabase) that are needed for processing
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

	log_info(PythonClusterDataLogLevel, "Enter");

	memset(&Cluster_DB_Interface, 0, sizeof(python_dps_cluster_db_t));
	do
	{
		// Get handle to an instance of ClusterDatabase
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_CLUSTER_DATABASE,
		                                 PYTHON_MODULE_CLASS_CLUSTER_DATABASE,
		                                 pyargs,
		                                 &Cluster_DB_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Node_Add
		Cluster_DB_Interface.Node_Add =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_ADD);
		if (Cluster_DB_Interface.Node_Add == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_ADD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		Cluster_DB_Interface.Node_Local_Update =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_UPDATE_LOCAL);
		if (Cluster_DB_Interface.Node_Local_Update == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_UPDATE_LOCAL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		Cluster_DB_Interface.Node_Local_Activate =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_ACTIVATE_LOCAL);
		if (Cluster_DB_Interface.Node_Local_Activate == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_ACTIVATE_LOCAL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Node_Get_Port
		Cluster_DB_Interface.Node_Get_Port =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_GET_PORT);
		if (Cluster_DB_Interface.Node_Get_Port == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_GET_PORT);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Node_Delete
		Cluster_DB_Interface.Node_Delete =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE);
		if (Cluster_DB_Interface.Node_Delete == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Node_Add_Domain
		Cluster_DB_Interface.Node_Add_Domain =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_ADD_DOMAIN);
		if (Cluster_DB_Interface.Node_Add_Domain == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_ADD_DOMAIN);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Node_Delete_Domain
		Cluster_DB_Interface.Node_Delete_Domain =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_DOMAIN);
		if (Cluster_DB_Interface.Node_Delete_Domain == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_DOMAIN);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Node_Delete_All_Domains
		Cluster_DB_Interface.Node_Delete_All_Domains =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_ALL_DOMAINS);
		if (Cluster_DB_Interface.Node_Delete_All_Domains == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DPS_NODE_DELETE_ALL_DOMAINS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Show
		Cluster_DB_Interface.Show =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_SHOW);
		if (Cluster_DB_Interface.Show == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_SHOW);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain Get Nodes
		Cluster_DB_Interface.Domain_Get_Nodes =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_GET_NODES);
		if (Cluster_DB_Interface.Domain_Get_Nodes == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_GET_NODES);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain Get Nodes
		Cluster_DB_Interface.Domain_Get_Random_Remote_Node =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_GET_RANDOM_REMOTE_NODE);
		if (Cluster_DB_Interface.Domain_Get_Random_Remote_Node == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_GET_RANDOM_REMOTE_NODE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain Get Nodes
		Cluster_DB_Interface.Node_Get_Domains =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_GET_DOMAINS);
		if (Cluster_DB_Interface.Node_Get_Domains == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_GET_DOMAINS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Get All Nodes
		Cluster_DB_Interface.Cluster_Get_All_Nodes =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_GET_ALL_NODES);
		if (Cluster_DB_Interface.Cluster_Get_All_Nodes == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_GET_ALL_NODES);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Get All Nodes
		Cluster_DB_Interface.Node_Set_Domain_Mapping =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_NODE_SET_DOMAIN_MAPPING);
		if (Cluster_DB_Interface.Node_Set_Domain_Mapping == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_NODE_SET_DOMAIN_MAPPING);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Get All Nodes
		Cluster_DB_Interface.Get_DomainID_From_VNID =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_GET_DOMAINID_FROM_VNID);
		if (Cluster_DB_Interface.Get_DomainID_From_VNID == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_GET_DOMAINID_FROM_VNID);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Nodes_Update
		Cluster_DB_Interface.Nodes_Update =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODES_UPDATE);
		if (Cluster_DB_Interface.Nodes_Update == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODES_UPDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Touch
		Cluster_DB_Interface.Node_Touch =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_TOUCH);
		if (Cluster_DB_Interface.Node_Touch == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_TOUCH);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Validate_Remote_DCS
		Cluster_DB_Interface.Node_Validate_Remote_DCS =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_VALIDATE);
		if (Cluster_DB_Interface.Node_Validate_Remote_DCS == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_VALIDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Heartbeat
		Cluster_DB_Interface.Node_Heartbeat =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT);
		if (Cluster_DB_Interface.Node_Heartbeat == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Nodes_Status
		Cluster_DB_Interface.Nodes_Status =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODES_STATUS);
		if (Cluster_DB_Interface.Nodes_Status == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODES_STATUS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Heartbeat_Request
		Cluster_DB_Interface.Node_Heartbeat_Request =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_REQUEST);
		if (Cluster_DB_Interface.Node_Heartbeat_Request == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_REQUEST);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Heartbeat_Process
		Cluster_DB_Interface.Node_Heartbeat_Process =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_PROCESS);
		if (Cluster_DB_Interface.Node_Heartbeat_Process == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_HEARTBEAT_PROCESS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Domain_Mapping_Exchange
		Cluster_DB_Interface.Domain_Mapping_Exchange =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_MAPPING_EXCHANGE);
		if (Cluster_DB_Interface.Domain_Mapping_Exchange == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_MAPPING_EXCHANGE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Domain_Activate_Mapping_Exchange_Synchronous
		Cluster_DB_Interface.Domain_Activate_Mapping_Exchange_Synchronous =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_ACTIVATE_MAPPING_EXCHANGE);
		if (Cluster_DB_Interface.Domain_Activate_Mapping_Exchange_Synchronous == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_ACTIVATE_MAPPING_EXCHANGE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Update_Domain_Statistics
		Cluster_DB_Interface.Node_Update_Domain_Statistics =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_UPDATE_DOMAIN_STATISTICS);
		if (Cluster_DB_Interface.Node_Update_Domain_Statistics == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_UPDATE_DOMAIN_STATISTICS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Node_Show_Load
		Cluster_DB_Interface.Node_Show_Load =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_NODE_SHOW_LOAD);
		if (Cluster_DB_Interface.Node_Show_Load == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_NODE_SHOW_LOAD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Get_Lowest_Loaded_Available_Nodes
		Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_AVAILABLE_NODES);
		if (Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_AVAILABLE_NODES);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Cluster Get_Lowest_Loaded_Available_Nodes
		Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes_Not_Hosting =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_AVAILABLE_NODES_NOT_HOSTING);
		if (Cluster_DB_Interface.Get_Lowest_Loaded_Available_Nodes_Not_Hosting == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_AVAILABLE_NODES_NOT_HOSTING);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Get_Replication_Factor
		Cluster_DB_Interface.Domain_Get_Replication_Factor =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_GET_REPLICATION_FACTOR);
		if (Cluster_DB_Interface.Domain_Get_Replication_Factor == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_GET_REPLICATION_FACTOR);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Get_Highest_Loaded_Nodes_Hosting
		Cluster_DB_Interface.Get_Highest_Loaded_Nodes_Hosting =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_GET_HIGHEST_LOADED_NODES_HOSTING);
		if (Cluster_DB_Interface.Get_Highest_Loaded_Nodes_Hosting == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_GET_HIGHEST_LOADED_NODES_HOSTING);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Delete_Send_To_All
		Cluster_DB_Interface.Domain_Delete_Send_To_All =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_DELETE_SEND_TO_ALL);
		if (Cluster_DB_Interface.Domain_Delete_Send_To_All == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_DELETE_SEND_TO_ALL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function VNID_Existence_Send_To_All
		Cluster_DB_Interface.VNID_Existence_Send_To_All =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_VNID_EXISTENCE_SEND_TO_ALL);
		if (Cluster_DB_Interface.VNID_Existence_Send_To_All == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_VNID_EXISTENCE_SEND_TO_ALL);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Delete
		Cluster_DB_Interface.Domain_Delete =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_DELETE);
		if (Cluster_DB_Interface.Domain_Delete == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_DELETE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Domain_Exists
		Cluster_DB_Interface.Domain_Exists =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_DOMAIN_EXISTS);
		if (Cluster_DB_Interface.Domain_Exists == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_DOMAIN_EXISTS);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Set_Heavy_Load_Value
		Cluster_DB_Interface.Set_Heavy_Load_Value =
			PyObject_GetAttrString(Cluster_DB_Interface.instance,
			                       PYTHON_FUNC_CLUSTER_HEAVY_LOAD_THRESHOLD);
		if (Cluster_DB_Interface.Set_Heavy_Load_Value == NULL)
		{
			log_emergency(PythonClusterDataLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_CLUSTER_HEAVY_LOAD_THRESHOLD);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}
		status = DOVE_STATUS_OK;
	}while(0);

	log_info(PythonClusterDataLogLevel, "Exit: %s",
	         DOVEStatusToString(status));

	return status;
}

/*
 ******************************************************************************
 * python_init_cluster_db_interface --                                    *//**
 *
 * \brief This routine initializes the DPS Cluster Database interface to PYTHON
 *        OBJECTS
 *
 * \param pythonpath - Pointer to the PYTHON Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_cluster_db_interface(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;
	status = python_functions_init(pythonpath);
	return status;
}

/** @} */
/** @} */
