/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Statistics
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
*  $Log: statistics.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#include "include.h"

#include <jansson.h>


int statistics_need_send_to_leader = 1;

/**
 * \brief The module location defines for the DPS Statistics
 *        Handling API
 */
#define PYTHON_MODULE_FILE_STATISTICS "statistics"

/**
 * \brief The PYTHON Class that handles the DPS Statistics
 *        Requests
 */
#define PYTHON_MODULE_CLASS_STATISTICS "DpsStatisticsHandler"

/**
 * \brief The PYTHON function to get all domain IDs
 */
#define PYTHON_FUNC_DOMAIN_LIST_GET "Domain_List_Get"

/**
 * \brief The PYTHON function to get load balancing
 */
#define PYTHON_FUNC_STATISTICS_LOAD_BALANCING_GET "Statistics_Load_Balancing_Get"

/**
 * \brief The PYTHON function to get general statistics
 */
#define PYTHON_FUNC_STATISTICS_GENERAL_STATISTICS_GET "Statistics_General_Statistics_Get"

/**
 * \brief The PYTHON function to update load balancing
 */
#define PYTHON_FUNC_STATISTICS_LOAD_BALANCING_UPDATE "Statistics_Load_Balancing_Update"

/**
 * \brief The DPS statistics handler function pointers data
 *        structure
 */

typedef struct python_dps_statistics_s{
	/**
	 * \brief The DpsClientHandler Object Instance
	 */
	PyObject *instance;
	/*
	 * \brief The function to get all domain IDs
	 */
	PyObject *Domain_List_Get;
	/*
	 * \brief The function to get load balancing
	 */
	PyObject *Statistics_Load_Balancing_Get;
	/*
	 * \brief The function to get general statistics
	 */
	PyObject *Statistics_General_Statistics_Get;
	/*
	 * \brief The function to update load balancing
	 */
	PyObject *Statistics_Load_Balancing_Update;
}python_dps_statistics_t;

/*
 * \brief The DPS Statistics Handler PYTHON Interface (Embed)
 */
static python_dps_statistics_t Statistics_Interface;

/**
 * \brief The mutex and condition variable used by thread.
 */
pthread_cond_t dps_statistics_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dps_statistics_mp = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief The Interval in which DPS collects statistics.
 *        Unit(second)
 */
int dps_statistics_sample_interval = 30; /* 30s*/
unsigned char dps_statistics_send = 1;

/**
 * \brief The thread ID.
 */
long statisticsTaskId;
extern int PythonDataHandlerLogLevel;

/* Set the statistics interval */
int set_statistics_interval(int interval)
{
	/* If the interval is 0, NOT send */
	if(interval != 0){
		dps_statistics_sample_interval = interval;
		dps_statistics_send = 1;
	}else{
		dps_statistics_sample_interval = 60;
		dps_statistics_send = 0;
	}
	return 0;
}

int get_statistics_interval(void)
{
	/* If the interval is 0, NOT send */
	if(dps_statistics_send){
		return dps_statistics_sample_interval;
	}else{
		return 0;
	}
}



/*
 ******************************************************************************
 * dps_statistics_domain_load_balancing_get                               *//**
 *
 * \brief - Get load balancing for a specified domain
 *
 * \param load_balancing - The Structure of the Message for load balancing
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status
dps_statistics_domain_load_balancing_get(
	dps_object_load_balancing_t *load_balancing)
{
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int endpoint_update_count, endpoint_lookup_count, endpoints_count;
	int policy_lookup_count;
	int ret_code = DOVE_STATUS_NO_MEMORY;

	// Ensure the PYTHON Global Interpreter Lock
	gstate = PyGILState_Ensure();
	do
	{
		// Statistics_Load_Balancing_Get(self, domain_id):
		strargs = Py_BuildValue("(I)", load_balancing->domain_id);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}

		// Invoke the Statistics_Load_Balancing_Get call
		strret = PyEval_CallObject(Statistics_Interface.Statistics_Load_Balancing_Get, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Statistics_Load_Balancing_Get returns NULL");
			break;
		}
		//@return: status, endpoint_update_count, endpoint_lookup_count, policy_lookup_count
		//         endpoints_count
		//@rtype: Integer, Integer, Integer, Integer, Integer
		PyArg_ParseTuple(strret, "IIIII", &ret_code,
		                 &endpoint_update_count, &endpoint_lookup_count,
		                 &policy_lookup_count, &endpoints_count);

		if (ret_code == DPS_NO_ERR)
		{
#if 0
			log_debug(RESTHandlerLogLevel,
			          "domain %d load balancing:\r\n",
			          load_balancing->domain_id);
			log_debug(RESTHandlerLogLevel,
			          "ep_up=%d, ep_lk=%d, pol_lk=%d, ep_num=%d\r\n",
			          endpoint_update_count, endpoint_lookup_count,
			          policy_lookup_count, endpoints_count);
#endif
			load_balancing->sample_interval = dps_statistics_sample_interval;
			load_balancing->endpoint_update_count = endpoint_update_count;
			load_balancing->endpoint_lookup_count = endpoint_lookup_count;
			load_balancing->policy_lookup_count = policy_lookup_count;
			load_balancing->endpoints_count = endpoints_count;
		}
		else
		{
			log_notice(RESTHandlerLogLevel,
			           "Fail to get statistics of domain %d\r\n",
			           load_balancing->domain_id);
		}
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);
	return (dove_status)ret_code;
}

/*
 ******************************************************************************
 * dps_statistics_domain_general_statistics_get                           *//**
 *
 * \brief - Get general statistics for a specified domain
 *
 * \param general_statistics - The Structure of the Message for general_statistics
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status
dps_statistics_domain_general_statistics_get(dps_object_general_statistics_t *general_statistics)
{
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int endpoint_update_count[2], endpoint_lookup_count[2];
	int policy_lookup_count[2], multicast_lookup_count[2], internal_gw_lookup_count[2];
	int i, ret_code = DOVE_STATUS_NO_MEMORY;

	for (i = 0; i < 2; i++)
	{
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();

		// Statistics_General_Statistics_Get(self, domain_id):
		strargs = Py_BuildValue("(I)", general_statistics->domain_id);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Statistics_General_Statistics_Get call
		strret = PyEval_CallObject(Statistics_Interface.Statistics_General_Statistics_Get, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
				 "PyEval_CallObject Statistics_General_Statistics_Get returns NULL");
			break;
		}
		//@return: status, endpoint_update_count, endpoint_lookup_count, policy_lookup_count
		//         multicast_lookup_count, internal_gw_lookup_count
		//@rtype: Integer, Integer, Integer, Integer, Integer
		PyArg_ParseTuple(strret, "IIIIII", &ret_code,
				 &endpoint_update_count[i], &endpoint_lookup_count[i],
				 &policy_lookup_count[i], &multicast_lookup_count[i],
				 &internal_gw_lookup_count[i]);

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		// Release the PYTHON Global Interpreter Lock
		PyGILState_Release(gstate);

		if (ret_code != DPS_NO_ERR) break;
		if (i == 0) sleep(1);
	}

	if (ret_code == DPS_NO_ERR)
	{
		general_statistics->counts.endpoint_update_count = endpoint_update_count[1];
		general_statistics->counts.endpoint_lookup_count = endpoint_lookup_count[1];
		general_statistics->counts.policy_lookup_count = policy_lookup_count[1];
		general_statistics->counts.multicast_lookup_count = multicast_lookup_count[1];
		general_statistics->counts.internal_gw_lookup_count = internal_gw_lookup_count[1];
		general_statistics->rates.endpoint_update_rate = endpoint_update_count[1]-endpoint_update_count[0];
		general_statistics->rates.endpoint_lookup_rate = endpoint_lookup_count[1]-endpoint_lookup_count[0];
		general_statistics->rates.policy_lookup_rate = policy_lookup_count[1]-policy_lookup_count[0];
	}
	else
	{
		log_notice(RESTHandlerLogLevel,
			   "Fail to get statistics of domain %d\r\n",
			   general_statistics->domain_id);
	}

	return (dove_status)ret_code;
}

static void dps_statistics_update_per_domain(int domain)
{
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	int ret_code;

	// Ensure the PYTHON Global Interpreter Lock
	log_debug(RESTHandlerLogLevel,"Enter: Domain %d", domain);

	gstate = PyGILState_Ensure();
	do
	{
		// Statistics_Load_Balancing_Update(self, domain_id):
		strargs = Py_BuildValue("(I)", domain);
		if (strargs == NULL)
		{
			log_notice(PythonClusterDataLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Statistics_Load_Balancing_Update call
		strret = PyEval_CallObject(Statistics_Interface.Statistics_Load_Balancing_Update, strargs);
		Py_DECREF(strargs);
		if (strret == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "PyEval_CallObject Statistics_Load_Balancing_Update returns NULL");
			break;
		}

		//@return: status
		//@rtype: Integer
		PyArg_Parse(strret, "I", &ret_code);

		if (ret_code == DPS_NO_ERR)
		{
			log_debug(RESTHandlerLogLevel,
			          "Success to update statistics of domain %d", domain);
		}
		else
		{
			log_notice(RESTHandlerLogLevel,
			           "Fail to update statistics of domain %d", domain);
		}

		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
	}while(0);

	// Release the PYTHON Global Interpreter Lock
	PyGILState_Release(gstate);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

static json_t *dps_form_json_load_balancing_json(json_t *js_stats)
{
	json_t *js_node = NULL, *js_root = NULL;
	char str[INET6_ADDRSTRLEN];

	inet_ntop(dcs_local_ip.family, dcs_local_ip.ip6, str, INET6_ADDRSTRLEN);
	do
	{
		js_node = json_pack("{s:i, s:s, s:s, s:s}",
		                    /* fields to describe the report node */
		                    "family", (int)dcs_local_ip.family,
		                    "ip", str,
		                    "UUID", dps_node_uuid,
		                    "Cluster_Leader", dps_cluster_leader_ip_string);
		if (js_node == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "json_pack (js_node) returns NULL");
			break;
		}
		js_root = json_pack("{s:o, s:o}",
		                    "dps_node", js_node,
		                    "statistics", js_stats);
		if (js_root == NULL)
		{
			json_decref(js_node);
			log_warn(RESTHandlerLogLevel,
			         "json_pack (js_root) returns NULL");
			break;
		}
	}while(0);

	return js_root;
}

static dove_status dps_statistics_array_append(json_t *js_stats_array,
                                               dps_object_load_balancing_t *load_balancing)
{
	dove_status status = DOVE_STATUS_OK;
	json_t *js_one_stat = NULL;

	log_debug(RESTHandlerLogLevel, "Enter");
	/* fields of actuall statistics */
	js_one_stat = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i}",
	                        "id", (int)load_balancing->domain_id,
	                        "sample_interval", load_balancing->sample_interval,
	                        "endpoint_update_count", load_balancing->endpoint_update_count,
	                        "endpoint_lookup_count", load_balancing->endpoint_lookup_count,
	                        "policy_lookup_count",  load_balancing->policy_lookup_count,
	                        "endpoints_count",  load_balancing->endpoints_count);

	if(js_one_stat)
	{
		json_array_append_new(js_stats_array, js_one_stat);
	}
	else
	{
		status = DOVE_STATUS_NO_RESOURCES;
	}
	log_debug(RESTHandlerLogLevel, "Exit %s", DOVEStatusToString(status));
	return status;
}


//static void dps_statistics_report_to_dove_controller(json_t *js_stats)
//{
//	json_t *js_res = NULL;
//	char uri[256];
//
//	log_info(RESTHandlerLogLevel, "Enter");
//	do
//	{
//		js_res = dps_form_json_load_balancing_json(js_stats);
//		if(!js_res)
//		{
//			log_error(RESTHandlerLogLevel, "json format error in statistics report!!!");
//			break;
//		}
//		memset(uri, 0 , 256);
//		DOVE_CONTROLLER_DPS_STATISTICS_URI_GEN(uri,0);
//		dps_rest_client_json_send_to_dove_controller(js_res, uri, EVHTTP_REQ_POST);
//	}while(0);
//	log_info(RESTHandlerLogLevel, "Exit");
//	return;
//}

static void dps_statistics_report_to_cluster_leader(json_t *js_stats)
{
	json_t *js_res = NULL;
	char uri[256];
	js_res = dps_form_json_load_balancing_json(js_stats);

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		if(!js_res)
		{
			log_error(RESTHandlerLogLevel, "json format error in statistics report!!!");
			break;
		}

		memset(uri, 0 , 256);
		CLUSTER_LEADER_DPS_STATISTICS_URI_GEN(uri);
		dps_rest_client_json_send_to_dps_node(js_res,
		                                      uri,
		                                      EVHTTP_REQ_PUT,
		                                      &dps_cluster_leader);
	}while(0);
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

//static json_t *dps_json_array_copy(json_t *array)
//{
//	json_t *result;
//	size_t i;
//
//	log_debug(RESTHandlerLogLevel, "Enter");
//	do
//	{
//		result = json_array();
//		if (result == NULL)
//		{
//			break;
//		}
//		for (i = 0; i < json_array_size(array); i++)
//		{
//			json_array_append(result, json_array_get(array, i));
//		}
//	}while(0);
//	log_debug(RESTHandlerLogLevel, "Exit, result %p", result);
//	return result;
//}

static void dps_statistics_process(void)
{
	PyObject *strret, *strargs;
	PyGILState_STATE gstate;
	char *domain_list_ret = NULL, *domain_list = NULL;
	char *str, *saveptr, *token;
	int domain;
	json_t *js_stats = NULL;  /* json statistics array of each domain */
	dps_object_load_balancing_t load_balancing;

	do
	{
		saveptr = NULL;
		/* No need to send the statistics */
		if(dps_statistics_send == 0){
			log_debug(PythonDataHandlerLogLevel, "Statistics has been set to NOT SENDING!");
			break;
		}
		// Ensure the PYTHON Global Interpreter Lock
		gstate = PyGILState_Ensure();
		// Domain_List_Get(self):
		strargs = Py_BuildValue("()");
		if (strargs == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel, "Py_BuildValue returns NULL");
			break;
		}
		// Invoke the Domain_List_Get call
		strret = PyEval_CallObject(Statistics_Interface.Domain_List_Get, strargs);
		Py_DECREF(strargs);

		if (strret == NULL)
		{
			// Release the PYTHON Global Interpreter Lock
			PyGILState_Release(gstate);
			log_warn(PythonDataHandlerLogLevel,
			         "PyEval_CallObject Domain_List_Get returns NULL");
			break;
		}
		//@return: The string of all Domain IDs
		//@rtype: String
		PyArg_Parse(strret, "z", &domain_list_ret);
		domain_list = strdup(domain_list_ret);
		// Lose the reference on all parameters and return arguments since they
		// are no longer needed.
		Py_DECREF(strret);
		// Release the PYTHON Global Interpreter Lock
		PyGILState_Release(gstate);

		if (!statistics_need_send_to_leader)
		{
			log_info(RESTHandlerLogLevel, "Not sending stats to leader");
			break;
		}

		js_stats = json_array();
		if(js_stats == NULL)
		{
			log_warn(RESTHandlerLogLevel, "Cannot allocate json_array");
			break;
		}
		//Parse domain list and collect statistics for every domain
		for (str = domain_list; ; str = NULL)
		{
			if ((token = strtok_r(str, ",", &saveptr)) == NULL)
			{
				break;
			}
			domain = atoi(token);
			dps_statistics_update_per_domain(domain);
			//TODO: Get statistics and push it to requestor
			//Currently the pushing action is OK, but now it is turned off,
			//it should be turned on when the system is actually deployed
			memset(&load_balancing, 0, sizeof(load_balancing));
			load_balancing.domain_id = domain;
			dps_statistics_domain_load_balancing_get(&load_balancing);
			if(dps_statistics_array_append(js_stats, &load_balancing) != DOVE_STATUS_OK)
			{
				log_error(RESTHandlerLogLevel,
				          "Cannot append statistics for domain %d",
				          domain);
				json_decref(js_stats);
				js_stats = NULL;
				break;
			}
		}
		if (js_stats == NULL)
		{
			break;
		}

		log_info(RESTHandlerLogLevel, "Send statistics to LEADER!");
		dps_statistics_report_to_cluster_leader(js_stats);

	}while(0);

	if (domain_list != NULL)
	{
		free(domain_list);
	}

	return;
}

static void dps_statistics_main(void *pDummy)
{
	struct timespec   ts;
	struct timeval    tp;
	int               it;
	int               rc;

	Py_Initialize();

	while (TRUE) {
		pthread_mutex_lock(&dps_statistics_mp);

		it = dps_statistics_sample_interval;
		gettimeofday(&tp, NULL);
		/* Convert from timeval to timespec */
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += it;
		rc = pthread_cond_timedwait(&dps_statistics_cv, &dps_statistics_mp, &ts);
		if (rc == ETIMEDOUT) {
			pthread_mutex_unlock(&dps_statistics_mp);
			/* Update statistics we are interested in. */
			//log_info(PythonDataHandlerLogLevel, "Updating statistics");
			dps_statistics_process();
			//log_info(PythonDataHandlerLogLevel, "Updated statistics");
			continue;
		}
		if (rc == 0) {
			log_info(PythonDataHandlerLogLevel, "Thread exiting");
			pthread_mutex_unlock(&dps_statistics_mp);
			pthread_mutex_destroy(&dps_statistics_mp);
			pthread_cond_destroy(&dps_statistics_cv);
			del_task(statisticsTaskId);
			return;
		}
		pthread_mutex_unlock(&dps_statistics_mp);
	}

	Py_Finalize();
	return;
}

/*
 ******************************************************************************
 * python_function_init --                                                *//**
 *
 * \brief This routine gets references to all functions in the PYTHON data
 *        handler code that are needed for processing
 *        requests received from the Controller or Leader node of cluster.
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

	log_info(RESTHandlerLogLevel, "Enter");

	memset(&Statistics_Interface, 0, sizeof(python_dps_statistics_t));
	do
	{
		// Get handle to an instance
		pyargs = Py_BuildValue("()");
		if (pyargs == NULL)
		{
			log_emergency(RESTHandlerLogLevel,
			              "ERROR! Py_BuildValue () failed...\n");
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}
		status = python_lib_get_instance(pythonpath,
		                                 PYTHON_MODULE_FILE_STATISTICS,
		                                 PYTHON_MODULE_CLASS_STATISTICS,
		                                 pyargs,
		                                 &Statistics_Interface.instance);
		Py_DECREF(pyargs);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		// Get handle to function Domain List Get
		Statistics_Interface.Domain_List_Get =
			PyObject_GetAttrString(Statistics_Interface.instance,
			                       PYTHON_FUNC_DOMAIN_LIST_GET);
		if (Statistics_Interface.Domain_List_Get == NULL)
		{
			log_emergency(RESTHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_DOMAIN_LIST_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Load Balancing Get
		Statistics_Interface.Statistics_Load_Balancing_Get =
			PyObject_GetAttrString(Statistics_Interface.instance,
			                       PYTHON_FUNC_STATISTICS_LOAD_BALANCING_GET);
		if (Statistics_Interface.Statistics_Load_Balancing_Get == NULL)
		{
			log_emergency(RESTHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_STATISTICS_LOAD_BALANCING_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function General Statistics Get
		Statistics_Interface.Statistics_General_Statistics_Get =
			PyObject_GetAttrString(Statistics_Interface.instance,
			                       PYTHON_FUNC_STATISTICS_GENERAL_STATISTICS_GET);
		if (Statistics_Interface.Statistics_General_Statistics_Get == NULL)
		{
			log_emergency(RESTHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_STATISTICS_GENERAL_STATISTICS_GET);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		// Get handle to function Load Balancing Update
		Statistics_Interface.Statistics_Load_Balancing_Update =
			PyObject_GetAttrString(Statistics_Interface.instance,
			                       PYTHON_FUNC_STATISTICS_LOAD_BALANCING_UPDATE);
		if (Statistics_Interface.Statistics_Load_Balancing_Update == NULL)
		{
			log_emergency(RESTHandlerLogLevel,
			              "ERROR! PyObject_GetAttrString (%s) failed...\n",
			              PYTHON_FUNC_STATISTICS_LOAD_BALANCING_UPDATE);
			status = DOVE_STATUS_NOT_FOUND;
			break;
		}

		status = DOVE_STATUS_OK;
	}while(0);

	log_info(RESTHandlerLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;
}

dove_status dcs_statistics_init(char *pythonpath)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		status = python_functions_init(pythonpath);
		if (status != DOVE_STATUS_OK)
		{
			break;
		}

		/* Initialize mutex and condition variable objects */
		if (pthread_mutex_init(&dps_statistics_mp, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		if (pthread_cond_init (&dps_statistics_cv, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		/* Create a thread for statistics collection */
		if (create_task((const char *)"Stat", 0, OSW_DEFAULT_STACK_SIZE,
		                dps_statistics_main, 0,
		                &statisticsTaskId) != OSW_OK)
		{
			status = DOVE_STATUS_THREAD_FAILED;
			break;
		}
	} while (0);

	return status;
}

dove_status dps_statistics_start(void)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		if (search_task("Stat") != 0)
		{
			log_info(PythonDataHandlerLogLevel, "Thread is running\r\n");
			break;
		}
		log_info(PythonDataHandlerLogLevel, "Starting thread\r\n");
		/* Re-initialize resources and create a thread */
		if (pthread_mutex_init(&dps_statistics_mp, NULL) != 0)
		{
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		if (pthread_cond_init (&dps_statistics_cv, NULL) != 0)
		{
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		if (create_task((const char *)"Stat", 0, OSW_DEFAULT_STACK_SIZE,
		                dps_statistics_main, 0,
		                &statisticsTaskId) != OSW_OK)
		{
			status = DOVE_STATUS_THREAD_FAILED;
			break;
		}

	} while (0);

	return status;
}


dove_status dps_statistics_stop(void)
{
	dove_status status = DOVE_STATUS_OK;

	do
	{
		if (search_task("Stat") == 0)
		{
			log_info(PythonDataHandlerLogLevel, "Thread not exist");
			break;
		}
		log_info(PythonDataHandlerLogLevel, "Waiting for thread exit...\r\n");
		/* Set condition var and then wait thread exit*/
		pthread_mutex_lock(&dps_statistics_mp);
		pthread_cond_broadcast(&dps_statistics_cv);
		pthread_mutex_unlock(&dps_statistics_mp);
		while (search_task("Stat") != 0)
		{
			sleep(1);
		}

	} while (0);

	return status;
}

