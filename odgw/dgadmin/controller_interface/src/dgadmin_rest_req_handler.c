/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#include "include.h"
#include <event.h>
#include <evhttp.h>
#include <evutil.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "../inc/dgadmin_rest_req_handler.h"
#include "../inc/dgadmin_rest_api.h"
#include "dgadmin_generic_api.h"
#include "cli_interface.h"

service_id_key_map service_map[MAX_SERVICE_TABLES];

int v4_addr_to_ip_id (char *str, UINT4 addr)
{
    int ret = -1;

    if (NULL != str)
    {
        sprintf (str, "%03u%03u%03u%03u", ((addr >> 24) & 0x00FF), ((addr >> 16) & 0x00FF), ((addr >> 8) & 0x00FF), (addr & 0x00FF));
        ret = 0;
    }

    return ret;
}

int v4_addr_to_ip_str (char *str, UINT4 addr)
{
    int ret = -1;

    if (NULL != str)
    {
        sprintf (str, "%u.%u.%u.%u", ((addr >> 24) & 0x00FF), ((addr >> 16) & 0x00FF), ((addr >> 8) & 0x00FF), (addr & 0x00FF));
        ret = 0;
    }

    return ret;
}

UINT4 ip_id_to_v4_addr (char *str, UINT4 *addr)
{
    UINT4 ret = -1;
        int ip1,ip2,ip3,ip4;

    if (NULL != str && NULL != addr && strlen(str) == IP_ID_LEN)
    {
        if (4 == sscanf(str, "%03d%03d%03d%03d", &ip1, &ip2, &ip3, &ip4))
        {
            *addr = (((ip1 << 24) & 0xFF000000)
                            | ((ip2 << 16) & 0x00FF0000)
                | ((ip3 << 8) & 0x0000FF00)
                | (ip4 & 0x000000FF));
            ret = 0;
        }
    }

    return ret;
}

int mac_str_to_byte (char *str, char *mac)
{
    UINT4 ret = -1;
    char tmp[12];
        
    if (NULL != str && NULL != mac && MAC_STR_LEN == strlen(str))
    {
        if (6 == sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x", (unsigned int *)&tmp[0], (unsigned int *)&tmp[1], (unsigned int *)&tmp[2], (unsigned int *)&tmp[3],(unsigned int *)&tmp[4],(unsigned int *)&tmp[5]))                        
        {
            memcpy (mac, tmp, 6);
                ret = 0;
        }
    }


    return ret;
}


int mac_id_to_byte (char *id, char *mac)
{
    UINT4 ret = -1;
    char tmp[12];
        
    if (NULL != id && NULL != mac && MAC_ID_LEN == strlen(id))
    {   
        if (6 == sscanf(id, "%03d%03d%03d%03d%03d%03d", (unsigned int *)&tmp[0], (unsigned int *)&tmp[1], (unsigned int *)&tmp[2], (unsigned int *)&tmp[3],(unsigned int *)&tmp[4],(unsigned int *)&tmp[5]))                
        {
            memcpy (mac, tmp, 6);
            ret = 0;
        }
    }

    return ret;
}


int mac_byte_to_id (char *id, char *mac)
{
    int ret = -1;
        
    if (NULL != id && NULL != mac)
    {   
        sprintf(id, "%03u%03u%03u%03u%03u%03u", ((unsigned int)mac[0] & 0xFF), ((unsigned int)mac[1] & 0xFF),((unsigned int)mac[2] & 0xFF),
            ((unsigned int)mac[3] & 0xFF),((unsigned int)mac[4] & 0xFF),((unsigned int)mac[5] & 0xFF));
        ret = 0;
    }

    return ret;
} 

int alloc_service_id (unsigned int *id, char *name)
{
    int i,j;
    static int last_alloc_id = 1;

    for (i = 0; i < MAX_SERVICE_TABLES; i++)
    {
        if (0 == service_map[i].id) // get a free service_map
        {
            service_map[i].id = last_alloc_id;
            strcpy (service_map[i].name, name);
            *id = last_alloc_id;
            break;
        }
    }

    if (MAX_SERVICE_TABLES == i)
    {
        return -1;
    }

    for (i = last_alloc_id; i < 65536; i++)
    {
        for (j = 0; j < MAX_SERVICE_TABLES; j++)
        {
            if (service_map[j].id == (unsigned int)i)
            {
                break;
            }
        }

        if (MAX_SERVICE_TABLES == j)
        {
            last_alloc_id = i;
            break;
        }

        if (65535 == i)
        {
            i = 0;
        }
    }

    return 0;
}

int get_service_key_from_id (unsigned int id, char *name)
{
    int i;

    for (i = 0; i < MAX_SERVICE_TABLES; i++)
    {
        if (service_map[i].id == id)
        {
            strcpy (name, service_map[i].name);
            break;
        }
    }

    if (MAX_SERVICE_TABLES == i)
    {
        return -1;
    }
    else 
    {
        return 0;
    }
}

void free_service_id (unsigned int id)
{
    int i;

    for (i = 0; i < MAX_SERVICE_TABLES; i++)
    {
        if (service_map[i].id == id)
        {
            memset (&service_map[i], 0 ,sizeof(service_map[i]));
            break;
        }
    }
    
    return;
}

/*
   POST /api/dove/dgw/service/dps
 */
void dgw_req_handler_dpses(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    char res_body_str[128]; 
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    const char *dps_ip;
    struct in_addr ipv4_addr;
    int port;
    char id[IP_ID_LEN + 1];

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();

            if (NULL == retbuf)
            {
                break;
            }
                        
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
            js_id = json_object_get(js_root, "ip");
            if (json_is_null(js_id))
            {
                break;
            }
            if (NULL == (dps_ip = json_string_value(js_id)))
                            break;

            if (0 == inet_pton(AF_INET, dps_ip, (void *)&ipv4_addr))
            {
                break;
            }

            js_id = json_object_get(js_root, "port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            port = json_integer_value(js_id);

            if (DOVE_STATUS_OK == dgw_rest_api_create_dps (ipv4_addr.s_addr, port))
            {
                v4_addr_to_ip_id(id, ntohl(ipv4_addr.s_addr));
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);                           
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);
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
}

/*
   Delete /api/dove/dgw/service/resetstat
 */
void dgw_req_handler_resetstats(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            dgwy_rest_api_ctrl_reset_stats();
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
   Delete /api/dove/dps/{dps_id}
 */
void dgw_req_handler_dps(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    uint32_t dps_ip;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
    if (argc != 1 || NULL == argv || IP_ID_LEN != strlen(argv[0]))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    if (0 != ip_id_to_v4_addr(argv[0], &dps_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == dgw_rest_api_del_dps(ntohl(dps_ip)))
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

/*
   POST /api/dove/dgw/service
 */
void dgw_req_handler_services(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128];
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    unsigned int id;
    json_error_t jerror;
    char name[SERVICE_NAME_MAX_LEN+1];
    char type[SERVICE_TYPE_MAX_LEN+1];

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();

            if (NULL == retbuf)
            {
                break;
            }        
        
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
            js_id = json_object_get(js_root, "name");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == json_string_value(js_id) || strlen (json_string_value(js_id)) > (SERVICE_NAME_MAX_LEN -1))
            {
                break;
            }

            strcpy (name, json_string_value(js_id));


            js_id = json_object_get(js_root, "type");
            if (json_is_null(js_id) || strlen (json_string_value(js_id)) > (SERVICE_TYPE_MAX_LEN -1))
            {
                break;
            }

            if (NULL == json_string_value(js_id))
            {
                break;
            }

            strcpy (type, json_string_value(js_id));

            if (0 != alloc_service_id(&id, name))
            {
                break;
            }           
            

            if (DOVE_STATUS_OK == dgw_rest_api_create_service (name, type))
            {
                res_code = 201;
                
                sprintf (res_body_str, "{\"id\":%d}",id);
                
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);
            }
            else
            {
                free_service_id (id);
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}

/*
   PUT /api/dove/dgw/service/{service_id}
   Delete /api/dove/dgw/service/{service_id}
 */
void dgw_req_handler_service(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    unsigned int id;
    char name[SERVICE_NAME_MAX_LEN+1];
    char *endptr = NULL;    
    int mtu, start;
    
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_PUT: 
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
            js_id = json_object_get(js_root, "mtu");
            if (json_is_integer(js_id))
            {
                mtu = json_integer_value(js_id);
            }

            js_id = json_object_get(js_root, "start");
            if (json_is_integer(js_id))
            {
                start = json_integer_value(js_id);
            }

            if (DOVE_STATUS_OK == dgw_rest_api_set_service (name, mtu, start))
            {
                res_code = HTTP_OK;
            }

            break;
        }   
        case EVHTTP_REQ_DELETE:
        {
            free_service_id (id);
                
            if (DOVE_STATUS_OK == dgw_rest_api_del_service(name))
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

/*
   POST /api/dove/dgw/service/{service_id}/nic
 */
void dgw_req_handler_service_nics(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    char name[SERVICE_NAME_MAX_LEN+1];
    char mac_str[MAC_STR_LEN+1];
    char mac[6];
    unsigned int service_id;
    char *endptr;
    char id[MAC_ID_LEN + 1];

    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();

            if (NULL == retbuf)
            {
                break;
            }        
        
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
            js_id = json_object_get(js_root, "mac");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == json_string_value(js_id) || strlen(json_string_value(js_id)) != MAC_STR_LEN)
            {
                break;
            }

            strcpy (mac_str, json_string_value(js_id));

            if (0 != mac_str_to_byte (mac_str, mac))
            {
                break;
            }


            if (DOVE_STATUS_OK == dgw_rest_api_create_service_nic (name, mac))
            {
                mac_byte_to_id (id, mac);                   
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);               
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
}

/*
   Delete /api/dove/dgw/service/{service_id}/nic/{nic_id}
 */
void dgw_req_handler_service_nic(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    char name[SERVICE_NAME_MAX_LEN+1];
    char mac[6];
    unsigned int id;
    char *endptr;
    
    if (argc != 2 || NULL == argv || MAC_ID_LEN != strlen(argv[1]))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   

    if (0 != mac_id_to_byte(argv[1], mac))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {           
            if (DOVE_STATUS_OK == dgw_rest_api_del_service_nic(name, mac))
            {
                res_code = HTTP_OK;
            }
            else
            {
                res_code = HTTP_EXPECTATIONFAILED;
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
   POST /api/dove/dgw/service/ipv4
 */
void dgw_req_handler_service_ipv4s(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    struct in_addr ipv4_addr;
    struct in_addr ipv4_mask;
    struct in_addr ipv4_nexthop;
    const char *str;
    char id[IP_ID_LEN + 1];
    char typestr[32];
    int vlan_id=0;


    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
    /*
    char *endptr;
    unsigned int service_id;
    char name[SERVICE_NAME_MAX_LEN+1];
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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


            js_root = json_object_get(js_root,"gw_ipv4_assignment");
            if (!js_root)
            {
                break;
            }

            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&ipv4_addr))
            {
                break;
            }

            js_id = json_object_get(js_root, "nexthop");
            if (json_is_null(js_id))
            {
                break;
            }
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }
            if (0 == inet_pton(AF_INET, str, (void *)&ipv4_nexthop))
            {
                break;
            }

            js_id = json_object_get(js_root, "mask");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&ipv4_mask))
            {
                break;
            }           

            js_id = json_object_get(js_root, "intf_type");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            memset(typestr,0,32);
            strncpy(typestr, str, (strlen(str)>31)?31:strlen(str));

            js_id = json_object_get(js_root, "vlan");
            if (!json_is_null(js_id))
            {
                vlan_id = json_integer_value(js_id);
            }

            log_info(ServiceUtilLogLevel,"typestr:%s\n", typestr);

            if (DOVE_STATUS_OK == 
                dgw_rest_api_create_service_ipv4 ((char*)"APBR", ipv4_addr.s_addr, 
                                                  ipv4_mask.s_addr,
                                                  ipv4_nexthop.s_addr, typestr, vlan_id))
            {
                dgw_rest_api_set_ipv4_type((char*)"APBR",
                                           ipv4_addr.s_addr,
                                           typestr,ipv4_nexthop.s_addr);
                v4_addr_to_ip_id (id, ntohl(ipv4_addr.s_addr));
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                           
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
}

/*
   Delete /api/dove/dgw/service/ipv4/{ipv4_id}
 */
void dgw_req_handler_service_ipv4(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    uint32_t ip;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
    
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    
    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    unsigned int id;
    char *endptr;       
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
        
    if (0 != ip_id_to_v4_addr(argv[0], &ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    */

    if(1 != sscanf(argv[0],"%u",&ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == 
                dgw_rest_api_del_service_ipv4((char*)"APBR", ip))
            {
                res_code = HTTP_OK;
            }
            else
            {
                res_code = HTTP_EXPECTATIONFAILED;
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
   POST /api/dove/dgw/service/ext-gw
 */
void dgw_req_handler_svc_ext_vips(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_root1 = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    const char *str;
    int domain_id;
    struct in_addr min_ip;
    struct in_addr max_ip;
    struct in_addr ext_ip;
    int min_port;
    int max_port;
    char id[VIP_ID_LEN + 1];
    char id1[IP_ID_LEN + 1], id2[IP_ID_LEN + 1];
    int extmcastvnid=0;
    int tenant_id=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    char *endptr;   
    unsigned int service_id;
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
       
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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

            log_debug(CliLogLevel,"%s: req_body_buf %s",
                      __FUNCTION__, req_body_buf);

            js_root = json_loads(req_body_buf, 0, &jerror);

            if (!js_root)
            {
                break;
            }

            js_root = json_object_get(js_root, "egw_snat_pool");
            if (!js_root)
            {
                break;
            }

            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "min_ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }
            
            if (0 == inet_pton(AF_INET, str, (void *)&min_ip))
            {
                break;
            }

            js_id = json_object_get(js_root, "max_ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&max_ip))
            {
                break;
            }
            js_id = json_object_get(js_root, "net_id");
            if (!json_is_integer(js_id))
            {
                break;
            }
            domain_id = json_integer_value(js_id);          


            js_id = json_object_get(js_root, "min_port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            min_port = json_integer_value(js_id);           

            js_id = json_object_get(js_root, "max_port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            max_port = json_integer_value(js_id);           

            js_id = json_object_get(js_root, "extmcastvnid");
            if (!json_is_integer(js_id))
            {
                break;
            }
            extmcastvnid = json_integer_value(js_id);           

            js_id = json_object_get(js_root, "domain_id");
            if (!json_is_integer(js_id))
            {
                break;
            }
            tenant_id = json_integer_value(js_id);           

            js_root1 = json_object_get(js_root, "ext_ip");
            if (js_root1 == NULL)
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get ext_ip from ext-snat pool %s\n",req->uri);
                break;
            }


            js_id = json_object_get(js_root1, "ip");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get ip in ext_ip from ext-snat pool %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&ext_ip))
            {
                break;
            }

        
            log_debug(CliLogLevel,"%s:%d",
                      __FUNCTION__, __LINE__);

            if(!min_ip.s_addr && !max_ip.s_addr )
            {
                /* shared vnid case:
                 * No IP/PORT ranges 
                 * SKIP Action
                 * */
                v4_addr_to_ip_id(id1, ntohl(min_ip.s_addr));
                v4_addr_to_ip_id(id2, ntohl(max_ip.s_addr));
                sprintf (id, "%s%s", id1, id2);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);   
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                         
                res_code = 201;

                api_dgadmin_add_extshared_vnids(domain_id, tenant_id, extmcastvnid);
                
                if(ext_ip.s_addr)
                {
                    dgw_rest_api_set_mcast_external(extmcastvnid, ext_ip.s_addr);
                }
            }
            else if(DOVE_STATUS_OK == 
                    dgw_rest_api_create_service_external_vip ((char*)"APBR", 
                                                              domain_id, min_ip.s_addr, max_ip.s_addr, 
                                                              min_port, max_port, tenant_id, extmcastvnid))
            {
                v4_addr_to_ip_id(id1, ntohl(min_ip.s_addr));
                v4_addr_to_ip_id(id2, ntohl(max_ip.s_addr));
                sprintf (id, "%s%s", id1, id2);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);   
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                         
                res_code = 201;
                    
                dgw_rest_api_set_mcast_external(extmcastvnid, min_ip.s_addr);
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}

/*
   Delete /api/dove/dgw/service/ext-gw/{external_vip_id}
 */
void dgw_req_handler_svc_ext_vip(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    char vip_id[VIP_ID_LEN + 1];
    char *ptr;
    char max_ip_id[IP_ID_LEN + 1];
    char vnid_str[11];
    uint32_t min_ip;
    uint32_t max_ip;
    int vnid=0;
    
    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));    
        
    if (argc != 1 || NULL == argv || VIP_ID_LEN > strlen (argv[0]))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    char *endptr;
    unsigned int id;
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */

    memset(vip_id, 0, sizeof(vip_id));
    strncpy (vip_id, argv[0], VIP_ID_LEN); //min_ip

    ptr = vip_id;

    memset(max_ip_id,0,sizeof(max_ip_id));
    strcpy (max_ip_id, ptr + IP_ID_LEN); // max_ip
    *(ptr + IP_ID_LEN) = '\0';

    ptr = argv[0]+VIP_ID_LEN;
    memset(vnid_str,0,sizeof(vnid_str));
    strncpy (vnid_str, ptr , 10); 

    
    if (0 != ip_id_to_v4_addr(vip_id, &min_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != ip_id_to_v4_addr(max_ip_id, &max_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if(1 != sscanf(vnid_str,"%010d",&vnid))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if(!min_ip && !max_ip && vnid)
            {
                api_dgadmin_del_extshared_vnids(vnid);
            }
            else if (DOVE_STATUS_OK == 
                     dgw_rest_api_del_service_external_vip((char*)"APBR", 
                                                           ntohl(min_ip), ntohl(max_ip)))
            {
                res_code = HTTP_OK;
            }
            else
            {
                res_code = HTTP_EXPECTATIONFAILED;
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
   POST /api/dove/dgw/service/{service_id}/internal-vip
 */
void dgw_req_handler_service_internal_vips(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    char name[SERVICE_NAME_MAX_LEN+1];
    const char *str;
    int domain_id;
    struct in_addr min_ip;
    struct in_addr max_ip;
    int min_port;
    int max_port;
    unsigned int service_id;
    char *endptr;   
    char id[VIP_ID_LEN + 1];
    char id1[IP_ID_LEN + 1], id2[IP_ID_LEN + 1];    
    
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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
            js_id = json_object_get(js_root, "min_ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&min_ip))
            {
                break;
            }

            js_id = json_object_get(js_root, "max_ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&max_ip))
            {
                break;
            }

            js_id = json_object_get(js_root, "domain");
            if (!json_is_integer(js_id))
            {
                break;
            }
            domain_id = json_integer_value(js_id);          


            js_id = json_object_get(js_root, "min_port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            min_port = json_integer_value(js_id);           


            js_id = json_object_get(js_root, "max_port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            max_port = json_integer_value(js_id);           
        
            if (DOVE_STATUS_OK == dgw_rest_api_create_service_internal_vip (name, domain_id, min_ip.s_addr, max_ip.s_addr, min_port, max_port))
            {
                v4_addr_to_ip_id(id1, ntohl(min_ip.s_addr));
                v4_addr_to_ip_id(id2, ntohl(max_ip.s_addr));
                sprintf (id, "%s%s", id1, id2);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                               
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
}

/*
   Delete /api/dove/dgw/service/{service_id}/internal-vip/{internal_vip_id}
 */
void dgw_req_handler_service_internal_vip(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    char *ptr;
    char name[SERVICE_NAME_MAX_LEN+1];
    char vip_id[VIP_ID_LEN + 1];
    char max_ip_id[IP_ID_LEN + 1];
    uint32_t min_ip;
    uint32_t max_ip;
    unsigned int id;
    char *endptr;   
    
    if (argc != 2 || NULL == argv || strlen (argv[1]) != VIP_ID_LEN)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    strcpy (vip_id, argv[1]); //min_ip

    ptr = vip_id;

    strcpy (max_ip_id, ptr + IP_ID_LEN); // max_ip

    *(ptr + IP_ID_LEN) = '\0';
    
    if (0 != ip_id_to_v4_addr(vip_id, &min_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != ip_id_to_v4_addr(max_ip_id, &max_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == dgw_rest_api_del_service_internal_vip(name, ntohl(min_ip), ntohl(max_ip)))
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

/*
   POST /api/dove/dgw/service/rule
 */
void dgw_req_handler_service_rules(struct evhttp_request *req, 
                                   void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    int domain_id;
    struct in_addr ip;
    struct in_addr real_ip;
    struct in_addr pip_min;
    struct in_addr pip_max;
    int port;
    int real_port;
    uint16_t protocol;
    const char *str;
    char id[128];
    char id1[IP_ID_LEN + 1], id2[IP_ID_LEN + 1];
    
    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    char *endptr;   
    unsigned int service_id;
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    */

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }        
        
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
                log_error(ServiceUtilLogLevel,
                          "Failed to load uri %s\n",req->uri);
                break;
            }

            js_root = json_object_get(js_root, "egw_fwd_rule");
            if (!js_root)
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get egw_fwd_rule from uri %s\n",req->uri);
                break;
            }

            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "ip");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get ip from uri %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get ip from js_id %s\n",req->uri);
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&ip))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed inet_pton ip in uri %s\n",req->uri);
                break;
            }

            js_id = json_object_get(js_root, "real_ip");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get real_ip from uri %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&real_ip))
            {
                break;
            }

            js_id = json_object_get(js_root, "protocol");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Set protocol 0 from uri %s\n",req->uri);
                protocol = 0;
                //break;
            } 
            else if ((!json_is_null(js_id)) && (!json_is_integer(js_id)))
            {
                if (NULL == (str = json_string_value(js_id)))
                {
                    log_error(ServiceUtilLogLevel,
                          "Failed to get protocol from uri %s set to 0\n",req->uri);
                    protocol = 0;
                    //break;
                }
                else if (0 == strcasecmp (str, "tcp"))
                {
                    protocol = 6;
                }
                else if (0 == strcasecmp (str, "udp"))
                {
                    protocol = 17;
                }
            }
            else if((!json_is_null(js_id)) && (json_is_integer(js_id)))
            {
                protocol = json_integer_value(js_id);
            }
            else {
                protocol = 0;
            }

            log_error(ServiceUtilLogLevel,
                  "GOT  protocol %d from uri %s\n",req->uri, protocol);

            js_id = json_object_get(js_root, "net_id");
            if (!json_is_integer(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get net_id from uri %s\n",req->uri);
                break;
            }
            domain_id = json_integer_value(js_id);          


            js_id = json_object_get(js_root, "port");
            if (!json_is_integer(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get port from uri %s\n",req->uri);
                break;
            }
            port = json_integer_value(js_id);           


            js_id = json_object_get(js_root, "real_port");
            if (!json_is_integer(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get real_port from uri %s\n",req->uri);
                break;
            }
            real_port = json_integer_value(js_id);      

            js_id = json_object_get(js_root, "pip_min");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get pip_min from uri %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&pip_min))
            {
                break;
            }

            js_id = json_object_get(js_root, "pip_max");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to get pip_max from uri %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&pip_max))
            {
                break;
            }

        
            if (DOVE_STATUS_OK == 
                dgw_rest_api_create_service_rule ((char*)"APBR", domain_id, 
                                                  ip.s_addr, real_ip.s_addr, 
                                                  port, real_port, protocol,
                                                  pip_min.s_addr, pip_max.s_addr))
            {
                v4_addr_to_ip_id(id1, ntohl(ip.s_addr));
                v4_addr_to_ip_id(id2, ntohl(real_ip.s_addr));
                sprintf (id, "%05d%s%s%05d%05d%03d", domain_id, id1, id2, port, real_port, protocol);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);       
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                               
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
}

/*
   Delete /api/dove/dgw/service/rule/{rule_id}
 */
void dgw_req_handler_service_rule(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    int domain_id;
    uint32_t ip;
    uint32_t real_ip=0;
    int port;
    int real_port=0;
    int protocol;
    char ip_str[IP_ID_LEN + 1];
    char *ptr=argv[0];
    /*char real_ip_str[IP_ID_LEN + 1];*/

    log_info(ServiceUtilLogLevel,"%s: argc %d argv %s",
             __FUNCTION__, argc, (argv[0]?argv[0]:"nil"));    

    /*
    char *endptr;
    unsigned int id;
    char name[SERVICE_NAME_MAX_LEN+1];
    if (argc != 2 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    */
    
    if (3 != sscanf (argv[0],"%05d%05d%03d", 
                     &domain_id,   
                     &port, &protocol))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    memset(ip_str,0,sizeof(ip_str));

    ptr += 13;
    strncpy(ip_str, ptr,IP_ID_LEN);

    if (0 != ip_id_to_v4_addr(ip_str, &ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   

    /*
    if (0 != ip_id_to_v4_addr(real_ip_str, &real_ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    } 
    */
    
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == 
                dgw_rest_api_del_service_rule ((char*)"APBR", domain_id, ntohl(ip), 
                                               real_ip, port, real_port, protocol))
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
   POST /api/dove/dgw/service/vlan-gw
 */
void dgw_req_handler_service_vlanmaps(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    int vlan_id;
    int domain_id;
    char id[VLAN_MAP_ID_LEN + 1];


    log_info(ServiceUtilLogLevel,
             "argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    /*
    char *endptr;
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   

    unsigned int service_id;
    char name[SERVICE_NAME_MAX_LEN+1];
    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    } 
    */

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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

            js_root = json_object_get(js_root, "vnid_mapping_rule");
            if (!js_root)
            {
                break;
            }

            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "net_id");
            if (!json_is_integer(js_id))
            {
                break;
            }
            domain_id = json_integer_value(js_id);

            js_id = json_object_get(js_root, "vlan");
            if (!json_is_integer(js_id))
            {
                break;
            }
            vlan_id = json_integer_value(js_id);

            if (DOVE_STATUS_OK == dgw_rest_api_create_service_vlan_map ((char*)"APBR", domain_id, vlan_id))
            {
                sprintf (id, "%05d%05d", domain_id, vlan_id);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                               
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
}

/*
   Delete /api/dove/dgw/service/vlan-gw/{vlan-map-id}
 */
void dgw_req_handler_service_vlanmap(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    uint32_t domain_id;
    uint32_t vlan_id;
    
    if (argc != 1 || NULL == argv || strlen (argv[0]) > VLAN_MAP_ID_LEN)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    log_info(ServiceUtilLogLevel,
             "argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));


    /*
    char *endptr = NULL;
    char name[SERVICE_NAME_MAX_LEN+1];
    unsigned int id;
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */

    if (2 != sscanf (argv[0], "%05d%05d", &domain_id, &vlan_id))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == dgw_rest_api_del_service_vlan_map ((char*)"APBR", domain_id, vlan_id))
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

/*
   POST /api/dove/dgw/service/subnet
   POST "/controller/sb/v2/opendove/odmc/networks/<network>/networkSubnets/<subnet-uuid>
 */
void dgw_req_handler_service_subnets(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    struct in_addr subnet_addr;
    struct in_addr subnet_mask;
    struct in_addr subnet_nexthop;
    const char *str;
    char id[IP_ID_LEN + 1];
    char id1[IP_ID_LEN + 1];
    char typestr[32];
    int domain_id;


    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    if (1 != sscanf(req->uri,DGW_SERVICE_SUBNET_URI_PARAM,
                    &domain_id)) {
        log_error(ServiceUtilLogLevel,
                 "Failed to get network id from sunet uri %s\n",req->uri);
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    /*
    char *endptr;
    unsigned int service_id;
    char name[SERVICE_NAME_MAX_LEN+1];
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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
                log_error(ServiceUtilLogLevel,
                          "Failed to load uri %s\n",req->uri);
                break;
            }

            js_root = json_object_get(js_root, "subnet");
            if (!js_root)
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to load subnet uri %s\n",req->uri);
                break;
            }

#if 0
            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "net_id");
            if (!json_is_integer(js_id))
            {
                break;
            }
            domain_id = json_integer_value(js_id);          
#endif

            js_id = json_object_get(js_root, "subnet");
            if (json_is_null(js_id))
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to load subnet in subnet uri %s\n",req->uri);
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&subnet_addr))
            {
                break;
            }

            js_id = json_object_get(js_root, "nexthop");
            if (json_is_null(js_id))
            {
                break;
            }
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }
            if (0 == inet_pton(AF_INET, str, (void *)&subnet_nexthop))
            {
                break;
            }

            js_id = json_object_get(js_root, "mask");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&subnet_mask))
            {
                break;
            }           

            js_id = json_object_get(js_root, "type");
            if (json_is_null(js_id))
            {
                break;
            }

            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            memset(typestr,0,32);
            strncpy(typestr, str, (strlen(str)>31)?31:strlen(str));
            log_info(ServiceUtilLogLevel,"typestr:%s\n", typestr);

            if (DOVE_STATUS_OK == 
                dgw_rest_api_set_service_subnet ((char*)"APBR", 
                                                 domain_id,
                                                 subnet_addr.s_addr, 
                                                 subnet_mask.s_addr,
                                                 subnet_nexthop.s_addr, 
                                                 typestr))
            {
                v4_addr_to_ip_id (id1, ntohl(subnet_addr.s_addr));
                sprintf (id, "%05d%s", domain_id, id1);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                           
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
}

/*
   Delete /api/dove/dgw/service/subnet/{subnet_id}
 */
void dgw_req_handler_service_subnet(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_res = NULL;
    char *res_body_str = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    uint32_t ip;
    int domain_id;
    char ip_str[IP_ID_LEN + 1];
    
    if (argc != 1 || NULL == argv )
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    
    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    unsigned int id;
    char *endptr;       
    id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
    */


    if (2 != sscanf (argv[0],"%05d%12s", 
                     &domain_id, ip_str))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }
        
    if (0 != ip_id_to_v4_addr(ip_str, &ip))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == 
                dgw_rest_api_del_service_subnet((char*)"APBR", domain_id, ntohl(ip)))
            {
                res_code = HTTP_OK;
            }
            else
            {
                res_code = HTTP_EXPECTATIONFAILED;
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

void dgw_req_handler_service_dmc(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    struct in_addr ip_adr;
    const char *str;
    char id[IP_ID_LEN + 1];
    int port=0;


    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
    /*
    char *endptr;
    unsigned int service_id;
    char name[SERVICE_NAME_MAX_LEN+1];
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
    
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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

            js_id = json_object_get(js_root, "ip");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }

            if (0 == inet_pton(AF_INET, str, (void *)&ip_adr))
            {
                break;
            }

            js_id = json_object_get(js_root, "port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            port = json_integer_value(js_id);


            if (DOVE_STATUS_OK == 
                dgw_rest_api_set_service_dmc ((char*)"APBR", 
                                              ip_adr.s_addr, 
                                              port))
            {
                v4_addr_to_ip_id (id, ntohl(ip_adr.s_addr));
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                           
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
}

void get_ext_session_action_str(int action, char *str)
{
    switch(action)
    {
        case ACTION_EXT_SNAT:
        {
            sprintf(str,"EXT-SNAT-FWD");
            break;
        }
        case ACTION_EXT_SNAT_REV:
        {
            sprintf(str,"EXT-DNAT-REV");
            break;
        }
        case ACTION_INT_SNAT:
        {
            sprintf(str,"INT-SNAT");
            break;
        }
        case ACTION_INT_SNAT_REV:
        {
            sprintf(str,"INT-SNAT-REV");
            break;
        }
        default:
        {
            sprintf(str,"UNKNOWN");
            break;
        }
    }
    return;
}

void get_int_session_action_str(int action, char *str)
{
    switch(action)
    {
        case ACTION_INT_SNAT:
        {
            sprintf(str,"INT-DNAT-FWD");
            break;
        }
        case ACTION_INT_SNAT_REV:
        {
            sprintf(str,"INT-SNAT-REV");
            break;
        }
        default:
        {
            sprintf(str,"UNKNOWN");
            break;
        }
    }
    return;
}


void get_fwddyn_session_type_str(int type, char *str)
{
    log_info(ServiceUtilLogLevel," type %d\n",type);
    switch(type)
    {
        case DGWY_FWDDYN_TYPE_EXT:
        {
            sprintf(str,"EXTERNAL_TO_OVERLAY");
            break;
        }
        case DGWY_FWDDYN_TYPE_INT_PORT:
        {
            sprintf(str,"OVERLAY_TO_EXTERNAL");
            break;
        }
        case DGWY_FWDDYN_TYPE_INT_IPONLY:
        {
            sprintf(str,"OVERLAY_TO_EXTERNAL_IPMAP");
            break;
        }
        default:
        {
            sprintf(str,"UNKNOWN");
            break;
        }
    }
    return;
}


int build_ext_session_json_response(ext_sesion_dump_t *sessions, 
                                char *start, int next_index,
                                int maxsessions)
{
    /* build below json body
    {
       [ 
            {
                "net_id":id,
                "age":time,
                "ovl_sip":"IP",
                "ovl_dip":"IP",
                "ovl_proto":proto,
                "ovl_sport":port,
                "ovl_dport":port,
                "orig_sip":"ip",
                "orig_dip":"ip",
                "orig_sport":port,
                "orig_dport":port,
                "sip":"ip",
                "dip":"ip",
                "sport":port,
                "dport":port,
                "proto":proto,
                "action":"ACTION",
                "snat_ip":"ip",
                "snat_port":port
            },
            {
                "net_id":id,
                "age":time,
                "ovl_sip":"IP",
                "ovl_dip":"IP",
                "ovl_proto":proto,
                "ovl_sport":port,
                "ovl_dport":port,
                "orig_sip":"ip",
                "orig_dip":"ip",
                "orig_sport":port,
                "orig_dport":port,
                "sip":"ip",
                "dip":"ip",
                "sport":port,
                "dport":port,
                "proto":proto,
                "action":"ACTION",
                "snat_ip":"ip",
                "snat_port":port
            },
            { ....... }
       ] 
    }
    */

    uint32_t i=0;
    int len=0;
    char *buffer=start;
    int sess_count=0;

    len = sprintf(buffer,"{\"external_sessions\":[\r\n\t\t");
    buffer+= len;

    for(i=0; i<sessions->count; i++)
    {
        char id[IP_ID_LEN + 1];
        struct in_addr ip_adr;
        //uint32_t vnid = (ntohl(sessions->sess_info[i].vnid&0xffffff00))>>8;
        uint32_t vnid = (ntohl(sessions->sess_info[i].vnid<<8));
        char action_str[32];

        if(i < (uint32_t)next_index)
        {
            continue;
        }

        sess_count++;
        len =  sprintf(buffer,"{\r\n\t\t\t");
        buffer+= len;
        
        len = sprintf(buffer,"\"net_id\":\"%u\",\r\n\t\t\t",vnid);
        buffer+= len;

        len =  sprintf(buffer,"\"age\":\"%ld\",\r\n\t\t\t",
                       sessions->sess_info[i].timesince);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_overlay_sip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"ovl_sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_overlay_dip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"ovl_dip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        /*
        len = sprintf(buffer,"\"ovl_proto\":%u,\n\t\t\t",
                      sessions->sess_info[i].orig_overlay_proto);
        buffer+= len;
        */

        len = sprintf(buffer,"\"ovl_sport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_overlay_sport));
        buffer+= len;

        len = sprintf(buffer,"\"ovl_dport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_overlay_dport));
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_sip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"orig_sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_dip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"orig_dip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"orig_sport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_sport));
        buffer+= len;

        len = sprintf(buffer,"\"orig_dport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_dport));
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].sip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].dip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"dip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"proto\":%u,\r\n\t\t\t",
                      sessions->sess_info[i].proto);
        buffer+= len;

        len = sprintf(buffer,"\"sport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].sport));
        buffer+= len;

        len = sprintf(buffer,"\"dport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].dport));
        buffer+= len;

        memset(action_str,0,32);
        get_ext_session_action_str(sessions->sess_info[i].action,action_str);
        len = sprintf(buffer,"\"action\":\"%s\",\r\n\t\t\t",action_str);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].snat_ip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"snat_ip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"snat_port\":%u\r\n\t\t",
                      ntohs(sessions->sess_info[i].snat_port));
        buffer+= len;
 

        if(sess_count >= maxsessions)
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
            break;
        }
        else
        if(i+1<sessions->count)
        {
            len = sprintf(buffer,"},\r\n\t\t");
            buffer+= len;
        }
        else
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
        }

        log_debug(ServiceUtilLogLevel,
                 "one object len = %d", buffer-start);
    }


    len = sprintf(buffer,"\r\n\t]\r\n}\r\n");
    buffer+= len;

    len = buffer-start;

    log_debug(ServiceUtilLogLevel,
             "Len = %d", len);

    return len;
}

int build_int_session_json_response(int_session_dump_t *sessions, 
                                    char *start, int next_index,
                                    int maxsessions)
{
    /* build below json body
    {
       [ 
            {
                "net_id":id,
                "age":time,
                "ovl_sip":"IP",
                "ovl_dip":"IP",
                "ovl_proto":proto,
                "ovl_sport":port,
                "ovl_dport":port,
                "orig_sip":"ip",
                "orig_dip":"ip",
                "orig_sport":port,
                "orig_dport":port,
                "sip":"ip",
                "dip":"ip",
                "sport":port,
                "dport":port,
                "proto":proto,
                "action":"ACTION",
                "snat_ip":"ip",
                "snat_port":port
            },
            {
                "net_id":id,
                "age":time,
                "ovl_sip":"IP",
                "ovl_dip":"IP",
                "ovl_proto":proto,
                "ovl_sport":port,
                "ovl_dport":port,
                "orig_sip":"ip",
                "orig_dip":"ip",
                "orig_sport":port,
                "orig_dport":port,
                "sip":"ip",
                "dip":"ip",
                "sport":port,
                "dport":port,
                "proto":proto,
                "action":"ACTION",
                "snat_ip":"ip",
                "snat_port":port
            },
            { ....... }
       ] 
    }
    */

    uint32_t i=0;
    int len=0;
    char *buffer=start;
    int sess_count=0;

    len = sprintf(buffer,"{\"internal_sessions\":[\r\n\t\t");
    buffer+= len;

    for(i=0; i<sessions->count; i++)
    {
        char id[IP_ID_LEN + 1];
        struct in_addr ip_adr;
        //uint32_t vnid = (ntohl(sessions->sess_info[i].vnid&0xffffff00))>>8;
        uint32_t vnid = (ntohl(sessions->sess_info[i].vnid<<8));
        char action_str[32];

        if(i < (uint32_t)next_index)
        {
            continue;
        }

        sess_count++;
        len =  sprintf(buffer,"{\r\n\t\t\t");
        buffer+= len;
        
        len = sprintf(buffer,"\"net_id\":\"%u\",\r\n\t\t\t",vnid);
        buffer+= len;

        len =  sprintf(buffer,"\"age\":\"%ld\",\r\n\t\t\t",
                       sessions->sess_info[i].timesince);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].sip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].dip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"dip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"proto\":%u,\r\n\t\t\t",
                      sessions->sess_info[i].proto);
        buffer+= len;


        /*
        len = sprintf(buffer,"\"ovl_proto\":%u,\n\t\t\t",
                      sessions->sess_info[i].orig_overlay_proto);
        buffer+= len;
        */

        len = sprintf(buffer,"\"sport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].sport));
        buffer+= len;

        len = sprintf(buffer,"\"dport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].dport));
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_sip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"orig_sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].orig_dip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"orig_dip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"orig_sport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_sport));
        buffer+= len;

        len = sprintf(buffer,"\"orig_dport\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].orig_dport));
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].dnat_ip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"dnat_ip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].snat_ip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"snat_ip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;


        len = sprintf(buffer,"\"dnat_port\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].dnat_port));
        buffer+= len;

        len = sprintf(buffer,"\"snat_port\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].snat_port));
        buffer+= len;

        memset(action_str,0,32);
        get_int_session_action_str(sessions->sess_info[i].action,action_str);
        len = sprintf(buffer,"\"action\":\"%s\"\r\n\t\t",action_str);
        buffer+= len;


        if(sess_count >= maxsessions)
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
            break;
        }
        else
        if(i+1<sessions->count)
        {
            len = sprintf(buffer,"},\r\n\t\t");
            buffer+= len;
        }
        else
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
        }

        log_debug(ServiceUtilLogLevel,
                 "one object len = %d", buffer-start);
    }


    len = sprintf(buffer,"\r\n\t]\r\n}\r\n");
    buffer+= len;

    len = buffer-start;

    log_debug(ServiceUtilLogLevel,
             "Len = %d", len);

    return len;
}



int build_fwddyn_session_json_response(fwddyn_session_dump_t *sessions,
                                       char *start, int next_index,
                                       int maxsessions)
{
    /* build below json body
    {
       [ 
            {
                "net_id":id,
                "age":time,
                "snat_ip":"ip",
                "snat_port":port
                "real_ip":"ip",
                "real_port":port,
                "proto":proto,
                "type":"TYPE"
            },
            {
                "net_id":id,
                "age":time,
                "snat_ip":"ip",
                "snat_port":port
                "real_ip":"ip",
                "real_port":port,
                "proto":proto,
                "type":"TYPE"
            },
            { ....... }
       ] 
    }
    */

    uint32_t i=0;
    int len=0;
    char *buffer=start;
    int sess_count=0;

    //len = sprintf(buffer,"{\n\t[\n\t\t");
    len = sprintf(buffer,"{\"fwddyn_sessions\":[\r\n\t\t");
    buffer+= len;

    for(i=0; i<sessions->count; i++)
    {
        char id[IP_ID_LEN + 1];
        struct in_addr ip_adr;
        //uint32_t vnid = (ntohl(sessions->sess_info[i].vnid&0xffffff00))>>8;
        uint32_t vnid = (ntohl(sessions->sess_info[i].vnid<<8));
        char type_str[32];

        if(i < (uint32_t)next_index)
        {
            continue;
        }
        sess_count++;

        len =  sprintf(buffer,"{\r\n\t\t\t");
        buffer+= len;
        
        len = sprintf(buffer,"\"net_id\":\"%u\",\r\n\t\t\t",vnid);
        buffer+= len;

        len =  sprintf(buffer,"\"age\":\"%ld\",\r\n\t\t\t",
                       sessions->sess_info[i].timesince);
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].svc_vip_ip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"snat_sip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"snat_port\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].svc_vport));
        buffer+= len;

        ip_adr.s_addr=sessions->sess_info[i].real_ip;
        v4_addr_to_ip_str(id, ntohl(ip_adr.s_addr));
        len = sprintf(buffer,"\"real_ip\":\"%s\",\r\n\t\t\t",id);
        buffer+= len;

        len = sprintf(buffer,"\"real_port\":%u,\r\n\t\t\t",
                      ntohs(sessions->sess_info[i].real_port));
        buffer+= len;

        len = sprintf(buffer,"\"proto\":%u,\r\n\t\t\t",
                      sessions->sess_info[i].svc_proto);
        buffer+= len;

        memset(type_str,0,sizeof(type_str));
        get_fwddyn_session_type_str(sessions->sess_info[i].type,
                                    type_str);

        len = sprintf(buffer,"\"type\":\"%s\"\r\n\t\t",type_str);
        buffer+= len;

        if(sess_count >= maxsessions)
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
            break;
        }
        else
        if(i+1<sessions->count)
        {
            len = sprintf(buffer,"},\r\n\t\t");
            buffer+= len;
        }
        else
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
        }

        log_debug(ServiceUtilLogLevel,
                 "one object len = %d", buffer-start);
    }


    len = sprintf(buffer,"\r\n\t]\r\n}\r\n");
    buffer+= len;

    len = buffer-start;

    log_debug(ServiceUtilLogLevel,
             "Len = %d", len);

    return len;
}


void dgw_req_handler_serivce_sessions_ext(struct evhttp_request *req, void *arg,
                                          int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id   = NULL;
    json_t *js_res  = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    struct evbuffer *req_body   = NULL;
    char req_body_buf[1024];
    int next_index=0;
    int maxsessions=0;
    json_error_t jerror;
    int n=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            ext_sesion_dump_t sessions;

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

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

            js_id = json_object_get(js_root, "next_index");
            if (!json_is_integer(js_id))
            {
                break;
            }
            next_index = json_integer_value(js_id);

            js_id = json_object_get(js_root, "maxsessions");
            if (!json_is_integer(js_id))
            {
                break;
            }
            maxsessions = json_integer_value(js_id);

            log_info(ServiceUtilLogLevel,
                     "next_index=%d\n",next_index);
            if (DOVE_STATUS_OK == 
                dgw_rest_api_show_all_ext_sessions(&sessions))
            {
                if(sessions.count>0)
                {
                    if((uint32_t)next_index > sessions.count)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "next_index=%d count=%u\n",
                                 next_index, sessions.count);

                        res_code = HTTP_NOCONTENT;
                        break;
                    }
                    else if(sessions.count >= MAX_SESSION_DUMP_COUNT)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "Sessions count %d >= %d\n",
                                 sessions.count,MAX_SESSION_DUMP_COUNT);
                        res_code = HTTP_INTERNAL;
                        break;
                    }
                    else
                    {
                        char *buffer = (char*) malloc (1024*sessions.count);                       
                        if(buffer)
                        {
                            int len = 
                                build_ext_session_json_response(&sessions, 
                                                            buffer,
                                                            next_index-1,
                                                            maxsessions);
                            evbuffer_add(retbuf, buffer, len+1);   
                            res_code = 200;
                        }
                    }
                }
                else
                {
                    res_code = HTTP_NOCONTENT;
                    break;
                }
            }
            else
            {
                log_info(ServiceUtilLogLevel,
                         "Failed to read ext session details\n");
                res_code = HTTP_INTERNAL;
                break;
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}

void dgw_req_handler_serivce_sessions_int(struct evhttp_request *req, void *arg,
                                          int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id   = NULL;
    json_t *js_res  = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    struct evbuffer *req_body   = NULL;
    char req_body_buf[1024];
    int next_index=0;
    int maxsessions=0;
    json_error_t jerror;
    int n=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            int_session_dump_t sessions;

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

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

            js_id = json_object_get(js_root, "next_index");
            if (!json_is_integer(js_id))
            {
                break;
            }
            next_index = json_integer_value(js_id);

            js_id = json_object_get(js_root, "maxsessions");
            if (!json_is_integer(js_id))
            {
                break;
            }
            maxsessions = json_integer_value(js_id);

            log_info(ServiceUtilLogLevel,
                     "next_index=%d\n",next_index);
            if (DOVE_STATUS_OK == 
                dgw_rest_api_show_all_int_sessions(&sessions))
            {
                if(sessions.count>0)
                {
                    if((uint32_t)next_index > sessions.count)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "next_index=%d count=%u\n",
                                 next_index, sessions.count);

                        res_code = HTTP_NOCONTENT;
                        break;
                    }
                    else if(sessions.count >= MAX_SESSION_DUMP_COUNT)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "Sessions count %d very large\n",
                                 sessions.count);
                        res_code = HTTP_INTERNAL;
                        break;
                    }
                    else
                    {
                        char *buffer = (char*) malloc (1024*sessions.count);                       
                        if(buffer)
                        {
                            int len = 
                                build_int_session_json_response(&sessions, 
                                                                buffer,
                                                                next_index-1,
                                                                maxsessions);
                            evbuffer_add(retbuf, buffer, len+1);   
                            res_code = 200;
                        }
                    }
                }
                else
                {
                    res_code = HTTP_NOCONTENT;
                    break;
                }
            }
            else
            {
                log_info(ServiceUtilLogLevel,
                         "Failed to read int session details\n");
                res_code = HTTP_INTERNAL;
                break;
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}


void dgw_req_handler_serivce_fwddyn_sessions(struct evhttp_request *req, void *arg,
                                             int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id   = NULL;
    json_t *js_res  = NULL;
    int res_code    = HTTP_BADREQUEST;
    int next_index  = 0;
    int maxsessions = 0;
    json_error_t jerror;
    struct evbuffer *req_body = NULL;
    struct evbuffer *retbuf   = NULL;
    char req_body_buf[1024];
    int n=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            fwddyn_session_dump_t sessions;

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

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

            js_id = json_object_get(js_root, "next_index");
            if (!json_is_integer(js_id))
            {
                break;
            }
            next_index = json_integer_value(js_id);

            js_id = json_object_get(js_root, "maxsessions");
            if (!json_is_integer(js_id))
            {
                break;
            }
            maxsessions = json_integer_value(js_id);

            log_info(ServiceUtilLogLevel,
                     "next_index=%d\n",next_index);

            if (DOVE_STATUS_OK == 
                dgw_rest_api_show_all_fwddyn_sessions(&sessions))
            {
                if(sessions.count>0)
                {
                    if((uint32_t)next_index > sessions.count)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "next_index=%d count=%u\n",
                                 next_index, sessions.count);

                        res_code = HTTP_NOCONTENT;
                        break;
                    }
                    else if(sessions.count >= MAX_SESSION_DUMP_COUNT)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "Sessions count %d very large\n",
                                 sessions.count);
                        res_code = HTTP_INTERNAL;
                        break;
                    }
                    else
                    {
                        char *buffer = (char*) malloc (1024*sessions.count);
                        log_info(ServiceUtilLogLevel,"Session COUNT=%d\n",
                                 sessions.count);
                        if(buffer)
                        {
                            int len = 
                                build_fwddyn_session_json_response(&sessions, 
                                                                   buffer,
                                                                   next_index-1,
                                                                   maxsessions);
                            evbuffer_add(retbuf, buffer, len+1);   
                            res_code = 200;
                        }
                    }
                }
                else
                {
                    res_code = HTTP_NOCONTENT;
                    break;
                }
            }
            else
            {
                log_info(ServiceUtilLogLevel,
                         "Failed to read ext session details\n");
                res_code = HTTP_INTERNAL;
                break;
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}

int build_vnid_stats_json_response(dgwy_vnid_stats_t *stats, 
                                   char *start, int next_index,
                                   int maxstats)
{
    /* build below json body
    {
       [ 
            {
                "net_id":id,
                "ovl_to_ext_leave_bytes":"X",
                "ovl_to_ext_leave_pkts":"X",
                "ovl_to_ext_leave_bps":"X",
                "ovl_to_ext_leave_pps":"X",
                "ext_to_ovl_enter_bytes":"X",
                "ext_to_ovl_enter_pkts":"X",
                "ext_to_ovl_enter_bps":"X",
                "ext_to_ovl_enter_pps":"X",
                "ovl_to_vlan_leave_bytes":"X",
                "ovl_to_vlan_leave_pkts":"X",
                "ovl_to_vlan_leave_bps":"X",
                "ovl_to_vlan_leave_pps":"X",
                "vlan_to_ovl_enter_bytes":"X",
                "vlan_to_ovl_enter_pkts":"X",
                "vlan_to_ovl_enter_bps":"X",
                "vlan_to_ovl_enter_pps":"X",
            },
            { ....... }
       ] 
    }
    */

    uint32_t i=0;
    int len=0;
    char *buffer=start;
    int stats_count=0;

    len = sprintf(buffer,"{\"vnid_stats\":[\r\n\t\t");
    buffer+= len;

    for(i=0; i<stats->count; i++)
    {
        //uint32_t vnid = (ntohl(stats->vnid_stats[i].vnid&0xffffff00))>>8;
        uint32_t vnid = (ntohl(stats->vnid_stats[i].vnid<<8));

        if(i < (uint32_t)next_index)
        {
            continue;
        }

        stats_count++;
        len =  sprintf(buffer,"{\r\n\t\t\t");
        buffer+= len;
        
        len = sprintf(buffer,"\"net_id\":\"%u\",\r\n\t\t\t",vnid);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_ext_leave_bytes\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_rxbytes);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_ext_leave_pkts\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_rxpkts);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_ext_leave_bps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_rxbps);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_ext_leave_pps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_rxpps);
        buffer+= len;

        len =  sprintf(buffer,"\"ext_to_ovl_enter_bytes\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_txbytes);
        buffer+= len;

        len =  sprintf(buffer,"\"ext_to_ovl_enter_pkts\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_txpkts);
        buffer+= len;

        len =  sprintf(buffer,"\"ext_to_ovl_enter_bps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_txbps);
        buffer+= len;

        len =  sprintf(buffer,"\"ext_to_ovl_enter_pps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].ext_gw_overlay_txpps);
        buffer+= len;



        len =  sprintf(buffer,"\"ovl_to_vlan_leave_bytes\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_rxbytes);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_vlan_leave_pkts\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_rxpkts);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_vlan_leave_bps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_rxbps);
        buffer+= len;

        len =  sprintf(buffer,"\"ovl_to_vlan_leave_pps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_rxpps);
        buffer+= len;

        len =  sprintf(buffer,"\"vlan_to_ovl_enter_bytes\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_txbytes);
        buffer+= len;

        len =  sprintf(buffer,"\"vlan_to_ovl_enter_pkts\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_txpkts);
        buffer+= len;

        len =  sprintf(buffer,"\"vlan_to_ovl_enter_bps\":\"%ld\",\r\n\t\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_txbps);
        buffer+= len;

        len =  sprintf(buffer,"\"vlan_to_ovl_enter_pps\":\"%ld\"\r\n\t\t",
                       stats->vnid_stats[i].vlan_gw_overlay_txpps);
        buffer+= len;

        if(stats_count >= maxstats)
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
            break;
        }
        else
        if(i+1<stats->count)
        {
            len = sprintf(buffer,"},\r\n\t\t");
            buffer+= len;
        }
        else
        {
            len = sprintf(buffer,"}\r\n\t\t");
            buffer+= len;
        }

        log_debug(ServiceUtilLogLevel,
                 "one object len = %d", buffer-start);
    }


    len = sprintf(buffer,"\r\n\t]\r\n}\r\n");
    buffer+= len;

    len = buffer-start;

    log_debug(ServiceUtilLogLevel,
             "Len = %d", len);

    return len;
}



void dgw_req_handler_vnid_stats(struct evhttp_request *req, void *arg,
                                int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id   = NULL;
    json_t *js_res  = NULL;
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    struct evbuffer *req_body   = NULL;
    char req_body_buf[1024];
    int next_index=0;
    int maxstats=0;
    json_error_t jerror;
    int n=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            dgwy_vnid_stats_t stats;

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

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

            js_id = json_object_get(js_root, "next_index");
            if (!json_is_integer(js_id))
            {
                break;
            }
            next_index = json_integer_value(js_id);

            js_id = json_object_get(js_root, "maxstats");
            if (!json_is_integer(js_id))
            {
                break;
            }
            maxstats = json_integer_value(js_id);

            log_info(ServiceUtilLogLevel,
                     "next_index=%d\n",next_index);
            if (DOVE_STATUS_OK == 
                dgw_rest_api_show_all_vnid_stats(&stats))
            {
                if(stats.count>0)
                {
                    if((uint32_t)next_index > stats.count)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "next_index=%d count=%u\n",
                                 next_index, stats.count);

                        res_code = HTTP_NOCONTENT;
                        break;
                    }
                    else if(stats.count >= MAX_SESSION_DUMP_COUNT)
                    {
                        log_info(ServiceUtilLogLevel,
                                 "Sessions count %d very large\n",
                                 stats.count);
                        res_code = HTTP_INTERNAL;
                        break;
                    }
                    else
                    {
                        char *buffer = (char*) malloc (1024*stats.count);                       
                        if(buffer)
                        {
                            int len = 
                                build_vnid_stats_json_response(&stats, 
                                                               buffer,
                                                               next_index-1,
                                                               maxstats);
                            evbuffer_add(retbuf, buffer, len+1);   
                            res_code = 200;
                        }
                    }
                }
                else
                {
                    res_code = HTTP_NOCONTENT;
                    break;
                }
            }
            else
            {
                log_info(ServiceUtilLogLevel,
                         "Failed to read ext session details\n");
                res_code = HTTP_INTERNAL;
                break;
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}


int build_stats_json_response(dgwy_stats_t *stats, char *start)
{
    /* build below json body
    {
        "ovl_to_ext_leave_bytes":"X",
        "ovl_to_ext_leave_pkts":"X",
        "ovl_to_ext_leave_bps":"X",
        "ovl_to_ext_leave_pps":"X",
        "ext_to_ovl_enter_bytes":"X",
        "ext_to_ovl_enter_pkts":"X",
        "ext_to_ovl_enter_bps":"X",
        "ext_to_ovl_enter_pps":"X",
        "ovl_to_vlan_leave_bytes":"X",
        "ovl_to_vlan_leave_pkts":"X",
        "ovl_to_vlan_leave_bps":"X",
        "ovl_to_vlan_leave_pps":"X",
        "vlan_to_ovl_enter_bytes":"X",
        "vlan_to_ovl_enter_pkts":"X",
        "vlan_to_ovl_enter_bps":"X",
        "vlan_to_ovl_enter_pps":"X",
    }
    */

    int len=0;
    char *buffer=start;

    len =  sprintf(buffer,"{\r\n\t");
    buffer+= len;
    
    len =  sprintf(buffer,"\"ovl_to_ext_leave_bytes\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_rxbytes);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_ext_leave_pkts\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_rxpkts);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_ext_leave_bps\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_rxbps);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_ext_leave_pps\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_rxpps);
    buffer+= len;

    len =  sprintf(buffer,"\"ext_to_ovl_enter_bytes\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_txbytes);
    buffer+= len;

    len =  sprintf(buffer,"\"ext_to_ovl_enter_pkts\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_txpkts);
    buffer+= len;

    len =  sprintf(buffer,"\"ext_to_ovl_enter_bps\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_txbps);
    buffer+= len;

    len =  sprintf(buffer,"\"ext_to_ovl_enter_pps\":\"%ld\",\r\n\t",
                   stats->ext_gw_overlay_txpps);
    buffer+= len;


    len =  sprintf(buffer,"\"ovl_to_vlan_leave_bytes\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_rxbytes);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_vlan_leave_pkts\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_rxpkts);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_vlan_leave_bps\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_rxbps);
    buffer+= len;

    len =  sprintf(buffer,"\"ovl_to_vlan_leave_pps\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_rxpps);
    buffer+= len;

    len =  sprintf(buffer,"\"vlan_to_ovl_enter_bytes\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_txbytes);
    buffer+= len;

    len =  sprintf(buffer,"\"vlan_to_ovl_enter_pkts\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_txpkts);
    buffer+= len;

    len =  sprintf(buffer,"\"vlan_to_ovl_enter_bps\":\"%ld\",\r\n\t",
                   stats->vlan_gw_overlay_txbps);
    buffer+= len;

    len =  sprintf(buffer,"\"vlan_to_ovl_enter_pps\":\"%ld\"\r\n",
                   stats->vlan_gw_overlay_txpps);
    buffer+= len;

    len = sprintf(buffer,"}\r\n");
    buffer+= len;
    

    log_debug(ServiceUtilLogLevel,
             "one object len = %d", buffer-start);

    len = buffer-start;

    log_debug(ServiceUtilLogLevel,
             "Len = %d", len);

    return len;
}



void dgw_req_handler_stats(struct evhttp_request *req, void *arg,
                           int argc, char **argv)
{
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_GET: 
        {
            dgwy_stats_t stats;

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

            if (DOVE_STATUS_OK == 
                dgw_rest_api_show_all_stats(&stats))
            {
                char *buffer = (char*) malloc (32*sizeof(stats));  
                if(buffer)
                {
                    int len = 0;

                    memset(buffer,0,sizeof(buffer));
                    len = build_stats_json_response(&stats, buffer);
                    evbuffer_add(retbuf, buffer, len+1);   
                    res_code = 200;
                }
                else
                {
                    res_code = HTTP_NOCONTENT;
                    break;
                }
            }
            else
            {
                log_info(ServiceUtilLogLevel,
                         "Failed to read ext session details\n");
                res_code = HTTP_INTERNAL;
                break;
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
}


void dgw_req_handler_serivce_ovlport(struct evhttp_request *req, void *arg,
                                     int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id   = NULL;
    json_t *js_res  = NULL;
    int res_code    = HTTP_BADREQUEST;
    int port        = 0;
    int n           = 0;
    struct evbuffer *retbuf     = NULL;
    struct evbuffer *req_body   = NULL;
    char req_body_buf[1024];
    json_error_t jerror;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));
       
    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {

            retbuf = evbuffer_new();
            if (NULL == retbuf)
            {
                break;
            }

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

            js_id = json_object_get(js_root, "port");
            if (!json_is_integer(js_id))
            {
                break;
            }
            port = json_integer_value(js_id);


            if((port>0) && (port<65535))
            {
                if(DOVE_STATUS_OK == 
                   dgw_rest_api_set_ovl_port(port))
                {
                    res_code = 200;
                }
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}



/*
   POST /api/dove/dgw/service/mcast-ext
 */
void dgw_req_handler_serivce_mcasts_external(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    json_t *js_root = NULL;
    json_t *js_id = NULL;
    json_t *js_res = NULL;
    char res_body_str[128]; 
    struct evbuffer *retbuf = NULL;
    struct evbuffer *req_body = NULL;
    char req_body_buf[1024];
    int res_code = HTTP_BADREQUEST;
    int n;
    json_error_t jerror;
    const char *str;
    int domain_id;
    struct in_addr ipv4;
    char id[VIP_ID_LEN + 1];
    char id1[IP_ID_LEN + 1];

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    /*
    char name[SERVICE_NAME_MAX_LEN+1];
    char *endptr;   
    unsigned int service_id;
    if (argc != 1 || NULL == argv)
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    service_id = strtoul(argv[0], &endptr, 10);
       
    if (*endptr != '\0')
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }

    if (0 != get_service_key_from_id (service_id, name))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
        return;
    }   
    */

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_POST: 
        {
            retbuf = evbuffer_new();
                
            if (NULL == retbuf)
            {
                break;
            }
                    
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

            log_debug(CliLogLevel,"%s: req_body_buf %s",
                      __FUNCTION__, req_body_buf);

            js_root = json_loads(req_body_buf, 0, &jerror);

            if (!js_root)
            {
                break;
            }
            /* Borrowed reference, no need to decref */
            js_id = json_object_get(js_root, "ipv4");
            if (json_is_null(js_id))
            {
                break;
            }
            
            if (NULL == (str = json_string_value(js_id)))
            {
                break;
            }
            
            if (0 == inet_pton(AF_INET, str, (void *)&ipv4))
            {
                break;
            }

            js_id = json_object_get(js_root, "net_id");
            if (!json_is_integer(js_id))
            {
                break;
            }
            domain_id = json_integer_value(js_id);          

            log_debug(CliLogLevel,"%s:%d",
                      __FUNCTION__, __LINE__);

            if(DOVE_STATUS_OK == 
                    dgw_rest_api_set_mcast_external(domain_id, ipv4.s_addr))
            {
                v4_addr_to_ip_id(id1, ntohl(ipv4.s_addr));
                sprintf (id, "%s%d",id1,domain_id);
                sprintf (res_body_str, "{\"id\":\"%s\"}",id);   
                evbuffer_add(retbuf, res_body_str, STRLEN(res_body_str) + 1);                                         
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
}


void dgw_req_handler_serivce_mcast_external(struct evhttp_request *req, void *arg, int argc, char **argv)
{

}
/*
   POST /api/dove/dgw/service/network
 */
void dgw_req_handler_networks(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));


    retbuf = evbuffer_new();
    if (retbuf)
    { 
        res_code = 200;
        evbuffer_add(retbuf, "OK", STRLEN("OK") + 1); 
    }

    evhttp_send_reply(req, res_code, NULL, retbuf);

    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}


/*
   DELETE /api/dove/dgw/service/network/{netid},{domainid},{type}
 */
void dgw_req_handler_network(struct evhttp_request *req, void *arg, int argc, char **argv)
{
    struct evbuffer *retbuf = NULL;
    int res_code = HTTP_BADREQUEST;
    int netid=0;
    int domainid=0;
    int type=0;

    log_info(ServiceUtilLogLevel,"argc %d argv %s",
             argc, (argv[0]?argv[0]:"nil"));

    if(3 != sscanf(argv[0],"%d,%d,%d", &netid, &domainid, &type))
    {
        evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);;
        return;
    }

    switch (evhttp_request_get_command(req))
    {
        case EVHTTP_REQ_DELETE:
        {
            if (DOVE_STATUS_OK == 
                dgw_rest_api_del_network(netid, domainid, type))
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
    if (retbuf)
    {
        evbuffer_free(retbuf);
    }
}




