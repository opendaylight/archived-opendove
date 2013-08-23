/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Message forwarding
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
*  $Log: rest_forward.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include <stdbool.h>
#include "include.h"
#include "../inc/evhttp_helper.h"
#include "../inc/rest_forward.h"

/*
 ******************************************************************************
 * dps_rest_forward_get_domainid --                                       *//**
 *
 * \brief This routine get domain ID from uri. If uri only includes VNID, need
 *        to ask cluster to get domain_vnid_mapping.
 *
 * \param [in]  uri		The URI of the HTTP request.
 * \param [out] id		A pointer to domain ID
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_forward_get_domainid(const char *uri, int *id)
{
	int domain_id = 0, vn_id = 0;

	if (helper_evhttp_get_id_from_uri(uri, &domain_id, &vn_id))
	{
		return DOVE_STATUS_INVALID_PARAMETER;
	}

	if (domain_id)
	{
		*id = domain_id;
		return DOVE_STATUS_OK;
	}
	else if(vn_id)
	{
		if (dps_cluster_get_domainid_from_vnid(vn_id, &domain_id) != DOVE_STATUS_OK)
		{
			return DOVE_STATUS_INVALID_DVG;
		}
	}
	*id = domain_id;

	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_rest_forward_source_check --                                       *//**
 *
 * \brief This routine check if a request is from DMC.
 *
 * \param [in] req  A pointer to a evhttp_request data structure.
 *
 * \retval true
 * \retval false
 *
 *****************************************************************************/
static bool dps_rest_forward_source_check(struct evhttp_request *req)
{
	struct evhttp_connection *evcon = NULL;
	char *remote_host = NULL;
	ev_uint16_t remote_port;
	bool ret = true;

	log_debug(RESTHandlerLogLevel,"Enter");
	do
	{
		evcon = evhttp_request_get_connection(req);
		if (evcon == NULL)
		{
			ret = false;
			break;
		}
		evhttp_connection_get_peer(evcon, &remote_host, &remote_port);
		if (remote_host == NULL)
		{
			ret = false;
			break;
		}
#if 1
		//TODO: Re-enable this logic when controller sends the VRRP IP of DMC (HA)
		if (strcmp(remote_host, controller_location_ip_string) &&
		    strcmp(remote_host, dps_local_ip_string))
		{
			log_info(RESTHandlerLogLevel,
			          "[%s] receive from non-DMC or non-local node [%s], forwarding logic not invoked",
			          evhttp_request_get_uri(req), remote_host);
			ret = false;
			break;
		}
#else
		enum evhttp_cmd_type cmd_type;
		//TODO: Disable this logic when controller sends the VRRP IP of DMC (HA)
		if (dps_cluster_node_validate_remote(remote_host))
		{
			log_info(RESTHandlerLogLevel,
			          "[%s] receive from DCS node [%s], forwarding logic not invoked",
			          evhttp_request_get_uri(req), remote_host);
			ret = false;
			break;
		}
		cmd_type = evhttp_request_get_command(req);
		if (cmd_type == EVHTTP_REQ_GET)
		{
			log_info(RESTHandlerLogLevel,
			          "GET request forwarding logic not invoked");
			ret = false;
			break;
		}
#endif
		log_info(RESTHandlerLogLevel,
		         "[%s] receive from DMC or local or non-DCS node[%s], forward it",
		         evhttp_request_get_uri(req), remote_host);
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit %d", ret);
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_foward_response_handler --                                    *//**
 *
 * \brief This routine handles response and decide whether the response is relayed
 *        to DMC. Once any condition of the below matches, relay happens:
 *        . it is a bad response or
 *        . it is a good response and it also is last one and local process is false
 *        If any internal error happens, reply HTTP_INTERNAL to DMC.
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  arg		A pointer to a dps_rest_forward_response_args_t
 *                              data structure, including original_req, last_one,
 *                              local_process.
 * \param [out] arg		A pointer to a dps_rest_forward_response_args_t
 *                              data structure. if relay happens, set relay_happened
 *                              to true.
 *
 * \retval None
 *
 *****************************************************************************/
static void dps_rest_foward_response_handler(struct evhttp_request *req, void *arg)
{
	dps_rest_forward_response_args_t *args = (dps_rest_forward_response_args_t *)arg;
	struct evbuffer *req_body = NULL, *new_req_body = NULL;
	int buf_len;
	int res_code;
	int n;

	do
	{
		/* valid check */
		if (req == NULL)
		{
			args->relay_happened = true;
			evhttp_send_reply(args->original_req, HTTP_INTERNAL, NULL, NULL);
			break;
		}

		args->res_code = res_code = evhttp_request_get_response_code(req);

		/* the response is relayed to DMC under one of the following conditions:	   
		   . it is a bad response
		   . it is a good response and it also is last one and local process is false
		 */
		if (((res_code != HTTP_OK) && (res_code != 201)) ||
		    (args->last_one && !args->local_process))
		{
			args->relay_happened = true;

			/* extract request input body */
			req_body = evhttp_request_get_input_buffer(req);
			if (req_body == NULL)
			{
				evhttp_send_reply(args->original_req, HTTP_INTERNAL, NULL, NULL);
				break;
			}

			buf_len = evbuffer_get_length(req_body)+1;
			char req_body_buf[buf_len];
			n = evbuffer_copyout(req_body, req_body_buf, buf_len);
			req_body_buf[n]='\0';

			/* fill buffer */
			new_req_body = evbuffer_new();
			if (new_req_body == NULL)
			{
				evhttp_send_reply(args->original_req, HTTP_INTERNAL, NULL, NULL);
				break;
			}
			evbuffer_add(new_req_body, req_body_buf, n);
			evhttp_send_reply(args->original_req, res_code, NULL, new_req_body);
		}
	} while(0);

	if (new_req_body)
	{
		evbuffer_free(new_req_body);
	}
	return;
}

/*
 ******************************************************************************
 * dps_rest_forward_sequence_process --                                   *//**
 *
 * \brief This routine iterates forwarding a REST request to all needed nodes
 *        via low leverl sync sending API. For any node, if leader fails to send
 *        or not receive successful response (HTTP_OK or 201), break iteration
 *        and return error.
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  nodes		A pointer to a array which stores nodes.
 * \param [in]  nodes_count	A array count.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_forward_sequence_process(struct evhttp_request *req, 
                                             ip_addr_t *nodes, uint32_t nodes_count,
                                             bool *local_process)
{
	struct evhttp_request *new_request = NULL;
	struct evbuffer *req_body = NULL, *new_req_body = NULL;
	char req_body_buf[1024];
	enum evhttp_cmd_type cmd_type;
	char ip_addr_str[INET6_ADDRSTRLEN];
	const char *uri;
	dps_rest_forward_response_args_t args;
	int n;
	uint32_t i;
	int ret = DOVE_STATUS_OK;

	log_info(RESTHandlerLogLevel, "Enter");
	do
	{
		cmd_type = evhttp_request_get_command(req);
		uri = evhttp_request_get_uri(req);

		/* step 1 - extract request body */
		req_body = evhttp_request_get_input_buffer(req);
		if (!req_body || evbuffer_get_length(req_body) > 1024)
		{
			ret = DOVE_STATUS_EXCEEDS_CAP;
			break;
		}
		n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
		req_body_buf[n]='\0';

		/* step 2 - check if leader self need to handle the request.
		   Note: The flag also is used in dps_rest_foward_response_handler(),
		         so need to get the flag in advance.
		 */
		for(i = 0; i < nodes_count; i++)
		{
			if (nodes[i].ip4 == dps_local_ip.ip4)
			{
				log_info(RESTHandlerLogLevel, "[Local Node] handling the domain");
				*local_process = true;
				break;
			}
		}

		/* step 3 - send request to all nodes which handle the domain */
		for(i = 0; i < nodes_count; i++)
		{
			/* check if leader is one of nodes which handle the domain. If yes, skip it */
			if (nodes[i].ip4 == dps_local_ip.ip4)
			{
				continue;
			}

			/* construct a new request */
			args.original_req = req;
			args.last_one = ((i+1) == nodes_count)? true : false;
			args.local_process = *local_process;
			args.relay_happened = false;
			new_request = evhttp_request_new(dps_rest_foward_response_handler, &args);
			new_req_body = evbuffer_new();
			if ((new_request == NULL) || (new_req_body == NULL))
			{
				ret = DOVE_STATUS_NO_MEMORY;
				break;
			}
			evbuffer_add(new_req_body, req_body_buf, n);
			evbuffer_add_buffer(evhttp_request_get_output_buffer(new_request), new_req_body);    

			/* forward the new request */
			inet_ntop(nodes[i].family, nodes[i].ip6, ip_addr_str, INET6_ADDRSTRLEN);
			log_info(PythonDataHandlerLogLevel, "Forwarding REQ to [%s]",ip_addr_str);
			ret = dove_rest_request_and_syncprocess(ip_addr_str, dps_rest_port,
			                                        cmd_type, uri,
			                                        new_request, NULL,
			                                        DPS_REST_FWD_TIMEOUT);
			evbuffer_free(new_req_body);

			/* One of the below two conditions will cause loop iteration break
			   C1 - internal error happened in dove_rest_request_and_syncprocess()
			   C2 - response relay happened in dps_rest_foward_response_handler()
			*/
			if (ret)
			{
				ret = DOVE_STATUS_ERROR;
				break;
			}
			if (args.relay_happened)
			{
				ret = DOVE_STATUS_INTERRUPT;
				break;
			}
			if ((args.relay_happened) &&
			    ((args.res_code != HTTP_OK) && (args.res_code != 201)) &&
			    (i != 0))
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! %dth response is different with all before", i+1);
			}
		}
		if (ret == DOVE_STATUS_NO_MEMORY)
		{
			if (new_request)
			{
				evhttp_request_free(new_request);
			}
			if (new_req_body)
			{
				evbuffer_free(new_req_body);
			}
		}
	} while (0);

	log_info(RESTHandlerLogLevel, "Exit, ret %d", ret);
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_forward_special_handler_for_deny --                           *//**
 *
 * \brief This routine prohibits cluster leader to forward a special REST request
 *        to any other nodes in cluster. The special REST requst only is processed
 *        by leader self. For example:
 *        DMC=>Leader:
 *          Service Role             - MUST not forward out
 *          Query Cluster Node
 *          Query Domain Node Mapping
 *        Member Node=>Leader:
 *          Node Hearbeat            - MUST not forward out
 *          Node Statistics
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  domain_id	A domain ID which the REST request is applied to.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 *
 *****************************************************************************/
static int dps_rest_forward_special_handler_for_deny(struct evhttp_request *req, int domain_id, bool *local_process)
{
	/* do nothing except setting local_process */
	*local_process = true;
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_rest_forward_special_handler_for_post_to_avail --                  *//**
 *
 * \brief This routine lets cluster leader to forward a special REST request
 *        to available nodes in cluster. So far the kind of special REST requests
 *        only includes Domain Add.
 *        DMC=>Leader:
 *          Domain Add               - forward to available nodes
 *
 * \param [in]  req             A pointer to a evhttp_request data structure.
 * \param [in]  domain_id       A domain ID which the REST request is applied to.
 * \param [out] local_process   A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \note domain_id 0 means domain id is unknown
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_forward_special_handler_for_to_avail(struct evhttp_request *req,
                                                         int domain_id,
                                                         bool *local_process)
{
	ip_addr_t nodes[MAX_NODES_PER_DOMAIN];
	uint32_t nodes_count = 0;
	uint32_t i;
	int status = DOVE_STATUS_OK;
	dove_status domain_add_status;
	char str[INET6_ADDRSTRLEN];

	do
	{
		//Domain doesn't exist: Create the domain
		status = dps_get_lowest_loaded_nodes(MIN_REPLICATION_FACTOR, nodes, &nodes_count);
		if ((status != DOVE_STATUS_OK)||(nodes_count < MIN_REPLICATION_FACTOR))
		{
			if (nodes_count == 0)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! No Available nodes for Domain Creation");
				break;
			}
			log_warn(RESTHandlerLogLevel,
			         "WARNING!!! Available nodes %d is lower than requested %d",
			         nodes_count, MIN_REPLICATION_FACTOR);
		}
		// Add domain to the nodes
		for (i = 0; i < nodes_count; i++)
		{
			domain_add_status = dps_cluster_node_add_domain(&nodes[i],
			                                                domain_id,
			                                                MIN_REPLICATION_FACTOR);
			if (domain_add_status != DOVE_STATUS_OK)
			{
				inet_ntop(nodes[i].family, nodes[i].ip6, str, INET6_ADDRSTRLEN);
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! Cannot add domain %d to node %s",
				          domain_id, str);
			}
		}
		/* send request to all available nodes which will handle the domain */
		status = dps_rest_forward_sequence_process(req, nodes,
		                                           nodes_count,
		                                           local_process);
	} while (0);

	return status;
}

/*
 ******************************************************************************
 * dps_rest_forward_special_handler_for_post_to_all --                    *//**
 *
 * \brief This routine lets cluster leader to forward a special REST request
 *        to all nodes in cluster. So far the kind of special REST requsts
 *        includes:.
 *        DMC=>Leader:
 *          Domain Delete            - forward to all nodes
 *          DVG Add                  - forward to all nodes
 *          DVG Delete               - forward to all nodes
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  domain_id	A domain ID which the REST request is applied to.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_forward_special_handler_for_to_all(struct evhttp_request *req, int domain_id, bool *local_process)
{
	ip_addr_t nodes[MAX_NODES_ON_STACK];
	uint32_t nodes_count = 0;
	int ret = DOVE_STATUS_OK;
	ip_addr_t *ipnodes;
	void *nodes_memory = NULL;
	int nodes_memory_allocated = 0;

	log_info(RESTHandlerLogLevel, "Enter");
	do
	{
		/* get all nodes in cluster */
		ret = dps_get_all_cluster_nodes(MAX_NODES_ON_STACK, nodes, &nodes_count);
		if (ret != DOVE_STATUS_OK && ret != DOVE_STATUS_RETRY) {
			log_alert(RESTHandlerLogLevel,
			          "ALERT!!! dps_get_all_cluster_nodes returned [%d]",
			          ret);
			break;
		}
		if (ret == DOVE_STATUS_RETRY)
		{
			// no of nodes in cluster > MAX_NODES_ON_STACK
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

		/* send request to all nodes in cluster */
		ret = dps_rest_forward_sequence_process(req, ipnodes, nodes_count, local_process);
	} while (0);

	if (nodes_memory_allocated)
	{
		free(nodes_memory);
		nodes_memory = NULL;
		ipnodes = NULL;
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_forward_generic_handler --                                    *//**
 *
 * \brief This routine lets cluster leader to forward a generic REST request to  
 *        appropriate nodes. Generic forwarding rule is:
 *        1 - Forward the POST/DELETE request to all nodes that handle the domain
 *        2 - Forward the GET request to any node that handle the domain
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  domain_id	A domain ID which the REST request is applied to.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_forward_generic_handler(struct evhttp_request *req, int domain_id, bool *local_process)
{
	enum evhttp_cmd_type cmd_type;
	ip_addr_t nodes[MAX_NODES_PER_DOMAIN];
	uint32_t nodes_count = 0;
	int ret = DOVE_STATUS_OK;

	do
	{
		log_debug(RESTHandlerLogLevel, "Enter");
		cmd_type = evhttp_request_get_command(req);

		/* domain validate */
		if (domain_id == 0)
		{
			ret = DOVE_STATUS_INVALID_DOMAIN;
			break;
		}

		/* get domain_node_mapping */
		ret = dps_get_domain_node_mapping(domain_id,
		                                  MAX_NODES_PER_DOMAIN,
		                                  nodes, &nodes_count);
		if ((ret != DOVE_STATUS_OK) || (nodes_count <=0))
		{
			log_alert(RESTHandlerLogLevel,
			          "ALERT!!! No of Nodes in the domain <= 0 or exceeds %d",
			          domain_id, MAX_NODES_PER_DOMAIN);
			break;
		}

		/* Generic forwarding rule:
		 *  1 - Forward the POST/DELETE request to all nodes that handle the domain
		 *  2 - Forward the GET request to any node that handle the domain
		 */
		if (cmd_type == EVHTTP_REQ_GET)
		{
			nodes_count = 1;
		}
		ret = dps_rest_forward_sequence_process(req, nodes, nodes_count, local_process);
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit -- %d",ret);
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_forward_flag_check --                                         *//**
 *
 * \brief This routine lets cluster leader to check if a REST request is a
 *        generic or special request via flag and return a appropriate callback
 *        function for each request.
 *
 * \param [in]  req        A pointer to a evhttp_request data structure.
 * \param [in]  domain_id  A domain ID which the REST request is applied to.
 * \param [in]  flag       The forwarding flag, for example,
 *                         DPS_REST_FWD_FLAG_POST_TO_ALL.
 * \param [out] cb         A pointer to callback function which handle the special
 *                         REST request.
 *
 * \retval none
 *
 *****************************************************************************/
static void dps_rest_forward_flag_check(struct evhttp_request *req, int domain_id, int flag, sp_cb_t *cb)
{
	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		*cb = dps_rest_forward_generic_handler;

		if (flag & DPS_REST_FWD_FLAG_DENY)
		{
			*cb = dps_rest_forward_special_handler_for_deny;
			break;
		}

		switch (evhttp_request_get_command(req))
		{
		case EVHTTP_REQ_PUT:
			if (flag & DPS_REST_FWD_FLAG_PUT_TO_AVAIL)
			{
				if (!dps_cluster_domain_exists(domain_id))
				{
					*cb = dps_rest_forward_special_handler_for_to_avail;
				}
			}
			else if (flag & DPS_REST_FWD_FLAG_PUT_TO_ALL)
			{
				*cb = dps_rest_forward_special_handler_for_to_all;
			}
			break;

		case EVHTTP_REQ_POST:
			if (flag & DPS_REST_FWD_FLAG_POST_TO_AVAIL)
			{
				*cb = dps_rest_forward_special_handler_for_to_avail;
			}
			else if (flag & DPS_REST_FWD_FLAG_POST_TO_ALL)
			{
				*cb = dps_rest_forward_special_handler_for_to_all;
			}
			break;
		case EVHTTP_REQ_DELETE:
			if (flag & DPS_REST_FWD_FLAG_DELETE_TO_ALL)
			{
				*cb = dps_rest_forward_special_handler_for_to_all;
			}
			break;
		case EVHTTP_REQ_GET:
		case EVHTTP_REQ_HEAD:
		case EVHTTP_REQ_OPTIONS:
		case EVHTTP_REQ_TRACE:
		case EVHTTP_REQ_CONNECT:
		case EVHTTP_REQ_PATCH:
		default:
			break;
		}
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_rest_forward_handler --                                                    *//**
 *
 * \brief This routine lets cluster leader to forward a REST request to  
 *        appropriate nodes according to flag. Forwarding rule is:
 *        1 - Forward the POST/DELETE request to all nodes that handle the domain
 *        2 - Forward the GET request to any node that handle the domain
 *        Here there are some special cases needed to be addressed
 *        DMC=>Leader:
 *          Domain Add               - forward to available nodes
 *          Domain Delete            - forward to all nodes
 *          DVG Add                  - forward to all nodes
 *          DVG Delete               - forward to all nodes
 *          Service Role             - MUST not forward out
 *          Query Cluster Node
 *          Query Domain Node Mapping
 *        Member Node=>Leader:
 *          Node Hearbeat            - MUST not forward out
 *          Node Statistics
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  flag		The forwarding flag, for example, 
 *                              DPS_REST_FWD_FLAG_POST_TO_ALL.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dps_rest_forward_handler(struct evhttp_request *req, int flag, bool *local_process)
{
	sp_cb_t cb = NULL;
	const char *uri = evhttp_request_get_uri(req);
	int domain_id = 0;
	int ret = DOVE_STATUS_OK;

	log_info(RESTHandlerLogLevel, "Enter");

	do
	{
		/* check if request is sent by DMC. If not, do nothing */
		if (dps_rest_forward_source_check(req) == false)
		{
			log_info(RESTHandlerLogLevel,
			         "dps_rest_forward_source_check returns false, local_proces set to true");
			*local_process = true;
			break;
		}
		
		/* get domain ID */
		ret = dps_rest_forward_get_domainid(uri, &domain_id);
		if (ret != DOVE_STATUS_OK)
		{
			log_info(RESTHandlerLogLevel,
			         "dps_rest_forward_get_domainid cannot get domain id");
			break;
		}

		log_info(RESTHandlerLogLevel, "Domain ID %d", domain_id);
		dps_rest_forward_flag_check(req, domain_id, flag, &cb);
		ret = cb(req, domain_id, local_process);
	} while (0); 

	if (ret && (ret != DOVE_STATUS_INTERRUPT))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
	}
	log_info(RESTHandlerLogLevel, "Exit: ret %d, *local_process %d", ret, *local_process);
	return ret;
}
