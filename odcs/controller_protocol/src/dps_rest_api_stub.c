/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client functionality
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
*  $Log: rest_api_stub.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include "include.h"
#include <jansson.h>
#include "../inc/dps_rest_api.h"
typedef struct 
{
    UINT4 id;
    char name[32];
    char status;
} dps_domain_t;
#define DPS_MAX_DOMAIN_NUM 1024
dps_domain_t gdps_domain[DPS_MAX_DOMAIN_NUM];

dove_status dps_rest_api_create_domain(UINT4 domain_id, char *domain_name)
{
    int i;
    int freeidx = -1;
    for (i=0;i<DPS_MAX_DOMAIN_NUM;i++)
    {
        if(!gdps_domain[i].status && freeidx == -1)
        {
            freeidx = i;
            continue;
        }
        if (gdps_domain[i].status && gdps_domain[i].id == domain_id)
        {
            STRNCPY(gdps_domain[i].name, domain_name, sizeof(gdps_domain[i].name) - 1);
            gdps_domain[i].name[sizeof(gdps_domain[i].name) - 1] = '\0';
            return DOVE_STATUS_OK;
        }
    }
    if(freeidx != -1)
    {
        gdps_domain[freeidx].id = domain_id;
        gdps_domain[freeidx].status = 1;
        STRNCPY(gdps_domain[freeidx].name, domain_name, sizeof(gdps_domain[freeidx].name) - 1);
        gdps_domain[freeidx].name[sizeof(gdps_domain[freeidx].name) - 1] = '\0';
        return DOVE_STATUS_OK;
    }
    return DOVE_STATUS_NO_RESOURCES;
}


dove_status dps_rest_api_update_domain(UINT4 domain_id, char *domain_name)
{
    int i;
    for (i=0;i<DPS_MAX_DOMAIN_NUM;i++)
    {
        if (gdps_domain[i].status && gdps_domain[i].id == domain_id)
        {
            STRNCPY(gdps_domain[i].name, domain_name, sizeof(gdps_domain[i].name) - 1);
            gdps_domain[i].name[sizeof(gdps_domain[i].name) - 1] = '\0';
            return DOVE_STATUS_OK;
        }
    }
    return DOVE_STATUS_NOT_FOUND;
}

dove_status dps_rest_api_del_domain(UINT4 domain_id)
{
    int i;
    for (i = 0; i < DPS_MAX_DOMAIN_NUM; i++)
    {
        if (gdps_domain[i].status && gdps_domain[i].id == domain_id)
        {
            gdps_domain[i].id = 0;
            MEMSET(gdps_domain[i].name, 0, sizeof(gdps_domain[i].name));
            gdps_domain[i].status = 0;
        }
    }
    return DOVE_STATUS_OK;
}

json_t *dps_rest_api_get_domain(UINT4 domain_id)
{
    int i;
    json_t *js_root = NULL;
    for (i = 0; i < DPS_MAX_DOMAIN_NUM; i++)
    {
        if (gdps_domain[i].status && gdps_domain[i].id == domain_id)
        {
            js_root = json_pack("{s:i, s:s}", "id", (INT4)domain_id, "name", gdps_domain[i].name);
            break;
        }
    }
    return js_root;
}


json_t *dps_rest_api_get_domain_all()
{
    int i;
    json_t *js_domains = NULL;
    json_t *js_domain = NULL;
    json_t *js_root = NULL;

    js_domains = json_array();

    for (i = 0; i < DPS_MAX_DOMAIN_NUM; i++)
    {
        if (gdps_domain[i].status)
        {
            js_domain = json_pack("{s:i,s:s}", "id", (INT4)gdps_domain[i].id, "name", gdps_domain[i].name);
            json_array_append_new(js_domains, js_domain);
        }
    }
    js_root = json_pack("{s:o}", "domains", js_domains);
    return js_root;
}

