/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Message Processing
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
*  $Log: rest_req_handler.c $
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
#include "../inc/rest_req_handler.h"
#include "../inc/rest_api.h"

#if 0
static dove_status dps_generic_domain_property_rest_handler(unsigned int domain_id,
                                                            struct evhttp_request *req,
                                                            int *fLocal, char *json_buf)
{
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	int status = DOVE_STATUS_ERROR;
	uint32_t nodes_count = 0, nodes_needed = 0;
	ip_addr_t nodes[MAX_NODES_PER_DOMAIN];
	int n;
	char ipstr[INET6_ADDRSTRLEN];

	do {
		if (dps_cluster_is_local_node_leader()) {
			nodes_needed = MAX_NODES_PER_DOMAIN;
			status = dps_get_domain_node_mapping(domain_id,nodes_needed,
			                                     nodes, &nodes_count);
			if (status != DOVE_STATUS_OK) {
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! No of Nodes in domain [%d] exceed %d",
				          domain_id, MAX_NODES_PER_DOMAIN);
				break;
			}

			struct evhttp_request *fwd_request;
			const char * uri = evhttp_request_get_uri(req);
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > 1024)
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';

			for(int i = 0; i < (int)nodes_count; i++)
			{
				if (nodes[i].ip4 == dcs_local_ip.ip4) {
					// domain handled locally
					*fLocal = 1;
					log_debug(RESTHandlerLogLevel,
					          "[LEADER] handling domain [%d]",
					          domain_id);
					continue;
				}
				else {
					fwd_request = dps_rest_client_dove_controller_request_new();
					if(fwd_request == NULL) {
						log_alert(RESTHandlerLogLevel,
							  "Can not alloc the evhttp request");
						break;
					}
					struct evbuffer * fwd_req_body = evbuffer_new();

					evbuffer_add(fwd_req_body,req_body_buf,n);
					evbuffer_add_buffer(evhttp_request_get_output_buffer(fwd_request),
							    fwd_req_body);

					inet_ntop(nodes[i].family, nodes[i].ip6,
						  ipstr, INET6_ADDRSTRLEN);
					log_debug(RESTHandlerLogLevel,
						  "Forwarding REQ to [%s]",ipstr);
					dps_rest_client_dove_controller_send_asyncprocess(
						ipstr,
						(char *)uri,
						dps_rest_port,EVHTTP_REQ_POST,
						fwd_request
						);
					if (fwd_req_body)
					{
						evbuffer_free(fwd_req_body);
					}
				}
			}

			strcpy(json_buf,req_body_buf);
			status = DOVE_STATUS_OK;
		}
	} while (0);

	return (dove_status)status;
}
#endif

/* GET /api/dps/domains
   POST /api/dps/domains */
void dps_req_handler_domains(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	char res_uri[DPS_URI_LEN]; 
	int res_code = HTTP_BADREQUEST;
	int n;
	unsigned int domain_id, replication_factor;
	json_error_t jerror;
	dove_status status = DOVE_STATUS_OK;
	unsigned int rep_factor = 0;
	unsigned int curr_rep_factor = 0;

	log_info(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			js_root = dps_rest_api_get_domain_all();
			if (js_root)
			{
				res_body_str = json_dumps(js_root, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_POST: 
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_info(RESTHandlerLogLevel,"js_root is NULL");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "id");
			if (!json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			domain_id = (unsigned int)json_integer_value(js_id);
			js_id = json_object_get(js_root, "replication_factor");
			if (!json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			replication_factor = (unsigned int)json_integer_value(js_id);
			if (DOVE_STATUS_OK != dps_rest_api_create_domain(domain_id, replication_factor))
			{
				log_error(RESTHandlerLogLevel,
				          "Failed to create domain...[%d]",
				          domain_id);
				break;
			}
			js_res = dps_rest_api_get_domain(domain_id);
			if (NULL == js_res)
			{
				log_warn(RESTHandlerLogLevel,"js_res is NULL");
				break;
			}
			res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
			if (NULL == res_body_str)
			{
				log_warn(RESTHandlerLogLevel,"res_body_str is NULL");
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				log_warn(RESTHandlerLogLevel,"retbuf is NULL");
				break;
			}
			DPS_DOMAIN_URI_GEN(res_uri, domain_id);
			evhttp_add_header(evhttp_request_get_output_headers(req),
			                  "Location", res_uri);

			evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
			/* CREATED 201 */
			res_code = 201;
			break;
		}
		case EVHTTP_REQ_PUT: {
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_warn(RESTHandlerLogLevel,"js_root is NULL");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "id");
			if (!json_is_integer(js_id))
			{
				log_warn(RESTHandlerLogLevel,"id is NOT integer");
				break;
			}
			domain_id = (unsigned int)json_integer_value(js_id);

			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "replication_factor");
			if (!json_is_integer(js_id))
			{
				log_warn(RESTHandlerLogLevel,
				         "replication_factor is NOT integer");
				break;
			}
			rep_factor = json_integer_value(js_id);
			if(0 == curr_rep_factor) {
				log_debug(PythonClusterDataLogLevel,
				          "Current replication factor = 0. Domain doesn't exist...");
				res_code = HTTP_NOTFOUND;
				break;
			}
			dps_controller_data_op_t data_op;

			data_op.type = DPS_CONTROLLER_DOMAIN_ADD;
			data_op.domain_add.domain_id = domain_id;
			data_op.domain_add.replication_factor = rep_factor;
			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK)
			{
				log_error(PythonClusterDataLogLevel,
				          " **ERROR** in processing DOMAIN_ADD operation -- status == %s",
				          DOVEStatusToString(status));
				break;
			}
			res_code = HTTP_OK;
			break;

		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_root)
	{
		json_decref(js_root);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;

}
/* 
	GET /api/dove/dps/domains/{domain_id}
	DELETE /api/dove/dps/domains/{domain_id}
	PUT /api/dove/dps/domains/{domain_id}
 */
void dps_req_handler_domain(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_rep_factor = NULL;
	json_t *js_res = NULL;
	json_error_t jerror;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	int res_code = HTTP_BADREQUEST;
	int n;
	unsigned long int domain_id;
	char *endptr = NULL;
	unsigned int rep_factor = 0;
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{	
			js_res = dps_rest_api_get_domain(domain_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_PUT: 
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_warn(RESTHandlerLogLevel,"js_root is NULL");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_rep_factor = json_object_get(js_root, "replication_factor");
			if (!json_is_integer(js_rep_factor))
			{
				log_warn(RESTHandlerLogLevel,
				         "replication_factor is NOT integer");
				break;
			}
			rep_factor = json_integer_value(js_rep_factor);
			data_op.type = DPS_CONTROLLER_DOMAIN_ADD;
			data_op.domain_add.domain_id = domain_id;
			data_op.domain_add.replication_factor = rep_factor;
			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK)
			{
				log_error(PythonClusterDataLogLevel,
				          " **ERROR** in processing DOMAIN_ADD operation -- status == %s",
				          DOVEStatusToString(status));
				break;
			}
			res_code = HTTP_OK;
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			if (DOVE_STATUS_OK == dps_rest_api_del_domain(domain_id))
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/* GET /api/dps/domains/{domain_id}/dvgs
   POST /api/dps/domains/{domain_id}/dvgs */
void dps_req_handler_dvgs(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	char res_uri[DPS_URI_LEN]; 
	int res_code = HTTP_BADREQUEST;
	unsigned long int domain_id;
	unsigned int dvg_id;
	int n;
	json_error_t jerror;
	char *endptr = NULL;
	char dvg_uri[DPS_URI_LEN];
	char ipv4subnets_uri[DPS_URI_LEN];
	char ipv4extgateway_uri[DPS_URI_LEN];

	log_debug(RESTHandlerLogLevel, "Enter");

	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		log_debug(RESTHandlerLogLevel, "Exit: %d", HTTP_BADREQUEST);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		log_debug(RESTHandlerLogLevel, "Exit: %d", HTTP_BADREQUEST);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "GET Request");
			js_res = dps_rest_api_get_dvg_all(domain_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_POST: 
		{
			log_debug(RESTHandlerLogLevel, "POST Request");
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_debug(RESTHandlerLogLevel,"js_root is NULL");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "id");
			if (!json_is_integer(js_id))
			{
				log_debug(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			dvg_id = (unsigned int)json_integer_value(js_id);

			if (DOVE_STATUS_OK != dps_rest_api_create_dvg(domain_id,
			                                              dvg_id,
			                                              DPS_CONTROLLER_DVG_ADD))
			{
				log_error(RESTHandlerLogLevel,
				          "Failed to create vnid %d",
				          dvg_id);
				break;
			}
			DPS_DVG_URI_GEN(dvg_uri, domain_id, dvg_id);
			DPS_DVG_IPV4SUBNETS_URI_GEN(ipv4subnets_uri, dvg_id);
			DPS_EXTERNAL_GATEWAYS_URI_GEN(ipv4extgateway_uri, dvg_id);
			js_res = json_pack("{s:i, s:s, s:s, s:s}", 
			                   "id", (int)dvg_id,
			                   "uri", dvg_uri,
			                   "IPv4SubnetsUri", ipv4subnets_uri,
			                   "IPv4ExternalGatewaysUri", ipv4extgateway_uri);
			if (NULL == js_res)
			{
				log_warn(RESTHandlerLogLevel,"js_res is NULL");
				break;
			}
			res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
			if (NULL == res_body_str)
			{
				log_warn(RESTHandlerLogLevel,
				          "res_body_str is NULL");
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				log_warn(RESTHandlerLogLevel,
				          "retbuf is NULL");
				break;
			}
			DPS_DVG_URI_GEN(res_uri, domain_id, dvg_id);
			evhttp_add_header(evhttp_request_get_output_headers(req),
			                  "Location", res_uri);
			evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
			/* CREATED 201 */
			res_code = 201;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_root)
	{
		json_decref(js_root);
	}
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* 
	GET /api/dps/domains/{domain_id}/dvgs/{dvg_id}
	DELETE /api/dps/domains/{domain_id}/dvgs/{dvg_id}
	PUT /api/dps/domains/{domain_id}/dvgs/{dvg_id}
 */
void dps_req_handler_dvg(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	char *res_body_str = NULL;
	unsigned long int domain_id;
	unsigned long int dvg_id;
	char *endptr = NULL;
	char dvg_uri[DPS_URI_LEN];
	char ipv4subnets_uri[DPS_URI_LEN];
	char ipv4extgateway_uri[DPS_URI_LEN];
	char res_uri[DPS_URI_LEN]; 

	log_debug(RESTHandlerLogLevel, "Enter");
	if (argc != 2 || NULL == argv )
	{
		log_debug(RESTHandlerLogLevel, "Exit: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_debug(RESTHandlerLogLevel, "Exit: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	dvg_id = strtoul(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		log_debug(RESTHandlerLogLevel, "Exit: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			js_res = dps_rest_api_get_dvg(domain_id, dvg_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_PUT: 
		{
			if (DOVE_STATUS_OK != dps_rest_api_create_dvg(domain_id,
			                                              dvg_id,
			                                              DPS_CONTROLLER_DVG_ADD))
			{
				log_error(RESTHandlerLogLevel,
				          "Failed to create vnid %d",
				          dvg_id);
				break;
			}
			DPS_DVG_URI_GEN(dvg_uri, domain_id, dvg_id);
			DPS_DVG_IPV4SUBNETS_URI_GEN(ipv4subnets_uri, dvg_id);
			DPS_EXTERNAL_GATEWAYS_URI_GEN(ipv4extgateway_uri, dvg_id);
			js_res = json_pack("{s:i, s:s, s:s, s:s}", 
			                   "id", (int)dvg_id,
			                   "uri", dvg_uri,
			                   "IPv4SubnetsUri", ipv4subnets_uri,
			                   "IPv4ExternalGatewaysUri", ipv4extgateway_uri);
			if (NULL == js_res)
			{
				log_warn(RESTHandlerLogLevel,"js_res is NULL");
				break;
			}
			res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
			if (NULL == res_body_str)
			{
				log_warn(RESTHandlerLogLevel,
				          "res_body_str is NULL");
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				log_warn(RESTHandlerLogLevel,
				          "retbuf is NULL");
				break;
			}
			DPS_DVG_URI_GEN(res_uri, domain_id, dvg_id);
			evhttp_add_header(evhttp_request_get_output_headers(req),
			                  "Location", res_uri);
			evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);

			res_code = HTTP_OK; 
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			if (DOVE_STATUS_OK == dps_rest_api_del_dvg(domain_id, dvg_id))
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* 
	GET /api/dps/domains/{domain_id}/policies
	POST /api/dps/domains/{domain_id}/policies
 */
void dps_req_handler_policies(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	json_t *js_root = NULL;
	json_error_t jerror;
	char res_uri[DPS_URI_LEN]; 
	char *res_body_str = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int domain_id;
	unsigned int src_dvg;
	unsigned int dst_dvg;
	unsigned int traffic_type;
	int n;
	char *endptr = NULL;

	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_domain_validate(domain_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			js_res = dps_rest_api_get_policies_all(domain_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_POST: 
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) >
				(sizeof(req_body_buf) - 1))
			{
				log_error(PythonClusterDataLogLevel,
				          "policy creation -- req_body is NULL");
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf,
					     (sizeof(req_body_buf) - 1));
			req_body_buf[n] = '\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_error(PythonClusterDataLogLevel,
				          "policy creation -- js_root is NULL");
				break;
			}

			if (DOVE_STATUS_OK == dps_rest_api_create_policy_from_jdata(domain_id, js_root, &src_dvg, &dst_dvg, &traffic_type))
			{
				js_res = dps_rest_api_get_policy(domain_id, src_dvg, dst_dvg, traffic_type);
				if (NULL == js_res)
				{
					log_error(PythonClusterDataLogLevel,
					          "policy creation -- js_res is NULL");
					break;
				}
				log_info(PythonClusterDataLogLevel,
				         "policy creation successful for domain [%d]",
				         domain_id);
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					log_error(PythonClusterDataLogLevel,
					          "policy creation -- res_body_str is NULL");
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					log_error(PythonClusterDataLogLevel,
					          "policy creation -- retbuf is NULL");
					break;
				}
				DPS_POLICY_URI_GEN(res_uri, domain_id, src_dvg, dst_dvg, traffic_type);
				evhttp_add_header(evhttp_request_get_output_headers(req),
				                  "Location", res_uri);
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				/* CREATED 201 */
				res_code = 201;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}

	if (js_root)
	{
		json_decref(js_root);
	}

	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/* 
	GET /api/dps/domains/{domain_id}/policies/{policy_id}
	DELETE /api/dps/domains/{domain_id}/policies/{policy_id}
	PUT //api/dps/domains/{domain_id}/policies/{policy_id}
 */
void dps_req_handler_policy(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	json_t *js_root = NULL;
	json_t *js_tok;
	json_error_t jerror;

	char *res_body_str = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int domain_id;
	unsigned long int src_dvg;
	unsigned long int dst_dvg;
	unsigned long int traffic_type;
	char *endptr = NULL, *saveptr = NULL, *p = NULL;
	int n;
	unsigned int type, ttl;
	dps_object_policy_action_t action;
	char res_uri[DPS_URI_LEN]; 

	log_info(RESTHandlerLogLevel, "Enter");

	if (argc != 2 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_domain_validate(domain_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
#if 1	
	/* Decode SRC DVG */
	p = strtok_r(argv[1], "-", &saveptr);
	if(NULL == p)
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	src_dvg = strtoul(p, &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	/* Decode DST DVG */
	p = strtok_r(NULL, "-", &saveptr);
	if(NULL == p)
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	dst_dvg = strtoul(p, &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	/* Decode Traffic Type */
	p = strtok_r(NULL, "-", &saveptr);
	if(NULL == p)
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	traffic_type = strtoul(p, &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
#else
#define SRC_DVG_ID_LEN 16
#define SRC_DVG_ID_MASK ((1 << SRC_DVG_ID_LEN) - 1)
	unsigned long int policy_id;
	policy_id = strtoul(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	dst_dvg = (unsigned int)((policy_id >> SRC_DVG_ID_LEN)&SRC_DVG_ID_MASK);
	src_dvg = (unsigned int)(policy_id & SRC_DVG_ID_MASK);
#endif
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{	
			js_res = dps_rest_api_get_policy(domain_id, src_dvg, dst_dvg, traffic_type);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_PUT: 
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, (sizeof(req_body_buf) - 1));
			req_body_buf[n] = '\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				break;
			}

			js_res = dps_rest_api_get_policy(domain_id, src_dvg, dst_dvg, traffic_type);
			if (js_res)
			{
				/* policy exists, update it */
				if (DOVE_STATUS_OK == dps_rest_api_update_policy(domain_id, src_dvg, dst_dvg, traffic_type, js_root)) 
				{
					res_code = HTTP_OK;
				}
			}
			else
			{
				/* policy not exists, create it */
				js_tok = json_object_get(js_root, "type");
				if (NULL == js_tok || !json_is_integer(js_tok))
				{
					break;
				}
				type = (unsigned int)json_integer_value(js_tok);

				js_tok = json_object_get(js_root, "ttl");
				if (NULL == js_tok || !json_is_integer(js_tok))
				{
					break;
				}
				ttl = (unsigned int)json_integer_value(js_tok);

				if(DPS_POLICY_TYPE_CONN == type)
				{
					js_tok = json_object_get(js_root, "action");
					if (NULL == js_tok || !json_is_integer(js_tok))
					{
						break;
					}
					action.connectivity = (unsigned short)json_integer_value(js_tok);
					action.connectivity = (action.connectivity == DPS_CONNECTIVITY_DROP)?DPS_CONNECTIVITY_DROP:DPS_CONNECTIVITY_ALLOW;
					action.ver = 1;
				}
				if (DOVE_STATUS_OK == dps_rest_api_create_policy(domain_id, src_dvg, dst_dvg, traffic_type, type, ttl, &action))
				{
					js_res = dps_rest_api_get_policy(domain_id, src_dvg, dst_dvg, traffic_type);
					if (NULL == js_res)
					{
						log_error(RESTHandlerLogLevel,
						          "policy creation -- js_res is NULL");
						break;
					}
					log_info(RESTHandlerLogLevel,
					         "policy creation successful for domain [%d]",
					         domain_id);
					res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
					if (NULL == res_body_str)
					{
						log_error(RESTHandlerLogLevel,
						          "policy creation -- res_body_str is NULL");
						break;
					}
					retbuf = evbuffer_new();
					if (NULL == retbuf)
					{
						log_error(RESTHandlerLogLevel,
						          "policy creation -- retbuf is NULL");
						break;
					}
					DPS_POLICY_URI_GEN(res_uri, domain_id, src_dvg, dst_dvg, traffic_type);
					evhttp_add_header(evhttp_request_get_output_headers(req),
					                  "Location", res_uri);
					evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
					/* CREATED 201 */
					res_code = 201;
				}
			}
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			log_info(RESTHandlerLogLevel,
			         "Deleting Policy: src_dvg %d, dst_dvg %d, traffic_type %d",
			         src_dvg, dst_dvg, traffic_type);
			if (DOVE_STATUS_OK == dps_rest_api_del_policy(domain_id, src_dvg, dst_dvg, traffic_type)) 
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}

	if (js_root)
	{
		json_decref(js_root);
	}

	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_req_handler_process_node_stats_array --                            *//**
 *
 * \brief This routine processes a JSON array, where each element of the array
 *        contains the statistics per domain
 *
 * \param node_ip     The IP Address of the Remote Node
 * \param js_stats    Array of JSON objects representing statistics
 *
 * \return None
 *
 *****************************************************************************/

static void dps_req_handler_process_node_stats_array(ip_addr_t *node_ip,
                                                     json_t *js_stats)
{
	json_t *js_id;
	json_t *js_node;
	uint32_t domain_id;
	uint32_t endpoints;
	uint32_t endpoint_update;
	uint32_t endpoint_lookup;
	uint32_t policy_lookup;
	unsigned int i;

	log_debug(RESTHandlerLogLevel, "Enter");

	/* Get every statistics info */
	for (i = 0; i < json_array_size(js_stats); i++)
	{
		js_id = json_array_get(js_stats, i);

		/* domain id */
		js_node = json_object_get(js_id, "id");
		if (json_is_null(js_node) || !json_is_integer(js_node))
		{
			log_info(RESTHandlerLogLevel, "id is Error");
			continue;
		}
		domain_id = json_integer_value(js_node);

		/* endpoints_count */
		js_node = json_object_get(js_id, "endpoints_count");
		if (json_is_null(js_node) || !json_is_integer(js_node))
		{
			log_info(RESTHandlerLogLevel, "endpoints_count is Error");
			continue;
		}
		endpoints = json_integer_value(js_node);

		/* endpoint_update_count */
		js_node = json_object_get(js_id, "endpoint_update_count");
		if (json_is_null(js_node) || !json_is_integer(js_node))
		{
			log_info(RESTHandlerLogLevel, "endpoint_update_count is Error");
			continue;
		}
		endpoint_update = json_integer_value(js_node);

		/* endpoint_lookup_count */
		js_node = json_object_get(js_id, "endpoint_lookup_count");
		if (json_is_null(js_node) || !json_is_integer(js_node))
		{
			log_info(RESTHandlerLogLevel, "endpoint_lookup_count is Error");
			continue;
		}
		endpoint_lookup = json_integer_value(js_node);

		/* policy_lookup_count */
		js_node = json_object_get(js_id, "policy_lookup_count");
		if (json_is_null(js_node) || !json_is_integer(js_node))
		{
			log_info(RESTHandlerLogLevel, "policy_lookup_count is Error");
			continue;
		}
		policy_lookup = json_integer_value(js_node);

		log_debug(RESTHandlerLogLevel, "domain id %d", domain_id);
		log_debug(RESTHandlerLogLevel, "endpoints %d", endpoints);
		log_debug(RESTHandlerLogLevel, "endpoint_update %d", endpoint_update);
		log_debug(RESTHandlerLogLevel, "endpoint_lookup %d", endpoint_lookup);
		log_debug(RESTHandlerLogLevel, "policy_lookup %d", policy_lookup);

		dps_cluster_update_statistics(domain_id, node_ip,
		                              endpoints, 0,
		                              endpoint_update, endpoint_lookup, policy_lookup);

	}

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* Deal with the node report its statistics */
void dps_req_handler_node_statistics(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_node = NULL;
	json_t *js_stats = NULL;
	struct evbuffer *req_body = NULL;
	int n;
	ip_addr_t node_ip;
	json_error_t jerror;
	int res_code = HTTP_BADREQUEST;
	const char *node_ipstr;

	log_debug(RESTHandlerLogLevel, "Enter ");

	memset(&node_ip, 0, sizeof(ip_addr_t));
	node_ip.family = AF_INET;

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "GET Request");
			break;
		}

		//case EVHTTP_REQ_POST:
		case EVHTTP_REQ_PUT:
		{
			//Not used: Compiler complains
			//uint32_t sample_interval = 0;
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body)
			{
				log_error(RESTHandlerLogLevel, "Cannot get req_body");
				break;
			}
			if(evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE - 1))
			{
				log_error(RESTHandlerLogLevel,
				          "Statistics Size %d too big, Quitting!!!",
				          evbuffer_get_length(req_body));
				break;
			}
			n = evbuffer_copyout(req_body, large_REST_buffer, LARGE_REST_BUFFER_SIZE - 1);
			large_REST_buffer[n] = '\0';

			log_debug(RESTHandlerLogLevel,
			          "req_body_buf is: %s",
			          large_REST_buffer);

			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_error(RESTHandlerLogLevel, "Bad js_root");
				break;
			}

			/* Borrowed reference, no need to decref */
			/* Get the report node info */
			js_node = json_object_get(js_root, "dps_node");
			if (!json_is_object(js_node))
			{
				log_error(RESTHandlerLogLevel, "Bad dps_node object");
				break;
			}

			js_id = json_object_get(js_node, "family");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Bad family");
				break;
			}
			node_ip.family = (uint16_t) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel, "family is %u", node_ip.family);

			js_id = json_object_get(js_node, "ip");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel,"ip error");
				break;
			}
			node_ipstr = json_string_value(js_id);
			if (node_ipstr == NULL)
			{
				log_error(RESTHandlerLogLevel, "Cannot get IP String Value");
				break;
			}
			log_debug(RESTHandlerLogLevel, "node ip is %s", json_string_value(js_id));

			if (!inet_pton(node_ip.family, node_ipstr, node_ip.ip6))
			{
				log_error(RESTHandlerLogLevel,
				         "Invalid ip %s error", node_ipstr);
				break;
			}

			js_id = json_object_get(js_node, "UUID");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel,"UUID error");
				break;
			}
			log_debug(RESTHandlerLogLevel, "UUID is %s", json_string_value(js_id));

			js_id = json_object_get(js_node, "Cluster_Leader");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel,"Cluster_Leader error");
				break;
			}
			log_debug(RESTHandlerLogLevel, "Cluster Leader is %s", json_string_value(js_id));

			/*Get the statistics object*/
			js_stats = json_object_get(js_root, "statistics");
			if (!json_is_array(js_stats))
			{
				log_error(RESTHandlerLogLevel, "statistics is not an array");
				break;
			}
			log_debug(RESTHandlerLogLevel,"Now parse statistics info");

			dps_req_handler_process_node_stats_array(&node_ip, js_stats);
			/* CREATED 201 */
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, NULL);
	log_debug(RESTHandlerLogLevel, "evhttp_send_reply: Done");
	if (js_root)
	{
		json_decref(js_root);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* Leader process the received DPS Heartbeat */
void dps_req_handler_node_heartbeat(struct evhttp_request *req,
                                    void *arg, int argc, char **argv)
{

	json_t *js_root = NULL;
	json_t *js_id = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	json_error_t jerror;
	int factive;
	long long config_version;
	ip_addr_t remote_ip;
	int res_code = HTTP_BADREQUEST;
	int n;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_PUT:
		{

			n = dps_rest_remote_node_get(req, &remote_ip);
			if (n != 1)
			{
				log_error(RESTHandlerLogLevel, "Cannot get remote node Location");
				break;
			}
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(remote_ip.family, remote_ip.ip6, str, INET6_ADDRSTRLEN);
				log_debug(RESTHandlerLogLevel, "Heartbeat from Node IP %s", str);
			}
#endif
			log_debug(RESTHandlerLogLevel, "I got a HTTP PUT Request ");
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body ||
			    evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf,
			                     sizeof(req_body_buf) - 1);
			req_body_buf[n] = '\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_error(RESTHandlerLogLevel, "Bad js_root");
				break;
			}

			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "Active");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get Node Active Y/N");
				break;
			}
			factive = (int) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel,
			          "Active is d",
			          factive);

			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "Config_Version");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get Configuration Version");
				break;
			}
			config_version = (long long) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel,
			          "config_version is %ld",
			          config_version);

			js_id = json_object_get(js_root, "UUID");
			if (!json_is_string(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel, "UUID is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "Cluster_Leader");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel, "Bad Cluster_Leader");
				break;
			}
			log_debug(RESTHandlerLogLevel, "Cluster Leader is %s", json_string_value(js_id));
			dps_cluster_node_heartbeat(&remote_ip, factive, config_version);
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, NULL);
	if (js_root)
	{
		json_decref(js_root);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* Leader process the received DPS Heartbeat */
void dps_req_handler_node_heartbeat_request(struct evhttp_request *req,
                                            void *arg, int argc, char **argv)
{

	ip_addr_t remote_ip;
	int res_code = HTTP_BADREQUEST;
	int n;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_PUT:
		{

			n = dps_rest_remote_node_get(req, &remote_ip);
			if (n != 1)
			{
				log_error(RESTHandlerLogLevel, "Cannot get remote node Location");
				break;
			}
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(remote_ip.family, remote_ip.ip6, str, INET6_ADDRSTRLEN);
				log_debug(RESTHandlerLogLevel, "Heartbeat Request from Node IP %s", str);
			}
#endif
			log_debug(RESTHandlerLogLevel, "I got a HTTP PUT Request ");
			dps_cluster_node_heartbeat_request(&remote_ip);
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, NULL);
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_req_handler_process_node_status_array --                           *//**
 *
 * \brief This routine processes a JSON array, where element represents the
 *        status of a DCS Node in the network
 *
 * \param node_ip The IP Address of the Remote Node
 * \param config_version The configuration version of the cluster as seen
 *                       by the other node
 * \param js_nodes Array of JSON objects representing statistics
 *
 * \return None
 *
 *****************************************************************************/

static void dps_req_handler_process_node_status_array(ip_addr_t *node_ip,
                                                      long long config_version,
                                                      json_t *js_nodes)
{
	json_t *js_node;
	json_t *js_node_info;
	ip_addr_t ipnode_array[MAX_NODES_ON_STACK];
	ip_addr_t *ipnodes;
	void *ipnodes_memory = NULL;
	int ipnodes_present = 0;
	int ipnodes_memory_allocated = 0;
	int i, total_nodes;
	const char *node_ipstr;
	char *ptr = NULL;
	int ret_val = -1;

	log_debug(RESTHandlerLogLevel, "Enter");

	total_nodes = json_array_size(js_nodes);

	do
	{
		if (total_nodes > MAX_NODES_ON_STACK)
		{
			ipnodes_memory = malloc(sizeof(ip_addr_t)*total_nodes);
			if (ipnodes_memory == NULL)
			{
				break;
			}
			ipnodes = (ip_addr_t *)ipnodes_memory;
			ipnodes_memory_allocated = 1;
		}
		else
		{
			ipnodes = &ipnode_array[0];
			ipnodes_memory = &ipnode_array[0];
		}
		ipnodes_present = 0;
		// For each node...extract info and add it to the cluster
		for (i = 0; i < total_nodes; i++)
		{
			memset(ipnodes, 0, sizeof(ip_addr_t));

			js_node = json_array_get(js_nodes, i);
			if (json_is_null(js_node) || !json_is_object(js_node))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] JSON node format ERROR", i);
				break;
			}
			/*IP Address*/
			js_node_info = json_object_get(js_node, "ip_address");
			if (json_is_null(js_node_info) || !json_is_string(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] DPS node IP info ERROR", i);
				break;
			}

			node_ipstr = json_string_value(js_node_info);
			if (node_ipstr == NULL)
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d]: Cannot get String Value", i);
				break;
			}

			/* Status */
			js_node_info = json_object_get(js_node, "status");
			if (json_is_null(js_node_info) || !json_is_integer(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] DCS node Status info ERROR", i);
				break;
			}

			ipnodes->status = (unsigned int)json_integer_value(js_node_info);
#if defined (NDEBUG)
			log_debug(RESTHandlerLogLevel, "Processing Node %s, Status %d",
			          node_ipstr, ipnodes->status);
#endif

			ptr = strchr((char *)node_ipstr, ':');
			if (NULL == ptr)
			{
				ipnodes->family = AF_INET;
				ret_val = inet_pton(AF_INET, node_ipstr, ipnodes->ip6);
			}
			else
			{
				ipnodes->family = AF_INET6;
				ret_val = inet_pton(AF_INET, node_ipstr, ipnodes->ip6);
			}
			if (ret_val != 1)
			{
				log_error(RESTHandlerLogLevel,
				          "ERROR: IP Address %s cannot be coverted",
				          node_ipstr);
				break;
			}
			ipnodes++;
			ipnodes_present++;
		}
		if (ipnodes_present != total_nodes)
		{
			log_error(RESTHandlerLogLevel,
			          "ERROR: Number of valid nodes %d, different from total %d",
			          ipnodes_present, total_nodes);
			break;
		}
		// Invoke the DPS Cluster Handler code
		dps_cluster_nodes_status_from_leader(node_ip,
		                                     config_version,
		                                     (ip_addr_t *)ipnodes_memory,
		                                     ipnodes_present);
	}while(0);

	if (ipnodes_memory_allocated)
	{
		free(ipnodes_memory);
		ipnodes_memory = NULL;
		ipnodes = NULL;
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_req_handler_node_status --                                         *//**
 *
 * \brief This routine handles when a node status list is received from the
 *        another node typically the leader
 *
 * \param req The evhttp_request
 * \param arg
 * \param argc The number of arguments
 * \param argv Array of arguments
 *
 * \return void
 *
 *****************************************************************************/
void dps_req_handler_node_status(struct evhttp_request *req,
                                 void *arg, int argc, char **argv)
{

	json_t *js_root = NULL;
	json_t *js_nodes = NULL;
	json_t *js_node = NULL;
	json_t *js_id = NULL;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	long long config_version;
	ip_addr_t remote_ip;
	int res_code = HTTP_BADREQUEST;
	int n;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_PUT:
		{

			n = dps_rest_remote_node_get(req, &remote_ip);
			if (n != 1)
			{
				log_error(RESTHandlerLogLevel, "Cannot get remote node Location");
				break;
			}
#if defined(NDEBUG)
			{
				char str[INET6_ADDRSTRLEN];
				inet_ntop(remote_ip.family, remote_ip.ip6, str, INET6_ADDRSTRLEN);
				log_debug(RESTHandlerLogLevel, "Node Status from Node IP %s", str);
			}
#endif
			log_debug(RESTHandlerLogLevel, "I got a HTTP PUT Request ");
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body)
			{
				log_error(RESTHandlerLogLevel, "Cannot get req_body");
				break;
			}
			if (evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE - 1))
			{
				log_error(RESTHandlerLogLevel,
				          "Node Status message too big %d",
				          evbuffer_get_length(req_body));
				res_code = HTTP_ENTITYTOOLARGE;
				break;
			}
			n = evbuffer_copyout(req_body, large_REST_buffer, LARGE_REST_BUFFER_SIZE - 1);
			large_REST_buffer[n] = '\0';

			log_debug(RESTHandlerLogLevel, "req_body_buf is: %s", large_REST_buffer);

			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_error(RESTHandlerLogLevel, "Bad js_root");
				break;
			}

			/* Borrowed reference, no need to decref */
			/* Get the report node info */
			js_node = json_object_get(js_root, "dps_node");
			if ((js_node == NULL) || (!json_is_object(js_node)))
			{
				log_error(RESTHandlerLogLevel, "Bad dps_node object");
				break;
			}

			js_id = json_object_get(js_node, "Config_Version");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get Configuration Version");
				break;
			}
			config_version = (long long) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel,
			          "config_version is %ld",
			          config_version);

			js_id = json_object_get(js_node, "UUID");
			if (!json_is_string(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel, "UUID is %s", json_string_value(js_id));

			js_id = json_object_get(js_node, "Cluster_Leader");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel, "Bad Cluster_Leader");
				break;
			}
			log_debug(RESTHandlerLogLevel, "Cluster Leader is %s", json_string_value(js_id));

			/*Get the Node Status object*/
			js_nodes = json_object_get(js_root, "nodes_status");
			if (!json_is_array(js_nodes))
			{
				log_error(RESTHandlerLogLevel, "Nodes Status is not an array");
				break;
			}
			dps_req_handler_process_node_status_array(&remote_ip,
			                                          config_version,
			                                          js_nodes);
			/* CREATED 201 */
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, NULL);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

	if (js_root)
	{
		json_decref(js_root);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

unsigned int ip_id_to_v4_addr (char *str, unsigned int *addr)
{
	unsigned int ret = -1;
	int ip1,ip2,ip3,ip4;

	if (NULL != str && NULL != addr && strlen(str) == 8)
	{
		if (4 == sscanf(str, "%2x%2x%2x%2x", &ip1, &ip2, &ip3, &ip4))
		{
			*addr =
			        (((ip1 << 24) & 0xFF000000)
			                | ((ip2 << 16) & 0x00FF0000)
			                | ((ip3 << 8) & 0x0000FF00)
			                | (ip4 & 0x000000FF));
			ret = 0;
		}
	}

	return ret;
}

/* 
   GET /api/dove/dps/vns/{vn_id}/ipv4-external-gateways
   POST /api/dove/dps/vns/{vn_id}/ipv4-external-gateways
   Delete /api/dove/dps/vns/{vn_id}/ipv4-external-gateways
*/
void dps_req_handler_gateways(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	char res_uri[DPS_URI_LEN]; 
	int res_code = HTTP_BADREQUEST;
	int n;
	unsigned int vn_id;
	json_error_t jerror;
	char *endptr = NULL;
	const char *gateway_ip;
	struct in_addr ipv4_addr;
	char ip_id_str[16];

	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vn_id = strtoul(argv[0], &endptr, 10);
#if 0
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_vn_validate(vn_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
#endif
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			js_res = dps_rest_api_get_gateway_all(vn_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_POST: 
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
			{
				break;
			}

			n = evbuffer_copyout(req_body, req_body_buf, (sizeof(req_body_buf) - 1));
			if (n < 1)
			{
				break;
			}
			req_body_buf[n] = '\0';

			js_root = json_loads(req_body_buf, 0, &jerror);

			if (!js_root)
			{
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "gateway_ip");
			if (json_is_null(js_id))
			{
				break;
			}
			if (NULL == (gateway_ip = json_string_value(js_id)))
			{
				break;
			}

			if (0 == inet_pton(AF_INET, gateway_ip, (void *)&ipv4_addr))
			{
				break;
			}

			if (DOVE_STATUS_OK == dps_rest_api_create_gateway(vn_id, ipv4_addr.s_addr))
			{
				js_res = dps_rest_api_get_gateway(vn_id, ipv4_addr.s_addr);
				if (NULL == js_res)
				{
					break;
				}

				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				if (0 != v4_addr_to_ip_id (ip_id_str, ipv4_addr.s_addr))
				{
					break;
				}
				DPS_EXTERNAL_GATEWAY_URI_GEN(res_uri, vn_id, ipv4_addr.s_addr);
				evhttp_add_header(evhttp_request_get_output_headers(req),
								  "Location", res_uri);
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				/* CREATED 201 */
				res_code = 201;
			}
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			if (DOVE_STATUS_OK == dps_rest_api_del_gateway_all(vn_id))
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_root)
	{
		json_decref(js_root);
	}
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/*
   GET /api/dove/dps/vns/{vn_id}/ipv4-external-gateways/{gateway_id}
   Delete /api/dove/dps/vns/{vn_id}/ipv4-external-gateways/{gateway_id}
 */
void dps_req_handler_gateway(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned int vn_id;
	unsigned int gateway_id;
	char *endptr = NULL;
	
	if (argc != 2 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vn_id = strtoul(argv[0], &endptr, 10);
#if 0
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_vn_validate(vn_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	if (0 != ip_id_to_v4_addr(argv[1], &gateway_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
#else
	gateway_id = strtoul(argv[1], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}	
#endif
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{	
			js_res = dps_rest_api_get_gateway(vn_id, gateway_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			if (DOVE_STATUS_OK == dps_rest_api_del_gateway(vn_id, gateway_id))
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	return;
}

/*
   GET /api/dps/domains/{domain_id}/load-balancing
 */
void dps_req_handler_statistics_load_balancing(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned int domain_id;
	char *endptr = NULL;
	
	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_domain_validate(domain_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{	
			js_res = dps_rest_api_get_statistics_load_balancing(domain_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/*
   GET /api/dps/domains/{domain_id}/general-statistics
 */
void dps_req_handler_statistics_general_statistics(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned int domain_id;
	char *endptr = NULL;
	
	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0' || DOVE_STATUS_OK != dps_rest_api_domain_validate(domain_id))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{	
			js_res = dps_rest_api_get_statistics_general_statistics(domain_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/* 
	GET /api/dove/dps/domains/{domain_id}/ipv4-subnets/{ip-mask}
        DELETE /api/dove/dps/domains/{domain_id}/ipv4-subnets/{ip-mask}
        or
	GET /api/dove/dps/vns/{vn_id}/ipv4-subnets/{ip-mask}
	DELETE /api/dove/dps/vns/{vn_id}/ipv4-subnets/{ip-mask}
 */
void dps_req_handler_ipsubnet(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_mode = NULL, *js_gateway = NULL;
	json_t *js_res = NULL;
	json_error_t jerror;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	char res_uri[DPS_URI_LEN]; 
	char subnet_id[64];
	int res_code = HTTP_BADREQUEST;
	int n;
	unsigned long int associated_type, associated_id;
	char *endptr = NULL, *saveptr = NULL, *token = NULL;
	char ip_str[INET6_ADDRSTRLEN], mask_str[INET6_ADDRSTRLEN]; 
	const char *mode_str, *gateway_str;
	unsigned int ip, mask, mode, gateway;

	if (argc != 2 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	if (strstr(evhttp_request_get_uri(req), "vns"))
	{
		associated_type = IP_SUBNET_ASSOCIATED_TYPE_VNID;
	}
	else
	{
		associated_type = IP_SUBNET_ASSOCIATED_TYPE_DOMAIN;
	}
	associated_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	/* Decode IP */
	token = strtok_r(argv[1], "-", &saveptr);
	if(NULL == token)
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	if (0 == inet_pton(AF_INET, token, (void *)&ip))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	/* Decode Mask */
	token = strtok_r(NULL, "-", &saveptr);
	if(NULL == token)
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	if (0 == inet_pton(AF_INET, token, (void *)&mask))
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			js_res = dps_rest_api_get_ipsubnet(associated_type, associated_id, AF_INET, (unsigned char *)&ip, mask);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			break;
		}
		case EVHTTP_REQ_PUT:
		{
			/* create a new element */
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
			{
				log_debug(RESTHandlerLogLevel,"Could not get input buffer");
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, (sizeof(req_body_buf) - 1));
			req_body_buf[n] = '\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_debug(RESTHandlerLogLevel,"JSON body NULL");
				break;
			}
			/* Borrowed reference, no need to decref */
			inet_ntop(AF_INET, &ip, ip_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &mask, mask_str, INET6_ADDRSTRLEN);
			js_mode = json_object_get(js_root, "mode");
			if (!json_is_string(js_mode))
			{
				break;
			}
			if ((mode_str = json_string_value(js_mode)) == NULL)
			{
				break;
			}
			if (strcmp(mode_str, "dedicated") == 0)
			{
				mode = 0;
			}
			else if (strcmp(mode_str, "shared") == 0)
			{
				mode = 1;
			}
			else
			{
				break;
			}
			js_gateway = json_object_get(js_root, "gateway");
			if (!json_is_string(js_gateway))
			{
				break;
			}
			if ((gateway_str = json_string_value(js_gateway)) == NULL)
			{
				break;
			}
			if (0 == inet_pton(AF_INET, gateway_str, (void *)&gateway))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel,"ip %s, mask %s gw %s mode %s", ip_str, mask_str,gateway_str,mode_str);
			if (DOVE_STATUS_OK == dps_rest_api_create_ipsubnet(associated_type, associated_id, AF_INET, (unsigned char *)&ip, mask, mode, (unsigned char *)&gateway))
			{
				/* {
					"id":"192.168.1.0-255.255.255.0",
					"uri":"/api/dove/dps/domains/12345/ipv4-subnets/192.168.1.0-255.255.255.0"
				   }
				   or
				   {
					"id":"192.168.1.0-255.255.255.0",
					"uri":"/api/dove/dps/vns/789/ipv4-subnets/192.168.1.0-255.255.255.0"
				   }
				*/
				snprintf(subnet_id, sizeof(subnet_id), "%s-%s", ip_str, mask_str);
				if (associated_type == IP_SUBNET_ASSOCIATED_TYPE_DOMAIN)
				{
					DPS_DOMAIN_IPV4SUBNET_URI_GEN(res_uri, associated_id, ip_str, mask_str);
				}
				else
				{
					DPS_DVG_IPV4SUBNET_URI_GEN(res_uri, associated_id, ip_str, mask_str);
				}
				js_res = json_pack("{s:s, s:s}", "id", subnet_id,  "uri", res_uri);
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evhttp_add_header(evhttp_request_get_output_headers(req), "Location", res_uri);
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				/* CREATED 201 */
				res_code = 201;
			}
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			if (DOVE_STATUS_OK == dps_rest_api_del_ipsubnet(associated_type, associated_id, AF_INET, (unsigned char *)&ip, mask))
			{
				res_code = HTTP_OK;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	return;
}

static dove_status dps_form_local_domain_mapping_json(struct evbuffer **retbuf)
{
	char *local_domain_str = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	dove_status status = DOVE_STATUS_OK;
	char ipstr[INET6_ADDRSTRLEN];

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		status = dps_node_get_domains(dcs_local_ip, &local_domain_str);
		if (status != DOVE_STATUS_OK)
		{
			log_error(RESTHandlerLogLevel,
			          "dps_node_get_domains returns %s",
			          DOVEStatusToString(status));
			break;
		}
		inet_ntop(dcs_local_ip.family, dcs_local_ip.ip6, ipstr, INET6_ADDRSTRLEN);
		/* form json string*/
		js_res = json_pack("{s:s,s:i,s:s}",
		                   "ip",ipstr,
		                   "port",(int)dcs_local_ip.port,
		                   "domains", local_domain_str);
		if(NULL == js_res)
		{
			log_error(RESTHandlerLogLevel, "Error in forming json string");
			status = DOVE_STATUS_ERROR;
			break;
		}
		res_body_str = json_dumps(js_res,JSON_PRESERVE_ORDER);
		if (NULL == res_body_str)
		{
			log_error(RESTHandlerLogLevel, "JSON string is NULL or Bad");
			status = DOVE_STATUS_ERROR;
			break;
		}
		*retbuf = evbuffer_new();
		if (NULL == *retbuf)
		{
			log_warn(RESTHandlerLogLevel,"Could NOT allocate evbuffer");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		evbuffer_add(*retbuf, res_body_str, strlen(res_body_str) + 1);

		if(local_domain_str)
		{
			free(local_domain_str);
		}
	}while(0);

	if (res_body_str)
	{
		free(res_body_str);
	}

	log_debug(RESTHandlerLogLevel, "Exit: Status %s",
	          DOVEStatusToString(status));
	return status;
}

/*
 * POST /api/dove/dps/local-domains
 * {
 * 	ip : "A.B.C.D"
 * 	port: 12345
 * 	domains : "1,10,15-17,20"
 * }
 */

void dps_req_handler_local_domain_mapping(struct evhttp_request *req,
                                          void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	int status;
	int res_code = HTTP_BADREQUEST;
	int n;
	json_error_t jerror;
	const char *domain_str;
	const char *ip;
	unsigned int port = 0;

	log_info(RESTHandlerLogLevel,"Enter");
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_POST:
		case EVHTTP_REQ_PUT:
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body ||
			    (evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE - 1)))
			{
				log_alert(RESTHandlerLogLevel,
				          "Mapping Buffer Size %d too big. Quitting!!!",
				          evbuffer_get_length(req_body));
				break;
			}

			n = evbuffer_copyout(req_body, large_REST_buffer, (LARGE_REST_BUFFER_SIZE - 1));
			if (n < 1)
			{
				log_error(RESTHandlerLogLevel, "evbuffer_copyout failed, size d%",
				          evbuffer_get_length(req_body));
				break;
			}
			large_REST_buffer[n] = '\0';

			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_error(RESTHandlerLogLevel, "json_loads failed");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "ip");
			if (json_is_null(js_id))
			{
				log_info(RESTHandlerLogLevel, "json_object_get for ip failed");
				break;
			}
			if (NULL == (ip = json_string_value(js_id)))
			{
				log_info(RESTHandlerLogLevel, "json_string_value for ip failed");
				break;
			}
			js_id = json_object_get(js_root, "port");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel, "json_object_get for port failed");
				break;
			}
			port = (unsigned int)json_integer_value(js_id);

			js_id = json_object_get(js_root, "domains");
			if (json_is_null(js_id))
			{
				log_info(RESTHandlerLogLevel, "json_object_get for domains failed");
				break;
			}
			if (NULL == (domain_str = json_string_value(js_id)))
			{
				log_info(RESTHandlerLogLevel, "json_string_value for domains failed");
				break;
			}
			status = dps_cluster_set_node_domain_mapping((char *)ip, port,
			                                             (char *)domain_str);
			if(status == DOVE_STATUS_OK) {
				int ret_val;
				ret_val = dps_form_local_domain_mapping_json(&retbuf);
				if(ret_val != DOVE_STATUS_OK) {
					log_error(RESTHandlerLogLevel,
					          "dps_form_local_domain_mapping_json returned error [%d]",
					          ret_val);
					res_code = HTTP_INTERNAL;
				}
				else {
					res_code = HTTP_OK;
				}
			}
			else {
				log_error(RESTHandlerLogLevel,
				          "dps_cluster_set_node_domain_mapping returned error [%d]",
				          status);
				res_code = HTTP_INTERNAL;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	if (js_root)
	{
		json_decref(js_root);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/* 
   GET /api/dove/dps/service/role
   POST /api/dove/dps/service/role
*/
void dps_req_handler_service_role(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_action = NULL;
	json_t *js_res = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	int res_code = 409;
	int n;
	const char *action_str;
	uint32_t action = 0;
	json_error_t jerror;
	dove_status status;

	log_info(RESTHandlerLogLevel, "Enter");
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			res_code = HTTP_NOCONTENT;
			break;
		}
		case EVHTTP_REQ_POST: 
		case EVHTTP_REQ_PUT:
		{

			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				break;
			}
			/* Borrowed reference, no need to decref */
			js_action = json_object_get(js_root, "action");
			if (!json_is_string(js_action))
			{
				log_alert(RESTHandlerLogLevel,
				          "%s: Bad field -- action", __FUNCTION__);
				break;
			}
			if ((action_str = json_string_value(js_action)) == NULL)
			{
				log_alert(RESTHandlerLogLevel,
				          "%s: Bad field -- action_str", __FUNCTION__);
				break;
			}
			if (strcmp(action_str, "start") == 0)
			{
				log_notice(RESTHandlerLogLevel, "DCS: From DMC set role");
				action = 1;
			}
			else if (strcmp(action_str, "stop") == 0)
			{
				log_notice(RESTHandlerLogLevel, "DCS: From DMC reset role");
				action = 0;
			}
			status = dcs_set_service_role(action);
			if (DOVE_STATUS_OK == status)
			{
				/* CREATED 201 */
				log_notice(RESTHandlerLogLevel,
				           "DCS: Set service role to %s returns Success!",
				           action_str);
				res_code = HTTP_NOCONTENT;
			}
			else
			{
				log_alert(RESTHandlerLogLevel,
				          "DCS: Set service role to %s returns Failure [%s]!",
				          action_str, DOVEStatusToString(status));
				show_print("DCS: Set service role to %s returns Failure [%s]!",
				           action_str, DOVEStatusToString(status));
				res_code = 409;
			}
			break;
		}
		default:
		{
			res_code = 409;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_root)
	{
		json_decref(js_root);
	}
	if (js_res)
	{
		json_decref(js_res);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_query_cluster_nodes_process                                        *//**
 *
 * \brief - This function processes the JSON body of the REST API when the
 *          DMC sends over the list of all nodes in the cluster.
 *
 * \param js_nodes  The JSON body containing the list of DCS Nodes
 *
 * \retval None
 *
 ******************************************************************************
 */
static void dps_dmc_query_cluster_nodes_process(json_t *js_nodes)
{

	int i, total_nodes;
	unsigned int node_family, node_service_port;
	json_t *js_node, *js_node_info;
	const char *node_ipstr;
	ip_addr_t ipnode_array[MAX_NODES_ON_STACK];
	ip_addr_t *ipnodes;
	void *ipnodes_memory = NULL;
	int ipnodes_present = 0;
	int ipnodes_memory_allocated = 0;
	unsigned int node_rest_port;
	const char *node_uuid;

	log_debug(RESTHandlerLogLevel, "Enter");

	total_nodes = json_array_size(js_nodes);

	do
	{
		if (total_nodes > MAX_NODES_ON_STACK)
		{
			ipnodes_memory = malloc(sizeof(ip_addr_t)*total_nodes);
			if (ipnodes_memory == NULL)
			{
				break;
			}
			ipnodes = (ip_addr_t *)ipnodes_memory;
			ipnodes_memory_allocated = 1;
		}
		else
		{
			ipnodes = &ipnode_array[0];
			ipnodes_memory = &ipnode_array[0];
		}
		ipnodes_present = 0;
		// For each node...extract info and add it to the cluster
		for (i = 0; i < total_nodes; i++) {
			js_node = json_array_get(js_nodes, i);
			if (json_is_null(js_node) || !json_is_object(js_node))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] JSON node format ERROR", i);
				break;
			}
			/*Family */
			js_node_info = json_object_get(js_node, "ip_family");
			if (json_is_null(js_node_info) || !json_is_integer(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] DCS node Family info ERROR", i);
				break;
			}

			node_family = (unsigned int)json_integer_value(js_node_info);
			/*IP */
			js_node_info = json_object_get(js_node, "ip");
			if (json_is_null(js_node_info) || !json_is_string(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d] DPS node IP info ERROR", i);
				break;
			}

			node_ipstr = json_string_value(js_node_info);
			if (node_ipstr == NULL)
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d]: Cannot get String Value", i);
				break;
			}

			/* REST Port */
			js_node_info = json_object_get(js_node, "dcs_rest_service_port");
			if (json_is_null(js_node_info) || !json_is_integer(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d]: DPS node REST Port ERROR", i);
				break;
			}

			node_rest_port = (unsigned int) json_integer_value(js_node_info);

			/* Service Port */
			js_node_info = json_object_get(js_node, "dcs_raw_service_port");
			if (json_is_null(js_node_info) || !json_is_integer(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d]: DPS node Client Procotol Port ERROR", i);
				break;
			}
			node_service_port = (unsigned int) json_integer_value(js_node_info);

			/* UUID */
			js_node_info = json_object_get(js_node, "uuid");
			if (json_is_null(js_node_info) || !json_is_string(js_node_info))
			{
				log_error(RESTHandlerLogLevel,
				         "Element [%d]: DPS node UUID ERROR", i);
				break;
			}
			node_uuid = json_string_value(js_node_info);
			log_notice(RESTHandlerLogLevel,
			           "Node info, family is %u, ip is %s, "
			           "service_port is %u, rest_port is %u uuid is %s",
			           node_family, node_ipstr, node_service_port,
			           node_rest_port, node_uuid);

			memset(ipnodes, 0, sizeof(ip_addr_t));
			/*Construct and Add the node to the cluster */
			ipnodes->family = node_family;
			if (!inet_pton(node_family, node_ipstr, ipnodes->ip6))
			{
				log_error(RESTHandlerLogLevel,
				         "Element [%d]: Invalid IP %s ERROR", i, node_ipstr);
				break;
			}
			ipnodes->xport_type = SOCK_DGRAM;
			ipnodes->port = node_service_port;
			ipnodes++;
			ipnodes_present++;
		}
		if (ipnodes_present != total_nodes)
		{
			break;
		}
		// Invoke the DCS Cluster Database handler call if all nodes
		// were processed
		dps_cluster_nodes_from_dmc_update((ip_addr_t *)ipnodes_memory,
		                                  ipnodes_present);
	}while(0);

	if (ipnodes_memory_allocated)
	{
		free(ipnodes_memory);
		ipnodes_memory = NULL;
		ipnodes = NULL;
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 * POST /api/dove/dps/cluster
 * {
 * 	"dps":[{"id":1111,"ip_family":2,"ip":"1.2.3.4","rest_port":1888,"service_port":902},
 * 		{},...]
 * }
 */

void dps_req_handler_query_cluster_nodes(struct evhttp_request *req,
                                         void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	struct evbuffer *req_body = NULL;
	int res_code = HTTP_BADREQUEST;
	int n;
	json_error_t jerror;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_PUT: {
			log_notice(PythonClusterDataLogLevel,
			          "Got list of updated DCS nodes from DMC");
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body ||
			    (evbuffer_get_length(req_body) > LARGE_REST_BUFFER_SIZE))
			{
				log_error(PythonClusterDataLogLevel,
				          "Process DCS Node List: could not get req_body");
				break;
			}

			n = evbuffer_copyout(req_body, large_REST_buffer,
			                     (LARGE_REST_BUFFER_SIZE - 1));
			if (n < 1)
			{
				log_error(PythonClusterDataLogLevel,
				          "Process DCS Node List: evbuffer_copyout failed...");
				break;
			}
			large_REST_buffer[n] = '\0';

			js_root = json_loads(large_REST_buffer, 0, &jerror);

			if (!js_root)
			{
				log_error(PythonClusterDataLogLevel,
				          "Process DCS Node List: js_root NULL...");
				break;
			}
			/* Borrowed reference, no need to decref */
			js_id = json_object_get(js_root, "dps");
			if (json_is_null(js_id))
			{
				log_error(PythonClusterDataLogLevel,
				          "Process DCS Node List: js_id NULL...");
				break;
			}
			dps_dmc_query_cluster_nodes_process(js_id);
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, NULL);
	if (js_root)
	{
		json_decref(js_root);
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

static dove_status dps_form_local_dcslist_json(struct evbuffer **retbuf,
                                               ip_addr_t *node_list, uint32_t nodes_count)
{
	char ipstr[INET6_ADDRSTRLEN];
	json_t *js_res = NULL;
	json_t * js_array = NULL;
	char *res_body_str = NULL;
	int i;

	js_array = json_array();

	for(i = 0; i < (int)nodes_count; i++)
	{
		inet_ntop(node_list[i].family, node_list[i].ip6,
		          ipstr, INET6_ADDRSTRLEN);
		js_res = json_pack("{s:i,s:s,s:s,s:i,s:i}",
		                    "ip_family", node_list[i].family,
		                    "ip",ipstr,
		                    "uuid",dps_node_uuid,
		                    "rest_port",dps_rest_port,
		                    "service_port",node_list[i].port
		                   );
		if(NULL == js_res)
		{
			log_debug(RESTHandlerLogLevel,"Error in forming json string");
			return DOVE_STATUS_ERROR;
		}
		if(json_array_append_new(js_array, js_res) < 0) {
			return DOVE_STATUS_ERROR;
		}
	}
	js_res = json_pack("{s:o}",
	                   "dcslist",js_array
	                   );
	res_body_str = json_dumps(js_res,JSON_PRESERVE_ORDER);
	if (NULL == res_body_str)
	{
		log_alert(RESTHandlerLogLevel,
		          "JSON string is NULL or Bad");
		return DOVE_STATUS_ERROR;
	}
	*retbuf = evbuffer_new();
	if (NULL == *retbuf)
	{
		log_alert(RESTHandlerLogLevel,"Could NOT allocate evbuffer");
		return DOVE_STATUS_ERROR;
	}
	evbuffer_add(*retbuf, res_body_str, strlen(res_body_str) + 1);

	if (js_res)
	{
		json_decref(js_res);
	}
	if(js_array){
		json_decref(js_array);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}

	return DOVE_STATUS_OK;
}
/*
	GET /api/dps/domains/{domain_id}/dcslist
 */
void dps_req_handler_dcslist(struct evhttp_request *req, void *arg, int argc,
                             char **argv)
{
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int domain_id;
	char *endptr = NULL;
	dove_status status = DOVE_STATUS_ERROR;
	uint32_t nodes_count = 0,nodes_needed = 0;
	ip_addr_t nodes[MAX_NODES_PER_DOMAIN];

	nodes_needed = MAX_NODES_PER_DOMAIN;
	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			status = dps_get_domain_node_mapping(domain_id,
			                                     nodes_needed,
			                                     nodes,
			                                     &nodes_count);
			if (status != DOVE_STATUS_OK)
			{
				log_warn(RESTHandlerLogLevel,
				          "Cannot determine the DCS Nodes handling domain %d, Error %s",
				          domain_id, DOVEStatusToString(status));
				res_code = HTTP_INTERNAL;
				break;
			}
			if (nodes_count <= 0)
			{
				log_warn(RESTHandlerLogLevel,
				          "Cannot find any DCS Nodes handling domain %d",
				          domain_id);
				res_code = HTTP_INTERNAL;
				break;
			}
			if (nodes_count > MAX_NODES_PER_DOMAIN)
			{
				log_alert(RESTHandlerLogLevel,
				          "ALERT!!! No of Nodes [%d] handling domain %d"
				          "exceed [%d]",
				          nodes_count, domain_id, MAX_NODES_PER_DOMAIN);
				res_code = HTTP_INTERNAL;
				break;
			}
			status = dps_form_local_dcslist_json(&retbuf,nodes, nodes_count);
			if (status != DOVE_STATUS_OK)
			{
				log_alert(RESTHandlerLogLevel,
				          "Error in forming DCS List JSON structure");
				res_code = HTTP_INTERNAL;
				break;
			}
			log_debug(RESTHandlerLogLevel,
			          "[%u] nodes handle domain [%u]", nodes_count,
			          domain_id);
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/*
 * POST /api/dove/dps/dmc
 * {
 * "ip": 1.2.3.4
 * "port": 80
 * }
 */

void dps_req_handler_set_dmc_location(struct evhttp_request *req, void *arg, int argc,
                                      char **argv)
{

	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	unsigned int port;
	const char *ipstr;
	dps_controller_data_op_t data;
	uint32_t ip_family = AF_INET;
	dove_status status = DOVE_STATUS_OK;

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_POST: {
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (sizeof(req_body_buf)-1))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf, sizeof(req_body_buf)-1);
			req_body_buf[n]='\0';
			js_root = json_loads(req_body_buf, 0, &jerror);
			if (!js_root)
			{
				log_debug(RESTHandlerLogLevel,"js_root is NULL");
				break;
			}

			js_id = json_object_get(js_root, "ip");
			if (NULL == js_id || !json_is_string(js_id))
			{
				log_debug(RESTHandlerLogLevel,"js_id is NOT string");
				break;
			}
			if((ipstr = json_string_value(js_id)) == NULL)
			{
				log_debug(RESTHandlerLogLevel,"ip is NULL");
				break;
			}

			js_id = json_object_get(js_root, "port");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_debug(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			port = (unsigned int)json_integer_value(js_id);

			log_debug(RESTHandlerLogLevel,"ip == >> [%s]", ipstr);
			log_debug(RESTHandlerLogLevel,"port == >> [%u]",port);

			
			data.type = DPS_CONTROLLER_LOCATION_SET;
			data.controller_location.family = ip_family;
			inet_pton(ip_family,ipstr,data.controller_location.ip6);
			data.controller_location.port_http = port;
			status = dps_controller_data_msg(&data);

			// send dps (dcs) appliance registration message to controller (dmc)
			if (DOVE_STATUS_OK == status)
			{
				//User explicitly enter an IP Address reset it.
				dcs_set_service_role(0);
			}
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	if (js_root)
	{
		json_decref(js_root);
	}
	return;
}

/* Trevor:
Runtime information to DMC, used for Debug 
2013-01-15
*/

/* 
    GET /api/dove/dps/vns/{vn_id}/endpoints
    First ,get the vnid
 */
void dps_req_handler_vnid_endpoints(struct evhttp_request *req,
                                    void *arg, int argc, char **argv)
{

	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int vn_id;
	char *endptr = NULL;

	log_debug(RESTHandlerLogLevel,"Enter");

	if (argc != 1 || NULL == argv)
	{
		log_info(RESTHandlerLogLevel, "Exit: (argc != 1 || NULL == argv) HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vn_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(RESTHandlerLogLevel, "Exit: strtoul failed: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	log_debug(RESTHandlerLogLevel,"vnid is %d", vn_id);

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			/* Get debug endpoints */
			js_res = vnid_get_endpoints_json(vn_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res,
				                          JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str,
				             strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_NOTFOUND;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_debug(RESTHandlerLogLevel,"Exit");
	return;
}

/* Trevor:
Runtime information to DMC, used for Debug 
2013-01-15
*/
/* 
 GET /api/dove/dps/vns/{vn_id}/tunnel-endpoints
 First ,get the vnid
 */
void dps_req_handler_vnid_tunnel_endpoints(struct evhttp_request *req,
                                           void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int vn_id;
	char *endptr = NULL;

	log_debug(RESTHandlerLogLevel,"Enter");
	if (argc != 1 || NULL == argv)
	{
		log_info(RESTHandlerLogLevel, "Exit: (argc != 1 || NULL == argv) HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vn_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(RESTHandlerLogLevel, "Exit: strtoul failed: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	log_debug(RESTHandlerLogLevel,"Vnid is %d", vn_id);
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			/* Get debug tunnel endpoints */
			js_res = vnid_get_tunnel_endpoints_json(vn_id);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_NOTFOUND;
			}
			break;
		}
		case EVHTTP_REQ_PUT:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
		case EVHTTP_REQ_DELETE:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_debug(RESTHandlerLogLevel,"Exit");
	return;
}

void dps_req_handler_vnid_get_domain_mapping(struct evhttp_request *req,
                                             void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;

	log_info(RESTHandlerLogLevel,"Enter");
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			/* Get debug tunnel endpoints */
			js_res = vnid_get_domain_mapping_json();
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_NOTFOUND;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel,"Exit");
	return;
}

void dps_req_handler_vnid_get_allow_policies(struct evhttp_request *req,
                                             void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int vnid;
	char *endptr = NULL;

	log_info(RESTHandlerLogLevel,"Enter");
	if (argc != 1 || NULL == argv)
	{
		log_info(RESTHandlerLogLevel, "Exit: (argc != 1 || NULL == argv) HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vnid = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(RESTHandlerLogLevel, "Exit: strtoul failed: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	log_debug(RESTHandlerLogLevel,"vnid is %d", vnid);
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			/* Get debug tunnel endpoints */
			js_res = vnid_get_allow_policies(vnid);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_NOTFOUND;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel,"Exit");
	return;
}

void dps_req_handler_vnid_get_subnets(struct evhttp_request *req,
                                      void *arg, int argc, char **argv)
{
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;
	unsigned long int vnid;
	char *endptr = NULL;

	log_info(RESTHandlerLogLevel,"Enter");
	if (argc != 1 || NULL == argv)
	{
		log_info(RESTHandlerLogLevel, "Exit: (argc != 1 || NULL == argv) HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	vnid = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(RESTHandlerLogLevel, "Exit: strtoul failed: HTTP_BADREQUEST");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	log_debug(RESTHandlerLogLevel,"vnid is %d", vnid);
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			/* Get debug VNID Subnets */
			js_res = vnid_get_subnets(vnid);
			if (js_res)
			{
				res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
				if (NULL == res_body_str)
				{
					break;
				}
				retbuf = evbuffer_new();
				if (NULL == retbuf)
				{
					break;
				}
				evbuffer_add(retbuf, res_body_str, strlen(res_body_str) + 1);
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_NOTFOUND;
			}
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, retbuf);
	if (js_res)
	{
		json_decref(js_res);
	}
	if (res_body_str)
	{
		free(res_body_str);
	}
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(RESTHandlerLogLevel,"Exit");
	return;
}

void dps_req_handler_cluster_display(struct evhttp_request *req,
                                     void *arg, int argc, char **argv)
{
	int res_code = HTTP_BADREQUEST;

	log_info(RESTHandlerLogLevel,"Enter");
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		case EVHTTP_REQ_POST:
		case EVHTTP_REQ_PUT:
		{
			dps_cluster_show();
			res_code = HTTP_OK;
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}
	evhttp_send_reply(req, res_code, NULL, NULL);
	log_info(RESTHandlerLogLevel,"Exit");
	return;
}

