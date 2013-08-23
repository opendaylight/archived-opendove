/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client Init
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
*  $Log: rest_main.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include "include.h"
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "../inc/rest_api.h"
#include "../inc/evhttp_helper.h"
#include "../inc/rest_req_handler.h"
#include "../inc/dove_rest_client.h"
#include "../inc/rest_forward.h"
#include "../inc/evhttp_helper.h"
#include "cluster_rest_req_handler.h"

int RESTHandlerLogLevel = DPS_SERVER_LOGLEVEL_NOTICE;

char large_REST_buffer_storage[LARGE_REST_BUFFER_SIZE];
char *large_REST_buffer = &large_REST_buffer_storage[0];

/**
 * \brief The DPS REST Services Port
 */
short dps_rest_port = 0;


/*
 ******************************************************************************
 * dps_rest_remote_node_get --                                            *//**
 *
 * \brief This routine derives the location of node which sent
 *
 * \param req - The REST request struct evhttp_request
 * \param remote_node - Pointer to a location which will be filled up by
 *                      this routine.
 *
 * \retval 1 Success
 * \retval 0 or -1 Failure
 *
 *****************************************************************************/

int dps_rest_remote_node_get(struct evhttp_request *req,
                             ip_addr_t *remote_node)
{
	struct evhttp_connection *evcon = NULL;
	char *remote_host = NULL;
	char *ptr = NULL;
	ev_uint16_t remote_port;
	int ret_val = -1;

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		evcon = evhttp_request_get_connection(req);
		if (evcon == NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "ERROR: evhttp_request_get_connection returns NULL");
			break;
		}
		evhttp_connection_get_peer(evcon, &remote_host, &remote_port);
		if (remote_host == NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "ERROR: evhttp_connection_get_peer returns NULL");
			break;
		}
		log_debug(RESTHandlerLogLevel, "Remote Host %s", remote_host);
		remote_node->port_http = remote_port;
		ptr = strchr(remote_host,':');
		if (NULL == ptr)
		{
			remote_node->family = AF_INET;
			ret_val = inet_pton(AF_INET, remote_host, remote_node->ip6);
		}
		else
		{
			remote_node->family = AF_INET6;
			ret_val = inet_pton(AF_INET6, remote_host, remote_node->ip6);
		}
		if (ret_val != 1)
		{
			log_error(RESTHandlerLogLevel,
			          "ERROR: evhttp_request has invalid remote host %s",
			          remote_host);
			break;
		}
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return ret_val;
}

/*
 ******************************************************************************
 * dps_rest_remote_node_touch --                                          *//**
 *
 * \brief This routine touches (in the cluster handler) the dps node which sent
 *        a REST request.
 *
 * \return void
 *
 *****************************************************************************/

void dps_rest_node_touch(struct evhttp_request *req)
{
	int ret_val;
	ip_addr_t remote_ip;

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		ret_val = dps_rest_remote_node_get(req, &remote_ip);
		if (ret_val != 1)
		{
			break;
		}
		dps_cluster_node_touch(&remote_ip);
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

///*
// ******************************************************************************
// * dps_rest_is_from_DMC --                                                *//**
// *
// * \brief This routine check if a request is from DMC.
// *
// * \param [in]  req 		A pointer to a evhttp_request data structure.
// *
// * \retval true
// * \retval false
// *
// *****************************************************************************/
//
//static bool dps_rest_is_from_controller(struct evhttp_request *req)
//{
//	struct evhttp_connection *evcon = NULL;
//	char *remote_host = NULL;
//	ev_uint16_t remote_port;
//	bool ret = true;
//
//	log_debug(RESTHandlerLogLevel, "Enter");
//
//	do
//	{
//		evcon = evhttp_request_get_connection(req);
//		if (evcon == NULL)
//		{
//			ret = false;
//			break;
//		}
//		evhttp_connection_get_peer(evcon, &remote_host, &remote_port);
//		if (remote_host == NULL)
//		{
//			ret = false;
//			break;
//		}
//
//		if (strcmp(remote_host, controller_location_ip_string))
//		{
//			log_info(RESTHandlerLogLevel,
//			         "[%s] received from non-DMC node [%s]",
//			         evhttp_request_get_uri(req), remote_host);
//			ret = false;
//			break;
//		}
//		log_info(RESTHandlerLogLevel, "[%s] receive from DMC[%s]",
//		         evhttp_request_get_uri(req), remote_host);
//	} while (0);
//
//	log_debug(RESTHandlerLogLevel, "Exit");
//
//	return ret;
//}

/*
 *****************************************************************************
 * http_request_version_update --                                        *//**
 *
 * \brief This routine update local node version when success to handle the req.
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int http_request_version_update(struct evhttp_request *req)
{
	int local_version, version_create = 0, version_update = 0;
	int res_code;
	int ret = DOVE_STATUS_ERROR;

	do
	{
		ret = dps_rest_sync_version_get_from_req(req, &version_create, &version_update);
		if (ret != DOVE_STATUS_OK)
		{
			break;
		}

		res_code = evhttp_request_get_response_code(req);
		log_debug(RESTHandlerLogLevel, "res_code: %d", res_code);
		if ((res_code != HTTP_OK) && (res_code != 201))
		{
			break;
		}

		/* update local node version if only sucess to handle the request */
		//cmd_type = evhttp_request_get_command(req);
		//if (cmd_type == EVHTTP_REQ_POST)
		//{
		//	local_version = version_create;
		//}
		//else
		{
			local_version = version_update;
		}
		dps_cluster_node_heartbeat(&dps_local_ip,
		                           dps_cluster_is_local_node_active(),
		                           local_version);
		log_notice(RESTHandlerLogLevel,
		           "Updated local node DMC config version to %d",
		           local_version);
		ret = DOVE_STATUS_OK;
	} while(0);

	return ret;
}

static int http_request_dispatch(struct evhttp_request *req, const char *uri)
{
	int argc = 0;
	char *argv[MAX_ARG_NUMBER] = {0};
	helper_cb_t *cb;
	char *str = NULL;
	bool local_process = false;
	int ret = -1;

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		if (NULL == req || NULL == uri)
		{
			ret = -1;
			break;
		}
		str = strdup(uri);
		if (NULL == str)
		{
			ret = -1;
			break;
		}
		//Touch the remote node, so that the cluster can mark the node
		//as up.
		dps_rest_node_touch(req);
		for (cb = helper_evhttp_get_cblist(); cb; cb = cb->next)
		{
			if (!helper_evhttp_match_uri(cb->token_chain, str, &argc, argv))
			{
				ret = dps_rest_forward_handler(req,
				                               cb->forward_flag,
				                               &local_process);
				if (ret)
				{
					ret = 0;
					break;
				}
				if (local_process)
				{
					cb->call_back(req, cb->arg, argc, argv);
					http_request_version_update(req);
					ret = 0;
					break;
				}
			}
			strcpy(str, uri);
		}
	}while(0);
	if (NULL != str)
	{
		free(str);
	}
	log_debug(RESTHandlerLogLevel, "Exit: %d", ret);
	return ret;
}


static struct evhttp *resthttpd_server = NULL;
static void http_request_handler(struct evhttp_request *req, void *arg)
{
    int ret = -1;
    const char *uri;

    uri = evhttp_request_get_uri(req);
    if (NULL != uri)
    {
        ret = http_request_dispatch(req, uri);
    }
    if(ret)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
    }
    return;
}


/* The original version has the 
 * problem associated with bind.
 * Need an IP address to bind
 * else
 * throw error
 * getaddrinfo family nodename  not supported
 * */


int event_bind_socket(int port) 
{
	int ret;
	int nfd;
	int one_val = 1;
	struct sockaddr_in addr;
	int flags;

	nfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nfd < 0)
		return -1;

	ret = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *) &one_val,
	                 sizeof(int));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	/*inet_aton(appsyshttpd_addr, &addr.sin_addr);*/
	addr.sin_port = htons(port);

	ret = bind(nfd, (struct sockaddr*) &addr, sizeof(addr));
	if (ret < 0)
		return -1;
	ret = listen(nfd, 256);
	if (ret < 0)
	{
		return -1;
	}

	if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
	        || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		return -1;
	}
	return nfd;
}

/*
 ******************************************************************************
 * http_server_main                                                       *//**
 *
 * \brief - Initializes the HTTP Server Thread
 *
 * \param[in] pDummy - The Port on which the REST Services should run on
 *
 * \return None
 *
 ******************************************************************************
 */

static void http_server_main (char *pDummy)
{
	short resthttp_port = (short)((size_t)pDummy);

	//const char *resthttpd_addr = DPS_REST_LISTEN_DEFAULT_IP;
	struct event_base *base;
	//struct evhttp_bound_socket *handle;
	
	int nfd=-1;
	int afd=-1;

	Py_Initialize();
	base = event_base_new();
	if (!base) 
	{
		show_print("ERROR! Couldn't create an event_base: exiting\n");
		return;
	}
	resthttpd_server = evhttp_new(base);
	if (!resthttpd_server)
	{
		show_print("ERROR! couldn't create evhttp. Exiting.\n");
		return;
	}

	/* The original version */
	/*
	handle = evhttp_bind_socket_with_handle(resthttpd_server, resthttpd_addr, resthttp_port);
	if (!handle) 
	{
		show_print("ERROR! Couldn't bind to port %d. Exiting.\n",
		           (int)resthttp_port);
		return;
	}
	*/

	nfd = event_bind_socket(resthttp_port);
	if (nfd < 0)
	{
		show_print("evhttp_bind_socket failed\n");
		return;
	}

	afd = evhttp_accept_socket(resthttpd_server, nfd);
	if (afd != 0)
	{
		printf("evhttp_accept_socket failed\n");
		return;
	}
	//Initialize Global REST port variable
	dps_rest_port = resthttp_port;

	helper_evhttp_set_cb_pattern(DPS_DOMAINS_URI, DPS_REST_FWD_FLAG_POST_TO_AVAIL,
	                             dps_req_handler_domains, NULL);
	helper_evhttp_set_cb_pattern(DPS_DOMAIN_URI, DPS_REST_FWD_FLAG_PUT_TO_AVAIL|DPS_REST_FWD_FLAG_DELETE_TO_ALL,
	                             dps_req_handler_domain, NULL);
	helper_evhttp_set_cb_pattern(DPS_DVGS_URI, DPS_REST_FWD_FLAG_POST_TO_ALL,
	                             dps_req_handler_dvgs, NULL);
	helper_evhttp_set_cb_pattern(DPS_DVG_URI, DPS_REST_FWD_FLAG_PUT_TO_ALL|DPS_REST_FWD_FLAG_DELETE_TO_ALL,
	                             dps_req_handler_dvg, NULL);
	helper_evhttp_set_cb_pattern(DPS_POLICIES_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_policies, NULL);
	helper_evhttp_set_cb_pattern(DPS_POLICY_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_policy, NULL);
	helper_evhttp_set_cb_pattern(DPS_EXTERNAL_GATEWAYS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_gateways, NULL);
	helper_evhttp_set_cb_pattern(DPS_EXTERNAL_GATEWAY_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_gateway, NULL);
	helper_evhttp_set_cb_pattern(DPS_STATISTICS_LOAD_BALANCING_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_statistics_load_balancing, NULL);
	helper_evhttp_set_cb_pattern(DPS_STATISTICS_GENERAL_STATISTICS_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_statistics_general_statistics, NULL);
	helper_evhttp_set_cb_pattern(DPS_DOMAIN_IPV4SUBNETS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_ipsubnets, NULL);
	helper_evhttp_set_cb_pattern(DPS_DOMAIN_IPV4SUBNET_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_ipsubnet, NULL);
	helper_evhttp_set_cb_pattern(DPS_DVG_IPV4SUBNETS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_ipsubnets, NULL);
	helper_evhttp_set_cb_pattern(DPS_DVG_IPV4SUBNET_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_ipsubnet, NULL);
	helper_evhttp_set_cb_pattern(DPS_SERVICE_ROLE_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_service_role, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_LOCAL_DOMAINS_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_local_domain_mapping, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_STATISTICS_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_node_statistics, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_HEARTBEAT_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_node_heartbeat, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_HEARTBEAT_REQUEST_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_node_heartbeat_request, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_NODE_STATUS_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_node_status, NULL);
	helper_evhttp_set_cb_pattern(DPS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_query_cluster_nodes, NULL);
	helper_evhttp_set_cb_pattern(DPS_DOMAIN_TO_NODE_MAPPING_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_dcslist, NULL);
	helper_evhttp_set_cb_pattern(DPS_CLUSTER_TRANSFER_DOMAIN_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_transfer_domain, NULL);
	helper_evhttp_set_cb_pattern(DPS_CONTROLLER_LOCATION_UPDATE_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_set_dmc_location, NULL);
	helper_evhttp_set_cb_pattern(DPS_NODE_GET_READY, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_get_ready, NULL);
	helper_evhttp_set_cb_pattern(DPS_NODE_DOMAIN_ACTIVATE, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_activate, NULL);
	helper_evhttp_set_cb_pattern(DPS_NODE_DOMAIN_DEACTIVATE, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_deactivate, NULL);
	helper_evhttp_set_cb_pattern(DPS_NODE_DOMAIN_RECOVER, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_recover, NULL);
	helper_evhttp_set_cb_pattern(DPS_NODE_DOMAIN_VNID_LIST, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_vnid_listing, NULL);
	helper_evhttp_set_cb_pattern(DOVE_CLUSTER_BULK_POLICY_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_bulk_policy, NULL);
	helper_evhttp_set_cb_pattern(DOVE_CLUSTER_BULK_SUBNET4_URI, DPS_REST_FWD_FLAG_DENY,
	                             dps_req_handler_domain_bulk_ip4subnets, NULL);

	/*  DPS DEBUG for DMC */
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_ENDPOINTS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_endpoints, NULL);
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_TUNNEL_ENDPOINTS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_tunnel_endpoints, NULL);
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_DOMAIN_MAPPING, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_get_domain_mapping, NULL);
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_ALLOW_POLICIES, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_get_allow_policies, NULL);
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_SUBNETS, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_get_subnets, NULL);
	/*TODO: The remaining 2 items to do */
	/*
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_DPS_CLIENTS_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_dps_clients, NULL);
	helper_evhttp_set_cb_pattern(DPS_DEBUG_VNID_MULTICAST_URI, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_vnid_multicast, NULL);
	*/
	helper_evhttp_set_cb_pattern(DPS_DEBUG_CLUSTER_DISPLAY, DPS_REST_FWD_FLAG_GENERIC,
	                             dps_req_handler_cluster_display, NULL);

#ifdef _DPS_REST_DEBUG
	evhttp_set_cb(resthttpd_server, "/test1", http_test, (void *)1);
	evhttp_set_cb(resthttpd_server, "/test2", http_test, (void *)2);
	evhttp_set_cb(resthttpd_server, "/test4", http_test, (void *)4);
#endif
	evhttp_set_gencb(resthttpd_server, http_request_handler, NULL);
	evhttp_set_timeout(resthttpd_server, 20);
	event_base_dispatch(base);
	Py_Finalize();
}



/*
 ******************************************************************************
 * dps_server_rest_init                                                    *//**
 *
 * \brief - Initializes the DPS Server REST infrastructure
 *
 * \param[in] rest_port - The Port on which the REST Services should run on
 *
 * \retval DOVE_STATUS_RESTC_INIT_FAILED Cannot initialize REST Client
 * \retval DOVE_STATUS_THREAD_FAILED Cannot start HTTP Server
 * \retval DOVE_STATUS_OK Success
 *
 ******************************************************************************
 */
dove_status dps_server_rest_init(short rest_port)
{
	int ret = 0;
	pthread_t rest_thread;
	ret = dove_rest_client_init();
	if (ret)
	{
		return DOVE_STATUS_RESTC_INIT_FAILED;
	}

    // Add pthread_replacement wrapper. Need to change this func call once the pthread wrapper is implemented 
	pthread_create(&rest_thread, NULL, (void *)http_server_main, (void *)&rest_port);
	return DOVE_STATUS_OK;
}

