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
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "../inc/dgadmin_rest_api.h"
#include "../inc/dgadmin_evhttp_helper.h"
#include "../inc/dgadmin_rest_req_handler.h"
#include "../inc/dgadmin_rest_client.h"
//#define _DPS_REST_DEBUG
int64_t httpsrvTaskId;
#ifdef _DPS_REST_DEBUG
int64_t TestTaskId;
static void http_response_handler(struct evhttp_request *req, void *arg)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;

	if (NULL == req)
	{
		return;
	}
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

	printf("Received a %s request for %s\r\nHeaders:\r\n",
	    cmdtype, evhttp_request_get_uri(req));

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("%s: %s\r\n", header->key, header->value);
	}

	buf = evhttp_request_get_input_buffer(req);
	printf("Input data: <<<");
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[128];
		n = evbuffer_remove(buf, cbuf, sizeof(buf)-1);
		if (n > 0)
			(void) fwrite(cbuf, 1, n, stdout);
	}
	printf(">>>\n");
}
void http_test(struct evhttp_request *req, void *arg)
{
    evhttp_send_reply(req, HTTP_OK, NULL, NULL);
    send_event (TestTaskId, (UINT4)arg);
}

static void
http_test_main (INT1 *pDummy)
{
    UINT4 u4Event = 0;
    struct evhttp_request *request;
    dove_rest_request_info_t *rinfo;
    int i;
    while(1)
    {
        recv_event (TestTaskId, 1|2|4,
                    (OSW_WAIT | OSW_EV_ANY), &u4Event);
        request = evhttp_request_new(http_response_handler, NULL);
        if (NULL != request)
        {
            if((u4Event&1)==1)
            {
                dove_rest_request_and_syncprocess("127.0.0.1",1888, 
                    EVHTTP_REQ_GET, "/api/dps/domains", request, NULL, 20);
            }
            else if((u4Event&2)==2)
            {
                dove_rest_request_and_syncprocess("192.0.0.1",1888, 
                    EVHTTP_REQ_GET, "/api/dps/domains", request, NULL, 1);
            }
            else if((u4Event&4)==4)
            {
                for (i = 0; i < 100; i++)
                {
                    request = evhttp_request_new(http_response_handler, NULL);
                    rinfo = (dove_rest_request_info_t *)malloc(sizeof(dove_rest_request_info_t));
                    if(rinfo==NULL)
                    {
                        printf("!\r\n");
                        continue;
                    }
                    memset(rinfo, 0, sizeof(dove_rest_request_info_t));
                    if (i%2==0)
                    {
                        DOVE_REST_REQ_INFO_INIT(rinfo, "127.0.0.1", "/api/dps/domains", 1888, EVHTTP_REQ_GET, request);
                    }
                    else
                    {
                        DOVE_REST_REQ_INFO_INIT(rinfo, "192.168.1.2", "/api/dps/domains", 1888, EVHTTP_REQ_GET, request);
                    }

                    dove_rest_request_and_asyncprocess(rinfo);
                }
            }
        }
    }
}


#endif
static struct evhttp *resthttpd_server = NULL;
static void http_request_handler(struct evhttp_request *req, void *arg)
{
    int ret = -1;
    const char *uri;

    uri = evhttp_request_get_uri(req);

    if (NULL != uri)
    {
        ret = helper_evhttp_request_dispatch(req, uri);
    }
    if(ret)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
    }
    return;
}

#if 0
/* Below version has the 
 * problem associated with bind.
 * Need an IP address to bind
 * else
 * throw error
 * getaddrinfo family nodename  not supported
 * */
static void
http_server_main (INT1 *pDummy)
{
	short resthttp_port = DPS_REST_LISTEN_DEFAULT_PORT;

	const char *resthttpd_addr = DPS_REST_LISTEN_DEFAULT_IP;
	struct event_base *base;
	struct evhttp_bound_socket *handle;

	base = event_base_new();
	if (!base) 
	{
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		return;
	}
	resthttpd_server = evhttp_new(base);
	
	if (!resthttpd_server)
	{
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		return;
	}

	handle = evhttp_bind_socket_with_handle(resthttpd_server, resthttpd_addr, resthttp_port);
    if(!handle)
    {
		fprintf(stderr, "\nCouldn't bind to port %d, will retry\n",
                (int)resthttp_port);
    }
	while(!handle) 
	{
        sleep(5);
	    handle = evhttp_bind_socket_with_handle(resthttpd_server, resthttpd_addr, resthttp_port);
	}
    
    helper_evhttp_set_cb_pattern(DGW_DPSES_URI, dgw_req_handler_dpses, NULL);
    helper_evhttp_set_cb_pattern(DGW_DPS_URI, dgw_req_handler_dps, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICES_URI, dgw_req_handler_services, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_URI, dgw_req_handler_service, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_NICS_URI, dgw_req_handler_service_nics, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_NIC_URI, dgw_req_handler_service_nic, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_IPV4S_URI, dgw_req_handler_service_ipv4s, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_IPV4_URI, dgw_req_handler_service_ipv4, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_VIPS_URI, dgw_req_handler_svc_ext_vips, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_VIP_URI, dgw_req_handler_svc_ext_vip, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_INTERNAL_VIPS_URI, dgw_req_handler_service_internal_vips, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_INTERNAL_VIP_URI, dgw_req_handler_service_internal_vip, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_RULES_URI, dgw_req_handler_service_rules, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_RULE_URI, dgw_req_handler_service_rule, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_VLANMAPS_URI, dgw_req_handler_service_vlanmaps, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_VLANMAP_URI, dgw_req_handler_service_vlanmap, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_SUBNETS_URI, dgw_req_handler_service_subnets, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_SUBNET_URI, dgw_req_handler_service_subnet, NULL);
    helper_evhttp_set_cb_pattern(DGW_APPLIANCE_DMC_INFO_URI, dgw_req_handler_service_dmc, NULL);
#ifdef _DPS_REST_DEBUG
    evhttp_set_cb(resthttpd_server, "/test1", http_test, (void *)1);
    evhttp_set_cb(resthttpd_server, "/test2", http_test, (void *)2);
    evhttp_set_cb(resthttpd_server, "/test4", http_test, (void *)4);
#endif
    evhttp_set_gencb(resthttpd_server, http_request_handler, NULL);
    evhttp_set_timeout(resthttpd_server, 20);
    /* fprintf(stderr, "REST HTTP Service started on %s:%d\r\n",resthttpd_addr,resthttp_port); */
    event_base_dispatch(base);
}
#endif 

int event_bind_socket(int port) 
{
    int ret;
    int nfd;
    int one_val = 1;
    struct sockaddr_in addr;
    int flags;

    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd < 0) return -1;

    ret = setsockopt(nfd, SOL_SOCKET, 
                     SO_REUSEADDR, 
                     (char *)&one_val, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    /*inet_aton(appsyshttpd_addr, &addr.sin_addr);*/
    addr.sin_port = htons(port);

    ret = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) return -1;
    ret = listen(nfd, 256);
    if (ret < 0) return -1;

    if ((flags = fcntl(nfd, F_GETFL, 0)) < 0
      || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return -1;
    }
    return nfd;
}


static void
http_server_main (INT1 *pDummy)
{
	short resthttp_port = DPS_REST_LISTEN_DEFAULT_PORT;
	/*const char *resthttpd_addr = DPS_REST_LISTEN_DEFAULT_IP;*/
	struct event_base *base;
	/*struct evhttp_bound_socket *handle; */
    int nfd=-1;
    int afd=-1;

	base = event_base_new();
	if (!base) 
	{
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		return;
	}
	resthttpd_server = evhttp_new(base);
	
	if (!resthttpd_server)
	{
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		return;
	}

  
    nfd = event_bind_socket(resthttp_port);
    if(nfd < 0)
    {
        printf("evhttp_bind_socket failed\n");
        return ;
    }

    afd = evhttp_accept_socket(resthttpd_server, nfd);
    if (afd != 0)
    {
        printf("evhttp_accept_socket failed\n");
        return ;
    }

    helper_evhttp_set_cb_pattern(DGW_SERVICES_URI, dgw_req_handler_services, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_URI, dgw_req_handler_service, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_NICS_URI, dgw_req_handler_service_nics, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_NIC_URI, dgw_req_handler_service_nic, NULL);
    helper_evhttp_set_cb_pattern(DGW_DPSES_URI, dgw_req_handler_dpses, NULL);
    helper_evhttp_set_cb_pattern(DGW_DPS_URI, dgw_req_handler_dps, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_IPV4S_URI, dgw_req_handler_service_ipv4s, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_IPV4_URI, dgw_req_handler_service_ipv4, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_VIPS_URI, dgw_req_handler_svc_ext_vips, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_VIP_URI, dgw_req_handler_svc_ext_vip, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_INTERNAL_VIPS_URI, dgw_req_handler_service_internal_vips, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_INTERNAL_VIP_URI, dgw_req_handler_service_internal_vip, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_RULES_URI, dgw_req_handler_service_rules, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_RULE_URI, dgw_req_handler_service_rule, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_VLANMAPS_URI, dgw_req_handler_service_vlanmaps, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_VLANMAP_URI, dgw_req_handler_service_vlanmap, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_SUBNETS_URI, dgw_req_handler_service_subnets, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_SUBNET_URI, dgw_req_handler_service_subnet, NULL);
    helper_evhttp_set_cb_pattern(DGW_APPLIANCE_DMC_INFO_URI, dgw_req_handler_service_dmc, NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXT_SESSION_URI, dgw_req_handler_serivce_sessions_ext,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_INT_SESSION_URI, dgw_req_handler_serivce_sessions_int,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_FWDDYN_SESSION_URI, dgw_req_handler_serivce_fwddyn_sessions,NULL);
    helper_evhttp_set_cb_pattern(DGW_OVERLAY_PORT_NUMBER, dgw_req_handler_serivce_ovlport,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_MCASTS_URI, dgw_req_handler_serivce_mcasts_external,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_EXTERNAL_MCAST_URI, dgw_req_handler_serivce_mcast_external,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_VNID_STATS_URI, dgw_req_handler_vnid_stats,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_STATS_URI, dgw_req_handler_stats,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_RESETSTATS_URI, dgw_req_handler_resetstats, NULL);

    helper_evhttp_set_cb_pattern(DGW_SERVICE_NETWORKS, dgw_req_handler_networks,NULL);
    helper_evhttp_set_cb_pattern(DGW_SERVICE_NETWORK,  dgw_req_handler_network,NULL);

#ifdef _DPS_REST_DEBUG
    evhttp_set_cb(resthttpd_server, "/test1", http_test, (void *)1);
    evhttp_set_cb(resthttpd_server, "/test2", http_test, (void *)2);
    evhttp_set_cb(resthttpd_server, "/test4", http_test, (void *)4);
#endif
    evhttp_set_gencb(resthttpd_server, http_request_handler, NULL);
    evhttp_set_timeout(resthttpd_server, 20);
    /* fprintf(stderr, "REST HTTP Service started on %s:%d\r\n",resthttpd_addr,resthttp_port); */
    event_base_dispatch(base);
}


/*
 ******************************************************************************
 * dps_server_rest_init                                                    *//**
 *
 * \brief - Initializes the DPS Server REST infrastructure
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_server_rest_init(void)
{
    int ret = 0;
    ret = dove_rest_client_init();
    if(ret)
    {
        return DOVE_STATUS_RESTC_INIT_FAILED;
    }

    if (create_task("RSTS", 0, 
        OSW_DEFAULT_STACK_SIZE, (task_entry) http_server_main,
		0, &httpsrvTaskId) != OSW_OK)
    {
        return DOVE_STATUS_THREAD_FAILED;
    }

#ifdef _DPS_REST_DEBUG
	if (create_task("TEST", 0, 
		OSW_DEFAULT_STACK_SIZE, (task_entry) http_test_main,
		0, &TestTaskId) != OSW_OK)
	{
		return DOVE_STATUS_THREAD_FAILED;
	}
#endif

    return DOVE_STATUS_OK;
}
