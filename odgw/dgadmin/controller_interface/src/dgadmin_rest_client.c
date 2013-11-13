/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


#include "include.h"
#include <stdlib.h>
#include <sys/reboot.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <jansson.h>
#include "../inc/dgadmin_rest_client.h"
#include "dgadmin_rest_api.h"

#define RCLIENT_THD_NUM 8
#define RCLIENT_QUEUE_LEN 100
#define RCLIENT_THD_EVENT_READQ 1
#define SYNC_REST_CLIENT_DEFAULT_TIMEOUT_SEC 20
#define ASYNC_REST_CLIENT_TIMEOUT_SEC 3

#define HTTPD_DEFAULT_PORT  80
#define RESP_BODY_LEN 2048

short dgwy_rest_dmc_port = 0;
char dmc_default_httpd_server_address[128]= "127.0.0.1";
int g_dgw_role = 0;
extern uint32_t g_dovenet_ipv4;
extern uint32_t g_mgmt_ipv4;
extern uint32_t g_dmc_ipv4;
extern uint16_t g_dmc_port;
extern int ServiceUtilLogLevel;
extern char g_node_uuid[40];
extern int g_dgwconfig_curversion;
extern int g_dgwconfig_valid_curversion;
extern uint16_t g_dgwy_rest_port;
extern uint8_t g_dmc_register_done;
extern char g_built_version[64];

typedef enum {
    DGWY_REGISTER   =   1,
    DGWY_HEARTBEAT  =   2,
    DGWY_DCS_GET    =   3,
    DGWY_VXLAN_GET  =   4,
}dgwy_rest_request_type;

typedef struct thd_cb
{
	int64_t qid;
	int64_t tid;
	struct event_base *evbase;
} thd_cb_t;

static thd_cb_t rclient_thd_cb[RCLIENT_THD_NUM];
static void syncprocess_callback(struct evhttp_request *request, void *args)
{
	struct event_base *base = (struct event_base *)((void **)args)[0];
	void (*cb)(struct evhttp_request *, void *) = (void (*)(struct evhttp_request *, void *))((void **)args)[1];
	if(NULL != cb)
	{
		(*cb)(request, ((void **)args)[2]);
	}
	if(NULL != base)
	{
		/* If base is not in loop, no any effect, nor the next loop iteration
		 * of the base */
		event_base_loopbreak(base);
	}
}

static void dove_rest_request_info_free(dove_rest_request_info_t *rinfo)
{
	if (NULL != rinfo->address)
	{
		free(rinfo->address);
	}
	if(NULL != rinfo->uri)
	{
		free(rinfo->uri);
	}
	if (NULL != rinfo->request)
	{
		if(NULL != rinfo->request->cb)
		{
			(*(rinfo->request->cb))(rinfo->request, rinfo->request->cb_arg);
        	}
		evhttp_request_free(rinfo->request);
	}
}
static unsigned int get_rclient_thd_idx(const char *addr, unsigned short port)
{
	unsigned int v = 0;
	while(*addr != 0)
	{
		v ^= (unsigned int)(*addr);
		addr++;
	}
	v ^= (unsigned int) port;
	v %= RCLIENT_THD_NUM;
	return v;
}
/*
 ******************************************************************************
 * dove_rest_request_and_syncprocess --           *//**
 *
 * \brief This routine sends a HTTP request to a server, and waits until
 * corresponding reponse is returned or internal error occurs or timeouts
 * , then invokes the callback
 * - void (*cb)(struct evhttp_request *request, void *arg)) -
 * registered in parameter "request" to handle the response. The first 
 * parameter passed to callback is the a pointer to a evhttp_request data
 * structure, and is always the same one in the input parameter of this routine
 * In some internal error cases, NULL might be passed to the callback as the
 * first parameter.
 *
 * \param[in] address The IP address string of the server to which the request
 * is sent.
 * \param[in] port The TCP port of the server to which the request is sent.
 * \param[in] type The HTTP method.
 * \param[in] uri The URI of the HTTP request.
 * \param[in] request A pointer to a evhttp_request data structure.
 * The data structure should be allocated on heap. The ownership of the data
 * structure will passed to this routine, this routine is reponsible to free
 * it before returns. The caller should not dereference it after invoking
 * this routine.
 * \param[in] pre_alloc_base  The event base used for the HTTP operation. If
 * provided, this routine will use it for all related event operations and this
 * routine never free the pre_alloc_base when returns. If NULL, this routine
 * will allocated new event base, and use the new event base for all related
 * event operations and free it when this routine returns.
 * \param[in] timeout_in_secs Sets the timeout (in second) for events related
 * to this HTTP connection.A negative or zero value will set the timeout to
 * default value
 * 
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dove_rest_request_and_syncprocess (
	const char *address, unsigned short port,
	enum evhttp_cmd_type type, const char *uri,
	struct evhttp_request *request , 
	struct event_base *pre_alloc_base, 
	int timeout_in_secs)
{
	struct event_base *base = NULL;
	struct evhttp_connection *conn = NULL;
	void *args[3];
	int ret = -1;
	if (NULL == address || NULL == uri || NULL == request)
	{
		if(request)
		{
			if(request->cb)
			{
				(*(request->cb))(request, request->cb_arg);
			}
			evhttp_request_free(request);
		}
		return -1;
	}
	do
	{
		if(NULL != pre_alloc_base)
		{
			base = pre_alloc_base;
		}
		else
		{
			base = event_base_new();
			if (NULL == base)
			{
				if(request->cb)
				{
					(*(request->cb))(request, request->cb_arg);
				}
				evhttp_request_free(request);
				break;
			}
		}
		conn = evhttp_connection_base_new(base, NULL, address, port);
		if (NULL == conn)
		{
			if(request->cb)
			{
				(*(request->cb))(request, request->cb_arg);
			}
			evhttp_request_free(request);
			break;
		}
		evhttp_connection_set_timeout(conn, (timeout_in_secs > 0) ? \
			timeout_in_secs : SYNC_REST_CLIENT_DEFAULT_TIMEOUT_SEC);
		/* Don't set retries more than 1, it will case memory leak and
		 * unexpected errors in libevent2 version 2.18 */
		evhttp_connection_set_retries(conn, 1);
		args[0] = base;
		args[1] = (void *)request->cb;
		args[2] = request->cb_arg;
		request->cb = syncprocess_callback;
		request->cb_arg = (void *)args;
		
		/* We give ownership of the request to the connection */
		ret = evhttp_make_request(conn, request, type, uri);
		if(ret)
		{
			break;
		}
		ret = event_base_dispatch(base);
	} while (0);
	if (NULL != conn)
	{
		evhttp_connection_free(conn);
	}
	
	if(NULL == pre_alloc_base && NULL != base)
	{
		event_base_free(base);
	}
	return ret;
}

/*
 ******************************************************************************
 * dove_rest_request_and_asyncprocess --                *//**
 *
 * \brief This routine is used to send HTTP request to a server and process the
 * response asynchronously. The callback for the response is register in the
 * struct evhttp_request data structure pointered by the member request.
 * The callback 
 * - void (*cb)(struct evhttp_request *request, void *arg))-
 * will be invoked when corresponding response is received or interanl error
 * occurs.The first parameter passed to callback is the a pointer to a 
 * evhttp_request data structure, and is always the same one in the input
 * parameter of this routine. In some internal error cases, NULL might be 
 * passed to the callback as the first parameter.
 * The REST client infrastructure ensures first-in-first-serviced for the HTTP
 * request to the same destination. (determined by IP address and TCP port 
 * number)
 *
 * \param[in] rinfo A pointer to a dove_rest_request_info_t data structure. 
 * The data structure should be allocated on heap. The ownership of the data 
 * structure will passed to the REST client infrastructure, which is reponsible
 * to free it when completes. The caller should not dereference it after
 * invoking this routine. Member of data structure dove_rest_request_info_t:
 *	char *address; Pointer to IP address string of the server to which the 
 *   request is sent, should be allocated on heap, the infrastructure will call
 *   free() to release it when completes the process.
 *	char *uri; The URI which the HTTP request is accessing, should be allocated
 *   on heap, the infrastructure will call free() to release it when completes
 *   the process.
 *	unsigned short port; The tcp port of the server to which the request is
 *   sent.
 *	enum evhttp_cmd_type type; HTTP method.
 *	struct evhttp_request *request; A pointer to a evhttp_request data 
 *   structure. The data structure should be allocated on heap. The 
 *   infrastructure will release it when completes the process.
 * 
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dove_rest_request_and_asyncprocess (dove_rest_request_info_t *rinfo)
{
	unsigned int thd_cb_idx;
	if(NULL == rinfo->address || NULL == rinfo->uri || NULL == rinfo->request)
	{
		dove_rest_request_info_free(rinfo);
		return -1;
	}
	thd_cb_idx = get_rclient_thd_idx(rinfo->address, rinfo->port);
	if(OSW_OK != queue_send(rclient_thd_cb[thd_cb_idx].qid, (char *)(&rinfo), sizeof(rinfo)))
	{
		dove_rest_request_info_free(rinfo);
		return -1;
	}
	send_event(rclient_thd_cb[thd_cb_idx].tid, RCLIENT_THD_EVENT_READQ);
	return 0;
}

static void
dove_rest_client_main (INT1 *arg)
{
	UINT4 u4Event;
	long idx = (long)arg;
	dove_rest_request_info_t *rinfo;

	while(1)
	{
		recv_event(rclient_thd_cb[idx].tid, RCLIENT_THD_EVENT_READQ,
		           (UINT4)(OSW_WAIT | OSW_EV_ANY), &u4Event);
		
		if ((u4Event & RCLIENT_THD_EVENT_READQ) == RCLIENT_THD_EVENT_READQ) 
		{
			while (queue_receive(rclient_thd_cb[idx].qid, (char *)&rinfo,
			                     sizeof(rinfo), OSW_NO_WAIT) == OSW_OK)
			{
				dove_rest_request_and_syncprocess(rinfo->address, rinfo->port, 
					rinfo->type, rinfo->uri, rinfo->request, rclient_thd_cb[idx].evbase, 
					ASYNC_REST_CLIENT_TIMEOUT_SEC);
				/* The ownership of request is given to dove_rest_request_and_syncprocess */
				rinfo->request = NULL;
				dove_rest_request_info_free(rinfo);
			}
		}
	}
}

int dove_rest_client_init(void)
{
	int ret = 0;
	int i;
	char thdnamebuf[16];
	for (i = 0; i < RCLIENT_THD_NUM; i++)
	{
		rclient_thd_cb[i].evbase = event_base_new();
		if(NULL == rclient_thd_cb[i].evbase)
		{
			ret = -1;
			break;
		}
		sprintf(thdnamebuf, "RC%d", i); 
		if (create_queue(thdnamebuf, OSW_MAX_Q_MSG_LEN, 
			RCLIENT_QUEUE_LEN, &rclient_thd_cb[i].qid) != OSW_OK)
		{
			ret = -1;
			break;
		}
		if (create_task(thdnamebuf, 0, 
		OSW_DEFAULT_STACK_SIZE, (task_entry) dove_rest_client_main,
		(void *)((long)i), &rclient_thd_cb[i].tid) != OSW_OK)
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

/* Construct and fill the http request */
void dgwy_rest_client_dmc_fill_evhttp(struct evhttp_request *req, 
                                      json_t *js_res)
{
    char *res_body_str = NULL;
	struct evbuffer *retbuf = NULL;
	
	char *dmc_httpd_addr = NULL;
	int dmc_rest_port = 0;
	char host_header_str[150];
    struct sockaddr_in sa;

	memset(host_header_str,0, 150);

	/* For GET request, the js_res may be NULL*/
	if(js_res == NULL) 
    {
        log_error(ServiceUtilLogLevel,
                  "js_res is NULL, No json resource!!!");
	}
    else
    {
		res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
		if (NULL == res_body_str)
		{
			log_error(ServiceUtilLogLevel,
                      "JSON string is NULL or Bad");
			return;
		}
		retbuf = evbuffer_new();
		if (NULL == retbuf)
		{
			log_error(ServiceUtilLogLevel,
                      "retbuf = evbuffer_new() ERROR");
			return;
		}
		/* Need header uri info ??????????????
         * add to the http request output buffer for sending */
        res_body_str = json_dumps(js_res, JSON_PRESERVE_ORDER);
		if (NULL == res_body_str)
		{
			log_error(ServiceUtilLogLevel, "JSON string is NULL or Bad");
			return;
		}
		retbuf = evbuffer_new();
		if (NULL == retbuf)
		{
			log_error(ServiceUtilLogLevel, "retbuf = evbuffer_new() ERROR");
			return;
		}
        
        /* Need header uri info ??????????????
         * add to the http request output buffer for sending 
         * */
        
        log_debug(ServiceUtilLogLevel,
                  "STRLEN(res_body_str) = %d, content is %s",
                  STRLEN(res_body_str), res_body_str);

		evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);
		evbuffer_add_buffer(evhttp_request_get_output_buffer(req), retbuf);
	}
	/* No need to add "content-length, 
     * as evhttp will add it for POST and PUT, and
     * GET should not have it */

    dmc_rest_port = g_dmc_port;
    
    log_debug(ServiceUtilLogLevel,
              "Controller IP ==>> [%s : %d]",
              dmc_default_httpd_server_address,
              dmc_rest_port);
    
    sa.sin_addr.s_addr = g_dmc_ipv4;
    dmc_httpd_addr = inet_ntoa(sa.sin_addr);
    
    if (dmc_rest_port != HTTPD_DEFAULT_PORT) 
    {
        sprintf(host_header_str,
                "%s:%d",
                dmc_httpd_addr,
                dmc_rest_port);
    }
    else 
    {
        sprintf(host_header_str,
                "%s",
                dmc_httpd_addr);
    }

    log_debug(ServiceUtilLogLevel,
              "Host Str: %s\n",host_header_str);
    
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
}

static void dgwy_dmc_register_resp(struct evhttp_request *req, void *arg)
{
    int resp_code=0;
	if (NULL == req)
	{
        return;
	}

    resp_code = evhttp_request_get_response_code(req);
    if(resp_code == HTTP_OK)
    {
        g_dmc_register_done=1;
    }
    else if(resp_code == HTTP_NOTFOUND)
    {
        printf("REGISTER ERROR : SHOULD REBOOT DSA \r\n");
        printf("REGISTER ERROR : SHOULD REBOOT DSA \r\n");
        printf("REGISTER ERROR : SHOULD REBOOT DSA \r\n");
    }
    else
    {
        log_error(ServiceUtilLogLevel, 
                  "Register response code is NOT HTTP_OK");
	}
    return;
}

static void dgwy_dmc_heartbeat_resp(struct evhttp_request *req, void *arg)
{
    json_t *js_root     = NULL;
    json_t *js_id       = NULL;
    struct evbuffer *res_body = NULL;
    char res_body_str[RESP_BODY_LEN];
    json_error_t jerror;
    int resp_code=0;

	if (NULL == req)
	{
        return;
	}

    resp_code = evhttp_request_get_response_code(req);

    if (resp_code == HTTP_NOTFOUND) 
    {
        log_error(ServiceUtilLogLevel, 
                  "HeartBeat response code is 404 NOT_FOUND");

        log_error(ServiceUtilLogLevel,
                  "\r\n\r\n------------------------------------------------------\r\n");
        log_error(ServiceUtilLogLevel,
                  "|\r\n|\r\n|\tDMC AND GW-DSA ROLE NOT IN SYNC: REBOOTING\r\n");
        log_error(ServiceUtilLogLevel,
                  "|\r\n|\r\n|\r\n\r\n------------------------------------------------------\r\n");
 

        printf("\r\n\r\n------------------------------------------------------\r\n");
        printf("|\r\n|\r\n|\tDMC AND GW-DSA ROLE NOT IN SYNC: REBOOTING\r\n");
        printf("|\r\n|\r\n|\r\n\r\n------------------------------------------------------\r\n");
        sync();
        sleep(5);
        reboot(RB_AUTOBOOT);
		return;
	}
    else if(resp_code == HTTP_OK)
    {
        int n=0;
        res_body = evhttp_request_get_input_buffer(req);
        if (!res_body || evbuffer_get_length(res_body) > (sizeof(res_body_str) - 1))
        {
            log_debug(ServiceUtilLogLevel,
                      "DGW GET Response No Buffer");
            return;
        }
        
        n = evbuffer_copyout(res_body, res_body_str, (sizeof(res_body_str) - 1));
        if (n < 1)
        {
            log_debug(ServiceUtilLogLevel, 
                      "DGW GET Response No Buffer");
            goto out;
        }
        
        res_body_str[n] = '\0';
        js_root = json_loads(res_body_str, 0, &jerror);
        if (!js_root)
        {
            log_error(ServiceUtilLogLevel, 
                      "DGW GET Response No Buffer");
            goto out;
        }
       
        /* Borrowed reference, no need to decref */
        //js_id = json_object_get(js_root, "role_assigned");
        js_id = json_object_get(js_root, "isDGW");
        if (json_is_null(js_id))
        {
            log_error(ServiceUtilLogLevel, 
                      "DGW GET Response no role_assigned object");
            goto out;
        }
        

        //g_dgw_role = json_integer_value(js_id);          
        g_dgw_role = json_is_true(js_id);          

        log_error(ServiceUtilLogLevel, 
                  "DGW GOT HeartBeat Response: %d", g_dgw_role);

out:
        if (js_root)
        {
            json_decref(js_root);
        }
        return;
    }
    else
    {
        return;
    }
}

static void dgwy_dmc_dcs_info_resp(struct evhttp_request *req, void *arg)
{
    json_t *js_root     = NULL;
    struct evbuffer *retbuf   = NULL;

	if (NULL == req)
	{
        log_error(ServiceUtilLogLevel, 
                  "DCS GET Response req is NULL");
        return;
	}

	if (evhttp_request_get_response_code(req) != HTTP_OK) 
    {
        log_error(ServiceUtilLogLevel, 
                  "DCS GET Response code is NOT HTTP_OK");
		return;
	}

    do
    {
        json_t *js_id       = NULL;
        struct evbuffer *res_body = NULL;
        char res_body_str[RESP_BODY_LEN];
        json_error_t jerror;
        int    n=0;
        struct in_addr ipv4_addr;
        const char *dcs_ip;
        int dcs_srvc_port=0;

        retbuf = evbuffer_new();
        if (NULL == retbuf)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }

        res_body = evhttp_request_get_input_buffer(req);
        if (!res_body || evbuffer_get_length(res_body) > (sizeof(res_body_str) - 1))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
        
        n = evbuffer_copyout(res_body, res_body_str, (sizeof(res_body_str) - 1));
        if (n < 1)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
        
        res_body_str[n] = '\0';
        js_root = json_loads(res_body_str, 0, &jerror);
        if (!js_root)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
       
        /* Borrowed reference, no need to decref */
        js_id = json_object_get(js_root, "ip");
        if (json_is_null(js_id))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response no ip object");
            break;
        }
        
        if (NULL == (dcs_ip = json_string_value(js_id)))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response no dcs_ip string");
            break;
        }

        if (0 == inet_pton(AF_INET, dcs_ip, (void *)&ipv4_addr))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response invalid IP ");
            break;
        }

        js_id = json_object_get(js_root, "dcs_raw_service_port");
        if (json_is_null(js_id))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response no dcs_raw_service_port object");
            break;
        }
 
        dcs_srvc_port = json_integer_value(js_id);          

        if (DOVE_STATUS_OK != dgw_rest_api_create_dps (ipv4_addr.s_addr, 
                                                       dcs_srvc_port))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS Server Set Failed ");
            break;
        }

    }while(0);

    if (js_root)
    {
        json_decref(js_root);
    }

    if (retbuf)
    {
        evbuffer_free(retbuf);
    }

}

#if 0
static void dgwy_dmc_dcs_info_resp(struct evhttp_request *req, void *arg)
{
    json_t *js_root     = NULL;
    struct evbuffer *retbuf   = NULL;

	if (NULL == req)
	{
        log_error(ServiceUtilLogLevel, 
                  "DCS GET Response req is NULL");
        return;
	}

	if (evhttp_request_get_response_code(req) != HTTP_OK) 
    {
        log_error(ServiceUtilLogLevel, 
                  "DCS GET Response code is NOT HTTP_OK");
		return;
	}

    do
    {
        json_t *js_id       = NULL;
        json_t *js_entry    = NULL;
        struct evbuffer *res_body = NULL;
        char res_body_str[RESP_BODY_LEN];
        json_error_t jerror;
        int    n=0,size=0;
        struct in_addr ipv4_addr;

        retbuf = evbuffer_new();
        if (NULL == retbuf)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }

        res_body = evhttp_request_get_input_buffer(req);
        if (!res_body || evbuffer_get_length(res_body) > (sizeof(res_body_str) - 1))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
        
        n = evbuffer_copyout(res_body, res_body_str, (sizeof(res_body_str) - 1));
        if (n < 1)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
        
        res_body_str[n] = '\0';
        js_root = json_loads(res_body_str, 0, &jerror);
        if (!js_root)
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response No Buffer");
            break;
        }
       
        /* Borrowed reference, no need to decref */
        js_id = json_object_get(js_root, "dcs_leader");
        if (json_is_null(js_id))
        {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response no dcs_leader object");
            break;
        }

        size = json_array_size(js_id);

        if(size >= 1)
        {
            int json_upk_ret=0;
            char *dcs_ip;
            char *uuid,*timestamp,*build_version;
            char *dcs_config,*dgw_config;
            int ip_family=0;
            int dcs_rest_port=0, dcs_srvc_port=0;
            int dgw_rest_port=0;
            int canBeDCS=0,canBeDGW=0;
            int isDCS=0, isDGW=0;

            js_entry = json_array_get (js_id, 0);
            
            if (NULL == js_entry) 
            {
                /*printf("%s:%d\r\n",__FUNCTION__,__LINE__);*/
                log_error(ServiceUtilLogLevel, 
                          "DCS GET Response no entry ");
                break;
            }
            /* 
            json_upk_ret = json_unpack(js_entry, "{s:s, s:i, s:i}",
                                       "ip",        &dcs_ip,
                                       "rest_port", &rest_port,
                                       "service_port", &srvc_port);
                                       */

            json_upk_ret = json_unpack(js_entry, 
                                       "{s:i, s:s, s:s, s:i,"
                                       " s:i, s:i, s:s, s:s,"
                                       " s:i, s:i, s:b,"
                                       " s:b, s:b, s:b}",
                                       "ip_family", &ip_family,
                                       "ip",        &dcs_ip,
                                       "uuid",      &uuid,
                                       "dcs_rest_service_port", &dcs_rest_port,
                                       "dgw_rest_service_port", &dgw_rest_port,
                                       "dcs_raw_service_port", &dcs_srvc_port,
                                       "timestamp", &timestamp,
                                       "build_version", &build_version,
                                       "dcs_config_version", &dcs_config,
                                       "dgw_config_version", &dgw_config,
                                       "canBeDCS", &canBeDCS,
                                       "canBeDGW", &canBeDGW,
                                       "isDCS", &isDCS,
                                       "isDCS", &isDGW);
            
            if(json_upk_ret)
            {
                log_error(ServiceUtilLogLevel, 
                          "DCS GET Response unpack error ");
                break;
            }


            if (0 == inet_pton(AF_INET, dcs_ip, (void *)&ipv4_addr))
            {
                log_error(ServiceUtilLogLevel, 
                          "DCS GET Response invalid IP ");
                break;
            }
            
            if (DOVE_STATUS_OK != dgw_rest_api_create_dps (ipv4_addr.s_addr, dcs_srvc_port))
            {
                log_error(ServiceUtilLogLevel, 
                          "DCS Server Set Failed ");
                break;
            }

        } else {
            log_error(ServiceUtilLogLevel, 
                      "DCS GET Response: size %d",size);
        }

    }while(0);

    if (js_root)
    {
        json_decref(js_root);
    }

    if (retbuf)
    {
        evbuffer_free(retbuf);
    }

}
#endif

static void dgwy_vxlan_info_resp(struct evhttp_request *req, void *arg)
{
    json_t *js_root     = NULL;
    struct evbuffer *retbuf   = NULL;

	if (NULL == req)
	{
        return;
	}

	if (evhttp_request_get_response_code(req) != HTTP_OK) 
    {
        log_error(ServiceUtilLogLevel, 
                  "VXLAN GET Response code is NOT HTTP_OK");
		return;
	}

    do
    {
        json_t *js_id       = NULL;
        struct evbuffer *res_body = NULL;
        char res_body_str[RESP_BODY_LEN];
        json_error_t jerror;
        int    n=0;
        int port=0;

        retbuf = evbuffer_new();
        if (NULL == retbuf)
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT GET Response No Buffer");
            break;
        }

        res_body = evhttp_request_get_input_buffer(req);
        if (!res_body || evbuffer_get_length(res_body) > (sizeof(res_body_str) - 1))
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT GET Response No Buffer");
            break;
        }
        
        n = evbuffer_copyout(res_body, res_body_str, (sizeof(res_body_str) - 1));
        if (n < 1)
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT GET Response No Buffer");
            break;
        }
        
        res_body_str[n] = '\0';
        js_root = json_loads(res_body_str, 0, &jerror);
        if (!js_root)
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT GET Response No Buffer");
            break;
        }
       
        /* Borrowed reference, no need to decref */
        js_id = json_object_get(js_root, "vxlan_port");
        if (json_is_null(js_id))
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT GET Response No Buffer");
            break;
        }

        port = json_integer_value(js_id);
        
        if (DOVE_STATUS_OK != dgw_rest_vxlan_port(port))
        {
            log_error(ServiceUtilLogLevel, 
                      "VXLAN PORT SET failed");
            break;
        }

    }while(0);

    if (js_root)
    {
        json_decref(js_root);
    }

    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}


struct evhttp_request *dgwy_rest_client_dmc_req_new(dgwy_rest_request_type type)
{
	struct evhttp_request *request = NULL;
    switch(type)
    {
        case DGWY_REGISTER:
        {
            request = evhttp_request_new(dgwy_dmc_register_resp, NULL);
            break;
        }
        case DGWY_HEARTBEAT:
        {
            request = evhttp_request_new(dgwy_dmc_heartbeat_resp, NULL);
            break;
        }
        case DGWY_DCS_GET:
        {
            request = evhttp_request_new(dgwy_dmc_dcs_info_resp, NULL);
            break;
        }
        case DGWY_VXLAN_GET:
        {
            request = evhttp_request_new(dgwy_vxlan_info_resp, NULL);
            break;
        }
        default:
            break;
    }
	return request;
}

int dgwy_rest_client_dmc_send_asyncprocess (char *address,
                                            char *uri,
                                            unsigned short port,
                                            enum evhttp_cmd_type type,
                                            struct evhttp_request *request)
{
	dove_rest_request_info_t *rinfo;
	rinfo = (dove_rest_request_info_t *)malloc(sizeof(dove_rest_request_info_t));
	if(rinfo==NULL)
	{
		log_debug(ServiceUtilLogLevel,
                  "!Can not alloc the dove_rest_request_info_t\r\n");
		return DPS_ERROR;
	}
	memset(rinfo, 0, sizeof(dove_rest_request_info_t));
	
	DOVE_REST_REQ_INFO_INIT(rinfo, address, uri, 
                            port, type, request);

    dove_rest_request_and_asyncprocess(rinfo);

	return DOVE_STATUS_OK;
}



/* 
*\Brief  A more general sending 
         function for PUSHING something 
         to the Dove Controller.
         It encapsulates the json string into a 
         http request and send it to the Dove Controller based on the
         uri and DC's address info
         XXX COPIED it from DPS CODE.
 */

void dgwy_rest_client_to_dmc(json_t *js_res, 
                             char *uri, 
                             enum evhttp_cmd_type cmd_type,
                             dgwy_rest_request_type req_type)
{
	struct evhttp_request *request;
	char *dmc_httpd_addr = NULL;
	char host_header_str[150];
	int dmc_rest_port;
    struct sockaddr_in sa;

	log_info(ServiceUtilLogLevel,
	         "Now send a RESTful HTTP request"
             " to Dove Controller, uri is %s",uri);
	memset(host_header_str,0, 150);
	
	do
	{
        if(js_res == NULL)
        {
			log_error(ServiceUtilLogLevel, 
                      "No js_res,send anything!");
			/* For GET request, there should be 
             * no js_res ,so it is ok to have 
             * js_res == NULL */
		}

		request = dgwy_rest_client_dmc_req_new(req_type);
		if(request == NULL) 
        {
            log_alert(ServiceUtilLogLevel,
                      "Can not alloc the evhttp request");
			break;
		}

		dgwy_rest_client_dmc_fill_evhttp(request,js_res);
		 
        sa.sin_addr.s_addr = g_dmc_ipv4;
		dmc_httpd_addr = inet_ntoa(sa.sin_addr);
		dmc_rest_port = g_dmc_port;

		log_debug(ServiceUtilLogLevel,
		          "Controller IP ==>> [%s : %d]",
		          dmc_httpd_addr,
		          dmc_rest_port);

		if (dmc_rest_port != 80) 
        {
			sprintf(host_header_str,
			        "%s:%d",
                    dmc_httpd_addr,
			        dmc_rest_port);
		}
		else 
        {
			sprintf(host_header_str,"%s",
                    dmc_httpd_addr);
		}

		if(evhttp_find_header(request->output_headers, "Host") == NULL){
				evhttp_add_header(evhttp_request_get_output_headers(request),
				"Host", host_header_str);
		}

		/*send it synchronously, 
         * maybe it should asynchronously 
         * to Dove Controller */

         // Set HTTP Authorization Field
         //  Basic YWRtaW46YWRtaW4=  Base_64 Encoded Value of the String 'Basic admin:admin"
         evhttp_add_header(evhttp_request_get_output_headers(request),
                                 "Authorization", "Basic YWRtaW46YWRtaW4=");


		dgwy_rest_client_dmc_send_asyncprocess
            (dmc_httpd_addr, uri,dmc_rest_port, cmd_type,request);

	} while (0);

	if (js_res)
	{
		json_decref(js_res);

	}
	return;
}

/*
 ******************************************************************************
 * dgwy_form_appliance_registration_json                                        *//**
 *
 * \brief - This function is called to construct a json string to register the
 * 	    DPS Appliance to the DOVE Controller.
 * \retval  packed json body.
 *
 ******************************************************************************
 */

json_t *dgwy_form_appliance_registration_json(void)
{
    json_t *js_root = NULL;
    //int role_assigned = g_dgw_role;
    uint32_t ipv4=0;
    char ipstr[INET6_ADDRSTRLEN];
    int fd;
    struct ifreq ifr;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(fd > 0)
    {
        int ret=0;
        memset(&ifr,0,sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, "APBR", IFNAMSIZ-1);
        ret = ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        if(ret == 0)
        {
            g_mgmt_ipv4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
            ipv4=g_mgmt_ipv4;
        }

	    log_debug(ServiceUtilLogLevel,"g_mgmt_ipv4=%x ret=%d\n",
                  g_mgmt_ipv4,ret);
    }

    if(ipv4)
    {
        inet_ntop(AF_INET, &ipv4, ipstr, INET6_ADDRSTRLEN);
        js_root = json_pack("{s:i,s:s,s:s,s:i,s:i,s:i,s:s}",
                            "ip_family", AF_INET,
                            "ip",ipstr,
                            "uuid",g_node_uuid,
                            "dgw_rest_service_port",g_dgwy_rest_port,
                            "canBeDGW",1,
                            "isDGW",0,
                            "build_version",g_built_version);
    }

    return js_root;
}

/*
 ******************************************************************************
 * dgwy_form_appliance_heartbeat_json                                     *//**
 *
 * \brief - This function is called to construct a json string to register the
 * 	    DPS Appliance to the DOVE Controller.
 * \retval  packed json body.
 *
 ******************************************************************************
 */

json_t *dgwy_form_appliance_heartbeat_json(void)
{
    json_t *js_root = NULL;
    //int role_assigned = g_dgw_role;
    uint32_t ipv4=0;
    char ipstr[INET6_ADDRSTRLEN];
    int fd;
    struct ifreq ifr;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(fd > 0)
    {
        int ret=0;
        memset(&ifr,0,sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, "APBR", IFNAMSIZ-1);
        ret=ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        if(ret == 0)
        {
            uint32_t tmp_mgmt_ipv4=g_mgmt_ipv4;
            g_mgmt_ipv4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
            if(g_mgmt_ipv4 != tmp_mgmt_ipv4)
            {
                /* IP Changed : Enforce re-register */
                g_dmc_register_done=0;
            }
            else
            {
                ipv4=g_mgmt_ipv4;
            }
        }
    }

    if(ipv4)
    {
        if(g_dgw_role==0)
        {
            /* cur version 1 */
            g_dgwconfig_valid_curversion=0;
        }
        inet_ntop(AF_INET, &ipv4, ipstr, INET6_ADDRSTRLEN);

        js_root = json_pack("{s:i, s:s, s:i, s:i, s:i}",
                            "ip_family", AF_INET,
                            "ip",ipstr,
                            "canBeDGW",1,
                            "isDGW",g_dgw_role,
                            "dgw_config_version",g_dgwconfig_valid_curversion);

                            //"uuid",g_node_uuid,
                            //"rest_port",g_dgwy_rest_port,
                            //"role_assigned",role_assigned,
    }
    return js_root;
}



/*
 ******************************************************************************
 * dgwy_dmc_registration                                        *//**
 *
 * \brief - This function is called to register a DPS (DCS) appliance with
 * 	    the Controller (DMC)
 * \retval - None
 ******************************************************************************
 */

void dgwy_dmc_registration(void)
{
    json_t *js_res = NULL;
    char uri[256];

    if((!g_dmc_ipv4) || (!g_dmc_port) )
    {
        log_alert(ServiceUtilLogLevel,
                  "Unknown: DMC Info!\n"); 
        return;
    }

    if(strlen(g_node_uuid) == 0)
    {
        /* UUID not set */
        log_alert(ServiceUtilLogLevel,
                  "UUID not set : DMC registration failed\n");
        return;
    }

    do 
    {
        /* form json string*/
        js_res = dgwy_form_appliance_registration_json();
        
        if(js_res == NULL)
        {
            log_alert(ServiceUtilLogLevel,
                      "Can not get the js_res");
            break;
        }
        /* set the uri */
        snprintf(uri,DGW_URI_MAX_LEN,DGW_APPLIANCE_REGISTRATION_URI);
        dgwy_rest_client_to_dmc(js_res, uri, EVHTTP_REQ_POST, DGWY_REGISTER);
    } while(0);

    if (js_res)
    {
        json_decref(js_res);
    }
    return;
}


void check_mgmt_ip_change(void)
{
    int fd;
    struct ifreq ifr;
    uint32_t mgmt_ipv4=0;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if(fd > 0)
    {
        int ret=0;
        memset(&ifr,0,sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, "APBR", IFNAMSIZ-1);
        ret = ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
        if(ret == 0)
        {
            mgmt_ipv4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
        }

	    log_debug(ServiceUtilLogLevel,"g_mgmt_ipv4=%x ret=%d\n",
                  g_mgmt_ipv4,ret);
    }

    if(mgmt_ipv4!= g_mgmt_ipv4)
    {
        /* MGMT IP changed :
         * Force to re-register */
        g_dmc_register_done=0;
    }
}

/*
 ******************************************************************************
 * dgwy_dmc_heartbeat                                               *//**
 *
 * \brief - This function is called to send heartbeats to dmc
 * \retval - None
 ******************************************************************************
 */

void dgwy_dmc_heartbeat(void)
{
    json_t *js_res = NULL;
    char uri[DGW_URI_MAX_LEN];
    int len = 0;

    check_mgmt_ip_change();

    if((!g_dmc_ipv4) || (!g_dmc_port) ||
       (!g_dmc_register_done))
    {
        return;
    }

    do 
    {
        /* form json string*/
        js_res = dgwy_form_appliance_heartbeat_json();
        
        if(js_res == NULL)
        {
            log_alert(ServiceUtilLogLevel,
                      "Can not get the js_res");
            break;
        }
        /* set the uri */
        len = snprintf(NULL, 0, "%s/%s", 
                       DGW_APPLIANCE_DMC_HEARTBEAT_URI,
                       g_node_uuid);

        if (len >= DGW_URI_MAX_LEN) {
            log_alert(ServiceUtilLogLevel,
                      "Can not set hearbeat URI: %d more/equal %d",
                      len, DGW_URI_MAX_LEN);
            break;
        }

        snprintf(uri,len+1,"%s/%s",
                 DGW_APPLIANCE_DMC_HEARTBEAT_URI,
                 g_node_uuid);

        dgwy_rest_client_to_dmc(js_res, uri, EVHTTP_REQ_PUT, DGWY_HEARTBEAT);
    } while(0);

    if (js_res)
    {
        json_decref(js_res);
    }
    return;
}

/*
 ******************************************************************************
 * dgwy_get_dcs_seed_info                                                 *//**
 *
 * \brief - This function is called to get DPS(DCS) seed info from DMC
 * \retval - None
 ******************************************************************************
 */

void dgwy_get_dcs_seed_info(void)
{
    json_t *js_res = NULL;
    char uri[256];

    if((!g_dmc_ipv4) || (!g_dmc_port) )
    {
        log_alert(ServiceUtilLogLevel,
                  "Unknown: DMC Info!\n"); 
        return;
    }


    do 
    {
        /* set the uri */
        snprintf(uri,DGW_URI_MAX_LEN,DGW_APPLIANCE_DCS_REQUEST_URI);
        dgwy_rest_client_to_dmc(js_res, uri, EVHTTP_REQ_GET, DGWY_DCS_GET);
    } while(0);

    if (js_res)
    {
        json_decref(js_res);
    }
    return;
}


void dgwy_get_vxlan_port(void)
{
    json_t *js_res = NULL;
    char uri[256];

    if((!g_dmc_ipv4) || (!g_dmc_port) )
    {
        log_alert(ServiceUtilLogLevel,
                  "Unknown: DMC Info!\n"); 
        return;
    }


    do 
    {
        /* set the uri */
        snprintf(uri,DGW_URI_MAX_LEN, DGW_VXLAN_REQUEST_URI);
        dgwy_rest_client_to_dmc(js_res, uri, EVHTTP_REQ_GET, DGWY_VXLAN_GET);
    } while(0);

    if (js_res)
    {
        json_decref(js_res);
    }
    return;
}
