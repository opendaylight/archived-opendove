/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client for Clustering functionality
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
*  $Log: cluster_rest_client.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include "include.h"
// #include <event2/event.h>
// #include <event2/http.h>
// #include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "cluster_rest_client.h"
#include "cluster_rest_req_handler.h"


json_t *dps_form_local_domains_json(char *ip,int port,char *domains)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:s,s:i,s:s}",
	                    "ip",ip,
	                    "port",port,
	                    "domains", domains);

	return js_root;

}

void dps_cluster_send_local_domains(ip_addr_t *nodes, uint32_t nodes_count,
                                    char *domains)
{
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	uint32_t i = 0;
	char str[INET6_ADDRSTRLEN];
	char ipstr[INET6_ADDRSTRLEN];

	do {
		inet_ntop(dps_local_ip.family, dps_local_ip.ip6, ipstr, INET6_ADDRSTRLEN);
		/* form json string*/
		js_res = dps_form_local_domains_json(ipstr,(int)dps_local_ip.port,domains);

		log_debug(PythonClusterDataLogLevel, "Domain Mapping [%s]",domains);
		if(js_res == NULL)
		{
			log_alert(PythonClusterDataLogLevel,"Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri, DPS_URI_LEN, DPS_CLUSTER_LOCAL_DOMAINS_URI);

		for(i = 0 ; i < nodes_count ; i++)
		{
			// send it synchronously
			if(nodes[i].family == AF_INET &&
			   nodes[i].ip4 == dps_local_ip.ip4)
			{
				continue;
			}
			else if(nodes[i].family == AF_INET6 &&
			        !memcmp(nodes[i].ip6, dps_local_ip.ip6, 16))
			{
				continue;
			}
			request = dps_cluster_request_new();
			if(request == NULL) {
				log_alert(PythonClusterDataLogLevel,
				          "Can not alloc the evhttp request");
				break;
			}
			if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
			{
				break;
			}
			inet_ntop(nodes[i].family, nodes[i].ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "Sending domain info to DPS [%s]",str);
			dps_rest_client_dove_controller_send_asyncprocess(
				str, uri, dps_rest_port, EVHTTP_REQ_PUT,
				request);
		}
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return;
}

json_t *dps_form_domain_transfer_json(int family,char *ipstr)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:i,s:s}",
	                    "ip_family",family,
	                    "ip", ipstr);

	return js_root;

}
/*
 * POST /api/dove/dps/move-domain
 * {
 * 	"ip_family":
 * 	"ip":
 * }
 */

dove_status dps_cluster_leader_initiate_domain_move(uint32_t domain,
                                                    ip_addr_t *src_node,
                                                    ip_addr_t *dst_node)
{
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];
	char ipstr[INET6_ADDRSTRLEN];
	dove_status status = DOVE_STATUS_ERROR;

	do {
		// 1. Get Ready to dst_node
		inet_ntop(dst_node->family, dst_node->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel,
		         "Domain %d: Sending GET READY initiation to DPS [%s]",
		         domain, str);
		status = dps_node_get_ready(dst_node, domain);
		if(status != DOVE_STATUS_OK)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Get Ready failed for node domain %u",domain);
			break;
		}

		//2. Initiate the mass transfer
		inet_ntop(dst_node->family, dst_node->ip6, ipstr, INET6_ADDRSTRLEN);
		/* form json string */
		js_res = dps_form_domain_transfer_json(dst_node->family, ipstr);

		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,"Can not get the js_res");
			break;
		}
		request = dps_cluster_request_new();
		if(request == NULL) {
			log_alert(PythonDataHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		// set the uri
		DPS_CLUSTER_TRANSFER_DOMAIN_URI_GEN(uri,domain);
		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}
		inet_ntop(src_node->family, src_node->ip6, str, INET6_ADDRSTRLEN);
		log_info(PythonDataHandlerLogLevel,
		         "Domain %d: Sending initiate Mass Transfer to DPS [%s]",
		          domain, str);
		dps_rest_client_dove_controller_send_asyncprocess(
			str, uri, DPS_REST_HTTPD_PORT, EVHTTP_REQ_POST,
			request);

		status = DOVE_STATUS_OK;

	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return status;
}

void dps_print_policy_info(dps_object_policy_add_t *policy)
{
	log_debug(RESTHandlerLogLevel,"Domain Id %d, type %d, sdvg %d ddvg %d ttl %d action %d",
	          policy->domain_id, policy->type, policy->src_dvg_id,policy->dst_dvg_id,policy->ttl,policy->action);
}

uint32_t dps_get_policy_from_jdata(uint32_t domain_id, json_t *data, dps_object_policy_add_t *policy)
{
	json_t *js_tok = NULL;
	uint32_t status = DOVE_STATUS_OK;
	do
	{
		policy->domain_id = domain_id;
		/* Borrowed reference, no need to decref */
		js_tok = json_object_get(data, "type");
		if (NULL == js_tok || !json_is_integer(js_tok))
		{
			status = DOVE_STATUS_INVALID_POLICY;
			break;
		}
		policy->type = (uint32_t)json_integer_value(js_tok);
		if(DPS_POLICY_TYPE_CONN == policy->type)
		{
			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "src_dvg");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				status = DOVE_STATUS_INVALID_POLICY;
				break;
			}
			policy->src_dvg_id = (uint32_t)json_integer_value(js_tok);

			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "dst_dvg");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				status = DOVE_STATUS_INVALID_POLICY;
				break;
			}
			policy->dst_dvg_id = (uint32_t)json_integer_value(js_tok);
#if 0
			if(!IS_DVG_ID_VALID(policy->src_dvg_id)||!IS_DVG_ID_VALID(policy->dst_dvg_id))
			{
				status = DOVE_STATUS_INVALID_POLICY;
				break;
			}
#endif
			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "ttl");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				status = DOVE_STATUS_INVALID_POLICY;
				break;
			}
			policy->ttl = (uint32_t)json_integer_value(js_tok);

			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "action");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				status = DOVE_STATUS_INVALID_POLICY;
				break;
			}
			policy->action.connectivity = (uint16_t)json_integer_value(js_tok);

			policy->version = 1;
		}
		else
		{
			/* TODO: support for other policy type */
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
	} while (0);
	return status;
}

static void dps_policy_replicate_response_handler(struct evhttp_request *req, void *arg)
{
	json_t *js_root = NULL;
	json_error_t jerror;
	struct evbuffer *req_body;
	int32_t http_response = HTTP_OK;
	char req_body_buf[1024];
	int n;
	uint32_t domain_id = 0;
	const char *uri;
	dps_object_policy_add_t policy = {0};

	if((http_response = evhttp_request_get_response_code(req)) != HTTP_OK) {
		log_debug(RESTHandlerLogLevel, "Policy Response Handler response error HTTP_NOT_OK...");
		http_response = DOVE_STATUS_ERROR;
	}
	uri = evhttp_request_get_uri(req);
	if (sscanf(uri, DOVE_CLUSTER_POLICY_URI, &domain_id) != 1)
	{
		log_debug(RESTHandlerLogLevel, "Policy Replication --error in parsing domain id ");
	}
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_PUT: 
		case EVHTTP_REQ_DELETE:
		case EVHTTP_REQ_POST:
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
			{
				log_debug(RESTHandlerLogLevel, "Policy Response Handler -- Could not get json data");
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, (sizeof(req_body_buf) - 1));
			req_body_buf[n] = '\0';
			js_root = json_loads(req_body_buf, 0, &jerror);

			if (!js_root)
			{
				log_debug(RESTHandlerLogLevel, "Policy Response Handler -- js_root is NULL");
				break;
			}
			dps_get_policy_from_jdata(domain_id, js_root, &policy);
			dps_print_policy_info(&policy);
			break;
		}
		default:
		{
			log_debug(RESTHandlerLogLevel, "Policy Response Handler -- Invalid Cmd");
			break;
		}
	}
	// No need to free buffer
	// TODO: Send the info to dps server with status set.
	if (js_root)
	{
		json_decref(js_root);
	}
	return;
}

struct evhttp_request *dps_rest_client_policy_replicate_request_new(void)
{
	struct evhttp_request *request = NULL;
	request = evhttp_request_new(dps_policy_replicate_response_handler, NULL);
	return request;
}

static json_t *dps_form_json_vnid_list_info(uint32_t domain_id, PyObject *pyList_vnid)
{
	json_t *js_root = NULL;
	json_t *json_vnid_array = NULL;
	json_t *json_vnid = NULL;
	int i;
	PyObject *pyVNID;
	uint32_t vnid;

	do
	{
		if(!PyList_Check(pyList_vnid))
		{
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			break;
		}
		json_vnid_array = json_array();
		if (json_vnid_array == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "json_array() for json_vnid_array returns NULL");
			break;
		}
		for (i = 0; i < PyList_Size(pyList_vnid); i++)
		{
			pyVNID = PyList_GetItem(pyList_vnid, i);
			if (!PyArg_Parse(pyVNID, "I", &vnid))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Invalid VNID in element %d", i);
				continue;
			}
			json_vnid = json_integer((json_int_t)vnid);
			if (json_vnid == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "Cannot create JSON Integer for VNID %d", vnid);
				continue;
			}
			if (json_array_append_new(json_vnid_array, json_vnid) == -1)
			{
				json_decref(json_vnid);
				continue;
			}
		}
		js_root = json_pack("{s:i, s:o}",
		                    "domain", domain_id,
		                    "vnids", json_vnid_array);
	}while(0);
	if (js_root == NULL)
	{
		if (json_vnid_array)
		{
			json_decref(json_vnid_array);
		}
	}
	return js_root;

}

json_t *dps_form_json_policy_info(uint32_t domain_id, uint32_t traffic_type, uint32_t type, 
                                  uint32_t sdvg, uint32_t ddvg, uint32_t ttl, uint32_t action)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i}",
	                    "traffic_type", traffic_type,
	                    "type", type,
	                    "src_dvg", (int)sdvg,
	                    "dst_dvg", ddvg,
	                    "ttl", ttl,
	                    "action", action);

	return js_root;

}

static void dps_replicate_synch_response_handler(struct evhttp_request *req, void *arg)
{

	log_info(PythonClusterDataLogLevel, "Enter req %p arg %p",req, arg);
	if (req == NULL)
	{
		log_info(PythonClusterDataLogLevel, "Request timed out");
		*((int *)arg) = 408;
	}
	else
	{
		*((int *)arg) = evhttp_request_get_response_code(req);
		log_info(PythonClusterDataLogLevel, "Resp Code = %d", *((int *)arg));
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

int dps_cluster_vnid_replication(char *dps_node, uint32_t crud,
                                 uint32_t domain_id, PyObject *pyList_vnid)
{
	json_t *js_res = NULL;
	struct evhttp_request *request = NULL;
	char uri[256];
	int args = HTTP_OK;
	int ret = 0;

	log_info(PythonDataHandlerLogLevel, "Enter");
	/* Get the json string */
	do
	{
		js_res = dps_form_json_vnid_list_info(domain_id, pyList_vnid);
		if(js_res == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "No js_res,just return, not send anything!");
			ret = -1;
			break;
		}

		request = evhttp_request_new(dps_replicate_synch_response_handler, &args);
		if(request == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "Can not alloc the evhttp request");
			ret = -1;
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request, js_res) != DOVE_STATUS_OK)
		{
			ret = -1;
			break;
		}

		//set the uri
		DOVE_CLUSTER_VNID_LISTING_URI_GEN(uri, domain_id);

		//send it synchronously to DPS Node
		log_info(PythonClusterDataLogLevel,
		         "DCS Node %s Port %d URI %s",
		         dps_node, dps_rest_port, uri);

		ret = dove_rest_request_and_syncprocess(dps_node, dps_rest_port,
		                                        (crud ? EVHTTP_REQ_PUT: EVHTTP_REQ_DELETE),
		                                        uri, request, NULL, 3);
		if (ret)
		{
			log_info(PythonClusterDataLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(PythonClusterDataLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}

	} while (0);
	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(PythonDataHandlerLogLevel, "Exit");
	return ret;
}

/*
 * This function replicates 1 policy in a domain
 */
int dps_cluster_policy_replication(char *dps_node, uint32_t crud, uint32_t domain_id, uint32_t traffic_type,
                                   uint32_t type, uint32_t sdvg, uint32_t ddvg, uint32_t ttl, uint32_t action)
{
	json_t *js_res = NULL;
	struct evhttp_request *request = NULL;
	char uri[256];
	int args = HTTP_OK;
	int ret = 0;
	
	/* Get the json string */
	js_res = dps_form_json_policy_info(domain_id, traffic_type, type, sdvg, ddvg, ttl, action);
	do
	{
		if(js_res == NULL){
			log_alert(PythonClusterDataLogLevel,
			          "No js_res,just return, not send anything!");
			return -1;
		}

#if 0   
		//Used for async
		request = dps_rest_client_policy_replicate_request_new();
#else
		request = evhttp_request_new(dps_replicate_synch_response_handler, &args);
#endif
		if(request == NULL) {
			log_alert(PythonClusterDataLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		//set the uri
		DOVE_CLUSTER_POLICY_INFO_URI_GEN(uri, domain_id);

		//send it asynchronously to Dove Controller
		log_info(PythonClusterDataLogLevel,
		         "DCS Node %s Port %d URI %s",
		         dps_node, dps_rest_port, uri);

#if 0
		ret = dps_rest_client_dove_controller_send_asyncprocess(dps_node, uri, dps_rest_port, 
		                                                        (crud ? EVHTTP_REQ_POST:EVHTTP_REQ_DELETE), 
		                                                        request);
#else
		ret = dove_rest_request_and_syncprocess(dps_node, dps_rest_port,
		                                        (crud ? EVHTTP_REQ_POST:EVHTTP_REQ_DELETE),
		                                        uri, request, NULL, 3);
#endif
		if (ret)
		{
			log_info(PythonClusterDataLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(PythonClusterDataLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		
	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return ret;
}

/*
 * This function creates an array of policies in a domain
 */
static json_t *dps_form_json_policy_list_info(uint32_t domain_id, PyObject *pyList_policy)
{
	uint32_t traffic_type, type, sdvg, ddvg, ttl, action;
	json_t *js_root = NULL;
	json_t *json_policy_array = NULL;
	json_t *json_policy = NULL;
	int i;
	PyObject *pyPolicy;

	do
	{
		if(!PyList_Check(pyList_policy))
		{
			log_warn(PythonClusterDataLogLevel, "Not List Type!!!");
			break;
		}
		json_policy_array = json_array();
		if (json_policy_array == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "json_array() for json_vnid_array returns NULL");
			break;
		}
		for (i = 0; i < PyList_Size(pyList_policy); i++)
		{
			pyPolicy = PyList_GetItem(pyList_policy, i);
			if (pyPolicy == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "Bulk Policy Transfer NULL Item in element %d", i);
				continue;
			}
			if (!PyArg_ParseTuple(pyPolicy, "IIIIII", &traffic_type, &type, &sdvg, &ddvg, &ttl, &action))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Bulk Policy Transfer Invalid Item in element %d", i);
				continue;
			}
			json_policy = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i}",
			                        "traffic_type", traffic_type,
			                        "type", type,
			                        "src_dvg", (int)sdvg,
			                        "dst_dvg", ddvg,
			                        "ttl", ttl,
			                        "action", action);
			if (json_policy == NULL)
			{
				log_warn(PythonClusterDataLogLevel,
				         "Cannot create JSON policy object for item %d", i);
				continue;
			}
			if (json_array_append_new(json_policy_array, json_policy) == -1)
			{
				json_decref(json_policy);
				continue;
			}
		}
		js_root = json_pack("{s:o}", "policies", json_policy_array);
	}while(0);
	if (js_root == NULL)
	{
		if (json_policy_array)
		{
			json_decref(json_policy_array);
		}
	}
	return js_root;

}

/*
 * This function replicates a bunch of policies in a domain
 */
int dps_cluster_bulk_policy_replication(char *dps_node, uint32_t crud,
                                        uint32_t domain_id, PyObject *pyList_policy)
{
	json_t *js_res = NULL;
	struct evhttp_request *request = NULL;
	char uri[256];
	int args = HTTP_OK;
	int ret = 0;

	log_info(PythonDataHandlerLogLevel, "Enter");
	/* Get the json string */
	do
	{
		js_res = dps_form_json_policy_list_info(domain_id, pyList_policy);
		if(js_res == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "No js_res,just return, not send anything!");
			ret = -1;
			break;
		}
		request = evhttp_request_new(dps_replicate_synch_response_handler, &args);
		if(request == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "Can not alloc the evhttp request");
			ret = -1;
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request, js_res) != DOVE_STATUS_OK)
		{
			ret = -1;
			break;
		}

		//set the uri
		DOVE_CLUSTER_BULK_POLICY_URI_GEN(uri, domain_id);

		//send it synchronously to DPS Node
		log_info(PythonClusterDataLogLevel,
		         "DCS Node %s Port %d URI %s",
		         dps_node, dps_rest_port, uri);

		ret = dove_rest_request_and_syncprocess(dps_node, dps_rest_port,
		                                        (crud ? EVHTTP_REQ_PUT: EVHTTP_REQ_DELETE),
		                                        uri, request, NULL, 3);
		if (ret)
		{
			log_info(PythonClusterDataLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(PythonClusterDataLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}

	} while (0);
	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(PythonDataHandlerLogLevel, "Exit");
	return ret;
}

static json_t *dps_form_new_domain_json(int domain_id, char * name,
                                        int replication_factor)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:i,s:s,s:i}",
	                    "id",domain_id,
	                    "name",name,
	                    "replication_factor", replication_factor
	                    );

	return js_root;

}



/*
 ******************************************************************************
 * dps_leader_create_domain                                               *//**
 *
 * \brief - This routine allows the DCS Leader to create a domain organically
 *          and use it for internal purposes. This domain is not available to
 *          DOVE consumers
 *
 * \param domain_id The Domain ID
 * \param name The Domain Name
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_leader_create_domain(int domain_id, char *name)
{
	ip_addr_t nodes[SHARED_REPLICATION_FACTOR];
	uint32_t nodes_count = 0;
	dove_status status = DOVE_STATUS_OK;
	uint32_t i;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];
	int fLocal = 0;

	log_info(PythonClusterDataLogLevel, "Enter: Domain Id %d", domain_id);

	do {
		status = dps_get_lowest_loaded_nodes(SHARED_REPLICATION_FACTOR,
		                                     nodes, &nodes_count);
		if ((status != DOVE_STATUS_OK) || (nodes_count == 0))
		{
			log_alert(PythonClusterDataLogLevel,
			          "ALERT!!! Cannot allocate Shared Domain");
			break;
		}

		if (nodes_count < SHARED_REPLICATION_FACTOR)
		{
			log_warn(PythonClusterDataLogLevel,
			          "WARNING!!! Available nodes %d is lower than requested %d",
			          nodes_count, SHARED_REPLICATION_FACTOR);
		}

		js_res = dps_form_new_domain_json(domain_id, name, SHARED_REPLICATION_FACTOR);
		if(js_res == NULL)
		{
			status = DOVE_STATUS_NO_RESOURCES;
			log_alert(PythonClusterDataLogLevel,"Can not get js_res");
			break;
		}
		snprintf(uri, DPS_URI_LEN, DPS_DOMAINS_URI);

		for (i = 0; i < nodes_count; i++)
		{
			if (((nodes[i].family == AF_INET) && (nodes[i].ip4 == dps_local_ip.ip4)) ||
			    (!memcmp(nodes[i].ip6, dps_local_ip.ip6, 16)))
			{
				log_info(PythonClusterDataLogLevel,
				         "Domain [%d] is going to be handled by the Local Node",
				         domain_id);
				fLocal = 1;
			}
			else
			{
				request = dps_cluster_request_new();
				if(request == NULL) {
					status = DOVE_STATUS_NO_RESOURCES;
					log_alert(PythonClusterDataLogLevel,
					          "Can not alloc the evhttp request");
					break;
				}
				if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
				{
					status = DOVE_STATUS_ERROR;
					break;
				}
				inet_ntop(nodes[i].family, nodes[i].ip6, str, INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel,
				         "Forwarding Domain [%d] creation request to [%s]",
				         domain_id, str);
				dps_rest_client_dove_controller_send_asyncprocess(
					str, uri, dps_rest_port, EVHTTP_REQ_POST,
					request);
			}
		}
		if (status != DOVE_STATUS_OK)
		{
			break;
		}
		if (!fLocal)
		{
			break;
		}
		// Create Domain Locally
		status = dps_rest_api_create_domain(domain_id, SHARED_REPLICATION_FACTOR);
		if (status != DOVE_STATUS_OK)
		{
			log_debug(PythonDataHandlerLogLevel,
			          "Failed to create domain...[%d]",domain_id);
			break;
		}
	} while (0);

	log_debug(PythonClusterDataLogLevel, "Exit: Status %s",
	           DOVEStatusToString(status));

	return status;
}

json_t *dps_form_new_vn_json(int vnid,char * name)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:i,s:s}",
	                    "id",vnid,
	                    "name",name
	                    );

	return js_root;

}

dove_status dps_leader_create_vn(int domain_id,int vnid,char *name)
{
	int ret = DOVE_STATUS_ERROR;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];
	ip_addr_t nodes[MAX_NODES_ON_STACK];
	ip_addr_t *ipnodes;
	void *nodes_memory = NULL;
	uint32_t nodes_count = 0;
	int fLocal = 0;
	int i, nodes_memory_allocated = 0;

	do {
		ret = dps_get_all_cluster_nodes(MAX_NODES_ON_STACK, nodes,
		                                &nodes_count);
		if (ret != DOVE_STATUS_OK && ret != DOVE_STATUS_RETRY) {
			log_alert(RESTHandlerLogLevel,
			          "ALERT!!! dps_get_all_cluster_nodes returned [%d]",
			          ret);
			break;
		}
		if (ret == DOVE_STATUS_RETRY) {
			// no of nodes in cluster > MAX_NODES_ON_STACK
			// allocate memory on heap
			// allocate memory on heap
			nodes_memory = malloc(sizeof(ip_addr_t)*nodes_count);
			if (nodes_memory == NULL)
			{
				break;
			}
			ipnodes = (ip_addr_t *)nodes_memory;
			nodes_memory_allocated = 1;
			ret = dps_get_all_cluster_nodes(nodes_count, ipnodes,
			                                &nodes_count);
			if (ret != DOVE_STATUS_OK) {
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! dps_get_all_cluster_nodes returned [%d]",
				          ret);
				break;
			}
		}
		else
		{
			ipnodes = &nodes[0];
			nodes_memory = &nodes[0];
		}

		js_res = dps_form_new_vn_json(vnid,name);
		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,"Can not get js_res");
			break;
		}
		DPS_DVGS_URI_GEN(uri,domain_id);
		for (i = 0; i < (int)nodes_count; i++) {
			if (ipnodes[i].ip4 == dps_local_ip.ip4) {
				log_info(PythonClusterDataLogLevel,
				         "VN [%d] handled by the LEADER",
				         vnid);
				fLocal = 1;
			}
			else {
				request = dps_cluster_request_new();
				if(request == NULL) {
					log_alert(PythonDataHandlerLogLevel,
					          "Cannot alloc the evhttp request");
					goto End;
				}
				if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
				{
					goto End;
				}
				inet_ntop(ipnodes[i].family, ipnodes[i].ip6, str,
				          INET6_ADDRSTRLEN);
				log_info(PythonClusterDataLogLevel,
				          "Forwarding VN [%d] creation request to [%s]",
				          vnid,str);
				dps_rest_client_dove_controller_send_asyncprocess(
					str, uri, dps_rest_port, EVHTTP_REQ_POST,
					request);
			}
		}
		if (fLocal)
		{
			if (DOVE_STATUS_OK != dps_rest_api_create_dvg(domain_id,
			                                              vnid,
			                                              DPS_CONTROLLER_DVG_ADD))
			{
				log_debug(PythonDataHandlerLogLevel,
				          "Failed to create vnid...[%d]",vnid);
				break;
			}
		}
		ret = DOVE_STATUS_OK;
	} while (0);
End:
	if (nodes_memory_allocated)
	{
		free(nodes_memory);
		nodes_memory = NULL;
		ipnodes = NULL;
	}
	return (dove_status)ret;
}

int dps_cluster_ipsubnet4_replication(char *dps_node, uint32_t crud,
                                      uint32_t vnid, char *ip, char *mask,
                                      char *gw, uint32_t mode)
{
	json_t *js_res = NULL;
	struct evhttp_request *request = NULL;
	char uri[256], mode_str[100];
	int args = HTTP_OK;
	int ret = 0;
	
	if (mode == DPS_SHARED_ADDR_SPACE)
	{
		strcpy(mode_str, "shared");
	}
	else
	{
		strcpy(mode_str, "dedicated");
	}

	/* Get the json string */
	js_res = json_pack("{s:s, s:s, s:s, s:s}", 
	                    "ip", ip,
	                    "mask", mask,
	                    "mode", mode_str,
	                    "gateway", gw);

	do
	{
		if(js_res == NULL){
			log_alert(PythonClusterDataLogLevel,
			          "No js_res,just return, not send anything!");
			return -1;
		}

#if 0   
		//Used for async
		request = dps_rest_client_subnet4_replicate_request_new();
#else
		request = evhttp_request_new(dps_replicate_synch_response_handler, &args);
#endif
		if(request == NULL) {
			log_alert(PythonClusterDataLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		//set the uri
		DOVE_CLUSTER_SUBNET4_URI_GEN(uri, vnid);

		//send it asynchronously to Dove Controller
		log_info(PythonClusterDataLogLevel,
		         "DCS Node %s Port %d URI %s",
		         dps_node, dps_rest_port, uri);

#if 0
		ret = dps_rest_client_dove_controller_send_asyncprocess(dps_node, uri, dps_rest_port, 
		                                                        (crud ? EVHTTP_REQ_POST:EVHTTP_REQ_DELETE), 
		                                                        request);
#else
		ret = dove_rest_request_and_syncprocess(dps_node, dps_rest_port, (crud ? EVHTTP_REQ_POST:EVHTTP_REQ_DELETE),
		                                        uri, request, NULL, 3);
#endif
		if (ret)
		{
			log_info(PythonClusterDataLogLevel,
			         "Return not success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(PythonClusterDataLogLevel,
			         "Return success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		
	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return ret;
}

dove_status dps_node_get_ready(ip_addr_t *node,uint32_t domain)
{
	int ret = DOVE_STATUS_ERROR;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];

	do {
		/* Get the json string */
		js_res = json_pack("{s:i}",
		                   "domain", domain
		                   );
		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri,DPS_URI_LEN,DPS_NODE_GET_READY);

		request = dps_cluster_request_new();
		if(request == NULL) {
			log_alert(PythonDataHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		inet_ntop(node->family, node->ip6, str, INET6_ADDRSTRLEN);
		log_debug(PythonDataHandlerLogLevel,
		          "Sending GET-READY for domain [%d] to node [%s]",
		          domain,str);
		if (dps_rest_client_dove_controller_fill_evhttp(request, js_res) != DOVE_STATUS_OK)
		{
			break;
		}
#if 1
		dps_rest_client_dove_controller_send_asyncprocess(
			str, uri, DPS_REST_HTTPD_PORT, EVHTTP_REQ_POST,
			request);
		ret = DOVE_STATUS_OK;
#else
		ret = dove_rest_request_and_syncprocess(str, DPS_REST_HTTPD_PORT,
		                                        EVHTTP_REQ_POST,uri,
		                                        request, NULL, 10);
		if (ret)
		{
			log_debug (PythonClusterDataLogLevel,
			           "syncprocess returned ERROR");
			ret = DOVE_STATUS_ERROR;
			break;
		}
		ret = DOVE_STATUS_OK;
#endif
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return (dove_status)ret;
}

dove_status dps_node_domain_activate(ip_addr_t *node,
                                     uint32_t domain,
                                     uint32_t replication_factor)
{
	int ret = DOVE_STATUS_ERROR;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");

	do {
		/* Get the json string */
		js_res = json_pack("{s:i, s:i}",
		                   "domain", domain,
		                   "replication_factor", replication_factor
		                   );
		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri, DPS_URI_LEN, DPS_NODE_DOMAIN_ACTIVATE);

		request = dps_cluster_request_new();
		if(request == NULL) {
			log_alert(PythonDataHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		inet_ntop(node->family, node->ip6, str, INET6_ADDRSTRLEN);
		log_notice(PythonDataHandlerLogLevel,
		           "Domain %d: Sending ACTIVATE to node [%s]",
		           domain, str);
		ret = dps_rest_client_dove_controller_fill_evhttp(request,js_res);
		if (ret != DOVE_STATUS_OK)
		{
			break;
		}

		//ACTIVATE MUST be a synchronous process since the local
		//node needs to maintain state if Activate works
		ret = dove_rest_request_and_syncprocess(str, dps_rest_port,
		                                        EVHTTP_REQ_POST,uri,
		                                        request, NULL, 10);
		if (ret)
		{
			log_error(PythonClusterDataLogLevel,
			          "syncprocess returned ERROR");
			ret = DOVE_STATUS_ERROR;
			break;
		}
		ret = DOVE_STATUS_OK;
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}

	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)ret));
	return (dove_status)ret;
}

dove_status dps_node_domain_deactivate(ip_addr_t *node,uint32_t domain)
{
	int ret = DOVE_STATUS_ERROR;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");
	do {
		/* Get the json string */
		js_res = json_pack("{s:i}",
		                   "domain", domain
		                   );
		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri,DPS_URI_LEN,DPS_NODE_DOMAIN_DEACTIVATE);

		request = dps_cluster_request_new();
		if(request == NULL) {
			log_alert(PythonDataHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		inet_ntop(node->family, node->ip6, str, INET6_ADDRSTRLEN);
		log_notice(PythonClusterDataLogLevel,
		          "Sending DEACTIVATE for domain [%d] to node [%s]",
		          domain,str);
		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}
		dps_rest_client_dove_controller_send_asyncprocess(
			str, uri, DPS_REST_HTTPD_PORT, EVHTTP_REQ_POST,
			request);
		ret = DOVE_STATUS_OK;
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)ret));
	return (dove_status)ret;
}

/*
 ******************************************************************************
 * dps_node_domain_recover --                                             *//**
 *
 * \brief This routine sends the Domain Recover Message to a specific DCS node
 *
 * \param node IP Address of the DCS Node
 * \param domain The Domain ID
 * \param replication_factor The Replication Factor of the Domain
 *
 * \retval DOVE_STATUS_OK Message was forwarded to a remote Node
 * \retval DOVE_STATUS_NO_MEMORY No Memory
 *
 *****************************************************************************/

dove_status dps_node_domain_recover(ip_addr_t *node,
                                    uint32_t domain,
                                    uint32_t replication_factor)
{
	int ret = DOVE_STATUS_NO_MEMORY;
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str[INET6_ADDRSTRLEN];

	log_info(PythonDataHandlerLogLevel, "Enter");

	do {
		/* Get the json string */
		js_res = json_pack("{s:i, s:i}",
		                   "domain", domain,
		                   "replication_factor", replication_factor
		                   );
		if(js_res == NULL)
		{
			log_alert(PythonDataHandlerLogLevel,
			          "Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri, DPS_URI_LEN, DPS_NODE_DOMAIN_RECOVER);

		request = dps_cluster_request_new();
		if(request == NULL) {
			log_alert(PythonDataHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		inet_ntop(node->family, node->ip6, str, INET6_ADDRSTRLEN);
		log_notice(PythonDataHandlerLogLevel,
		           "Sending RECOVER for domain [%d] to node [%s]",
		           domain,str);
		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}
		dps_rest_client_dove_controller_send_asyncprocess(
			str, uri, DPS_REST_HTTPD_PORT, EVHTTP_REQ_POST,
			request);
		ret = DOVE_STATUS_OK;
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}

	log_info(PythonDataHandlerLogLevel, "Exit %s",
	         DOVEStatusToString((dove_status)ret));
	return (dove_status)ret;
}

/*
 ******************************************************************************
 * dps_rest_domain_delete_send_to_dps_node --                             *//**
 *
 * \brief This routine sends a REST Domain Delete message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_domain_delete_send_to_dps_node(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	ip_addr_t remote_node;
	uint32_t domain_id;
	char *remote_ip;
	int remote_ip_size;
	char domain_delete_uri[128];

	log_debug(PythonClusterDataLogLevel, "Enter");
	do
	{
		if (!PyArg_ParseTuple(args, "z#I",
		                      &remote_ip,
		                      &remote_ip_size,
		                      &domain_id))
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
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "Remote IP Address %s, Domain %d", str, domain_id);
		}
#endif

		sprintf(domain_delete_uri,"%s/%d", DPS_DOMAINS_URI, domain_id);
		dps_rest_client_json_send_to_dps_node(NULL,
		                                      domain_delete_uri,
		                                      EVHTTP_REQ_DELETE,
		                                      &remote_node);

	}while(0);
	ret_val = Py_BuildValue("i", 0);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_rest_vnid_add_send_to_dps_node --                                 *//**
 *
 * \brief This routine sends a REST VNID Add message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_vnid_add_send_to_dps_node(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	ip_addr_t remote_node;
	uint32_t domain_id;
	uint32_t vnid;
	json_t *js_root = NULL;
	char *remote_ip;
	int remote_ip_size;
	char vnid_add_uri[128];
	char vnid_name[16];

	log_debug(PythonClusterDataLogLevel, "Enter");
	do
	{
		if (!PyArg_ParseTuple(args, "z#II",
		                      &remote_ip,
		                      &remote_ip_size,
		                      &domain_id,
		                      &vnid))
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
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "Remote IP Address %s, Domain %d, VNID %d", str, domain_id, vnid);
		}
#endif
		sprintf(vnid_name, "VNID_%d", vnid);
		js_root = dps_form_new_vn_json(vnid, vnid_name);
		if(js_root == NULL)
		{
			log_warn(PythonDataHandlerLogLevel,"Can create js_root");
			break;
		}
		DPS_DVGS_URI_GEN(vnid_add_uri, domain_id);
		dps_rest_client_json_send_to_dps_node(js_root,
		                                      vnid_add_uri,
		                                      EVHTTP_REQ_POST,
		                                      &remote_node);

	}while(0);
	ret_val = Py_BuildValue("i", 0);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

/*
 ******************************************************************************
 * dps_rest_vnid_delete_send_to_dps_node --                               *//**
 *
 * \brief This routine sends a REST VNID Delete message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_vnid_delete_send_to_dps_node(PyObject *self, PyObject *args)
{
	PyObject *ret_val;
	ip_addr_t remote_node;
	uint32_t domain_id;
	uint32_t vnid;
	char *remote_ip;
	int remote_ip_size;
	char vnid_delete_uri[128];

	log_debug(PythonClusterDataLogLevel, "Enter");
	do
	{
		if (!PyArg_ParseTuple(args, "z#II",
		                      &remote_ip,
		                      &remote_ip_size,
		                      &domain_id,
		                      &vnid))
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
#if defined(NDEBUG)
		{
			char str[INET6_ADDRSTRLEN];
			inet_ntop(remote_node.family, remote_node.ip6, str, INET6_ADDRSTRLEN);
			log_info(PythonClusterDataLogLevel,
			         "Remote IP Address %s, Domain %d, VNID %d", str, domain_id, vnid);
		}
#endif
		DPS_DVG_URI_GEN(vnid_delete_uri, domain_id, vnid);
		dps_rest_client_json_send_to_dps_node(NULL,
		                                      vnid_delete_uri,
		                                      EVHTTP_REQ_DELETE,
		                                      &remote_node);

	}while(0);
	ret_val = Py_BuildValue("i", 0);
	log_debug(PythonClusterDataLogLevel, "Exit");

	return ret_val;
}

static json_t *dps_form_json_bulk_ip4subnet_info(uint32_t domain, PyObject *pyList_subnet)
{
	json_t *js_root = NULL;
	json_t *js_res = NULL;
	json_t *json_subnet_array = NULL;
	int i;
	uint32_t vnid;
	PyObject *pysubnet;
	char *ip, *mask, *gw, *mode;

	log_info(PythonClusterDataLogLevel, "Enter");
	do
	{
		if(!PyList_Check(pyList_subnet))
		{
			log_warn(PythonClusterDataLogLevel,
			         "Form JSON Bulk Subnet: Not List Type!!!");
			break;
		}
		json_subnet_array = json_array();
		if (json_subnet_array == NULL)
		{
			log_warn(PythonClusterDataLogLevel,
			         "Form JSON Bulk Subnet: json_array() for json_subnet_array returns NULL");
			break;
		}
		for (i = 0; i < PyList_Size(pyList_subnet); i++)
		{
			pysubnet = PyList_GetItem(pyList_subnet, i);
			if (!PyArg_ParseTuple(pysubnet, "Issss",
			                      &vnid, &ip, &mask, &gw, &mode))
			{
				log_warn(PythonClusterDataLogLevel,
				         "Form JSON Bulk Subnet: Invalid element %d", i);
				continue;
			}
			log_info(PythonDataHandlerLogLevel, "Parsed Element %d", i);
			js_res = json_pack("{s:i, s:s, s:s, s:s, s:s}",
			                    "vnid", vnid,
			                    "ip", ip,
			                    "mask", mask,
			                    "mode", mode,
			                    "gateway", gw);

			if(js_res == NULL){
				log_alert(PythonClusterDataLogLevel,
				          "Form JSON Bulk Subnet: json_pack failed. js_res is NULL");
				continue;
			}
			if (json_array_append_new(json_subnet_array, js_res) == -1)
			{
				json_decref(js_res);
				continue;
			}
		}
		js_root = json_pack("{s:o}",
		                    "ip4subnets", json_subnet_array);
	}while(0);
	if (js_root == NULL)
	{
		if (json_subnet_array)
		{
			json_decref(json_subnet_array);
		}
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return js_root;
}

int dps_cluster_bulk_ip4subnet_replication(char *dps_node, uint32_t crud,
                                           uint32_t domain, PyObject *pyList_subnet)
{
	json_t *js_res = NULL;
	struct evhttp_request *request = NULL;
	char uri[256];
	int args = HTTP_OK;
	int ret = 0;

	log_info(PythonClusterDataLogLevel, "Enter");
	/* Get the json string */
	do
	{
		js_res = dps_form_json_bulk_ip4subnet_info(domain, pyList_subnet);
		if(js_res == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "No js_res,just return, not send anything!");
			ret = -1;
			break;
		}

		request = evhttp_request_new(dps_replicate_synch_response_handler,
		                             &args);
		if(request == NULL)
		{
			log_alert(PythonClusterDataLogLevel,
			          "Can not alloc the evhttp request");
			ret = -1;
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request, js_res) != DOVE_STATUS_OK)
		{
			ret = -1;
			break;
		}

		//set the uri
		DOVE_CLUSTER_BULK_SUBNET4_URI_GEN(uri, domain);

		//send it synchronously to DPS Node
		log_info(PythonClusterDataLogLevel,
		         "DCS Node %s Port %d URI %s",
		         dps_node, dps_rest_port, uri);

		ret = dove_rest_request_and_syncprocess(dps_node, dps_rest_port,
		                                        (crud ? EVHTTP_REQ_POST: EVHTTP_REQ_DELETE),
		                                        uri, request, NULL, 3);
		if (ret)
		{
			log_info(PythonClusterDataLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(PythonClusterDataLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}

	} while (0);
	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return ret;
}

