/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Heartbeat Functionality
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
*  $Log: heartbeat.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include "include.h"

/**
 * \brief The module location defines for the DPS Statistics
 *        Handling API
 */
#define HEARTBEAT_MODULE "dps_heartbeat"

//I don't know how to represent the UUID
char uuid[] = "000000000000000000000000";
//char dps_heartbeat_uri[] = DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI;
char dps_cluster_heartbeat_uri[] = DPS_CLUSTER_HEARTBEAT_URI;
char dps_cluster_heartbeat_request_uri[] = DPS_CLUSTER_HEARTBEAT_REQUEST_URI;
char dps_cluster_nodestatus_uri[] = DPS_CLUSTER_NODE_STATUS_URI;

/**
 * \brief Contains the configuration version of the Local Node
 */
long long local_config_version = 0;

/**
 * \brief Contains the configuration version of the DCS Cluster
 */
long long cluster_config_version = 0;
/**
 * \brief The mutex and condition variable used by thread.
 */
pthread_cond_t dps_heartbeat_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dps_heartbeat_mp = PTHREAD_MUTEX_INITIALIZER;

/*
mutex and retry variable used by thread
*/
pthread_mutex_t dps_appliance_registration_needed_mp = PTHREAD_MUTEX_INITIALIZER;
unsigned char dps_appliance_registration_needed = 0;

unsigned char dps_inter_node_heartbeat_send = 1;

uint32_t dps_registration_fail_count = 0;

/**
 * \brief The thread ID.
 */
long heartbeatTaskId;

int set_dps_appliance_registration_needed(unsigned char value)
{
	pthread_mutex_lock(&dps_appliance_registration_needed_mp);
	dps_appliance_registration_needed = value;
	pthread_mutex_unlock(&dps_appliance_registration_needed_mp);
	return 0;
}

unsigned char get_dps_appliance_registration_needed(void)
{
	unsigned char value = 0;
	pthread_mutex_lock(&dps_appliance_registration_needed_mp);
	value = dps_appliance_registration_needed;
	pthread_mutex_unlock(&dps_appliance_registration_needed_mp);
	return value;
}

/**
 * \brief The Interval in which DPS collects statistics.
 *        Unit(second)
 */
static int dps_heartbeat_sample_interval = 15; /* 15s*/
unsigned char dps_heartbeat_send = 1;

/* Count the heartbeat success number , to send the leader report according to it every 8 times */
int dps_heartbeat_to_dmc_send_count = 0;
/* Record if last heartbeat is success or not*/
int last_heartbeat_to_dmc_is_success = 0;

int heartbeat_send_sock = 0;

/* Set the heartbeat interval */
int set_heartbeat_interval(int interval)
{
	/* If the interval is 0, NOT send */
	if(interval != 0){
		dps_heartbeat_sample_interval = interval;
		dps_heartbeat_send = 1;
		dps_cluster_node_heartbeat_process(1);
	}else{
		dps_heartbeat_sample_interval = 60;
		dps_heartbeat_send = 0;
		dps_cluster_node_heartbeat_process(0);
	}
	return 0;
}

int get_heartbeat_interval(void)
{
	/* If the interval is 0, NOT send */
	if(dps_heartbeat_send){
		return dps_heartbeat_sample_interval;
	}else{
		return 0;
	}
}

static json_t *dps_form_json_heartbeat()
{
	json_t *js_root = NULL;
	char str[INET6_ADDRSTRLEN];

	log_debug(RESTHandlerLogLevel, "Enter");
	inet_ntop(dcs_local_ip.family, dcs_local_ip.ip6, str, INET6_ADDRSTRLEN);

	log_info(RESTHandlerLogLevel,
	         "Heartbeat: Version %ld, Role Assigned %d",
	         cluster_config_version, dcs_role_assigned);

	js_root = json_pack("{s:i, s:i, s:s, s:i, s:i}",
	                    "ip_family", (int)dcs_local_ip.family,
	                    "dcs_config_version", (int)cluster_config_version,
	                    "ip", str,
	                    "canBeDCS",CAN_BE_DCS,
	                    "isDCS",dcs_role_assigned
	                    );

	log_debug(RESTHandlerLogLevel, "Exit");
	return js_root;
}

static json_t *dps_form_json_heartbeat_to_dps_node(int factive,
                                                   long long config_version)
{
	json_t *js_root = NULL;

	log_debug(RESTHandlerLogLevel, "Enter");
	js_root = json_pack("{s:i, s:I, s:s, s:s}",
	                    "Active", factive,
	                    "Config_Version", config_version,
	                    "UUID", dps_node_uuid,
	                    "Cluster_Leader", dps_cluster_leader_ip_string);
	log_debug(RESTHandlerLogLevel, "Exit");
	return js_root;
}

static void dps_heartbeat_send_to_dove_controller()
{
	json_t *js_res = NULL;
	char dps_heartbeat_uri[MAX_URI_LEN];

	sprintf (dps_heartbeat_uri, DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI, dps_node_uuid);

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		js_res = dps_form_json_heartbeat();
		if(js_res == NULL)
		{
			log_alert(RESTHandlerLogLevel,
			          "Can not form json for heartbeat");
			break;
		}
		dps_rest_client_json_send_to_dove_controller(js_res,
		                                             dps_heartbeat_uri,
		                                             EVHTTP_REQ_PUT);
	}while(0);
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_rest_heartbeat_send_to_dps_node --                                 *//**
 *
 * \brief This routine sends a REST heartbeat message to a remote DPS Node.
 *
 * \param dps_node: The location of the remote node
 * \param factive: Whether the Node is active
 * \param config_version: The configuration version as seen by this node
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_heartbeat_send_to_dps_node(ip_addr_t *dps_node,
                                         int factive,
                                         long long config_version)
{
	json_t *js_res = NULL;

	if (!dps_heartbeat_send)
	{
		return;
	}

	log_debug(RESTHandlerLogLevel, "Enter: config_version %ld", config_version);
	do
	{
		js_res = dps_form_json_heartbeat_to_dps_node(factive,
		                                             config_version);
		if(js_res == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "Can not form json for heartbeat");
			break;
		}

		dps_rest_client_json_send_to_dps_node(js_res,
		                                      dps_cluster_heartbeat_uri,
		                                      EVHTTP_REQ_PUT,
		                                      dps_node);
	}while(0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_rest_heartbeat_request_send_to_dps_node --                         *//**
 *
 * \brief This routine sends a REST heartbeat request message to a remote
 *        DPS Node. Typically initiated by the Leader when its unable to
 *        contact a node for a while.
 *
 * \param dps_node: The location of the remote node
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_heartbeat_request_send_to_dps_node(ip_addr_t *dps_node)
{
	if (!dps_heartbeat_send)
	{
		return;
	}
	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		dps_rest_client_json_send_to_dps_node(NULL,
		                                      dps_cluster_heartbeat_request_uri,
		                                      EVHTTP_REQ_PUT,
		                                      dps_node);
	}while(0);
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

static json_t *dps_form_json_node_status_json(json_t *js_nodes)
{
	json_t *js_node = NULL, *js_root = NULL;
	do
	{
		js_node = json_pack("{s:I, s:s, s:s}",
		                    "Config_Version", cluster_config_version,
		                    "UUID", dps_node_uuid,
		                    "Cluster_Leader", dps_cluster_leader_ip_string);
		if (js_node == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "Can not form js_node for node_status");
			break;
		}
		js_root = json_pack("{s:o, s:o}",
		                    "dps_node", js_node,
		                    "nodes_status", js_nodes);
		if (js_root == NULL)
		{
			json_decref(js_node);
			log_warn(RESTHandlerLogLevel,
			         "Can not form js_root for node_status");
			break;
		}
	}while(0);
	return js_root;
}

/*
 ******************************************************************************
 * dps_rest_nodes_status_send_to --                                       *//**
 *
 * \brief This routine sends a REST message containing this node's view of all
 *        other node statuses list to another dps node.
 *        Typically it's used by the leader to send it's view of the cluster
 *        to another node.
 *
 * \param dps_node: The location of the remote node
 * \param config_version: The configuration version as seen by this node
 * \param nodes_status: An array of node location and status
 * \param num_nodes: The number of nodes in the status
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_nodes_status_send_to(ip_addr_t *dps_node,
                                   long long config_version,
                                   ip_addr_t *nodes_status,
                                   int num_nodes)
{
	json_t *js_res = NULL;
	json_t *js_nodes = NULL;
	json_t *js_node_state = NULL;
	ip_addr_t *curr_node = nodes_status;
	char ip_string[INET6_ADDRSTRLEN];
	int i;

	if (!dps_heartbeat_send)
	{
		return;
	}
	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		// Form the Arrays of Node Status(es)
		js_nodes = json_array();
		if (js_nodes == NULL)
		{
			log_warn(RESTHandlerLogLevel, "Cannot allocate json_array");
			break;
		}

		for (i = 0; i < num_nodes; i++)
		{
			inet_ntop(curr_node->family, curr_node->ip6, ip_string, INET6_ADDRSTRLEN);
			js_node_state = json_pack("{s:s, s:i}",
			                          "ip_address", ip_string,
			                          "status", curr_node->status);
			if(js_node_state == NULL)
			{
				break;
			}
			json_array_append_new(js_nodes, js_node_state);
			curr_node++;
		}

		js_res = dps_form_json_node_status_json(js_nodes);
		if(js_res == NULL)
		{
			json_decref(js_nodes);
			log_warn(RESTHandlerLogLevel,
			         "Cannot form json for Node Status");
			break;
		}

		dps_rest_client_json_send_to_dps_node(js_res,
		                                      dps_cluster_nodestatus_uri,
		                                      EVHTTP_REQ_PUT,
		                                      dps_node);
	}while(0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

static void dps_heartbeat_main(void *pDummy)
{

	struct timespec   ts;
	struct timeval    tp;
	int               it;
	int               rc;

	Py_Initialize();
	while (TRUE) {
		pthread_mutex_lock(&dps_heartbeat_mp);
		it = dps_heartbeat_sample_interval;
		gettimeofday(&tp, NULL);
		/* Convert from timeval to timespec */
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += it;
		rc = pthread_cond_timedwait(&dps_heartbeat_cv, &dps_heartbeat_mp, &ts);
		if (rc == ETIMEDOUT)
		{
			pthread_mutex_unlock(&dps_heartbeat_mp);
			if(dps_heartbeat_send && controller_location_set)/* Whether to send heartbeat */
			{
				/* Send Heartbeat to DOVE Controller*/
				log_info(RESTHandlerLogLevel, "Send Heartbeat to Dove Controller");
				//Send the heartbeat is NOT success as the correct repsponse is not received yet
				//set it in response handler when received a HTTP_OK
				last_heartbeat_to_dmc_is_success = 0;
				dps_heartbeat_send_to_dove_controller();
				dps_heartbeat_to_dmc_send_count ++;
			}

			/* Check if we need registration to DMC again */
			if(get_dps_appliance_registration_needed())
			{
				log_info(RESTHandlerLogLevel, "Send DCS appliance registration to DMC!");
				dps_appliance_registration();
				if (dcs_role_assigned)
				{
					log_alert(RESTHandlerLogLevel,
					          "DCS: Sending Query to DMC for list of Nodes");
					dps_rest_client_query_dove_controller_cluster_info();
				}
			}
			continue;
		}
		pthread_mutex_unlock(&dps_heartbeat_mp);
	}
	Py_Finalize();

	return;
}

dove_status dcs_heartbeat_init(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
#if 0
		status = python_functions_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
#endif
		/* Initialize mutex and condition variable objects */
		if (pthread_mutex_init(&dps_heartbeat_mp, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}

		/* Initialize mutex and condition variable objects */
		if (pthread_mutex_init(&dps_appliance_registration_needed_mp, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}

		if (pthread_cond_init(&dps_heartbeat_cv, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		/* Create a thread for statistics collection */
		if (create_task((const char *)"Heartbeat", 0, OSW_DEFAULT_STACK_SIZE,
		                dps_heartbeat_main, 0,
		                &heartbeatTaskId) != OSW_OK)
		{
			status = DOVE_STATUS_THREAD_FAILED;
			log_error(PythonDataHandlerLogLevel,"task create failed");
			break;
		}
	} while (0);

	return status;
}
