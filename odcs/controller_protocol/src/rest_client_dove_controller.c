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
*  $Log: rest_client_dove_controller.c $
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
#include "../inc/controller_rest_api.h"
#include "../inc/evhttp_helper.h"
#include "../inc/rest_req_handler.h"
#include "../inc/dove_rest_client.h"
#include "../inc/rest_client_dove_controller.h"

int dcs_role_assigned = 0;

/*
 ******************************************************************************
 * dps_generic_query_synchronous_response_handler --                      *//**
 *
 * \brief This routine can be used to handle GET request for synchronous
 *        operation. The routine copies the buffer provided into a buffer which
 *        can be read by the caller.
 *
 * \param [in]  req  A pointer to a evhttp_request data structure.
 * \param [out] arg  A pointer to a dps_rest_sync_response_args_t data structure
 *                   including a pointer to input buffer and response code.
 *
 * \retval None
 *
 *****************************************************************************/
static void dps_generic_query_synchronous_response_handler(struct evhttp_request *req,
                                                           void *arg)
{
	dps_rest_sync_response_args_t *args = (dps_rest_sync_response_args_t *)arg;
	struct evbuffer *req_body = NULL;
	unsigned int buf_len;
	int n;

	do
	{
		args->res_code = HTTP_NOCONTENT;
		/* sanity check */
		if (req == NULL)
		{
			log_info(RESTHandlerLogLevel,"No evhttp_request");
			break;
		}
		/* extract request body */
		if ((req_body = evhttp_request_get_input_buffer(req)) == NULL)
		{
			log_info(RESTHandlerLogLevel,
			         "No evhttp_request_get_input_buffer");
			break;
		}
		buf_len = evbuffer_get_length(req_body)+1;
		args->req_body_buf = (char *)malloc(buf_len);
		if (args->req_body_buf == NULL)
		{
			log_warn(RESTHandlerLogLevel,
			         "Domain Query Response Handler: malloc fails");
			break;
		}
		n = evbuffer_copyout(req_body, args->req_body_buf, buf_len);
		args->req_body_buf[n]='\0';
		args->res_code = evhttp_request_get_response_code(req);
	}while(0);

	return;
}

static void dps_process_vnids(struct evhttp_request *req)
{
	struct evbuffer *buf;
	int n = 0;
	char *input_buffer = NULL;
	size_t input_buffer_size;
	json_t *js_root = NULL;
	json_t *js_vnid = NULL;
	json_error_t jerror;
	json_t *js_id = NULL;
	uint32_t vnid = 0;
	uint32_t domain_id = 0;
	uint32_t vnid_in_domain = 0;
	unsigned int i;

	log_debug(RESTHandlerLogLevel, "Enter: uri is vnid info, parse it");
	do{

		buf = evhttp_request_get_input_buffer(req);
		if (buf == NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "evhttp_request_get_input_buffer failed!!!");
			break;
		}
		input_buffer_size = evbuffer_get_length(buf);
		input_buffer = (char *) malloc(input_buffer_size+1);
		if(input_buffer == NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "VNIDs Response, cannot allocate buffer of size",
			          input_buffer_size);
			break;
		}
		n = evbuffer_copyout(buf, input_buffer, input_buffer_size);
		if (n < 1)
		{
			log_error(RESTHandlerLogLevel,
			          "VNIDs Response, evbuffer_copyout failed");
			break;
		}
		input_buffer[n] = '\0';
		js_root = json_loads(input_buffer, 0, &jerror);
		if (!js_root)
		{
			break;
		}
		/* Borrowed reference, no need to decref */
		js_id = json_object_get(js_root, "vnid");
		if (json_is_null(js_id) || !json_is_integer(js_id))
		{
			log_info(RESTHandlerLogLevel, "vnid in the request response is not an integer");
			break;
		}

		vnid = (unsigned int) json_integer_value(js_id);
		log_info(RESTHandlerLogLevel, "vnid is %u", vnid);

		js_id = json_object_get(js_root, "domain");
		if (json_is_null(js_id) || !json_is_integer(js_id))
		{
			log_info(RESTHandlerLogLevel, "domain_id in the request response is not an integer");
			break;
		}

		domain_id = (unsigned int) json_integer_value(js_id);
		log_info(RESTHandlerLogLevel, "domain is %u", domain_id);

		// Register the base vnid
		dps_rest_api_create_dvg(domain_id, vnid, DPS_CONTROLLER_DVG_ADD_QUERY);

		/* Get every vnids info */
		js_id = json_object_get(js_root, "vnids");

		if (json_is_null(js_id) || !json_is_array(js_id))
		{
			log_info(RESTHandlerLogLevel,
			         "DCS Domain VNIDs info in the request response is not an Array");
			break;
		}

		for (i = 0; i < json_array_size(js_id); i++)
		{
			js_vnid = json_array_get(js_id, i);
			if (json_is_null(js_vnid) || !json_is_integer(js_vnid))
			{
				log_info(RESTHandlerLogLevel, "node format ERROR, vnid is not Integer");
				break;
			}
			vnid_in_domain = (unsigned int) json_integer_value(js_vnid);
			log_info(RESTHandlerLogLevel, "vnid is %u", vnid_in_domain);
			if (vnid_in_domain == vnid)
			{
				//Already done
				continue;
			}
			/* dvg add */

			if (DOVE_STATUS_OK == dps_rest_api_create_dvg(domain_id,
			                                              vnid_in_domain,
			                                              DPS_CONTROLLER_DVG_ADD_QUERY))
			{
			
				log_debug(RESTHandlerLogLevel, "Dvg add OK, domain_id = %u, vnid = %u",
				domain_id, vnid);
			}else{
				log_error(RESTHandlerLogLevel,
				          "Dvg add error, domain_id = %u, vnid = %u",
				          domain_id, vnid);
			}
		}
	}while(0);

	if (js_root)
	{
		json_decref(js_root);
	}
	if (input_buffer)
	{
		free(input_buffer);
	}

	log_debug(RESTHandlerLogLevel, "Exit");
	return;

}
/* 
 *  \brief Procesing a list of subnets config for the specific vnid. This is in response to a query to the DMC 
 *         for subnet config
 *         /api/dove/dps/vnid/<vnid>/ipv4-subnets
*/

static void dps_process_vnid_subnets(struct evhttp_request *req)
{
	struct evbuffer *req_body;
	size_t buff_len = 0;
	char *buff = NULL;
	json_t *js_entry = NULL, *js_root = NULL, *js_ids = NULL;
	json_t *js_id = NULL;
	json_error_t error;
	const char *ip_str;
	const char *mask_str;
	const char *gateway_str;
	const char *mode_str;
	unsigned int ip, mask, mode, gateway;
	int i, size;
	unsigned int vnid = 0;
	const char *uri;

	log_info(RESTHandlerLogLevel, "Enter");

	uri = evhttp_request_get_uri(req);
	if (sscanf(uri, DOVE_CONTROLLER_VNID_SUBNET_URI, &vnid) != 1)
	{
		log_info(RESTHandlerLogLevel, "Error in parsing vnid from uri");
	}

	req_body = evhttp_request_get_input_buffer(req);
	if (req_body == NULL)
	{
		log_info(RESTHandlerLogLevel,
		          "evhttp_request_get_input_buffer failed!!!");
		goto end;
	}
	buff_len = evbuffer_get_length(req_body);
	
	if((buff = (char *)malloc(buff_len + 1)) == NULL)
	{
		log_info(RESTHandlerLogLevel,
		          "VNID subnet Buffers Size %d too big, Quitting!!!",
		          buff_len);
		goto end;
	}

	evbuffer_copyout(req_body, buff, buff_len);
	buff[buff_len] = '\0';

	log_info(RESTHandlerLogLevel, "Subnet Buffer %s", buff);

	js_root = json_loads(buff, 0, &error);

	if (NULL == js_root)
	{
		log_info(RESTHandlerLogLevel,"JSON body NULL");
		goto end;
	}

	js_ids = json_object_get(js_root, "ipv4-subnets");

	size = json_array_size(js_ids);

	for (i = 0; i < size; i++)
	{
		js_entry = json_array_get (js_ids, i);
		if (NULL == js_entry)
		{
			log_info(RESTHandlerLogLevel, "js_entry = NULL    i = %d\r\n", i);
			goto end;
		}

		js_id = json_object_get(js_entry, "ip");
		if (NULL == js_id )
		{
			log_info(RESTHandlerLogLevel,"js_id for ip is NULL");
			break;
		}
		if (!json_is_string(js_id))
		{
			log_info(RESTHandlerLogLevel,"js_id for ip is NOT string");
			break;
		}
		if((ip_str = json_string_value(js_id)) == NULL)
		{
			log_info(RESTHandlerLogLevel,"ip_str is NULL");
			break;
		}

		js_id = json_object_get(js_entry, "mask");
		if (NULL == js_id)
		{
			log_info(RESTHandlerLogLevel,"js_id for mask is NULL");
			break;
		}
		if (!json_is_string(js_id))
		{
			log_info(RESTHandlerLogLevel,"js_id for mask is NOT string");
			break;
		}
		if((mask_str = json_string_value(js_id)) == NULL)
		{
			log_info(RESTHandlerLogLevel,"mask_str is NULL");
			break;
		}

		js_id = json_object_get(js_entry, "gateway");
		if (NULL == js_id)
		{
			log_info(RESTHandlerLogLevel,"js_id for gateway is NULL");
			break;
		}
		if (!json_is_string(js_id))
		{
			log_info(RESTHandlerLogLevel,"js_id for gateway is NOT string");
			break;
		}
		if((gateway_str = json_string_value(js_id)) == NULL)
		{
			log_debug(RESTHandlerLogLevel,"gateway_str is NULL");
			break;
		}

		js_id = json_object_get(js_entry, "mode");
		if (NULL == js_id || !json_is_string(js_id))
		{
			log_debug(RESTHandlerLogLevel,"js_id is NOT string");
			break;
		}
		if((mode_str = json_string_value(js_id)) == NULL)
		{
			log_debug(RESTHandlerLogLevel,"mode_str is NULL");
			break;
		}

		if (0 == inet_pton(AF_INET, ip_str, (void *)&ip))
		{
			log_info(RESTHandlerLogLevel, "ip address conversion problem i = %d\r\n", i);
			continue;
		}
		if (0 == inet_pton(AF_INET, mask_str, (void *)&mask))
		{
			log_info(RESTHandlerLogLevel, "ip mask conversion problem i = %d\r\n", i);
			continue;
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
			log_info(RESTHandlerLogLevel, "Incorrect mode i = %d", i);
			continue;
		}
		if (0 == inet_pton(AF_INET, gateway_str, (void *)&gateway))
		{
			continue;
		}
		log_info(RESTHandlerLogLevel,"ip %s, mask %s gw %s mode %s", ip_str, mask_str,gateway_str,mode_str);

		// Call to create the subnet
		if (DOVE_STATUS_OK == dps_rest_api_create_ipsubnet(IP_SUBNET_ASSOCIATED_TYPE_VNID,
		                                                   vnid, AF_INET,
		                                                   (unsigned char *)&ip,
		                                                   mask, mode,
		                                                   (unsigned char *)&gateway))
		{
			log_info(RESTHandlerLogLevel,"Successfully created ipsubnet");
		}
	}
end:
	if (buff)
	{
		free(buff);
	}
	if (js_root)
	{
		json_decref(js_root);
	}

	log_info(RESTHandlerLogLevel,"Exit");

	return;
}

static void dps_dove_controller_http_response_handler(struct evhttp_request *req, void *arg)
{
#if defined(NDEBUG)
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
#endif
	struct evbuffer *buf;
	int response_code;
	const char *uri;
	char save_uri[256];
	char save_uri_token[256];

	log_debug(RESTHandlerLogLevel, "Enter");

	do
	{
		if (NULL == req)
		{
			break;
		}
		uri = evhttp_request_get_uri(req);
		if (uri == NULL)
		{
			log_info(RESTHandlerLogLevel,
			         "evhttp_request_get_uri returned NULL");
			break;
		}
		memset(save_uri,0, 256);

#if defined(NDEBUG)
		switch (evhttp_request_get_command(req)) {
			case EVHTTP_REQ_GET: cmdtype = "GET"; break;
			case EVHTTP_REQ_POST: cmdtype = "POST"; break;
			case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
			case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
			case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
			case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
			case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
			case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
			case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
			default: cmdtype = "unknown"; break;
		}
#endif
		response_code = evhttp_request_get_response_code(req);
		if (response_code != HTTP_OK)
		{
			log_notice(RESTHandlerLogLevel,
			           "FAILED, the response code is NOT HTTP_OK [%d], "
			           "URI [%s] from Host [%s]",
			           response_code, uri,
			           evhttp_request_get_host(req));

			/* Trevor: If the req is response to the heartbeat and the code is NOT HTTP_OK, we need to
			send the registration again as the DMC may be rebooted
			*/
			if(URI_IS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI(uri))
			{
				log_notice(RESTHandlerLogLevel,"DCS Heartbeat failed, Send Registration again!");
				set_dps_appliance_registration_needed(1);
			}

			//If this is answer to DPS_APPLIANCE_REGISTRATION_URI, check it
			/* Bug number: */
			if(helper_uri_is_same(DPS_APPLIANCE_REGISTRATION_URI, uri))
			{
				dps_registration_fail_count ++;
				if(dps_registration_fail_count%10 == 0)
				{
					log_alert(RESTHandlerLogLevel,
					           "DCS Appliance Registration to DMC failed %d times", dps_registration_fail_count);
					show_print("DCS registration to DMC had failed %d times", dps_registration_fail_count);
				}

				do{
					buf = evhttp_request_get_input_buffer(req);
					char tmpbuf[256] = {0};
					if (!buf)
					{
						log_info(RESTHandlerLogLevel, "No Buffer");
						break;
					}
					if (evbuffer_get_length(buf) > (256-1))
					{
						log_error(RESTHandlerLogLevel, "Buffer Size %d too big",
						          evbuffer_get_length(buf));
						break;
					}
					int nnnn = evbuffer_copyout(buf, tmpbuf, (256 - 1));
					if (nnnn < 1)
					{
						log_error(RESTHandlerLogLevel, "evbuffer_copyout error");
						break;
					}
					tmpbuf[nnnn] = '\0';
					show_print("DCS: Registration with DMC failed [%s]",tmpbuf);
					log_alert(RESTHandlerLogLevel,
					          "DCS: Registration with DMC failed, [%s]",tmpbuf);
				}while(0);
			}
			break;
		}

#if defined(NDEBUG)
		log_debug(RESTHandlerLogLevel,
		          "Received a %s response for %s",
		          cmdtype, evhttp_request_get_uri(req));
#endif
		strcpy((char *)save_uri, (const char *)uri);
#if defined(NDEBUG)
		log_debug(RESTHandlerLogLevel, "Headers: <<<");
		headers = evhttp_request_get_input_headers(req);
		for (header = headers->tqh_first;
		     header;
		     header = header->next.tqe_next)
		{
			log_debug(RESTHandlerLogLevel,
			          "Key [%s]: Value [%s]",
			          header->key,
			          header->value);
		}
		log_debug(RESTHandlerLogLevel, "Headers: >>>");
#endif

		//If this is answer to DPS_APPLIANCE_REGISTRATION_URI, check it
		if(helper_uri_is_same(DPS_APPLIANCE_REGISTRATION_URI,
		                      (const char *)save_uri))
		{
			log_info(RESTHandlerLogLevel,
			         "DPS Appliance Registration response handler");
			if (evhttp_request_get_response_code(req) == HTTP_OK)
			{
				log_notice(RESTHandlerLogLevel,
				           "DCS: Registration with DMC successful!");
				set_dps_appliance_registration_needed(0);
				dps_registration_fail_count = 0;
			}
		}

		//If this is answer to DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI, check it
		if(helper_uri_is_same(DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI,
		                      (const char *)save_uri))
		{
			log_info(RESTHandlerLogLevel,
			         "DPS Heartbeat response handler");
			if (evhttp_request_get_response_code(req) == HTTP_OK)
			{
				log_debug(RESTHandlerLogLevel,
				          "DCS Heartbeat to DMC successful!");
				last_heartbeat_to_dmc_is_success = 1;
			}
		}

		strcpy((char *)save_uri_token, (const char *)evhttp_request_get_uri(req));
		//If this is answer to DOVE_CONTROLLER_VNID_INFO_URI_GEN, process the VNIDs
		if(helper_uri_is_same_pattern(DPS_DOVE_CONTROLLER_QUERY_DPS_VNID_INFO_URI,
		                              save_uri_token))
		{
			log_info(RESTHandlerLogLevel,
			           "VNID Query Response Handler");
			dps_process_vnids(req);
		}

		buf = evhttp_request_get_input_buffer(req);
		log_debug(RESTHandlerLogLevel, "Input data: <<<");
		while (evbuffer_get_length(buf))
		{
			int n;
			char cbuf[1024];
			memset((void *)cbuf, 0, sizeof(cbuf));
			n = evbuffer_remove(buf, cbuf, sizeof(cbuf)-1);
			if (n > 0)
			{
				log_debug(RESTHandlerLogLevel,"%s", cbuf);
			}
		}
		log_debug(RESTHandlerLogLevel,">>>");
	}while(0);

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}


static void dps_cluster_response_handler(struct evhttp_request *req, void *arg)
{
#if defined(NDEBUG)
	const char *cmdtype;
#endif
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_error_t jerror;
	int response_code;
	struct evbuffer *buf;
	char *input_buffer = NULL;
	size_t input_buffer_size;
	const char *domain_str;
	const char *ip;
	int n,status;
	unsigned int port = 0;

	log_info(RESTHandlerLogLevel, "Enter");

	do
	{
		if (NULL == req)
		{
			break;
		}
#if defined(NDEBUG)
		switch (evhttp_request_get_command(req)) {
		case EVHTTP_REQ_GET: cmdtype = "GET"; break;
		case EVHTTP_REQ_POST: cmdtype = "POST"; break;
		case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
		case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
		case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
		case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
		case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
		case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
		case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
		default: cmdtype = "unknown"; break;
		}
#endif

		response_code = evhttp_request_get_response_code(req);
		if ((response_code != HTTP_OK) && (response_code != 201))
		{
			log_info(RESTHandlerLogLevel,
			         "Response FAILED %d for URI [%s]",
			         response_code, evhttp_request_get_uri(req));
			break;
		}

#if defined(NDEBUG)
		log_debug(RESTHandlerLogLevel, "Received a %s request for %s",
		          cmdtype, evhttp_request_get_uri(req));
#endif

		if(0 != strcmp(evhttp_request_get_uri(req),DPS_CLUSTER_LOCAL_DOMAINS_URI))
		{
			// do not process further
			log_info(RESTHandlerLogLevel, "No need to process further");
			break;
		}
		buf = evhttp_request_get_input_buffer(req);
		if (!buf)
		{
			log_info(RESTHandlerLogLevel, "No Buffer");
			break;
		}

		input_buffer_size = evbuffer_get_length(buf);
		input_buffer = (char *) malloc(input_buffer_size+1);
		if(input_buffer == NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "Domain Mapping Response: cannot allocate buffer %d",
			          input_buffer_size);
			break;
		}
		n = evbuffer_copyout(buf, input_buffer, input_buffer_size);
		if (n < 1)
		{
			log_error(RESTHandlerLogLevel,
			          "Domain Mapping Response: evbuffer_copyout failed");
			break;
		}
		input_buffer[n] = '\0';
		js_root = json_loads(input_buffer, 0, &jerror);

		if (!js_root)
		{
			log_error(RESTHandlerLogLevel,
			          "Domain Mapping Response: js_root is NULL...");
			break;
		}
		/* Borrowed reference, no need to decref */
		js_id = json_object_get(js_root, "ip");
		if (json_is_null(js_id))
		{
			log_debug(RESTHandlerLogLevel,"could not get ip field...");
			break;
		}
		if (NULL == (ip = json_string_value(js_id)))
		{
			break;
		}
		js_id = json_object_get(js_root, "port");
		if (NULL == js_id || !json_is_integer(js_id))
		{
			log_debug(RESTHandlerLogLevel,"port field invalid...");
			break;
		}
		port = (unsigned int)json_integer_value(js_id);

		js_id = json_object_get(js_root, "domains");
		if (json_is_null(js_id))
		{
			log_debug(RESTHandlerLogLevel,"could not get domains field...");
			break;
		}
		if (NULL == (domain_str = json_string_value(js_id)))
		{
			log_debug(RESTHandlerLogLevel,"domain field is not string...");
			break;
		}
		status = dps_cluster_set_node_domain_mapping((char *)ip, port,
		                                             (char *)domain_str);
		if(status != DOVE_STATUS_OK)
		{
			log_alert(RESTHandlerLogLevel,
			          "Error in dps_cluster_set_node_domain_mapping (%d)",
			          status);
		}
	}while(0);

	if (js_root)
	{
		json_decref(js_root);
	}
	if (input_buffer)
	{
		free(input_buffer);
	}

	log_info(RESTHandlerLogLevel, "Exit");

	return;
}

int dps_rest_client_dove_controller_send_asyncprocess(
	char *address, char *uri, unsigned short port,
	enum evhttp_cmd_type type, struct evhttp_request *request)
{
	dove_status status = DOVE_STATUS_OK;
	dove_rest_request_info_t *rinfo;

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		rinfo = (dove_rest_request_info_t *)malloc(sizeof(dove_rest_request_info_t));
		if(rinfo==NULL)
		{
			log_error(RESTHandlerLogLevel,
			          "ALERT!!!Can not alloc the dove_rest_request_info_t");
			status = DOVE_STATUS_NO_MEMORY;
			break;
		}
		memset(rinfo, 0, sizeof(dove_rest_request_info_t));
		DOVE_REST_REQ_INFO_INIT(rinfo,
		                        address,
		                        uri,
		                        port,
		                        type,
		                        request);
		dove_rest_request_and_asyncprocess(rinfo);
	}while(0);

	log_debug(RESTHandlerLogLevel, "Exit: Status %s",
	          DOVEStatusToString(status));
	return status;
}

static int dps_rest_client_dove_controller_send_syncprocess(
	char *address, char *uri, unsigned short port,
	enum evhttp_cmd_type type, struct evhttp_request *request)
{

	dove_rest_request_and_syncprocess(address,
	                                  port,
	                                  type,
	                                  uri,
	                                  request,
	                                  NULL,
	                                  20);
	return DOVE_STATUS_OK;
}



int dps_rest_client_dove_controller_send(char *address,
	char *uri, unsigned short port, enum evhttp_cmd_type type, 
	struct evhttp_request *request, enum evhttp_send_mode mode)
{

		if(mode == EVHTTP_SEND_ASYNCHRONOUS) 
		{

			dove_rest_request_info_t *rinfo;
			rinfo = (dove_rest_request_info_t *)malloc(sizeof(dove_rest_request_info_t));
			if(rinfo==NULL)
			{
				log_debug(RESTHandlerLogLevel,
				          "Can not alloc the dove_rest_request_info_t");
				return DPS_ERROR;
			}
			memset(rinfo, 0, sizeof(dove_rest_request_info_t));
			DOVE_REST_REQ_INFO_INIT(rinfo,
			                        address,
			                        uri,
			                        port,
			                        type,
			                        request);
			dove_rest_request_and_asyncprocess(rinfo);
		}else
		{
			dove_rest_request_and_syncprocess(address,
			                                  port,
			                                  type,
			                                  uri,
			                                  request,
			                                  NULL,
			                                  20);
		}
		return DOVE_STATUS_OK;
}


/* Construct and fill the http request */
int dps_rest_client_dove_controller_fill_evhttp(struct evhttp_request *req, json_t *js_res)
{
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	char host_header_str[150];

	log_debug(RESTHandlerLogLevel, "Enter");
	memset(host_header_str,0, 150);

	/* For GET request, the js_res may be NULL*/
	if(js_res == NULL)
	{
		log_debug(RESTHandlerLogLevel,"js_res is NULL, No json resource!!!");
	}
	else
	{
		res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
		if (NULL == res_body_str)
		{
			log_alert(RESTHandlerLogLevel, "JSON string is NULL or Bad");
			return DOVE_STATUS_INVALID_PARAMETER;
		}
		retbuf = evbuffer_new();
		if (NULL == retbuf)
		{
			free(res_body_str);
			log_error(RESTHandlerLogLevel, "retbuf = evbuffer_new() ERROR");
			//break;
			return DOVE_STATUS_NO_MEMORY;
		}

		log_debug(RESTHandlerLogLevel,
		          "strlen(res_body_str) = %d, content is %s",
		          strlen((const char *)res_body_str), res_body_str);

		evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);
		evbuffer_add_buffer(evhttp_request_get_output_buffer(req), retbuf);
		free(res_body_str);
		res_body_str = NULL;
	}
	/* No need to add "content-length, as evhttp will add it for POST and PUT, and
	GET should not have it */
#if 0

	if(evhttp_find_header(req->output_headers, "Content-Length") == NULL){
		snprintf(size, sizeof(size), "%lu",
		         (unsigned long)(evbuffer_get_length(req->output_buffer)));
		evhttp_add_header(req->output_headers, "Content-Length", size);
	}
#endif


	log_debug(RESTHandlerLogLevel,
	          "Controller IP ==>> [%s : %d]",
	          controller_location_ip_string,
	          controller_location.port_http);

	if (controller_location.port_http != HTTPD_DEFAULT_PORT)
	{
		sprintf(host_header_str,"%s:%d",
		        controller_location_ip_string,
		        controller_location.port_http);
	}
	else
	{
		sprintf(host_header_str, "%s", controller_location_ip_string);
	}

	if(evhttp_find_header(req->output_headers, "Host") == NULL){
		evhttp_add_header(evhttp_request_get_output_headers(req),
		                  "Host", host_header_str);
	}

	if(evhttp_find_header(req->output_headers, "Content-Type") == NULL)
	{
		evhttp_add_header(evhttp_request_get_output_headers(req), 
		"Content-Type", "application/json");
	}

	log_debug(RESTHandlerLogLevel, "Exit");
	return DOVE_STATUS_OK;
}

struct evhttp_request *dps_rest_client_dove_controller_request_new(void)
{
	struct evhttp_request *request = NULL;
	request = evhttp_request_new(dps_dove_controller_http_response_handler, NULL);
	return request;
}

struct evhttp_request *dps_cluster_request_new(void)
{
	struct evhttp_request *request = NULL;
	request = evhttp_request_new(dps_cluster_response_handler, NULL);
	return request;
}

json_t *dps_form_json_endpoint_update_and_conflict_json(
	uint32_t domain_id, uint32_t dvg1, uint32_t dvg2, uint32_t version,
	char *vMac1, char *vMac2, char *pIP_address1, char *pIP_address2,
	char *vIP_addr, uint32_t family)
{
	json_t *js_root = NULL;
	json_t *js_dps_node = NULL;
	char str[256];

	inet_ntop(AF_INET, &dps_local_ip.ip4, str, INET_ADDRSTRLEN);

	js_dps_node = json_pack("{s:i, s:s, s:s, s:s}",
	                    "family", (int)dps_local_ip.family,
	                    "ip", str,
	                    "UUID", dps_node_uuid,
	                    "Cluster_Leader", dps_cluster_leader_ip_string);

	js_root = json_pack("{s:o, s:i, s:i, s:i, s:s, s:s, s:i, s:s, s:s, s:s}", 
	                    "dps_node", js_dps_node,
	                    "id", (int)domain_id,
	                    "dvg1", dvg1,
	                    "dvg2", dvg2,
	                    "vm1_MAC", vMac1,
	                    "vm2_MAC", vMac2,
	                    "family",  (int)family,
	                    "phy_IP1",  pIP_address1,
	                    "phy_IP2",  pIP_address2,
	                    "vIP", vIP_addr);

	return js_root;

}

void dps_rest_client_send_endpoint_conflict_to_dove_controller(
	uint32_t domain_id, uint32_t dvg1, uint32_t dvg2, uint32_t version,
	unsigned char *vMac1, unsigned char *vMac2, char *pIP_address1, char *pIP_address2,
	char *vIP_addr, uint32_t family)
{
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char mac1_str[128], mac2_str[128];

	log_notice(RESTHandlerLogLevel,
	           "vMac1 = " MAC_FMT ", vMac2 = " MAC_FMT ", pIP1 = %s, pIP2 = %s, vIP = %s",
	           MAC_OCTETS(vMac1), MAC_OCTETS(vMac2), pIP_address1, pIP_address2, vIP_addr);

	memset(mac1_str,0,128);
	memset(mac2_str,0,128);

	sprintf(mac1_str, MAC_FMT, MAC_OCTETS(vMac1));
	sprintf(mac2_str, MAC_FMT, MAC_OCTETS(vMac2));

	/* Get the json string */
	js_res = dps_form_json_endpoint_update_and_conflict_json( domain_id,
	                                                          dvg1,
	                                                          dvg2,
	                                                          version,
	                                                          mac1_str,
	                                                          mac2_str,
	                                                          pIP_address1,
	                                                          pIP_address2,
	                                                          vIP_addr,
	                                                          family);
	do
	{
		if(js_res == NULL){
			log_alert(RESTHandlerLogLevel,
			          "No js_res,just return, not send anything!");
			return;
		}

		request = dps_rest_client_dove_controller_request_new();
		if(request == NULL) {
			log_alert(RESTHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request, js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		//set the uri
		DOVE_CONTROLLER_ENDPOINT_CONFLICT_URI_GEN(uri, domain_id);

		//send it synchronously, maybe it should asynchronously to Dove Controller
#if 0
		dps_rest_client_dove_controller_send_syncprocess(
			controller_location_ip_string, uri,
			controller_location.port_http,EVHTTP_REQ_POST,
			request);
#else
		dps_rest_client_dove_controller_send_asyncprocess(
			controller_location_ip_string, uri,
			controller_location.port_http, EVHTTP_REQ_POST,
			request);
#endif
	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return;
}

/*
 ******************************************************************************
 * dps_rest_client_json_send_to_dove_controller --                        *//**
 *
 * \brief A more general sending function for PUSHING something to the DMC.
 *        It encapsulates the json string into a http request and sends it to
 *        the Dove Controller based on the uri and DC's address info
 *
 * \param[in] js_res The JSON Body as a string. NOTE that this routine will
 *                   free the JSON string, so the calling MUST NOT touch
 *                   the JSON Body after this call is made
 * \param[in] uri The URI in String Format
 * \param[in] cmd_type Command Type (POST/PUT/GET etc).
 *
 * \note The calling routine MUST NOT touch the js_res parameter after this
 *       routine is invoked
 *
 * \retval None
 *
 ******************************************************************************/
void dps_rest_client_json_send_to_dove_controller(json_t *js_res,
                                                  char *uri,
                                                  enum evhttp_cmd_type cmd_type)
{

	struct evhttp_request *request;
	char host_header_str[150];

	log_info(RESTHandlerLogLevel, "Enter - URI %s", uri);
	memset(host_header_str,0, 150);
	do
	{
		if(js_res == NULL)
		{
			log_info(RESTHandlerLogLevel, "URI %s - NO js_res, Still sending", uri);
			//printf_console("%s, json is NULL!",__FUNCTION__);
			/* For GET request, there should be no js_res ,so it is ok to have js_res == NULL */
			//return;
		}

		request = dps_rest_client_dove_controller_request_new();
		if(request == NULL)
		{
			log_alert(RESTHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		log_debug(RESTHandlerLogLevel,
		          "Controller IP ==>> [%s : %d]",
		          controller_location_ip_string,
		          controller_location.port_http);

		if (controller_location.port_http != 80)
		{
			sprintf(host_header_str, "%s:%d",
			        controller_location_ip_string,
			        controller_location.port_http);
		}
		else
		{
			sprintf(host_header_str,"%s", controller_location_ip_string);
		}
		log_debug(RESTHandlerLogLevel, "host_header_str %s", host_header_str);

		if(evhttp_find_header(request->output_headers, "Host") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(request),
			                  "Host", host_header_str);
		}

		
		if(evhttp_find_header(request->output_headers, "Content-Type") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(request), 
			"Content-Type", "application/json");
		}

		//send it synchronously, maybe it should asynchronously to Dove Controller
#if 0
		dps_rest_client_dove_controller_send_syncprocess(
			controller_location_ip_string, uri,
			controller_location.port_http, cmd_type,
			request);
#else
		dps_rest_client_dove_controller_send_asyncprocess(
			controller_location_ip_string, uri,
			controller_location.port_http, cmd_type,
			request);
#endif
	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(RESTHandlerLogLevel, "Exit", uri);
	return;
}

/*
 ******************************************************************************
 * dps_form_cluster_leader_json                                        *//**
 *
 * \brief - This function is called to construct a json string to report the
 * 			DPS Cluster leader to the DOVE Controller.
 * \retval  packed json body.
 *
 ******************************************************************************
 */

json_t *dps_form_cluster_leader_json()
{
	json_t *js_root = NULL;
	int node_id = 1;
	char ipstr[INET6_ADDRSTRLEN];

	inet_ntop(dps_local_ip.family, dps_local_ip.ip6, ipstr, INET6_ADDRSTRLEN);

	js_root = json_pack("{s:i,s:i,s:s,s:i,s:i,s:s}",
	                    "id",node_id,
	                    "ip_family",dps_local_ip.family,
	                    "ip",ipstr,
	                    "service_port",dps_local_ip.port,
	                    "rest_port",dps_rest_port,
	                    "uuid",dps_node_uuid
	                   );
	return js_root;

}

/*
 ******************************************************************************
 * dps_cluster_leader_reporting                                           *//**
 *
 * \brief - This function is called by the Cluster Leader to report to the
 *          DOVE Controller (DMC)
 * \retval - None
 ******************************************************************************
 */

void dps_cluster_leader_reporting()
{
	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char host_header_str[150];

	log_info(RESTHandlerLogLevel, "Enter");

	do {
		/* form json string*/
		js_res = dps_form_cluster_leader_json();

		if(js_res == NULL)
		{
			log_alert(RESTHandlerLogLevel,"Can not get the js_res");
			break;
		}
		// set the uri
		snprintf(uri,DPS_URI_LEN,DPS_CLUSTER_LEADER_URI);

		request = dps_rest_client_dove_controller_request_new();
		if(request == NULL) {
			log_alert(RESTHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		memset(host_header_str,0,150);

		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}
		log_info(RESTHandlerLogLevel,
		         "Sending Local as Leader to Controller");

		log_debug(RESTHandlerLogLevel,
		          "Controller IP ==>> [%s : %d]",
		          controller_location_ip_string,
		          controller_location.port_http);
		if (controller_location.port_http != HTTPD_DEFAULT_PORT) {
			sprintf(host_header_str,"%s:%d",
			        controller_location_ip_string,
			        controller_location.port_http);
		}
		else {
			sprintf(host_header_str,"%s",
			        controller_location_ip_string);
		}

		if(evhttp_find_header(request->output_headers, "Host") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(request),
			"Host", host_header_str);
		}

		if(evhttp_find_header(request->output_headers, "Content-Type") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(request), 
			"Content-Type", "application/json");
		}

		dps_rest_client_dove_controller_send_asyncprocess(
			controller_location_ip_string, uri,
			controller_location.port_http, EVHTTP_REQ_POST, request
			);
	} while(0);

	if (js_res)
	{
		json_decref(js_res);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/*
 ******************************************************************************
 * dps_form_appliance_registration_json                                   *//**
 *
 * \brief - This function is called to construct a json string to register the
 *          DPS Appliance to the DOVE Controller.
 *
 * \retval  packed json body.
 *
 ******************************************************************************
 */

static json_t *dps_form_appliance_registration_json()
{
	json_t *js_root = NULL;
	char ipstr[INET6_ADDRSTRLEN];
	inet_ntop(dps_local_ip.family, dps_local_ip.ip6, ipstr, INET6_ADDRSTRLEN);

	js_root = json_pack("{s:i,s:s,s:s,s:i,s:i,s:i,s:s}",
	                    "ip_family", dps_local_ip.family,
	                    "ip",ipstr,
	                    "uuid",dps_node_uuid,
	                    "rest_port",dps_rest_port,
	                    "service_port",dps_local_ip.port,
	                    "role_assigned",dcs_role_assigned,
	                    "build_version", dsa_version_string
	                   );

	return js_root;

}

/*
 ******************************************************************************
 * dps_appliance_registration                                        *//**
 *
 * \brief - This function is called to register a DPS (DCS) appliance with
 * 	    the Controller (DMC)
 * \retval - None
 ******************************************************************************
 */

void dps_appliance_registration()
{
	json_t *js_res = NULL;
	char uri[256];

	log_info(RESTHandlerLogLevel, "Enter");
	/* Set the retry flag */
	log_info(RESTHandlerLogLevel, "Before registration, set DPS Appliance Registration Retry flag");

	do {
		set_dps_appliance_registration_needed(1);
		if (!controller_location_set)
		{
			log_info(RESTHandlerLogLevel, "Controller not set by user");
			break;
		}
		if (dps_local_ip.ip4 == htonl(INADDR_LOOPBACK))
		{
			log_info(RESTHandlerLogLevel, "Local IP not yet set. Not point in registering");
			break;
		}
		/* form json string*/
		js_res = dps_form_appliance_registration_json();

		if(js_res == NULL)
		{
			log_alert(RESTHandlerLogLevel, "Can not get the js_res");
			break;
		}
		// set the uri
		memset(uri, 0, DPS_URI_LEN);
		//snprintf(uri,DPS_URI_LEN,DPS_APPLIANCE_REGISTRATION_URI);
		strncpy(uri, DPS_APPLIANCE_REGISTRATION_URI, DPS_URI_LEN);
		log_info(RESTHandlerLogLevel, "URI %s", uri);
		dps_rest_client_json_send_to_dove_controller(js_res,
		                                             uri,
		                                             EVHTTP_REQ_POST);
	} while(0);

	log_info(RESTHandlerLogLevel,"Exit");
	return;
}

/* 
 *  \brief Use the new endpoint_conflict_t structure to get the related parameters sent to Dove Controller
*/
void dps_rest_client_send_endpoint_conflict_to_dove_controller2(dps_rest_client_to_dove_controller_endpoint_conflict_t *endpoint_conflict_msg)
{

	json_t *js_res = NULL;
	struct evhttp_request *request;
	char uri[256];
	char str1[INET6_ADDRSTRLEN], str2[INET6_ADDRSTRLEN], str3[INET6_ADDRSTRLEN];

	memset((void *)str1,0, sizeof(str1));
	memset((void *)str2,0, sizeof(str2));
	memset((void *)str3,0, sizeof(str3));

	//Form the request body, first construct the json string
	if(endpoint_conflict_msg->physical_addr1.family == AF_INET)
	{
		//phyIP1, the phy IP got by lookup
		inet_ntop(AF_INET, &endpoint_conflict_msg->physical_addr1.ip4, str1, INET_ADDRSTRLEN);
		//phyIP2, the phy IP in the update msg 
		inet_ntop(AF_INET, &endpoint_conflict_msg->physical_addr2.ip4, str2, INET_ADDRSTRLEN);
		//virtual IP, which is the same for the one in local stored and in the update msg
		inet_ntop(AF_INET, &endpoint_conflict_msg->virtual_addr.ip4, str3, INET_ADDRSTRLEN);
	}else /* IPv6 */{
		//phyIP1, the phy IP got by lookup
		inet_ntop(AF_INET6, endpoint_conflict_msg->physical_addr1.ip6, str1, INET6_ADDRSTRLEN);
		//pIPv6 = pIP_ret_packed;
		//phyIP2, the phy IP in the update msg
		inet_ntop(AF_INET6, endpoint_conflict_msg->physical_addr2.ip6, str2, INET6_ADDRSTRLEN);
		//pIPv6_2 = pIP_address.ip6;
		//virtual IP, which is the same for the one in local stored and in the update msg
		//vIPv6 = (char *)endpoint_update_msg->virtual_addr.ip6;
		inet_ntop(AF_INET6, endpoint_conflict_msg->virtual_addr.ip6, str3, INET6_ADDRSTRLEN);
	}

	do
	{
		/* Get the json string */
		char mac1_str[128], mac2_str[128];

		memset(mac1_str,0,128);
		memset(mac2_str,0,128);

		sprintf(mac1_str, MAC_FMT, MAC_OCTETS(endpoint_conflict_msg->vMac1));
		sprintf(mac2_str, MAC_FMT, MAC_OCTETS(endpoint_conflict_msg->vMac2));

		js_res = dps_form_json_endpoint_update_and_conflict_json(
				endpoint_conflict_msg->domain,
				endpoint_conflict_msg->dvg1,
				endpoint_conflict_msg->dvg2,
				endpoint_conflict_msg->version,
				(char *)endpoint_conflict_msg->vMac1,
				(char *)endpoint_conflict_msg->vMac2, str1, str2,
				str3, endpoint_conflict_msg->physical_addr1.family);


		if(js_res == NULL)
		{
			log_alert(RESTHandlerLogLevel,"Can not get the js_res");
			break;
		}

		request = dps_rest_client_dove_controller_request_new();
		if(request == NULL) {
			log_alert(RESTHandlerLogLevel,"Can not alloc the evhttp request");
			break;
		}

		if (dps_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		//set the uri
		DOVE_CONTROLLER_ENDPOINT_CONFLICT_URI_GEN(uri, endpoint_conflict_msg->domain);

		//send it synchronously, maybe it should asynchronously to Dove Controller
		dps_rest_client_dove_controller_send_syncprocess(controller_location_ip_string,
		                                                 uri,
		                                                 controller_location.port_http,
		                                                 EVHTTP_REQ_POST,
		                                                 request);

	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return;

}

/*
*\Brief:
* The current DPS node send a query to Dove Controller to get the cluster information,
* Dove Controller should responds with the following info:
* 1. The Cluster Leader IP who the DC thinks should be;
* 2. the cluster nodes, which should contain: node ip, node port, node UUID
* the current implementation first send a node-number to indicate the number of the cluster nodes ,
* then send a json array to show the info of each node : ip,port,UUID.
* The query is sent by Restful HTTP Get Request, which contains:
* the current node info: family, node ip, port, UUID
*/

/*
	char ipstr[INET6_ADDRSTRLEN];
	inet_ntop(dps_local_ip.family, dps_local_ip.ip6,
	          ipstr, INET6_ADDRSTRLEN);
	//form json string
	js_res = json_pack("{s:s,s:i,s:s}",
			"ip",ipstr,
			"port",(int)dps_local_ip.port,
			"domains", local_domain_str);
*/
//extern char uuid[];
static char dps_query_dove_controller_cluster_info_uri[] = DPS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO_URI;

/* Form the query cluster json string */
static json_t *dps_form_query_dove_controller_cluster_info_json(uint32_t family,
                                                                char *node_IP,
                                                                uint32_t port,
                                                                char *UUID)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:i, s:s, s:i, s:s}", 
	                    "family", (int)family,
	                    "node_ip", node_IP,
	                    "port", port,
	                    "UUID",  dps_node_uuid);
	return js_root;

}

///* Form the query vnid json string */
//static json_t *dps_form_query_dove_controller_vnid_info_json(uint32_t family, char *node_IP,
//                                                             uint32_t port,
//                                                             char *UUID, uint32_t vnid)
//{
//	json_t *js_root = NULL;
//	json_t *js_res = NULL;
//
//	log_debug(RESTHandlerLogLevel, "Enter");
//	js_res = json_pack("{s:i, s:s, s:i, s:s}",
//	                   "family", (int)family,
//	                   "node_ip", node_IP,
//	                   "port", port,
//	                   "UUID",  dps_node_uuid);
//
//	js_root = json_pack("{s:o, s:i}",
//	                    "node", js_res,
//	                    "vnid", vnid);
//
//	log_debug(RESTHandlerLogLevel, "Exit");
//	return js_root;
//}

/* Send the Restful Get Request to Dove Controller for all DPS cluster nodes info */
void dps_rest_client_query_dove_controller_cluster_info()
{
	json_t *js_res = NULL;
	char ipstr[INET6_ADDRSTRLEN];

	log_debug(RESTHandlerLogLevel, "Enter");

	inet_ntop(dps_local_ip.family, dps_local_ip.ip6, ipstr, INET6_ADDRSTRLEN);
	js_res = dps_form_query_dove_controller_cluster_info_json(dps_local_ip.family, 
	                                                          ipstr, dps_local_ip.port,
	                                                          dps_node_uuid);
	if(js_res)
	{
		log_info(RESTHandlerLogLevel, "Send Query for cluster info to Dove Controller");
		/* There should be no Body for GET request */
		json_decref(js_res);
		js_res = NULL;
		dps_rest_client_json_send_to_dove_controller(js_res,
		                                             dps_query_dove_controller_cluster_info_uri,
		                                             EVHTTP_REQ_GET);
	}else
	{
		log_error(RESTHandlerLogLevel, "ERROR: js_res is NULL!!!");
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

/* Send the Restful Get Request to Dove Controller for all DPS cluster nodes info */
void dps_rest_client_query_dove_controller_vnid_info(uint32_t vnid)
{
	char uri[DOVE_CONTROLLER_URI_LEN];

	log_info(RESTHandlerLogLevel, "Enter");
	memset(uri,0, DOVE_CONTROLLER_URI_LEN);
	log_info(RESTHandlerLogLevel, "Send Query for vnid info to Dove Controller");
	/* Get the VNID uri */
	DOVE_CONTROLLER_VNID_INFO_URI_GEN(uri, vnid);
	log_info(RESTHandlerLogLevel, "uri is %s", uri);
	/* Send to DC */
	dps_rest_client_json_send_to_dove_controller(NULL, uri, EVHTTP_REQ_GET);
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

static void dps_vnid_subnet_config_response_handler(struct evhttp_request *req, void *arg)
{

	log_info(RESTHandlerLogLevel, "Enter req %p arg %p",req, arg);
	if (req == NULL)
	{
		log_debug(RESTHandlerLogLevel, "Request timed out");
		*((int *)arg) = 408;
	}
	else
	{
		*((int *)arg) = evhttp_request_get_response_code(req);
		log_info(RESTHandlerLogLevel, "Resp Code = %d", *((int *)arg));
	}
	if (*((int *)arg) == HTTP_OK)
	{
		log_info(RESTHandlerLogLevel,
		         "HTTP_OK: GET response handler for query VNID subnet info!");
		dps_process_vnid_subnets(req);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/* 
 *  \brief Query the DMC for subnet config for the specific vnid. The response may contain 1 or more
 *         subnets associated with the vnid.
 *         /api/dove/dps/vnid/<vnid>/ipv4-subnets
*/

int dps_query_dmc_vnid_subnet_config(uint32_t vnid)
{
	char uri[DOVE_CONTROLLER_URI_LEN];
	int args = HTTP_OK;
	int ret = 0;
	struct evhttp_request *request = NULL;

	log_info(RESTHandlerLogLevel, "Enter: VNID %d", vnid);
	do
	{
		memset(uri,0, DOVE_CONTROLLER_URI_LEN);
		/* Get the VNID uri */
		DOVE_CONTROLLER_VNID_SUBNET_URI_GEN(uri, vnid);
		log_info(RESTHandlerLogLevel, "uri is %s", uri);

		request = evhttp_request_new(dps_vnid_subnet_config_response_handler, &args);
		if(request == NULL)
		{
			log_info(RESTHandlerLogLevel,"Can not alloc the evhttp request");
			ret = -1;
			break;
		}
		if (dps_rest_client_dove_controller_fill_evhttp(request, NULL) != DOVE_STATUS_OK)
		{
			ret = -1;
			break;
		}
		/* Send to DMC */
		ret = dove_rest_request_and_syncprocess(controller_location_ip_string, controller_location.port_http,
		                                        EVHTTP_REQ_GET, uri, request, NULL, 3);

		if (ret)
		{
			log_info(RESTHandlerLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(RESTHandlerLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
	} while (0);
	log_info(RESTHandlerLogLevel, "Exit");
	return ret;
}

/*
 *  \brief This routine queries the DMC about a domain
 *         using the URI /api/dove/dps/domains/<id>
 * \param [in]  domain_id  The domain ID
 * \param [out] replication_factor A pointer to a location to be filled
 *                                 with the replication factor
 *
 * \return dove_status
*/

dove_status dps_query_domain_config(uint32_t domain_id,
                                    uint32_t *replication_factor)
{
	char uri[DOVE_CONTROLLER_URI_LEN];
	dps_rest_sync_response_args_t args;
	int ret = 0;
	dove_status status = DOVE_STATUS_NO_MEMORY;
	struct evhttp_request *request = NULL;
	char *response_buffer = NULL;
	json_t *js_root = NULL;
	json_t *js_deleted = NULL;
	json_t *js_replication_factor = NULL;
	json_error_t jerror;
	int fdelete = 0;

	log_info(RESTHandlerLogLevel, "Enter: Domain %d", domain_id);

	do
	{
		*replication_factor = 0;

		/* construct a new request */
		memset(&args, 0, sizeof(args));
		request = evhttp_request_new(dps_generic_query_synchronous_response_handler, &args);
		if(request == NULL) 
		{
			log_warn(RESTHandlerLogLevel,"evhttp_request_new returns NULL");
			break;
		}
		status = (dove_status) dps_rest_client_dove_controller_fill_evhttp(request, NULL);
		if (status != DOVE_STATUS_OK)
		{
			log_warn(RESTHandlerLogLevel,
			         "dps_rest_client_dove_controller_fill_evhttp returns %s",
			         DOVEStatusToString(status));
			ret = -1;
			break;
		}

		// Set the DOMAIN URI
		memset(uri,0, DOVE_CONTROLLER_URI_LEN);
		DPS_DOMAIN_URI_GEN(uri, domain_id);
		log_info(RESTHandlerLogLevel, "URI is %s", uri);

		/* Send to DMC */
		ret = dove_rest_request_and_syncprocess(controller_location_ip_string,
		                                        controller_location.port_http,
		                                        EVHTTP_REQ_GET, uri, request, NULL, 3);

		if (ret)
		{
			log_notice(RESTHandlerLogLevel,
			           "Domain Query Synchronous Processing returns %d", ret);
			status = DOVE_STATUS_ERROR;
			break;
		}
		status = DOVE_STATUS_NOT_FOUND;
		if (args.res_code == HTTP_NOTFOUND)
		{
			break;
		}
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			status = DOVE_STATUS_ERROR;
			break;
		}
		// args.req_body_buf contains the response buffer
		response_buffer = args.req_body_buf;
		if (response_buffer == NULL)
		{
			break;
		}
		//Get the js_root
		js_root = json_loads(response_buffer, 0, &jerror);
		if (!js_root)
		{
			log_info(RESTHandlerLogLevel,"js_root is NULL");
			break;
		}
		js_deleted = json_object_get(js_root, DPS_REST_SYNC_OBJECT_DELETED);
		if (js_deleted == NULL)
		{
			log_info(RESTHandlerLogLevel,"No deleted flag");
			break;
		}
		if (!json_is_integer(js_deleted))
		{
			break;
		}
		fdelete = (int)json_integer_value(js_deleted);
		if (fdelete)
		{
			status = DOVE_STATUS_INACTIVE;
			break;
		}
		js_replication_factor = json_object_get(js_root, "replication_factor");
		if (js_replication_factor == NULL)
		{
			log_info(RESTHandlerLogLevel,"No replication_factor");
			break;
		}
		if (!json_is_integer(js_replication_factor))
		{
			break;
		}
		*replication_factor = (uint32_t)json_integer_value(js_replication_factor);
		status = DOVE_STATUS_OK;

	} while (0);

	if (js_root)
	{
		json_decref(js_root);
	}

	log_info(RESTHandlerLogLevel, "Exit: %s", DOVEStatusToString(status));
	return status;
}

#if 0
static void dps_dove_controller_query_policy_response_handler(struct evhttp_request *req, void *arg)
{
	json_t *js_root = NULL;
	json_t *js_ids = NULL;
	json_t *js_id = NULL;
	json_t *js_domain = NULL;
	json_error_t jerror;
	int response_code;
	int i,n;
	struct evbuffer *resp_body = NULL;
	char resp_body_buf[1024];
	unsigned long int domain_id;
	unsigned int src_dvg;
	unsigned int dst_dvg;
	unsigned int traffic_type;
	int no_of_policies = 0;

	do {
		response_code = evhttp_request_get_response_code(req);
		if ((response_code != HTTP_OK) && (response_code != 201))
		{
			log_notice(RESTHandlerLogLevel,
			           "Response FAILED %d for URI [%s]",
			           response_code, evhttp_request_get_uri(req));
			break;
		}
		resp_body = evhttp_request_get_input_buffer(req);
		if (!resp_body || evbuffer_get_length(resp_body) >
			(sizeof(resp_body_buf) - 1))
		{
			log_error(PythonClusterDataLogLevel,
			          "policy creation -- req_body is NULL");
			break;
		}
		n = evbuffer_copyout(resp_body, resp_body_buf,
		                     (sizeof(resp_body_buf) - 1));
		resp_body_buf[n] = '\0';
		js_root = json_loads(resp_body_buf, 0, &jerror);
		if (!js_root)
		{
			log_error(PythonClusterDataLogLevel,
			          "policy re-creation -- js_root is NULL");
			break;
		}
		js_ids = json_object_get(js_root, "policies");
		if (json_is_null(js_ids))
		{
			log_error(PythonClusterDataLogLevel,
			          "policy re-creation js_id is NULL");
			break;
		}
		no_of_policies = json_array_size(js_ids);

		for(i = 0; i < no_of_policies; i++) {
			js_id = json_array_get(js_ids, i);
			if (json_is_null(js_id) || !json_is_object(js_id))
			{
				log_error(RESTHandlerLogLevel,
				          "policy re-create Element [%d] JSON node format ERROR", i);
				break;
			}
			/* extract Domain */
			js_domain = json_object_get(js_id, "domain");
			if (json_is_null(js_domain) || !json_is_integer(js_domain))
			{
				log_error(RESTHandlerLogLevel,
				          "Element [%d]: Domain Id ERROR", i);
				break;
			}
			domain_id = (unsigned int) json_integer_value(js_domain);

			if (DOVE_STATUS_OK != dps_rest_api_create_policy_from_jdata(
				domain_id, js_id, &src_dvg, &dst_dvg, &traffic_type))
			{
				log_error(PythonClusterDataLogLevel,
				          "policy creation FAILED for Domain %u Src_Dvg %u Dst_Dvg %u",
				          domain_id,src_dvg,dst_dvg);
			}
		}
	}while(0);

	if (js_root)
	{
		json_decref(js_root);
	}
}
#endif

static void dps_dove_controller_query_policy_response_handler(struct evhttp_request *req,
                                                              void *arg)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_error_t jerror;
	int response_code;
	struct evbuffer *resp_body = NULL;
	char resp_body_buf[1024];
	dps_controller_query_policy_args_t *policy_args;
	int n;

	log_info(RESTHandlerLogLevel, "Enter");
	do {
		policy_args = (dps_controller_query_policy_args_t *)arg;
		response_code = evhttp_request_get_response_code(req);
		if ((response_code != HTTP_OK) && (response_code != 201))
		{
			policy_args->res_code = response_code;
			log_info(RESTHandlerLogLevel,
			         "Response FAILED %d for URI [%s]",
			         response_code, evhttp_request_get_uri(req));
			break;
		}
		resp_body = evhttp_request_get_input_buffer(req);
		if (!resp_body || evbuffer_get_length(resp_body) >
		   (sizeof(resp_body_buf) - 1))
		{
			policy_args->res_code = HTTP_ENTITYTOOLARGE;
			log_error(PythonClusterDataLogLevel,
			          "policy query -- req_body is NULL");
			break;
		}
		n = evbuffer_copyout(resp_body, resp_body_buf,
		                     (sizeof(resp_body_buf) - 1));
		resp_body_buf[n] = '\0';
		js_root = json_loads(resp_body_buf, 0, &jerror);
		if (!js_root)
		{
			policy_args->res_code = HTTP_INTERNAL;
			log_error(PythonClusterDataLogLevel,
			          "policy query -- js_root is NULL");
			break;
		}

		js_id = json_object_get(js_root, "type");
		if (NULL == js_id || !json_is_integer(js_id))
		{
			log_notice(PythonClusterDataLogLevel,
			           "policy query -- no type");
			break;
		}
		policy_args->type = (unsigned int)json_integer_value(js_id);
		if(DPS_POLICY_TYPE_CONN != policy_args->type)
		{
			policy_args->res_code = HTTP_NOTIMPLEMENTED;
			log_notice(PythonClusterDataLogLevel,
			           "policy query -- not connectivity type");
			break;
		}

		js_id = json_object_get(js_root, "ttl");
		if (NULL == js_id || !json_is_integer(js_id))
		{
			policy_args->ttl = 1000;
			break;
		}
		else
		{
			policy_args->ttl = (uint32_t)json_integer_value(js_id);
		}

		js_id = json_object_get(js_root, "action");
		if (NULL == js_id || !json_is_integer(js_id))
		{
			policy_args->res_code = HTTP_NOTIMPLEMENTED;
			break;
		}
		policy_args->action = (uint32_t)json_integer_value(js_id);
		policy_args->res_code = HTTP_OK;
	}while(0);

	if (js_root)
	{
		json_decref(js_root);
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/* Send the Restful Get Request to Dove Controller for all policies that
 * are configured for a domain, vnid (src or dst) combination */
int dps_query_dmc_policy_info(uint32_t domain, uint32_t src_dvg,
                              uint32_t dst_dvg, uint32_t traffic_type)
{
	struct evhttp_request *request;
	char uri[DOVE_CONTROLLER_URI_LEN];
	char host_header_str[150];
	int ret = 0;
	dps_controller_query_policy_args_t args;
	dps_object_policy_action_t action;
	dps_controller_data_op_t data_op;
	dove_status status;

	log_info(RESTHandlerLogLevel, "Enter");

	do
	{
		memset(uri, 0, DOVE_CONTROLLER_URI_LEN);
		memset(host_header_str, 0, 150);
		args.domain = domain;
		args.src_dvg = src_dvg;
		args.dst_dvg = dst_dvg;
		args.traffic_type = traffic_type;

		log_info(RESTHandlerLogLevel, "Send Query for policies info to Dove Controller");
		/* Get the Policy uri */
		DOVE_CONTROLLER_POLICY_INFO_URI_GEN(uri, domain, src_dvg, dst_dvg, traffic_type);
		log_info(RESTHandlerLogLevel, "uri is %s", uri);

		/* Send to DC */
		request = evhttp_request_new(dps_dove_controller_query_policy_response_handler,
		                             &args);
		if(request == NULL)
		{
			log_alert(RESTHandlerLogLevel,
			          "Can not alloc the evhttp request");
			break;
		}
		if (dps_rest_client_dove_controller_fill_evhttp(request,NULL) != DOVE_STATUS_OK)
		{
			break;
		}

		/* Send to DMC */
		ret = dove_rest_request_and_syncprocess(controller_location_ip_string,
		                                        controller_location.port_http,
		                                        EVHTTP_REQ_GET, uri, request, NULL, 3);

		if (ret)
		{
			log_info(RESTHandlerLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if (args.res_code != HTTP_OK)
		{
			log_info(RESTHandlerLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		memset(&action, 0, sizeof(dps_object_policy_action_t));
		action.connectivity = (unsigned short)args.action;
		action.connectivity = (action.connectivity == DPS_CONNECTIVITY_DROP)?DPS_CONNECTIVITY_DROP:DPS_CONNECTIVITY_ALLOW;
		action.ver = 1;
		if(DPS_POLICY_TYPE_CONN != args.type)
		{
			break;
		}
		data_op.type = DPS_CONTROLLER_POLICY_ADD;
		data_op.policy_add.traffic_type = args.traffic_type;
		data_op.policy_add.domain_id = args.domain;
		data_op.policy_add.type = args.type;
		data_op.policy_add.src_dvg_id = args.src_dvg;
		data_op.policy_add.dst_dvg_id = args.dst_dvg;
		data_op.policy_add.ttl = args.ttl;
		memcpy(&data_op.policy_add.action, &action, sizeof(dps_object_policy_action_t));
		status = dps_controller_data_msg(&data_op);
		log_info(RESTHandlerLogLevel, "Policy create <%d:%d> returns %s",
		         args.src_dvg, args.dst_dvg, DOVEStatusToString(status));

	} while (0);

	log_info(RESTHandlerLogLevel, "Exit");
	return ret;
}

static void dps_domain_endpoint_register_response_handler(struct evhttp_request *req, void *arg)
{

	log_info(RESTHandlerLogLevel, "Enter req %p arg %p",req, arg);
	if (req == NULL)
	{
		log_debug(RESTHandlerLogLevel, "Request timed out");
		*((int *)arg) = 408;
	}
	else
	{
		*((int *)arg) = evhttp_request_get_response_code(req);
		log_info(RESTHandlerLogLevel, "Resp Code = %d", *((int *)arg));
	}
	if (*((int *)arg) == HTTP_OK)
	{
		log_info(RESTHandlerLogLevel,
		         "HTTP_OK: GET response handler for query VNID subnet info!");
	}
	log_info(RESTHandlerLogLevel, "Exit");
	return;
}

/* 
 *  \brief Tell the DMC to send registration msgs to the Dove Switches to register the VMs for the vnid.
 *         This msg is sent in an extreme case when all DCS nodes hosting that domain are down and we need
 *         to start over. So all the VMs have to register again. When the vnid is 0 all vnids in the domain
 *         have to registered.
 *         /api/dove/dps/domain/<id>/vnid/<vnid>/endpoint-register
*/
int dps_dmc_register_domain_endpoints(uint32_t domain, uint32_t vnid)
{
	char uri[DOVE_CONTROLLER_URI_LEN];
	int args = HTTP_OK;
	int ret = 0;
	struct evhttp_request *request = NULL;

	log_info(RESTHandlerLogLevel, "Enter");
	memset(uri,0, DOVE_CONTROLLER_URI_LEN);
	log_info(RESTHandlerLogLevel, "Send register endpoint for domain %d vnid %d to Dove Controller", domain, vnid);
	/* Get the VNID uri */
	DOVE_CONTROLLER_DOMAIN_ENDPOINT_REG_URI_GEN(uri, domain, vnid);
	log_info(RESTHandlerLogLevel, "uri is %s", uri);

	do
	{
		request = evhttp_request_new(dps_domain_endpoint_register_response_handler, &args);
		if(request == NULL) 
		{
			log_info(RESTHandlerLogLevel,"Can not alloc the evhttp request");	
			ret = -1;
			break;
		}
		if (dps_rest_client_dove_controller_fill_evhttp(request, NULL) != DOVE_STATUS_OK)
		{
			ret = -1;
			break;
		}
		/* Send to DMC */
		ret = dove_rest_request_and_syncprocess(controller_location_ip_string, controller_location.port_http, 
		                                        EVHTTP_REQ_POST, uri, request, NULL, 3);

		if (ret)
		{
			log_info(RESTHandlerLogLevel,"Ret value %d, args %d", ret, args);
			ret = -1;
			break;
		}
		if ((args != HTTP_OK) && (args != 201))
		{
			log_info(RESTHandlerLogLevel,"Ret success value %d, args %d", ret, args);
			ret = -1;
			break;
		}
	} while (0);
	log_info(RESTHandlerLogLevel, "Exit");
	return ret;
}

/*
 * The following functions with prefix of dsa is used for sending log msgs to the 
 * controller. The rest client functions are the same except that logging information
 * has been replaced with syslog msgs.
 *
 */


/* Construct and fill the http request */
static int dsa_rest_client_dove_controller_fill_evhttp(struct evhttp_request *req, json_t *js_res)
{
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	char host_header_str[150];

	memset(host_header_str,0, 150);

	res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
	if (NULL == res_body_str)
	{
		return DOVE_STATUS_INVALID_PARAMETER;
	}
	retbuf = evbuffer_new();
	if (NULL == retbuf)
	{
		free(res_body_str);
		return DOVE_STATUS_NO_MEMORY;
	}
	
	evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);
	evbuffer_add_buffer(evhttp_request_get_output_buffer(req), retbuf);
	free(res_body_str);
	res_body_str = NULL;

	if (controller_location.port_http != HTTPD_DEFAULT_PORT)
	{
		sprintf(host_header_str,"%s:%d",
		        controller_location_ip_string,
		        controller_location.port_http);
	}
	else
	{
		sprintf(host_header_str, "%s", controller_location_ip_string);
	}

	if(evhttp_find_header(req->output_headers, "Host") == NULL){
		evhttp_add_header(evhttp_request_get_output_headers(req),
		                  "Host", host_header_str);
	}

	if(evhttp_find_header(req->output_headers, "Content-Type") == NULL)
	{
		evhttp_add_header(evhttp_request_get_output_headers(req), 
		"Content-Type", "application/json");
	}

	return DOVE_STATUS_OK;
}

static void dsa_synch_response_handler(struct evhttp_request *req, void *arg)
{
	if (req == NULL)
	{
		*((int *)arg) = 408;
	}
	else
	{
		*((int *)arg) = evhttp_request_get_response_code(req);
	}
	return;
}

static void dsa_rest_client_json_send_to_dove_controller(json_t *js_res,
                                                  char *uri,
                                                  enum evhttp_cmd_type cmd_type)
{

	struct evhttp_request *request;
	char host_header_str[150];
	int args = HTTP_OK;
	int ret = 0;

	memset(host_header_str,0, 150);
	do
	{

		request = evhttp_request_new(dsa_synch_response_handler, &args);
		if(request == NULL)
		{
			syslog(DPS_SERVER_LOGLEVEL_ALERT, "Can not alloc the evhttp request");
			break;
		}

		if (dsa_rest_client_dove_controller_fill_evhttp(request,js_res) != DOVE_STATUS_OK)
		{
			break;
		}

		syslog(DPS_SERVER_LOGLEVEL_INFO, "Controller IP ==>> [%s : %d]", controller_location_ip_string, controller_location.port_http);

		if (controller_location.port_http != 80)
		{
			sprintf(host_header_str, "%s:%d",
			        controller_location_ip_string,
			        controller_location.port_http);
		}
		else
		{
			sprintf(host_header_str,"%s", controller_location_ip_string);
		}

		if(evhttp_find_header(request->output_headers, "Host") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(request),
			                  "Host", host_header_str);
		}

		//send it synchronously
		ret = dove_rest_request_and_syncprocess(controller_location_ip_string, controller_location.port_http,
		                                        cmd_type, uri, request, NULL,3);
		syslog(DPS_SERVER_LOGLEVEL_INFO, "Send syslog msg to controller ret %d, args %d", ret, args);

	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}
	return;
}


static json_t *dsa_form_json_syslog_msg(char *dsa_name, int log_level, char *msg)
{
	json_t *js_root = NULL;

	js_root = json_pack("{s:s, s:i, s:s}",
	                    "dsa_name",dsa_name,
	                    "severity_level", (int)log_level,
	                    "msg_log", msg);
	return js_root;
}

void dsa_syslog(char *dsa_name, ip_addr_t *ip_addr, int log_level, char *msg)
{
	json_t *js_res = NULL;
	char uri[256], buff[2048], curr_time[256];
	time_t now;
	struct in_addr ipv4;
	struct in6_addr ipv6;
	char hostname[INET6_ADDRSTRLEN];

	if (ip_addr->family == AF_INET)
	{
		ipv4.s_addr = ip_addr->ip4;
		inet_ntop(AF_INET, &(ipv4.s_addr), hostname, INET_ADDRSTRLEN);
	}
	else if (ip_addr->family == AF_INET6)
	{
		memcpy(ipv6.s6_addr, ip_addr->ip6, 16);
		inet_ntop(AF_INET6, &(ipv6.s6_addr), hostname,
		          INET6_ADDRSTRLEN);
	}

	now = time(NULL);
	sprintf(buff, "%.15s %s: %s", ((ctime_r(&now, curr_time)) + 4),
	        hostname, msg);

	js_res = dsa_form_json_syslog_msg(dsa_name, log_level, buff);

	if (js_res == NULL)
	{
		syslog(DPS_SERVER_LOGLEVEL_NOTICE, "No js_res");
		return;
	}

	//set the uri
	strncpy(uri, DOVE_CONTROLLER_DSA_SYSLOG_URI, DPS_URI_LEN);

	//send it synchronously, maybe it should asynchronously to Dove Controller
	dsa_rest_client_json_send_to_dove_controller(js_res, uri,
	                                             EVHTTP_REQ_PUT);
	return;
}


/* GET /api/dps/domains
   POST /api/dps/domains */
void dove_controller_rest_httpd_server_log_endpoint_conflict(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_dps_node = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	int n;
#if defined (NDEBUG)
	unsigned int domain_id;
	unsigned int family;
#endif
	json_error_t jerror;
	int res_code = HTTP_BADREQUEST;

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_POST:
		{
			log_debug(RESTHandlerLogLevel, "I got a POST Request ");
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
			js_id = json_object_get(js_root, "dps_node");
			if (!json_is_object(js_id))
			{
				log_info(RESTHandlerLogLevel, "dps_node is error");
				break;
			}

			js_dps_node = json_object_get(js_id, "ip");
			if (!json_is_string(js_dps_node))
			{
				log_info(RESTHandlerLogLevel, "dps_node ip is error");
				break;
			}
			log_debug(RESTHandlerLogLevel, "dps_node ip is %s", json_string_value(js_dps_node));

			js_id = json_object_get(js_root, "id");
			if (!json_is_integer(js_id))
			{
				break;
			}
#if defined (NDEBUG)
			domain_id = (unsigned int)json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel, "domain id is %u", domain_id);
#endif
			js_id = json_object_get(js_root, "vm1_MAC");
			if (!json_is_string(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel, "vm1 MAC is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "vm2_MAC");
			if (!json_is_string(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel, "vm2 MAC is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "family");
			if (!json_is_integer(js_id))
			{
				break;
			}
#if defined (NDEBUG)
			family = (unsigned int)json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel,"family is %u", family);
#endif
			js_id = json_object_get(js_root, "phy_IP1");
			if (!json_is_string(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel, "physical IP address1 is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "phy_IP2");
			if (!json_is_string(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel, "physical IP address2 is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "vIP");
			if (!json_is_string(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel, "virtual IP address is %s", json_string_value(js_id));

			res_body_str = (char *)malloc(200);
			memset((void *)res_body_str, 0, 200);

			memcpy((void *)res_body_str,(const void *)"Hello1",sizeof("Hello1"));
			
			if (NULL == res_body_str)
			{
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				break;
			}
			evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);

			/* CREATED 201 */
			res_code = HTTP_OK;
			log_debug(RESTHandlerLogLevel, "I got a POST Request ");
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

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

void dove_controller_rest_httpd_server_log_dps_statistics(struct evhttp_request *req, void *arg, int argc, char **argv)
{

	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_res = NULL;
	json_t *js_node = NULL;
	json_t *js_stats = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	//char res_uri[DPS_URI_LEN];
	int n;
#if defined (NDEBUG)
	unsigned int family;
#endif
	json_error_t jerror;
	int res_code = HTTP_BADREQUEST;
	unsigned int i;

	log_debug(RESTHandlerLogLevel, "Enter ");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "I got a GET Request ");
			break;
		}
		case EVHTTP_REQ_POST: 
		{
			//Not used: Compiler complains
			//uint32_t sample_interval = 0;
			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body ||
			    (evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1)))
			{
				break;
			}
			n = evbuffer_copyout(req_body, req_body_buf,
			                     sizeof(req_body_buf) - 1);
			req_body_buf[n] = '\0';

			log_debug(RESTHandlerLogLevel, "req_body_buf is:%s", req_body_buf);

			js_root = json_loads(req_body_buf, 0, &jerror);
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

#if defined (NDEBUG)
			family = (unsigned int) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel, "family is %u", family);
#endif
			js_id = json_object_get(js_node, "ip");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel,"ip error"); 
				break;
			}
			log_debug(RESTHandlerLogLevel, "node id is %s", json_string_value(js_id));

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

			/*Get the statistics */
			js_stats = json_object_get(js_root, "statistics");
			if (!json_is_array(js_stats))
			{
				log_error(RESTHandlerLogLevel, "statistics is not an array");
				break;
			}

			log_debug(RESTHandlerLogLevel,"Now parse statistics info");

			/* Get every statistics info */
			for (i = 0; i < json_array_size(js_stats); i++)
			{
				js_id = json_array_get(js_stats, i);

				/* sample_interval */
				js_node = json_object_get(js_node, "sample_interval");
				if (json_is_null(js_node) || !json_is_integer(js_node))
				{
					log_info(RESTHandlerLogLevel, "sample_interval is Error");
					break;
				}

				//Not used: Compiler complains
				//sample_interval = (unsigned int)json_integer_value(js_node);
				/* Other fields */
			}
#if 0
#if defined (NDEBUG)
			domain_id = (unsigned int) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel, "domain id is %u", domain_id);
#endif
			js_id = json_object_get(js_root, "sample_interval");
			if (!json_is_integer(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel,
			          "sample_interval is %u",
			          json_integer_value(js_id));

			js_id = json_object_get(js_root,
			                        "endpoint_update_count");
			if (!json_is_integer(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel,
			          "endpoint_update_count is %u",
			          json_integer_value(js_id));

			js_id = json_object_get(js_root,
			                        "endpoint_lookup_count");
			if (!json_is_integer(js_id))
			{
				break;
			}
#if defined (NDEBUG)
			endpoint_lookup_count = (unsigned int) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel,
			          "endpoint_lookup_count is %u",
			          endpoint_lookup_count);
#endif

			js_id = json_object_get(js_root, "policy_lookup_count");
			if (!json_is_integer(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel,
			          "policy_lookup_count is %u",
			          json_integer_value(js_id));

			js_id = json_object_get(js_root, "endpoints_count");
			if (!json_is_integer(js_id))
			{
				break;
			}

			log_debug(RESTHandlerLogLevel,
			          "endpoints_count is %u",
			          json_integer_value(js_id));
#endif

			res_body_str = (char *)malloc(200);
			memset((void *)res_body_str, 0, 200);
			memcpy((void *)res_body_str, (const void *)"Hello1", sizeof("Hello1"));
			if (NULL == res_body_str)
			{
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				break;
			}

			/* Copy the input buffer to the response http */
			evbuffer_add(retbuf, res_body_str,
			             strlen((const char *)res_body_str) + 1);
			//evbuffer_add_buffer(retbuf, req_body);

			/* CREATED 201 */
			res_code = HTTP_OK;
			log_debug(RESTHandlerLogLevel, "I got a POST Request ");
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

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


/* Dove Controller process the received DPS Heartbeat */
void dove_controller_rest_httpd_server_process_dps_heartbeat(struct evhttp_request *req, void *arg, int argc, char **argv)
{

	json_t *js_root = NULL;
	json_t *js_id = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	struct evbuffer *req_body = NULL;
	char req_body_buf[1024];
	//char res_uri[DPS_URI_LEN];
	int n;
	json_error_t jerror;
#if defined (NDEBUG)
	unsigned int family;
#endif
	int res_code = HTTP_BADREQUEST;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "I got a GET Request ");
			break;
		}
		case EVHTTP_REQ_POST: 
		{

			log_debug(RESTHandlerLogLevel, "I got a HTTP POST Request ");

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
			js_id = json_object_get(js_root, "family");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get family");
				break;
			}
#if defined (NDEBUG)
			family = (unsigned int) json_integer_value(js_id);
			log_debug(RESTHandlerLogLevel, "family is %u", family);
#endif

			js_id = json_object_get(js_root, "ip");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get ip");
				break;
			}
			log_debug(RESTHandlerLogLevel, "node id is %s", json_string_value(js_id));

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

			res_body_str = (char *)malloc(200);
			memset((void *)res_body_str, 0, 200);
			memcpy((void *)res_body_str, (const void *)"Hello1", sizeof("Hello1"));
			if (NULL == res_body_str)
			{
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				break;
			}
			/* Copy the input buffer to the response http */
			evbuffer_add(retbuf, res_body_str,
			             strlen((const char *)res_body_str) + 1);
			//evbuffer_add_buffer(retbuf, req_body);

			/* CREATED 201 */
			res_code = HTTP_OK;
			log_debug(RESTHandlerLogLevel, "I got a POST Request ");
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

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


/* Dove Controller process the received DPS Query for Cluster info
   Dove Controller should get the DPS nodes from its database, but currently
   the simulated one only construct several nodes.
   The truly Dove Controller Query response should follow the packet format used in this
   reply
   */
static json_t *dc_form_one_dps_cluster_node(uint32_t family, char *ipstr, unsigned short port, char *uuid)
{

	json_t *js_root = NULL;
	js_root = json_pack("{s:i, s:s, s:i, s:s}", 
	                    "family", (int)family,
	                    "node_ip", ipstr,
	                    "port", port,
	                    "UUID",  dps_node_uuid);

	return js_root;
}

/* Dove Controller form the cluster json format:
   {s:i,s:o}, the i is the number of the cluster nodes which the DC knows currently,
   the o is the array of the cluster nodes, in which each array element is
   a complete description of a cluster node, which contains its
   family, ip, port, uuid
*/
static json_t *dc_form_dps_cluster_nodes_info()
{
	json_t *js_root = NULL;
	json_t *js_nodes = NULL;
	json_t *js_node = NULL;
	char ipstr[128], ipstr2[128], ipstr3[128];
	char uuid[128], uuid2[128], uuid3[128];

	log_debug(RESTHandlerLogLevel, "Enter");

	memset(ipstr,0,sizeof(ipstr));
	memset(ipstr2,0,sizeof(ipstr2));
	memset(ipstr3,0,sizeof(ipstr3));

	memcpy((void *)ipstr, (const void *)"192.168.1.1",sizeof("192.168.1.1"));
	memcpy((void *)ipstr2, (const void *)"192.168.1.2",sizeof("192.168.1.2"));
	memcpy((void *)ipstr3, (const void *)"192.168.1.3",sizeof("192.168.1.3"));

	memset(uuid,0,sizeof(uuid));
	memset(uuid2,0,sizeof(uuid2));
	memset(uuid3,0,sizeof(uuid3));

	memcpy((void *)uuid, (const void *)"00000000000",sizeof("00000000000"));
	memcpy((void *)uuid2, (const void *)"11111111111",sizeof("11111111111"));
	memcpy((void *)uuid3, (const void *)"22222222222",sizeof("22222222222"));

	js_nodes = json_array();

	//node1
	js_node = dc_form_one_dps_cluster_node(AF_INET, ipstr, 5555, uuid);
	json_array_append_new(js_nodes, js_node);

	//node2
	js_node = dc_form_one_dps_cluster_node(AF_INET, ipstr2, 5555, uuid2);
	json_array_append_new(js_nodes, js_node);

	//node3
	js_node = dc_form_one_dps_cluster_node(AF_INET, ipstr3, 5555, uuid3);
	json_array_append_new(js_nodes, js_node);

	js_root = json_pack("{s:i, s:o}", "number", 3, "nodes", js_nodes);

	log_debug(RESTHandlerLogLevel, "Exit");

	return js_root;

}

void dove_controller_rest_httpd_server_process_dps_query_cluster_info(
	struct evhttp_request *req, void *arg, int argc, char **argv
	)
{
	json_t *js_root = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "EVHTTP_REQ_GET");
			/* Get and Store the current DPS Query Node info */
			/* Get the DPS Cluster info and construct the response */
			js_res = dc_form_dps_cluster_nodes_info();
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
				evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);
				res_code = HTTP_OK;
			}

			log_debug(RESTHandlerLogLevel, "I got a GET Request for Cluster info ");
			break;

		}
		case EVHTTP_REQ_POST:
		{

			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

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

/* Dove Controller form the VNID json format:
   {s:i, s:i, s:o}, the i is the VNID and the Domain ID that the vnid belongs to,
   the o is the array of the vnids of the domain
   TODO:  maybe any further information of the domain could be appended to the response 
   in addition to the info described above
*/
static json_t *dc_form_dps_vnid_nodes_info(uint32_t vnid)
{
	json_t *js_root = NULL;
	json_t *js_vnids = NULL;
	json_t *js_vnid = NULL;

	uint32_t domain_id;

	log_debug(RESTHandlerLogLevel, "Enter");	

	js_vnids = json_array();

	//vnid1
	//js_node = dc_form_one_dps_cluster_node(AF_INET, ipstr, 5555, uuid);
	js_vnid = json_pack("i",5);
	json_array_append_new(js_vnids, js_vnid);

	//vnid2
	js_vnid = json_pack("i",6);
	json_array_append_new(js_vnids, js_vnid);

	//vnid3
	js_vnid = json_pack("i",7);
	json_array_append_new(js_vnids, js_vnid);

	domain_id = 1000;
	js_root = json_pack("{s:i, s:i, s:o}", "vnid", vnid, "domain", domain_id, "vnids", js_vnids);

	log_debug(RESTHandlerLogLevel, "Exit");

	return js_root;

}

static int dps_uri_get_query_vnid(char *uri)
{
	int vnid = -1;
	char *saveptr = NULL;
	char *tok = NULL;
//	char *endptr = NULL;

	tok = strtok_r((char *)uri, "/", &saveptr);
	while (NULL != tok)
	{

		if(strcmp((const char *)tok, (const char *)"vnid") == 0)
		{
			tok = strtok_r(NULL, "/", &saveptr); /* Get next */
			vnid = atoi(tok);
//			vnid = strtoul(tok, &endptr, 10);
			break;
		}
		tok = strtok_r(NULL, "/", &saveptr);

	}
	return vnid;

}

void dove_controller_rest_httpd_server_process_dps_query_vnid_info(struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_root = NULL;
	json_t *js_res = NULL;
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	char req_body_buf[2048];
	int vnid = 0;
	int res_code = HTTP_BADREQUEST;
	log_debug(RESTHandlerLogLevel, "Enter");

	memset(req_body_buf, 0, 2048);
	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			log_debug(RESTHandlerLogLevel, "EVHTTP_REQ_GET for VNID info");
			log_info(RESTHandlerLogLevel, "uri is %s", evhttp_request_get_uri(req));
			if((vnid = dps_uri_get_query_vnid((char *)evhttp_request_get_uri(req)))<0)
			{
				log_error(RESTHandlerLogLevel,"Bad uri to get VNID, vnid = %d", vnid);
				res_code = HTTP_BADREQUEST;
				break;
			}

			log_debug(RESTHandlerLogLevel, "I got the Query VNID Request, vnid = %d!", vnid);

			/* Get and Store the current DPS Query Node info */
			/* Get the DPS Cluster info and construct the response */

			js_res = dc_form_dps_vnid_nodes_info(vnid);
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
				evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);
				res_code = HTTP_OK;
			}

			log_debug(RESTHandlerLogLevel, "I got a GET Request for VNID info ");
			break;

		}

		case EVHTTP_REQ_POST:
		{
			log_debug(RESTHandlerLogLevel, "EVHTTP_REQ_POST for VNID info");
			break;
		}
		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	log_debug(RESTHandlerLogLevel, "After evhttp_send_reply");

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

void dove_controller_rest_httpd_server_process_dps_appliance_registration(
	struct evhttp_request *req, void *arg, int argc, char **argv)
{
	json_t *js_id = NULL;
	json_t *js_root = NULL;
	struct evbuffer *req_body = NULL;
	int n;
	json_error_t jerror;
	char req_body_buf[1024];
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	int res_code = HTTP_BADREQUEST;

	log_debug(RESTHandlerLogLevel, "Enter");

	switch (evhttp_request_get_command(req))
	{
		case EVHTTP_REQ_GET:
		{
			res_code = HTTP_BADMETHOD;
			log_debug(RESTHandlerLogLevel, "I got a GET Request for Cluster info ");
			break;
		}
		
		case EVHTTP_REQ_POST:
		{
			log_debug(RESTHandlerLogLevel,"I got a HTTP POST Request for Appliance Registration");

			memset(req_body_buf,0,1024);

			req_body = evhttp_request_get_input_buffer(req);
			if (!req_body ||evbuffer_get_length(req_body) > (sizeof(req_body_buf) - 1))
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

			js_id = json_object_get(js_root, "ip");
			if (!json_is_string(js_id))
			{
				log_error(RESTHandlerLogLevel, "Can not get ip");
				break;
			}

			log_debug(RESTHandlerLogLevel, "node IP addr is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "uuid");
			if (!json_is_string(js_id))
			{
				break;
			}
			log_debug(RESTHandlerLogLevel, "UUID is %s", json_string_value(js_id));

			js_id = json_object_get(js_root, "rest_port");
			if (!json_is_integer(js_id))
			{
				log_error(RESTHandlerLogLevel, "Bad rest_port");
				break;
			}
			log_debug(RESTHandlerLogLevel, "rest_port is %d", json_integer_value(js_id));

			res_body_str = (char *)malloc(200);
			memset((void *)res_body_str, 0, 200);

			memcpy((void *)res_body_str, (const void *)"Hello1",sizeof("Hello1"));

			if (NULL == res_body_str)
			{
				show_print("res_body_str is NULL");
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				break;
			}
			evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);

			/* CREATED 201 */
			res_code = HTTP_OK;
			log_debug(RESTHandlerLogLevel,"DMC got DPS Appliance Registration");
			//show_print("%s:DMC got DPS Appliance Registration", __FUNCTION__);
			break;
		}

		default:
		{
			res_code = HTTP_BADMETHOD;
			break;
		}
	}

	evhttp_send_reply(req, res_code, NULL, retbuf);

	if (res_body_str)
	{
		free(res_body_str);
		res_body_str = NULL;
	}
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

