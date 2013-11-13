/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


#include <stdbool.h>
#include "include.h"
#include <event.h>
#include <evhttp.h>
#include <evutil.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "../inc/dgadmin_rest_sync.h"
#include "../inc/dgadmin_rest_client.h"
#include "../inc/dgadmin_rest_api.h"



#define DGWY_SYNC_TASK_TIMER_EVENT        0x00000001

#define DGADMIN_REST_SYNC_INTERVAL        16 /* sec */
/*#define DGWY_SYNC_TASK_MSG_EVENT          0x00000002*/

int64_t     gDgwySyncSemId; 
int64_t     gDgwySyncTaskId;
int64_t     gDgwySyncMsgId;
int64_t     gDgwySyncTimerListId;

extern uint32_t g_dmc_ipv4;
extern uint16_t g_dmc_port;
extern uint16_t g_dgwy_rest_port;
extern uint16_t g_dmc_port;
extern char g_node_uuid[40];
extern int v4_addr_to_ip_id (char *str, UINT4 addr);
extern int g_dgw_role;

#define DGADMIN_URI_LEN 128

/**
 * \brief The mutex and condition variable used by thread. 
 */
pthread_cond_t dgadmin_rest_sync_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dgadmin_rest_sync_mp = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief The Interval in which DCS start new sync circle. 
 *        Unit(second)
 */
int dgadmin_rest_sync_interval = 5;

/**
 * \brief The thread ID.
 */
int64_t restsyncTaskId;

/**
 * \brief The last successful config version.
 */
int g_dgwconfig_curversion=1;
int g_dgwconfig_valid_curversion=1;

static dgw_uri_list_t URI_LIST[] = {
    {DGW_SERVICE_NETWORK,"network"},
    {DGW_SERVICE_SUBNETS_DMC_URI,"subnet"},
    {DGW_SERVICE_IPV4_URI,"gw_ipv4_assignment"},
    {DGW_SERVICE_EXTERNAL_VIP_URI, "egw_snat_pool"},
    {DGW_SERVICE_VLANMAP_URI, "vnid_mapping_rule"},
    {DGW_SERVICE_RULE_URI, "egw_fwd_rule"},
    {DGW_SERVICES_URI,"service"},
    {DGW_SERVICE_NICS_URI,"nic"},
};

int strmatch(char *first, char * second)
{
    if (*first == '\0' && *second == '\0') {
        return 1;
    }
 
    if (*first == '*' && *(first+1) != '\0' && *second == '\0') {
        return 0;
    }
 
    if (*first == '?' || *first == *second) {
        return strmatch(first+1, second+1);
    }

    if (*first == '*' && (*(first+1) == '\0') && *second == '/') {
        return 0;
    }
 
    if (*first == '*') {
        return strmatch(first+1, second) || strmatch(first, second+1);
    }

    return 0;
}


char *get_object_name_from_uri(char *uri)
{
    int cnt = sizeof(URI_LIST) /sizeof(URI_LIST[0]);
    int i=0; size_t len = strlen(uri);

    for(i=0; i<cnt; i++) {
        size_t chk_len = strlen(URI_LIST[i].uri);
        log_notice(ServiceUtilLogLevel,"%s[%zu %zu] %s \n",
                   URI_LIST[i].uri,chk_len, len, URI_LIST[i].json_object_name);
        if (chk_len <= len) {
            //if (strncmp(URI_LIST[i].uri, uri, chk_len) == 0) {
            if (strmatch(URI_LIST[i].uri, uri) == 1) {
                log_notice(ServiceUtilLogLevel,
                           "MATCHED: %s %s \n",URI_LIST[i].uri, 
                           URI_LIST[i].json_object_name);
                return URI_LIST[i].json_object_name;
            }
        }
    }

    return NULL;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_g_dgwconfig_curversion_get --                                   *//**
 *
 * \brief This routine returns the last cluster version.
 *
 * \retval g_dgwconfig_curversion 
 *
 *****************************************************************************/
int dgadmin_rest_sync_g_dgwconfig_curversion_get(void)
{
	return g_dgwconfig_curversion;
}

#if 0
/*
 ******************************************************************************
 * dgadmin_rest_sync_version_get_from_req --                                  *//**
 *
 * \brief This routine extracts create version and update version from 
 *        request body of a specific URI
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 *
 * \param [out]	version_create	The creation version.
 * \param [out]	version_update	The last update version.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dgadmin_rest_sync_version_get_from_req(struct evhttp_request *req, int *version_create, int *version_update)
{
	struct evbuffer *req_body = NULL;
    json_t *js_root = NULL;
    json_t *js_version_create = NULL, *js_version_update = NULL;
    json_error_t jerror;
    int buf_len, n;
    int ret = DOVE_STATUS_ERROR;

    do
    {
        /* extract request input body */
        req_body = evhttp_request_get_input_buffer(req);
        if (req_body == NULL)
        {
            log_notice(ServiceUtilLogLevel,"REQ body NULL");
            break;
        }
        
        buf_len = evbuffer_get_length(req_body)+1;
        char req_body_buf[buf_len];
        n = evbuffer_copyout(req_body, req_body_buf, buf_len);
        req_body_buf[n]='\0';
        
        /* extract both create_version and update_version */
        js_root = json_loads(req_body_buf, 0, &jerror);
        if (!js_root)
        {
            log_notice(ServiceUtilLogLevel,"JSON body NULL");
            break;
        }
        
        /* Borrowed reference, no need to decref */
        js_version_create = json_object_get(js_root, "version_create");
        if (!js_version_create || !json_is_integer(js_version_create))
        {
            log_notice(ServiceUtilLogLevel,"ERROR");
            break;
        }
        *version_create = json_integer_value(js_version_create);
        
        js_version_update = json_object_get(js_root, "version_update");
        if (!js_version_update || !json_is_integer(js_version_update))
        {
            log_notice(ServiceUtilLogLevel,"ERROR");
            break;
        }
        *version_update = json_integer_value(js_version_update);
        ret = DOVE_STATUS_OK;
    } while(0);


    if (js_root)
    {
        json_decref(js_root);
    }

    return ret;
}
#endif

/* Get delete uri
 * 1-Success
 * 0-Fail
 * */
static int get_del_uri(char *new_uri, char *deluri,
                       json_t *js_root)
{
    int retval=0;
    json_t *js_objt = NULL;
    do
    {
        if(strcmp(DGW_SERVICE_IPV4S_URI, new_uri)==0)
        {
            struct in_addr ip;
            const char *ip_str = NULL;

            js_objt = json_object_get(js_root, "ip");
            if (js_objt && json_is_string(js_objt))
            {
                 ip_str = json_string_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }

            inet_pton(AF_INET, ip_str, (void *)&ip);
            sprintf(deluri,"%s/%u",new_uri,ip.s_addr);

            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
        }
        else if(strcmp(DGW_SERVICE_EXTERNAL_VIPS_URI,new_uri)==0)
        {
            struct in_addr min_ip;
            struct in_addr max_ip;
            const char *min_ip_str = NULL;
            const char *max_ip_str = NULL;
            char ip_id[IP_ID_LEN + 1] = {0};
            char *ptr = NULL;
            char gw_id_str[2*IP_ID_LEN + 1] = {0};
            int net_id=0;

            memset(ip_id,0,sizeof(ip_id));
            memset(gw_id_str,0,sizeof(gw_id_str));

            js_objt = json_object_get(js_root, "min_ip");
            if (js_objt && json_is_string(js_objt))
            {
                 min_ip_str = json_string_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }

            js_objt = json_object_get(js_root, "max_ip");
            if (js_objt && json_is_string(js_objt))
            {
                 max_ip_str = json_string_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }

            js_objt = json_object_get(js_root, "net_id");
            if (js_objt && json_is_integer(js_objt))
            {
                net_id = json_integer_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }


            inet_pton(AF_INET, min_ip_str, (void *)&min_ip);
            inet_pton(AF_INET, max_ip_str, (void *)&max_ip);
            
            v4_addr_to_ip_id(ip_id, ntohl(min_ip.s_addr));
            strcpy(gw_id_str, ip_id);
            ptr = gw_id_str;
            ptr = ptr + strlen(gw_id_str);
            memset(ip_id, 0, sizeof(ip_id));
            v4_addr_to_ip_id(ip_id, ntohl(max_ip.s_addr));
            strcpy(ptr, ip_id);

            sprintf(deluri,"%s/%s%010d",new_uri,gw_id_str,net_id);

                
            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
            
        }
        else if(strcmp(DGW_SERVICE_RULES_URI,new_uri)==0)
        {
            int net_id          = 0;
            int protocol        = 0;
            int port            = 0;
            const char *ip_str  = NULL;
            char ip_id[IP_ID_LEN + 1] = {0};
            struct in_addr ip;

            js_objt = json_object_get(js_root, "net_id");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            net_id = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "protocol");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            protocol = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "port");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            port = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "ip");
            if (js_objt && json_is_string(js_objt))
            {
                ip_str = json_string_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }

            memset(ip_id,0,sizeof(ip_id));
            inet_pton(AF_INET, ip_str, (void *)&ip);
            v4_addr_to_ip_id(ip_id, ntohl(ip.s_addr));

            sprintf(deluri,"%s/%05d%05d%03d%s",new_uri,net_id,
                    port,protocol,ip_id);
            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
        }
        else if(strcmp(DGW_SERVICE_VLANMAPS_URI,new_uri)==0)
        {
            int net_id          = 0;
            int vlan            = 0;

            js_objt = json_object_get(js_root, "net_id");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            net_id = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "vlan");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            vlan = json_integer_value(js_objt);
            sprintf(deluri,"%s/%05d%05d",new_uri,net_id,vlan);
            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
        }
        else if(strcmp(DGW_SERVICE_SUBNETS_URI, new_uri)==0)
        {
            struct in_addr ip;
            char ip_id[IP_ID_LEN + 1] = {0};
            const char *ip_str = NULL;
            int net_id;

            js_objt = json_object_get(js_root, "ip");
            if (js_objt && json_is_string(js_objt))
            {
                 ip_str = json_string_value(js_objt);
            }
            else
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }

            js_objt = json_object_get(js_root, "net_id");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            net_id = json_integer_value(js_objt);

            memset(ip_id,0,sizeof(ip_id));
            inet_pton(AF_INET, ip_str, (void *)&ip);
            v4_addr_to_ip_id(ip_id, ntohl(ip.s_addr));

            sprintf(deluri,"%s/%05d%s",new_uri,net_id,ip_id);

            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
        }
        else if(strcmp(DGW_SERVICE_NETWORKS, new_uri)==0)
        {
            int net_id;int domainid;int type;

            js_objt = json_object_get(js_root, "net_id");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            net_id = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "domainid");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            domainid = json_integer_value(js_objt);

            js_objt = json_object_get(js_root, "type");
            if(!js_objt || !json_is_integer(js_objt))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
                break;
            }
            type = json_integer_value(js_objt);

            sprintf(deluri,"%s/%d,%d,%d",new_uri,net_id,domainid,type);

            log_notice(ServiceUtilLogLevel,"DELURI:%s",deluri);
            retval=1;
        }
        else
        {
            log_notice(ServiceUtilLogLevel,"ERROR");
        }
    }while(0);

    return retval;

}



/*
 ******************************************************************************
 * dgadmin_rest_sync_version_get_from_buf --                                  *//**
 *
 * \brief This routine extracts create version and update version from a buffer
 *
 * \param [in]  buf 		A pointer to a buffer.
 *
 * \param [out]	version_create	The creation version.
 * \param [out]	version_update	The last update version.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dgadmin_rest_sync_version_get_from_buf(char *buf, char *target_uri, 
                                           int *version_create, int *version_update, 
                                           int *is_tombstone, int *is_uuid, char *deluri)
{
    json_t *js_root = NULL;
    json_t *js_version_create = NULL, *js_version_update = NULL;
    json_error_t jerror;
    int ret = DOVE_STATUS_ERROR;
    char *object_name = NULL;

    do
    {
        /* extract both create_version and update_version */
        js_root = json_loads(buf, 0, &jerror);
        if (!js_root)
        {
            log_notice(ServiceUtilLogLevel,"JSON body NULL");
            break;
        }

        object_name = get_object_name_from_uri(target_uri);

        if (object_name == NULL) {
            log_notice(ServiceUtilLogLevel,"JSON OBJECT NULL for uri %s",
                       target_uri);
            break;
        }

        js_root = json_object_get(js_root, object_name);
        if (!js_root)
        {
            log_notice(ServiceUtilLogLevel,"ERROR: Failed to get JSON object %s",
                       object_name);
            break;
        }

        /* Borrowed reference, no need to decref */
        js_version_create = json_object_get(js_root, "create_version");
        if (!js_version_create || !json_is_integer(js_version_create))
        {
            log_notice(ServiceUtilLogLevel,"ERROR");
            break;
        }
        *version_create = json_integer_value(js_version_create);

        js_version_update = json_object_get(js_root, "change_version");
        if (!js_version_update || !json_is_integer(js_version_update))
        {
            log_notice(ServiceUtilLogLevel,"ERROR");
            break;
        }
        *version_update = json_integer_value(js_version_update);

        js_version_update = json_object_get(js_root, "is_tombstone");
        if (!js_version_update || !json_is_integer(js_version_update))
        {
            /* does not have is_tombstone
             * take it as 0 */
            *is_tombstone = 0;
        }
        else
        {
            *is_tombstone = json_integer_value(js_version_update);
        }


        js_version_update = json_object_get(js_root, "uuid");
        if (js_version_update && json_is_string(js_version_update))
        {
            const char *uuid = json_string_value(js_version_update);
            if(uuid)
            {
                if(strcmp(uuid,g_node_uuid))
                {
                    log_notice(ServiceUtilLogLevel,"UUID NOT match\n");
                    *is_uuid=0;
                }
                else
                {
                    *is_uuid=1;
                }
            }
            else
            {
                *is_uuid=0;
                log_notice(ServiceUtilLogLevel,"ERROR");
            }
        }
        else
        {
            /* NO UUID : This is a generic config for
             * all dgw nodes (subnet)
             * */
            *is_uuid=1;
        }

        if(*is_tombstone)
        {
            /* make the delete URI */
            char new_uri[DGADMIN_URI_LEN];
            char *ptr=NULL;
            int retval=0;
                
            memset(new_uri,0,sizeof(new_uri));
            strcpy(new_uri, target_uri);
            ptr = new_uri + strlen(target_uri);
            while (*ptr != '/')
            {
                ptr--;
            }
            *ptr = '\0';
            retval = get_del_uri(new_uri,deluri,js_root);
            if(retval==0)
            {
                break;
            }
        }
        ret = DOVE_STATUS_OK;
    } while(0);


    if (js_root)
    {
        json_decref(js_root);
    }

    return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_version_resp_parser --                                   *//**
 *
 * \brief This routine parses response of version GET request
 *
 * \param [in]	body_buf	A pointer to a version response buffer. 
 *
 * \param [out]	version		The version.
 * \param [out]	target_uri	The URI of the HTTP request.
 * \param [out]	operation	The operation which acts on the URI. Now
 * 				support GET and DELETE.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dgadmin_rest_sync_version_resp_parser(char *body_buf, int *version, char *target_uri, int *operation)
{
	json_t *js_root = NULL;
	json_t *js_version = NULL, *js_uri = NULL, *js_operation = NULL;
	json_error_t jerror;
	const char *uri, *operation_str;
	int ret = DOVE_STATUS_ERROR;

	do
	{
		js_root = json_loads(body_buf, 0, &jerror);
		if (!js_root)
		{
			log_notice(ServiceUtilLogLevel,"JSON body NULL");
			break;
		}

		/* Borrowed reference, no need to decref */
		js_version = json_object_get(js_root, "next_change");
		if (!json_is_integer(js_version))
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		*version = json_integer_value(js_version);

		js_uri = json_object_get(js_root, "uri");
		if (!json_is_string(js_uri))
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		if ((uri = json_string_value(js_uri)) == NULL)
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		strcpy(target_uri, uri);

		js_operation = json_object_get(js_root, "method");
		if (!json_is_string(js_operation))
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		if ((operation_str = json_string_value(js_operation)) == NULL)
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		if (!strcmp(operation_str, "GET"))
		{
			*operation = DGADMIN_REST_SYNC_OPERATION_GET;
		}
		else if (!strcmp(operation_str, "DELETE"))
		{
			*operation = DGADMIN_REST_SYNC_OPERATION_DELETE;
		}
		else
		{
            if(uri && (strlen(uri)))
            {
                log_notice(ServiceUtilLogLevel,"ERROR");
			    break;
            }
		}

		ret = DOVE_STATUS_OK;
	} while(0);


	if (js_root)
	{
		json_decref(js_root);
	}

	return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_response_handler --                                      *//**
 *
 * \brief This routine handles response: 
 *        . copy out request input buffer 
 *        . return response code 
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 *
 * \param [out] arg		A pointer to a dgadmin_rest_sync_response_args_t
 *                              data structure including a pointer to input buffer
 *                              and response code.
 *
 * \retval None
 *
 *****************************************************************************/
static void dgadmin_rest_sync_response_handler(struct evhttp_request *req, void *arg)
{
	dgadmin_rest_sync_response_args_t *args = (dgadmin_rest_sync_response_args_t *)arg;
	struct evbuffer *req_body = NULL;
	unsigned int buf_len;
	int n;

	/* sanity check */
	if (req == NULL)
	{
        log_notice(ServiceUtilLogLevel,"ERROR");
		return;
	}

	/* extract request body */
	if ((req_body = evhttp_request_get_input_buffer(req)) == NULL)
	{
        log_notice(ServiceUtilLogLevel,"ERROR");
		return;
	}
	buf_len = evbuffer_get_length(req_body)+1;
	args->req_body_buf = (char *)malloc(buf_len);
	n = evbuffer_copyout(req_body, args->req_body_buf, buf_len);
	args->req_body_buf[n]='\0';

	args->res_code = evhttp_request_get_response_code(req);

	return;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_dmc_agent --                                             *//**
 *
 * \brief This routine constructs new request and sends to REST server.
 *
 * \param [in]	req_body_buf	A pointer to request input body buffer.
 * \param [in]  cmd_type	The method of the HTTP request.
 * \param [in]  uri		The URI of the HTTP request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dgadmin_rest_sync_dmc_agent(char *req_body_buf, 
                                enum evhttp_cmd_type cmd_type, 
                                const char *uri)
{
	struct evhttp_request *new_request = NULL;
	struct evbuffer *new_req_body = NULL;
	char ip_addr_str[]="127.0.0.1";
	dgadmin_rest_sync_response_args_t args;
	int ret = DOVE_STATUS_OK;

	do
	{
		/* step 1 - construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dgadmin_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
            log_notice(ServiceUtilLogLevel,"ERROR");
			ret = DOVE_STATUS_NO_MEMORY;
			break;
		}
		if (req_body_buf != NULL)
		{
			new_req_body = evbuffer_new();
			if (new_req_body == NULL)
			{
				evhttp_request_free(new_request);
				ret = DOVE_STATUS_NO_MEMORY;
                log_notice(ServiceUtilLogLevel,"ERROR");
				break;
			}
			evbuffer_add(new_req_body, req_body_buf, strlen(req_body_buf));
			evbuffer_add_buffer(evhttp_request_get_output_buffer(new_request), new_req_body);
		}

		/* step 2 - forward the new request to local REST handler */

		log_notice(ServiceUtilLogLevel, "Routing %s REQ to local REST handler[%s]", uri, ip_addr_str);
		ret = dove_rest_request_and_syncprocess(ip_addr_str, g_dgwy_rest_port,
                                                cmd_type, uri,
                                                new_request, NULL,
                                                DGADMIN_REST_SYNC_CONNECT_TIMEOUT);
		if (new_req_body)
		{
			evbuffer_free(new_req_body);
		}

		if (ret)
		{
			ret = DOVE_STATUS_ERROR;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			ret = DOVE_STATUS_ERROR;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
	} while (0);

	if (args.req_body_buf)
	{
		free(args.req_body_buf);
	}
	return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_target_uri_get --                                        *//**
 *
 * \brief This routine sends a GET request for target URI to DMC.
 *
 * \param [in]  uri		The URI of the HTTP request.
 *
 * \param [out]	req_body_buf	A pointer to request input body buffer.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dgadmin_rest_sync_target_uri_get(const char *uri, char **req_body_buf, int *resp_code)
{
	struct evhttp_request *new_request = NULL;
	dgadmin_rest_sync_response_args_t args;
	char host_header_str[128];
	int ret = DOVE_STATUS_OK;

	do
	{
        struct sockaddr_in sa;
	    char *dmc_httpd_addr = NULL;
		/* construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dgadmin_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
			ret = DOVE_STATUS_NO_MEMORY;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}

        sa.sin_addr.s_addr = g_dmc_ipv4;
        dmc_httpd_addr = inet_ntoa(sa.sin_addr);
		sprintf(host_header_str,"%s:%d", dmc_httpd_addr, g_dmc_port);

		evhttp_add_header(evhttp_request_get_output_headers(new_request), "Host", host_header_str);
        
        /* Set HTTP Authorization Field
          Basic YWRtaW46YWRtaW4=  Base_64 Encoded Value of the String 'Basic admin:admin" */
        evhttp_add_header(evhttp_request_get_output_headers(new_request),
                          "Authorization", "Basic YWRtaW46YWRtaW4=");


		/* send the GET request for target URI to DMC */
		log_notice(ServiceUtilLogLevel, "Sending %s GET REQ to DMC[%s:%d]", 
                  uri, dmc_httpd_addr, g_dmc_port);
		ret = dove_rest_request_and_syncprocess(dmc_httpd_addr,
                                                g_dmc_port,
                                                EVHTTP_REQ_GET, uri,
                                                new_request, NULL,
                                                DGADMIN_REST_SYNC_CONNECT_TIMEOUT);

		*req_body_buf = args.req_body_buf;

		if (ret)
		{
			ret = DOVE_STATUS_ERROR;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
        *resp_code = args.res_code;
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			ret = DOVE_STATUS_ERROR;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}
	} while (0);

	return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_version_query --                                         *//**
 *
 * \brief This routine sends version GET request to DMC.
 *
 * \param [out]	req_body_buf	A pointer to request input body buffer.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dgadmin_rest_sync_version_query(char **req_body_buf, int *resp)
{
	struct evhttp_request *new_request = NULL;
	dgadmin_rest_sync_response_args_t args;
	char uri[DGADMIN_URI_LEN];
	char host_header_str[128];
	int ret = DOVE_STATUS_OK;

    if((!g_dmc_ipv4) || (!g_dmc_port) )
    {
        log_alert(ServiceUtilLogLevel,
                  "Unknown: DMC Info!\n"); 
        return 0;
    }


	do
	{
        struct sockaddr_in sa;
	    char *dmc_httpd_addr = NULL;
		/* construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dgadmin_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
			ret = DOVE_STATUS_NO_MEMORY;
            log_notice(ServiceUtilLogLevel,"ERROR");
			break;
		}

        sa.sin_addr.s_addr = g_dmc_ipv4;
        dmc_httpd_addr = inet_ntoa(sa.sin_addr);
		sprintf(host_header_str,"%s:%d", dmc_httpd_addr, g_dmc_port);

		evhttp_add_header(evhttp_request_get_output_headers(new_request), "Host", host_header_str);

        /* Set HTTP Authorization Field
          Basic YWRtaW46YWRtaW4=  Base_64 Encoded Value of the String 'Basic admin:admin" */
        evhttp_add_header(evhttp_request_get_output_headers(new_request),
                          "Authorization", "Basic YWRtaW46YWRtaW4=");

		/* send the GET request to DMC */
		memset(uri, 0, sizeof(uri));
		sprintf(uri, "%s/%d", DGADMIN_REST_SYNC_VERSION_URI, g_dgwconfig_curversion);
		log_notice(ServiceUtilLogLevel, "Sending %s GET REQ to DMC[%s:%d]", 
                  uri, dmc_httpd_addr, g_dmc_port);
		ret = dove_rest_request_and_syncprocess(dmc_httpd_addr,
                                                g_dmc_port,
                                                EVHTTP_REQ_GET, uri,
                                                new_request, NULL,
                                                DGADMIN_REST_SYNC_CONNECT_TIMEOUT);

		*req_body_buf = args.req_body_buf;

		if (ret)
		{
            log_alert(ServiceUtilLogLevel,
                      "ERROR:REST syncprocess\n");
			ret = DOVE_STATUS_ERROR;
			break;
		}
        *resp = args.res_code;
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
            log_alert(ServiceUtilLogLevel,
                      "ERROR:REST DMC changeversion respose %d\n",args.res_code);
			ret = DOVE_STATUS_ERROR;
			break;
		}
	} while (0);

	return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_process --                                               *//**
 *
 * \brief This routine starts a new sync circle.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dgadmin_rest_sync_process(void)
{
	char *version_query_body_buf = NULL, *target_uri_body_buf = NULL, *ptr = NULL;
	char target_uri[DGADMIN_URI_LEN];
    char new_uri[DGADMIN_URI_LEN];
	enum evhttp_cmd_type cmd_type;
	int version;
	int operation;
	int version_create, version_update;
	bool more = false;
	int ret = DOVE_STATUS_OK;
    int is_tombstone=0;
    int is_uuid=0;
    int resp_code=0;
	char deluri[DGADMIN_URI_LEN];

	do
	{
		do
		{
			/* step 1 - send version query request to DMC */
			ret = dgadmin_rest_sync_version_query(&version_query_body_buf, &resp_code);
			if (ret != DOVE_STATUS_OK)
			{
                log_alert(ServiceUtilLogLevel,
                          "ERROR\n");
                more = false;
				break;
			}

			/* step 2.a - parse and extract version/uri/cmd_type from response */
			memset(target_uri, 0, sizeof(target_uri));
			ret = dgadmin_rest_sync_version_resp_parser(version_query_body_buf, 
                                                        &version, target_uri, 
                                                        (int *)&operation);
			if (ret != DOVE_STATUS_OK)
			{
                log_alert(ServiceUtilLogLevel,
                          "ERROR\n");
                more = false;
				break;
			}
            else
            {
                if(strlen(target_uri)==0)
                {
                    g_dgwconfig_curversion = version;
                    /* NO URI: go to next version */
                    more = true;
                    break;
                }
            }

            if((operation == DGADMIN_REST_SYNC_OPERATION_GET)||
               (operation == DGADMIN_REST_SYNC_OPERATION_DELETE))
            {
                resp_code=0;
                /* step 2.b - send the GET request for target URI to DMC */
                ret = dgadmin_rest_sync_target_uri_get(target_uri, &target_uri_body_buf,&resp_code);
                if (ret != DOVE_STATUS_OK)
                {
                    if(resp_code==204)
                    {
                        /* content not found 
                         * SKIP this version */
                        g_dgwconfig_valid_curversion = g_dgwconfig_curversion;
                        g_dgwconfig_curversion = version;
                        more = true;
                    }
                    else
                    {
                        log_alert(ServiceUtilLogLevel,
                                  "ERROR %d\n",resp_code);
                        more = false;
                    }
                    break;
                }

                memset(deluri,0,sizeof(deluri));
                ret = dgadmin_rest_sync_version_get_from_buf(target_uri_body_buf, 
                                                             target_uri,
                                                             &version_create, 
                                                             &version_update,
                                                             &is_tombstone,
                                                             &is_uuid, deluri);
                if (ret != DOVE_STATUS_OK)
                {
                    log_alert(ServiceUtilLogLevel, "Fail to get version_create and version_update!");
                    more = false;
                    break;
                }

                if(is_uuid==0)
                {
                    /* SKIP this version */
                    g_dgwconfig_valid_curversion = g_dgwconfig_curversion;
                    g_dgwconfig_curversion = version;
                    more = true;
                    break;
                }


                if (is_tombstone && 
                    (version_create != g_dgwconfig_curversion))
                    /* tombstoned entry ! still lets run thru create 
                     * if we are at create version;
                     * expected change/delete version later*/
                {
                    cmd_type = EVHTTP_REQ_DELETE;
                    if(strlen(deluri) == 0)
                    {
                        log_alert(ServiceUtilLogLevel, "ERROR\n");
                        break;
                    }
                    /* step 3.c - route the DELETE response to dmc agent */
                    ret = dgadmin_rest_sync_dmc_agent(NULL, cmd_type, deluri);
                    if (ret != DOVE_STATUS_OK)
                    {
                        log_alert(ServiceUtilLogLevel, "ERROR\n");
                    }
                }
                else
                {
                    strcpy(new_uri, target_uri);
                    
                    /* step 3.b - 
                     * build new uri. for example, /api/dove/dgw/xyz/1 => /api/dove/dgw/xyz
                     * */
                    ptr = new_uri + strlen(target_uri);
                    while (*ptr != '/')
                    {
                        ptr--;
                    }
                    *ptr = '\0';

                    cmd_type = EVHTTP_REQ_POST;
                    log_alert(ServiceUtilLogLevel, "POST %s", new_uri);
                    log_alert(ServiceUtilLogLevel, "%s\r\n",target_uri_body_buf);

                    /* step 3.c - route the GET response to dmc agent */
                    ret = dgadmin_rest_sync_dmc_agent(target_uri_body_buf, cmd_type, new_uri);
                    if (ret != DOVE_STATUS_OK)
                    {
                        log_alert(ServiceUtilLogLevel, "ERROR\n");
                    }
                }

                g_dgwconfig_valid_curversion = g_dgwconfig_curversion;
                g_dgwconfig_curversion = version;
                log_notice(ServiceUtilLogLevel,"curversion %d valid_curversion %d\r\n",
                           g_dgwconfig_curversion, g_dgwconfig_valid_curversion);
                more = true;

            }
#if 0
            else if(operation == DGADMIN_REST_SYNC_OPERATION_DELETE)
            {
                cmd_type = EVHTTP_REQ_DELETE;
                ret = dgadmin_rest_sync_dmc_agent(NULL, cmd_type, target_uri);
                if (ret != DOVE_STATUS_OK)
                {
                    log_alert(ServiceUtilLogLevel,
                              "ERROR\n");
                    //break;
                }
            
                /* step 4 - update the cluster version if all things success */
                g_dgwconfig_curversion = version;
                more = true;                
            }
#endif
            else
            {
                ret = DOVE_STATUS_NOT_SUPPORTED;
                log_alert(ServiceUtilLogLevel,
                          "ERROR\n");
                break;
            }
        } while(0);

		if (version_query_body_buf)
		{
			free(version_query_body_buf);
			version_query_body_buf = NULL;
		}
		if (target_uri_body_buf)
		{
			free(target_uri_body_buf);
			target_uri_body_buf = NULL;
		}
        /*
		if (ret != DOVE_STATUS_OK)
		{
			more = false;
		}
        */
	} while (more);

	return ret;
}

/*
 ******************************************************************************
 * dgadmin_rest_sync_main --                                                  *//**
 *
 * \brief This routine periodically starts sync circle.
 *
 * \param [in] pDummy	Not used.
 *
 * \retval None
 *
 *****************************************************************************/
static void dgadmin_rest_sync_main(UINT1 *pDummy)
{
    unsigned int 			listenEvent   = 0;
    unsigned int 			recvEvent     = 0;

    if (sem_give(gDgwySyncSemId) != OSW_OK) 
	{
		return;
    }
    if (create_queue("RSYNC", OSW_MAX_Q_MSG_LEN, 10, &gDgwySyncMsgId) != OSW_OK)
    {
        printf("%s:error when create queue\n",__FUNCTION__);
        return;
    }

    if (create_timer("RSTASK", DGWY_SYNC_TASK_TIMER_EVENT,
                     NULL, 0, &gDgwySyncTimerListId) != OSW_OK) 
    {
        printf("%s:error when create_timer\n",__FUNCTION__);
        return;
    }

    /* start the timer */
    start_timer(gDgwySyncTimerListId, DGADMIN_REST_SYNC_INTERVAL, 0);

    listenEvent = DGWY_SYNC_TASK_TIMER_EVENT;/* | DGWY_SYNC_TASK_MSG_EVENT; */

    log_info(ServiceUtilLogLevel,
             "INIT REST SYNC TASK ");

	while (1) 
    {
        if(recv_event(gDgwySyncTaskId, listenEvent, OSW_NO_WAIT, &recvEvent) == OSW_ERROR)
        {
            sleep(10);
            continue;
        }
    
        log_debug(ServiceUtilLogLevel,"recvEvent %d\n",
                  recvEvent);

        if(recvEvent & DGWY_SYNC_TASK_TIMER_EVENT)
        {
            if(g_dgw_role)
            {
                /* Try to be in sync with DMC */
                log_notice(ServiceUtilLogLevel,
                           "SYNC START: version %d\n", 
                           g_dgwconfig_curversion);
                dgadmin_rest_sync_process();
                log_notice(ServiceUtilLogLevel,
                           "SYNC END : version %d\n",
                           g_dgwconfig_curversion);
            }
            start_timer(gDgwySyncTimerListId, DGADMIN_REST_SYNC_INTERVAL, 0);
        }
        
    }
}


/*
 ******************************************************************************
 * dgadmin_rest_sync_init --                                                  *//**
 *
 * \brief This routine creates sync task and resource.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
dove_status dgadmin_rest_sync_init(void)
{
	dove_status retval = DOVE_STATUS_OK;
    
    if (create_sem("RSSEM", 0, 0, &gDgwySyncSemId) != OSW_OK)
    {
        printf("Failed to create semaphore \n");
        retval = DOVE_STATUS_ERROR;
        goto init_exit;
    }
            
    /* Create the system task */
    if (create_task("RSTASK", 10,
                    OSW_DEFAULT_STACK_SIZE,
                    (task_entry) dgadmin_rest_sync_main,
                    0, &gDgwySyncTaskId) != OSW_OK) 
    {
        printf("Failed to create task \n");
        goto init_failed;
    }

    if (sem_take(gDgwySyncSemId) != OSW_OK) 
    {
        retval = DOVE_STATUS_ERROR;
        goto init_failed;
    }
    else
    {
        goto init_exit;
    }

init_failed:
    del_sem(gDgwySyncSemId);

init_exit:
    return retval;
}


