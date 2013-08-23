/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client Message Creation
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
*  $Log: rest_client_cluster_leader.c $
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
#include "../inc/rest_client_dove_controller.h"
#include "../inc/rest_client_cluster_leader.h"

static void dps_node_http_response_handler(struct evhttp_request *req, void *arg)
{
#if defined(NDEBUG)
	const char *cmdtype;
#endif
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;
	int response_code;

	log_debug(RESTHandlerLogLevel, "Enter: Host");
	do
	{
		if (NULL == req)
		{
			log_debug(RESTHandlerLogLevel, "NULL == req");
			break;
		}
		log_debug(RESTHandlerLogLevel, "URI %s", evhttp_request_get_uri(req));
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
		// 201 is created
		if ((response_code != HTTP_OK) && (response_code != 201))
		{
			log_info(RESTHandlerLogLevel,
			         "FAILED, URI %s, Response Code NOT HTTP_OK [%d]",
			         evhttp_request_get_uri(req), response_code);
			break;
		}
		//dps_rest_node_touch(req);

#if defined(NDEBUG)
		log_debug(RESTHandlerLogLevel, "Received a %s request for %s",
		          cmdtype, evhttp_request_get_uri(req));
#endif

		log_debug(RESTHandlerLogLevel, "Headers: <<<");
		headers = evhttp_request_get_input_headers(req);
		for (header = headers->tqh_first; header; header = header->next.tqe_next)
		{
			log_debug(RESTHandlerLogLevel,"[Key] %s: [Value] %s",
			          header->key, header->value);
		}
		log_debug(RESTHandlerLogLevel, "Headers: >>>");

		/* Process the response for DPS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO_URI
		   Get all the node info from DC response, and add the cluster node into our db
		*/

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
		log_debug(RESTHandlerLogLevel,"Input data: >>>\n");
	} while(0);
	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

static struct evhttp_request *dps_rest_client_dps_node_request_new(void)
{
	struct evhttp_request *request = NULL;
	request = evhttp_request_new(dps_node_http_response_handler, NULL);
	return request;
}

/*
 ******************************************************************************
 * dps_rest_client_fill_evhttp --                                         *//**
 *
 * \brief This routine crate the HTTP Request from the JSON Body
 *
 * \param req: The EVHTTP_REQUEST
 * \param js_res: The JSON Body of the Request
 * \param remote_ip_string: The DPS IP Address to send the request to.
 *
 * \return PyObject
 *
 *****************************************************************************/
static void dps_rest_client_fill_evhttp(struct evhttp_request *req,
                                        json_t *js_res,
                                        char *remote_ip_string)
{
	char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	char host_header_str[64];

	log_debug(RESTHandlerLogLevel, "Enter");

	memset(host_header_str, 0, 64);
	do
	{
		if(js_res != NULL)
		{
			//add to the http request output buffer for sending
			res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
			if (NULL == res_body_str)
			{
				log_alert(RESTHandlerLogLevel, "JSON string is NULL or Bad");
				break;
			}
			retbuf = evbuffer_new();
			if (NULL == retbuf)
			{
				log_error(RESTHandlerLogLevel, "retbuf = evbuffer_new() ERROR");
				break;
			}
			//add to the http request output buffer for sending
			log_debug(RESTHandlerLogLevel,
			          "strlen(res_body_str) = %d, content is %s",
			          strlen((const char *)res_body_str), res_body_str);
			evbuffer_add(retbuf, res_body_str, strlen((const char *)res_body_str) + 1);
			evbuffer_add_buffer(evhttp_request_get_output_buffer(req), retbuf);
		}
		/* No need to add "content-length,
		 * as evhttp will add it for POST and PUT, and
		 * GET should not have it
		*/

		log_debug(RESTHandlerLogLevel,
		          "Remote DPS Node IP ==>> [%s : REST Port %d]",
		          remote_ip_string, DPS_REST_HTTPD_PORT);
		if (DPS_REST_HTTPD_PORT != HTTPD_DEFAULT_PORT)
		{
			sprintf(host_header_str,"%s:%d",
			        remote_ip_string,
			        DPS_REST_HTTPD_PORT);
		}
		else
		{
			sprintf(host_header_str,"%s",remote_ip_string);
		}
		log_debug(RESTHandlerLogLevel, "host_header_str %s", host_header_str);

		if(evhttp_find_header(req->output_headers, "Host") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(req),
			                  "Host", host_header_str);
		}

		
		if(evhttp_find_header(req->output_headers, "Content-Type") == NULL)
		{
			evhttp_add_header(evhttp_request_get_output_headers(req), 
			"Content-Type", "application/json");
		}
	} while(0);

	if (res_body_str)
	{
		free(res_body_str);
	}

	log_debug(RESTHandlerLogLevel, "Exit");

	return;

}

/*
 ******************************************************************************
 * dps_rest_client_json_send_to_dps_node --                               *//**
 *
 * \brief This routine sends heartbeat message to the remote DPS Node
 *
 * \param js_res: The JSON Body
 * \param uri: The URI of the request
 * \param cmd_type: GET/POST/PUT/DELETE etc
 * \param dps_node_ip: The IP Address of the DPS Node
 *
 * \return PyObject
 *
 *****************************************************************************/
void dps_rest_client_json_send_to_dps_node(json_t *js_res,
                                           char *uri,
                                           enum evhttp_cmd_type cmd_type,
                                           ip_addr_t *dps_node_ip)
{
	struct evhttp_request *request;
	char dps_node_ip_string[INET6_ADDRSTRLEN];
	char host_header_str[64];

	log_debug(RESTHandlerLogLevel, "Enter - uri %s", uri);

	memset(host_header_str,0, 64);

	do
	{
		inet_ntop(dps_node_ip->family, dps_node_ip->ip6,
		          dps_node_ip_string, INET6_ADDRSTRLEN);
		request = dps_rest_client_dps_node_request_new();
		if(request == NULL) {
			log_warn(RESTHandlerLogLevel,
			         "Can not allocate the evhttp request");
			break;
		}

		dps_rest_client_fill_evhttp(request, js_res, dps_node_ip_string);

		log_debug(RESTHandlerLogLevel,
		          "DCS Node IP ==>> [%s : REST Port %d]",
		          dps_node_ip_string,
		          DPS_REST_HTTPD_PORT);

		if (DPS_REST_HTTPD_PORT != 80) {
			sprintf(host_header_str, "%s:%d",
			        dps_node_ip_string,
			        DPS_REST_HTTPD_PORT);
		}
		else
		{
			sprintf(host_header_str, "%s", dps_node_ip_string);
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
		//Send asynchronously to DPS Node
		dps_rest_client_dove_controller_send_asyncprocess(
			dps_node_ip_string, uri,
			DPS_REST_HTTPD_PORT, cmd_type,
			request);
	} while (0);

	if (js_res)
	{
		json_decref(js_res);
	}

	log_debug(RESTHandlerLogLevel, "Exit");
	return;
}

