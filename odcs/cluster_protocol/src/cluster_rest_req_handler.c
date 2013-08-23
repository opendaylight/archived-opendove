/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest handler functions for Clustering functionality
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
*  $Log: cluster_rest_req_handler.c $
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
#include "cluster_rest_req_handler.h"

/*
 * POST /api/dove/dps/domains/X/transfer
 * {
 * 	"ip_family":
 * 	"ip":
 * }
 */

void dps_req_handler_transfer_domain(struct evhttp_request *req, void *arg,
                                 int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	char *endptr = NULL;
	unsigned long int domain;
	uint32_t family;
	const char *ip;
	dps_controller_data_op_t data_op;
	dove_status status = DOVE_STATUS_OK;
	char IPv6[16];

	if (argc != 1 || NULL == argv )
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
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
				log_debug(PythonDataHandlerLogLevel,"js_root is NULL");
				break;
			}
			js_id = json_object_get(js_root, "ip_family");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			family = (unsigned int)json_integer_value(js_id);
			js_id = json_object_get(js_root, "ip");
			if (NULL == js_id || !json_is_string(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT string");
				break;
			}
			if((ip = json_string_value(js_id)) == NULL)
			{
				log_info(RESTHandlerLogLevel,"ip is NULL");
				break;
			}

			log_debug(PythonDataHandlerLogLevel,"domain_id == >> [%u]",
			          domain);
			log_debug(PythonDataHandlerLogLevel,"ip == >> [%s]",ip);
			memset(IPv6,0,16);
			inet_pton(family, ip, IPv6);
			data_op.type = DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_START;
			data_op.domain_mass_transfer_start.domain_id = domain;
			data_op.domain_mass_transfer_start.dps_server.family = family;
			data_op.domain_mass_transfer_start.dps_server.port = 0;
			memcpy(data_op.domain_mass_transfer_start.dps_server.ip6,
				       IPv6, 16);
			status = dps_controller_data_msg(&data_op);
			if (status != DOVE_STATUS_OK) {
				log_error(PythonClusterDataLogLevel,
				          "Domain %d Mass transfer START returned %s",
				          domain, DOVEStatusToString(status));
				res_code = HTTP_INTERNAL;
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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/*
 * POST /api/dove/dps/get-ready
 * {
 * 	"domain":1
 * }
 */
void dps_req_handler_get_ready(struct evhttp_request *req, void *arg,
                                 int argc, char **argv)
{
	json_t *js_root = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	int domain;
	dps_controller_data_op_t data_op;
	dove_status status = DOVE_STATUS_OK;
	json_t *js_id = NULL;

	switch (evhttp_request_get_command(req))
	{
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
				log_debug(PythonDataHandlerLogLevel,"js_root is NULL");
				break;
			}
			js_id = json_object_get(js_root, "domain");
			if (!json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			domain = (unsigned int)json_integer_value(js_id);

			log_info(PythonClusterDataLogLevel,
			         "Domain %d: Received GET_READY Message", domain);
			data_op.type = DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_GET_READY;
			data_op.domain_mass_transfer_get_ready.domain_id = domain;
			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK)
			{
				log_error(PythonClusterDataLogLevel,
				          "Domain %d: Error in GET_READY - %s",
				          domain, DOVEStatusToString(status));
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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
}

/*
 * POST /api/dove/dps/domain-activate
 * {
 * 	"domain":1
 * }
 */
void dps_req_handler_domain_activate(struct evhttp_request *req, void *arg,
                                     int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n, ret_val;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	int domain;
	uint32_t replication_factor;
	dps_controller_data_op_t data_op;
	ip_addr_t remote_ip;
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonClusterDataLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
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
				log_debug(PythonDataHandlerLogLevel,"js_root is NULL");
				break;
			}
			js_id = json_object_get(js_root, "domain");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			domain = (unsigned int)json_integer_value(js_id);
			js_id = json_object_get(js_root, "replication_factor");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			replication_factor = (unsigned int)json_integer_value(js_id);

			log_info(PythonClusterDataLogLevel,
			         "Domain %d: Received DOMAIN_ACTIVATE, Replication Factor %d",
			         domain, replication_factor);
			data_op.type = DPS_CONTROLLER_DOMAIN_ACTIVATE;
			data_op.domain_add.domain_id = domain;
			data_op.domain_add.replication_factor = replication_factor;

			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK) {
				log_error(PythonClusterDataLogLevel,
				          " **ERROR** in processing domain activate -- status == %s",
				          DOVEStatusToString(status));
				break;
			}

			res_code = HTTP_OK;
			//Exchange domain mapping synchronously with other nodes
			ret_val = dps_rest_remote_node_get(req, &remote_ip);
			if (ret_val != 1)
			{
				log_error(PythonClusterDataLogLevel,
				          "Cannot determine Node that sent the Activate");
				break;
			}

			dps_cluster_exchange_domain_mapping_activate(&remote_ip);

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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 * PUT /api/dove/dps/domains/<domain_id>/vnid-listing
 * {
 * 	"domain":1
 * 	"vnids":[array of vnids]
 * }
 */
void dps_req_handler_domain_vnid_listing(struct evhttp_request *req, void *arg,
                                         int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_vnid = NULL;
	int res_code = HTTP_BADREQUEST;
	int n, domain;
	uint32_t i, vnid;
	enum evhttp_cmd_type cmd_type;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char *endptr = NULL;
	dps_controller_data_op_t data_op;
	json_error_t jerror;

	log_info(PythonClusterDataLogLevel, "Enter");

	if (argc != 1 || NULL == argv )
	{
		log_info(PythonClusterDataLogLevel,
		         "Exit: HTTP_BADREQUEST: (argc != 1 || NULL == argv )");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(PythonClusterDataLogLevel,
		         "Exit: HTTP_BADREQUEST: Can't get domain id in request");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	cmd_type = evhttp_request_get_command(req);
	switch (cmd_type)
	{
		case EVHTTP_REQ_PUT:
		case EVHTTP_REQ_DELETE:
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body)
			{
				log_info(PythonClusterDataLogLevel,
				         "evhttp_request_get_input_buffer returns NULL");
				break;
			}
			if (evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE-1))
			{
				log_error(PythonClusterDataLogLevel,
				          "VNIDs Buffers Size %d too big, Quitting!!!",
				          evbuffer_get_length(req_body));
				break;
			}
			n = evbuffer_copyout(req_body, large_REST_buffer, (LARGE_REST_BUFFER_SIZE - 1));
			if (n < 1)
			{
				break;
			}
			large_REST_buffer[n] = '\0';

			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_debug(PythonClusterDataLogLevel,"js_root is NULL");
				break;
			}
			/* Get every vnids info */
			js_id = json_object_get(js_root, "vnids");

			if (json_is_null(js_id) || !json_is_array(js_id))
			{
				log_info(PythonClusterDataLogLevel,
				         "DCS Domain VNIDs info in the request response is not an Array");
				break;
			}
			for (i = 0; i < json_array_size(js_id); i++)
			{
				js_vnid = json_array_get(js_id, i);
				if (json_is_null(js_vnid) || !json_is_integer(js_vnid))
				{
					log_info(PythonClusterDataLogLevel,
					         "node format ERROR, vnid is not Integer");
					break;
				}
				vnid = (uint32_t) json_integer_value(js_vnid);
				log_debug(RESTHandlerLogLevel, "vnid is %u", vnid);
				if (cmd_type == EVHTTP_REQ_PUT)
				{
					log_info(PythonClusterDataLogLevel,
					         "Adding VNID %d in Domain %d", vnid, domain);
					data_op.type = DPS_CONTROLLER_DVG_ADD;
					data_op.dvg_add.domain_id = domain;
					data_op.dvg_add.dvg_id = vnid;
				}
				else
				{
					log_info(PythonClusterDataLogLevel,
					         "Deleting VNID %d in Domain %d", vnid, domain);
					data_op.type = DPS_CONTROLLER_DVG_DELETE;
					data_op.dvg_delete.dvg_id = vnid;
				}
				dps_controller_data_msg(&data_op);
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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 * POST /api/dove/dps/domain-recover
 * {
 * 	"domain":1
 * 	"replication_factor":2
 * }
 */
void dps_req_handler_domain_recover(struct evhttp_request *req, void *arg,
                                    int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	int domain;
	uint32_t replication_factor;
	dps_controller_data_op_t data_op;
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonClusterDataLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
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
				log_debug(PythonDataHandlerLogLevel,"js_root is NULL");
				break;
			}
			js_id = json_object_get(js_root, "domain");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			domain = (unsigned int)json_integer_value(js_id);
			js_id = json_object_get(js_root, "replication_factor");
			if (NULL == js_id || !json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			replication_factor = (unsigned int)json_integer_value(js_id);

			log_notice(PythonClusterDataLogLevel,
			           "Received **DOMAIN_RECOVER** Message for "
			           "DOMAIN %d, Replication Factor %d",
			           domain, replication_factor);
			data_op.type = DPS_CONTROLLER_DOMAIN_RECOVERY_START;
			data_op.domain_recover.domain_id = domain;
			data_op.domain_recover.replication_factor = replication_factor;

			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK) {
				log_error(PythonClusterDataLogLevel,
				          " **ERROR** in processing domain recover -- status == %s",
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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 * POST /api/dove/dps/domain-deactivate
 * {
 * 	"domain":1
 * }
 */
void dps_req_handler_domain_deactivate(struct evhttp_request *req, void *arg,
                                     int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	int res_code = HTTP_BADREQUEST;
	char req_body_buf[1024];
	int n;
	struct evbuffer *req_body = NULL;
	json_error_t jerror;
	int domain;
	dps_controller_data_op_t data_op;
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonClusterDataLogLevel, "Enter");
	switch (evhttp_request_get_command(req))
	{
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
				log_debug(PythonDataHandlerLogLevel,"js_root is NULL");
				break;
			}
			js_id = json_object_get(js_root, "domain");
			if (!json_is_integer(js_id))
			{
				log_info(RESTHandlerLogLevel,"js_id is NOT integer");
				break;
			}
			domain = (unsigned int)json_integer_value(js_id);

			log_info(PythonClusterDataLogLevel,
			          "Domain %d: Received DOMAIN_DEACTIVATE Message",
			          domain);
			data_op.type = DPS_CONTROLLER_DOMAIN_DEACTIVATE;
			data_op.domain_delete.domain_id = domain;

			status = dps_controller_data_msg(&data_op);
			if(status != DOVE_STATUS_OK) {
				log_error(PythonClusterDataLogLevel,
				          "ERROR: processing domain deactivate [%s]",
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
	evhttp_send_reply(req, res_code, NULL, NULL);
	if (js_root)
	{
		json_decref(js_root);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
 * PUT/DELETE /api/dove/dps/domains/<domain_id>/bulk_policy
 * {
 * 	"policies":[array of policy info]
 * }
 */
void dps_req_handler_domain_bulk_policy(struct evhttp_request *req, void *arg,
                                         int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_policy = NULL;
	json_t *js_node = NULL;
	int n, domain;
	uint32_t i, no_of_policies, traffic_type, type, src_dvg, dst_dvg, ttl;
	enum evhttp_cmd_type cmd_type;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char *endptr = NULL;
	json_error_t jerror;
	dps_object_policy_action_t action;
	int res_code = HTTP_BADREQUEST;
	dove_status status = DOVE_STATUS_OK;

	log_info(PythonClusterDataLogLevel, "Enter");

	if (argc != 1 || NULL == argv )
	{
		log_info(PythonClusterDataLogLevel,
		         "Exit: HTTP_BADREQUEST: (argc != 1 || NULL == argv )");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	domain = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(PythonClusterDataLogLevel,
		         "Exit: HTTP_BADREQUEST: Can't get domain id in request");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	memset(&action, 0, sizeof(dps_object_policy_action_t));

	cmd_type = evhttp_request_get_command(req);
	switch (cmd_type)
	{
		case EVHTTP_REQ_PUT:
		case EVHTTP_REQ_DELETE:
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body)
			{
				log_info(PythonClusterDataLogLevel,
				         "evhttp_request_get_input_buffer returns NULL");
				break;
			}
			if (evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE-1))
			{
				log_error(PythonClusterDataLogLevel,
				          "Policy Buffers Size %d too big, Quitting!!!",
				          evbuffer_get_length(req_body));
				break;
			}
			n = evbuffer_copyout(req_body, large_REST_buffer, (LARGE_REST_BUFFER_SIZE - 1));
			if (n < 1)
			{
				break;
			}
			large_REST_buffer[n] = '\0';

			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_debug(PythonClusterDataLogLevel,"js_root is NULL");
				break;
			}
			/* Get every vnids info */
			js_id = json_object_get(js_root, "policies");

			if (json_is_null(js_id) || !json_is_array(js_id))
			{
				log_info(PythonClusterDataLogLevel,
				         "Domain policy info in the request response is not an array");
				break;
			}
			no_of_policies = json_array_size(js_id);
			for (i = 0; i < no_of_policies; i++)
			{
				status = DOVE_STATUS_INVALID_PARAMETER;
				js_policy = json_array_get(js_id, i);
				if (json_is_null(js_policy))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d error",
					         i);
					break;
				}

				js_node = json_object_get(js_policy,"traffic_type");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d Traffic_Type error",
					         i);
					break;
				}
				traffic_type = (uint32_t)json_integer_value(js_node);

				js_node = json_object_get(js_policy,"type");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d Type error",
					         i);
					break;
				}
				type = (uint32_t)json_integer_value(js_node);

				js_node = json_object_get(js_policy,"src_dvg");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d Src_DVG error",
					         i);
					break;
				}
				src_dvg = (uint32_t)json_integer_value(js_node);

				js_node = json_object_get(js_policy,"dst_dvg");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d Dst_DVG error",
					         i);
					break;
				}
				dst_dvg = (uint32_t)json_integer_value(js_node);

				js_node = json_object_get(js_policy,"ttl");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d TTL error",
					         i);
					break;
				}
				ttl = (uint32_t)json_integer_value(js_node);

				js_node = json_object_get(js_policy,"action");
				if (NULL == js_node || !json_is_integer(js_node))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d Action error",
					         i);
					break;
				}
				action.connectivity = (uint32_t)json_integer_value(js_node);
				action.connectivity = (action.connectivity == DPS_CONNECTIVITY_DROP)?DPS_CONNECTIVITY_DROP:DPS_CONNECTIVITY_ALLOW;
				action.ver = 1;

				if (cmd_type == EVHTTP_REQ_PUT)
				{
					log_info(PythonClusterDataLogLevel,
					         "Adding policy in domain %d sdvg %d ddvg %d "
					         "ttype %d type %d ttl %d action %d",
					         domain, src_dvg, dst_dvg, traffic_type, type, ttl, action);

					status = dps_rest_api_create_policy(domain, src_dvg,
					                                    dst_dvg, traffic_type,
					                                    type, ttl, &action);
					if (status != DOVE_STATUS_OK)
					{
						log_error(PythonClusterDataLogLevel,
						          "Bulk Policy: Could not create policy [%s]",
						          DOVEStatusToString(status));
						break;
					}
				}
				else
				{
					log_info(PythonClusterDataLogLevel,
					         "Deleting policy in domain %d sdvg %d ddvg %d ttype %d",
					         domain, src_dvg, dst_dvg, traffic_type);

					status = dps_rest_api_del_policy(domain, src_dvg, dst_dvg, traffic_type);
					if (status != DOVE_STATUS_OK)
					{
						log_info(PythonClusterDataLogLevel,
						         "Bulk Policy: Could not delete policy [%s]",
						         DOVEStatusToString(status));
						break;
					}

				}
			}
			if (status == DOVE_STATUS_OK)
			{
				res_code = HTTP_OK;
			}
			else
			{
				res_code = HTTP_BADREQUEST;
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
	if (retbuf)
	{
		evbuffer_free(retbuf);
	}
	log_info(PythonClusterDataLogLevel, "Exit");
	return;
}

/*
  POST /api/dove/dps/domains/{domain_id}/bulk_ipv4-subnets
  {
  	  "ip4subnets":{//array of subnets}
  }
*/
void dps_req_handler_domain_bulk_ip4subnets(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_vnid, *js_subnet, *js_ip, *js_mask, *js_mode, *js_gateway;
	json_t *js_res = NULL;
	json_t *js_id = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char res_uri[DPS_URI_LEN];
	int res_code = HTTP_BADREQUEST;
	int n;
	uint32_t vnid, associated_type, associated_id, domain_id;
	json_error_t jerror;
	char *endptr = NULL;
	const char *ip_str, *mask_str, *mode_str, *gateway_str;
	unsigned int ip, mask, mode, gateway;
	int i,total_subnets = 0;
	dove_status status = DOVE_STATUS_OK;

	log_info(RESTHandlerLogLevel,"Enter");
	if (argc != 1 || NULL == argv )
	{
		log_info(RESTHandlerLogLevel,"Argument count invalid");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}
	domain_id = strtoul(argv[0], &endptr, 10);
	if (*endptr != '\0')
	{
		log_info(RESTHandlerLogLevel,"Invalid Domain");
		evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
		return;
	}

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_POST:
		{
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body || evbuffer_get_length(req_body) > (LARGE_REST_BUFFER_SIZE - 1))
			{
				log_debug(RESTHandlerLogLevel,"Could not get input buffer");
				break;
			}
			n = evbuffer_copyout(req_body, large_REST_buffer, (LARGE_REST_BUFFER_SIZE - 1));
			if(n < 1)
			{
				log_debug(RESTHandlerLogLevel,"evbuffer_copyout failed.");
				break;
			}
			large_REST_buffer[n] = '\0';
			js_root = json_loads(large_REST_buffer, 0, &jerror);
			if (!js_root)
			{
				log_debug(RESTHandlerLogLevel,"JSON body NULL");
				break;
			}

			js_id = json_object_get(js_root, "ip4subnets");
			if (json_is_null(js_id))
			{
				log_error(RESTHandlerLogLevel,"js_id NULL...");
				break;
			}
			total_subnets = json_array_size(js_id);
			/* Borrowed reference, no need to decref */
			for (i = 0; i < total_subnets; i++)
			{
				status = DOVE_STATUS_INVALID_PARAMETER;
				js_subnet = json_array_get(js_id, i);
				if (json_is_null(js_subnet))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d error",
					         i);
					break;
				}

				js_vnid = json_object_get(js_subnet, "vnid");
				if (NULL == js_vnid || !json_is_integer(js_vnid))
				{
					log_info(PythonClusterDataLogLevel,
					         "Bulk Policy Process: Element %d VNID error",
					         i);
					break;
				}
				vnid = (uint32_t)json_integer_value(js_vnid);
				if (vnid == 0)
				{
					associated_type = IP_SUBNET_ASSOCIATED_TYPE_DOMAIN;
					associated_id = domain_id;
				}
				else
				{
					associated_type = IP_SUBNET_ASSOCIATED_TYPE_VNID;
					associated_id = vnid;
				}

				js_ip = json_object_get(js_subnet, "ip");
				if (js_ip == NULL || !json_is_string(js_ip))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d IP error",
					         i);
					break;
				}
				if ((ip_str = json_string_value(js_ip)) == NULL)
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d IP error",
					         i);
					break;
				}
				if (0 == inet_pton(AF_INET, ip_str, (void *)&ip))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d IP error",
					         i);
				}

				js_mask = json_object_get(js_subnet, "mask");
				if (!json_is_string(js_mask))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mask error",
					         i);
					break;
				}
				if ((mask_str = json_string_value(js_mask)) == NULL)
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mask error",
					         i);
					break;
				}
				if (0 == inet_pton(AF_INET, mask_str, (void *)&mask))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mask error",
					         i);
					break;
				}

				js_gateway = json_object_get(js_subnet, "gateway");
				if (!json_is_string(js_gateway))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Gateway error",
					         i);
					break;
				}
				if ((gateway_str = json_string_value(js_gateway)) == NULL)
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Gateway error",
					         i);
					break;
				}
				if (0 == inet_pton(AF_INET, gateway_str, (void *)&gateway))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Gateway error",
					         i);
					break;
				}

				js_mode = json_object_get(js_subnet, "mode");
				if (!json_is_string(js_mode))
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mode error",
					         i);
					break;
				}
				if ((mode_str = json_string_value(js_mode)) == NULL)
				{
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mode error",
					         i);
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
					log_info(RESTHandlerLogLevel,
					         "Bulk Subnet Process: Element %d Mode %s bad type",
					         i, mode_str);
					break;
				}
				log_info(RESTHandlerLogLevel,
				         "ip %s, mask %s gw %s mode %s",
				         ip_str, mask_str, gateway_str, mode_str);

				status = dps_rest_api_create_ipsubnet(associated_type,
				                                      associated_id,
				                                      AF_INET,
				                                      (unsigned char *)&ip,
				                                      mask,
				                                      mode,
				                                      (unsigned char *)&gateway);

				if (DOVE_STATUS_OK != status)
				{
					log_error(RESTHandlerLogLevel,
					          "Bulk Subnet Process: Element %d IPSubnet creation failed.");
					break;
				}
			}
			evhttp_add_header(evhttp_request_get_output_headers(req), "Location", res_uri);
			/* CREATED 201 */
			if (status == DOVE_STATUS_OK)
			{
				res_code = 201;
			}
			else
			{
				res_code = HTTP_BADREQUEST;
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
	log_info(RESTHandlerLogLevel,"Exit");
	return;
}
