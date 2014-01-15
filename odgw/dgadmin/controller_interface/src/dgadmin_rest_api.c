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
#include <jansson.h>
#include "../inc/dgadmin_rest_api.h"
#include "dgadmin_generic_api.h"
#include "service.h"

dove_status dgw_rest_vxlan_port(int port)
{
    if(port>0 && port <=65535 )
    {
        if(port != OVL_PROTO_DST_PORT)
        {
            OVL_PROTO_DST_PORT = port;
            log_alert(ServiceUtilLogLevel,"Set OVL_PROTO_DST_PORT=%d\n",
                      OVL_PROTO_DST_PORT);
        }
    }
    else
    {
        /* DEFAULT OTV PORT */
        if(OVL_PROTO_DST_PORT != 8472)
        {
            OVL_PROTO_DST_PORT = 8472;
            log_alert(ServiceUtilLogLevel,"Set OVL_PROTO_DST_PORT=%d\n",
                      OVL_PROTO_DST_PORT);
        }
    }
    return DOVE_STATUS_OK;
}

dove_status dgw_rest_api_create_dps (uint32_t ip, int port)
{
    cli_dps_server_t dps;

    dps.domain = 0;
    dps.dps_IP = ip;
    dps.port = (uint16_t)port;

    if (DOVE_STATUS_OK == api_dgadmin_add_dps_server(&dps))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}

dove_status dgw_rest_api_del_dps (uint32_t ip)
{
    cli_dps_server_t dps;

    dps.dps_IP = ip;

    if (DOVE_STATUS_OK == api_dgadmin_del_dps_server(&dps))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}

dove_status dgw_rest_api_create_service (char *name, char *type)
{
    cli_service_type_t service;

    strcpy (service.bridge_name, name);

    if (!strcasecmp (type, "external"))
    {
        strcpy (service.type, "EXT");
    }
    else if (!strcasecmp (type, "vlan"))
    {
        strcpy (service.type, "VLAN");
    }
    else if (!strcasecmp (type, "extvlan"))
    {
        strcpy (service.type, "EXTVLAN");
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER; 
    }
        

    if (DOVE_STATUS_OK == api_dgadmin_add_service_type(&service))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}


dove_status dgw_rest_api_del_service (char *name)
{
    cli_service_type_t service;
    
    strcpy (service.bridge_name, name);
    
    if (DOVE_STATUS_OK == api_dgadmin_del_service_type(&service))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}


dove_status dgw_rest_api_set_service(char *name, int mtu, int start)
{
    cli_service_mtu_t mtu_s;
    dove_status ret;

    if (-1 != mtu)
    {
        strcpy (mtu_s.bridge_name, name);
        mtu_s.mtu = (uint16_t)mtu;
        
        if (DOVE_STATUS_OK == api_dgadmin_set_service_mtu(&mtu_s))
        {
            ret = DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;         
        }
    }

    if (-1 != start)
    {
        if (start)
        {
            if (0 == dgwy_ctrl_set_global(CMD_START))
            {
                ret = DOVE_STATUS_OK;
            }
            else
            {
                return DOVE_STATUS_INVALID_PARAMETER;         
            }
        }
        else
        {
            if (0 == dgwy_ctrl_set_global(CMD_STOP))
            {
                ret = DOVE_STATUS_OK;
            }
            else
            {
                return DOVE_STATUS_INVALID_PARAMETER;         
            }       
        }
    }
    
    return ret;     
}

dove_status dgw_rest_api_create_service_nic(char *name, char *mac)
{
    cli_service_ifmac_t ifmac;

    mac[0] = mac[0] & 0xff; mac[1] = mac[1] & 0xff;
    mac[2] = mac[2] & 0xff; mac[3] = mac[3] & 0xff;
    mac[4] = mac[4] & 0xff; mac[5] = mac[5] & 0xff;

    strcpy (ifmac.bridge_name, name);
    memcpy (ifmac.mac, mac, 6);

    if (DOVE_STATUS_OK == (dove_status)api_dgadmin_add_service_nic(&ifmac))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}


dove_status dgw_rest_api_del_service_nic(char *name, char *mac)
{
    cli_service_ifmac_t ifmac;

    mac[0] = mac[0] & 0xff; mac[1] = mac[1] & 0xff;
    mac[2] = mac[2] & 0xff; mac[3] = mac[3] & 0xff;
    mac[4] = mac[4] & 0xff; mac[5] = mac[5] & 0xff;

    strcpy (ifmac.bridge_name, name);
    memcpy (ifmac.mac, mac, 6);

    if (DOVE_STATUS_OK == api_dgadmin_del_service_nic(&ifmac))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}


dove_status dgw_rest_api_set_ipv4_type(char *name, uint32_t ip,
                                       char *type, uint32_t nxthop)
{
    int len=0;

    if(type==NULL)
    {
        return DOVE_STATUS_OK;
    }

    len = strlen(type);

    if( (len==strlen("dovetunnel")) &&
        (memcmp(type,"dovetunnel",len)==0))
    {
        cli_dove_net_ipv4_t ipv4;
        ipv4.IPv4 = ip;
        ipv4.IPv4_nexthop = nxthop;
        strcpy (ipv4.bridge_name, name);

        if (DOVE_STATUS_OK == (dove_status)api_dgadmin_set_dove_net_ipv4(&ipv4))
        {
            return DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
    else
    if((len==strlen("mgmt")) &&
       (memcmp(type,"mgmt",len)==0))
    {
        cli_mgmt_ipv4_t ipv4;
        ipv4.IPv4 = ip;
        //strcpy (ipv4.bridge_name, name);

        if (DOVE_STATUS_OK == (dove_status)api_dgadmin_set_mgmt_ipv4(&ipv4))
        {
            return DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
    else
    if((len==strlen("external")) &&
       (memcmp(type,"external",len)==0))
    {
        cli_dove_net_ipv4_t ipv4;
        ipv4.IPv4 = ip;
        ipv4.IPv4_nexthop = nxthop;
        //strcpy (ipv4.bridge_name, name);

        if (DOVE_STATUS_OK == (dove_status)api_dgadmin_set_ext_net_ipv4(&ipv4))
        {
            return DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }

        
        return DOVE_STATUS_OK;
    }
    else
    if((len==strlen("dcsnet")) &&
       (memcmp(type,"dcsnet",len)==0))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_OK;
    }
    return DOVE_STATUS_OK;
}

dove_status dgw_rest_api_create_service_ipv4(char *name, uint32_t ip, 
                                             uint32_t mask, uint32_t nexthop,
                                             char *type, int vlan_id)
{
    cli_service_addressv4_t ipv4;
    int len=0;
    int dovetunnel=0;

    if(type==NULL)
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    len = strlen(type);

    if( (len==strlen("dovetunnel")) &&
        (memcmp(type,"dovetunnel",len)==0))
    {
        dovetunnel=1;
    }

    strcpy (ipv4.bridge_name, name);
    ipv4.IPv4 = ip;
    ipv4.IPv4_netmask = mask;
    ipv4.IPv4_nexthop = nexthop;
    ipv4.vlan_id = vlan_id;

    if(dovetunnel)
    {
        if (DOVE_STATUS_OK == (dove_status)api_dgadmin_add_dovenet_svc_ipv4(&ipv4))
        {
            return DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        if (DOVE_STATUS_OK == (dove_status)api_dgadmin_add_service_ipv4(&ipv4))
        {
            return DOVE_STATUS_OK;
        }
        else
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
}

dove_status dgw_rest_api_del_service_ipv4(char *name, uint32_t ip)
{
    cli_service_addressv4_t ipv4;

    strcpy (ipv4.bridge_name, name);
    ipv4.IPv4 = ip;

    if (DOVE_STATUS_OK == api_dgadmin_del_service_ipv4(&ipv4))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }
}

/*
typedef unsigned long long timestamp_t;
static timestamp_t get_timestamp ()
{
    struct timeval now;
    gettimeofday (&now, NULL);
    return  now.tv_usec + (timestamp_t)now.tv_sec * 1000000;
}
*/

dove_status dgw_rest_api_create_service_external_vip (char *name, int domain, uint32_t min_ip, 
                                                      uint32_t max_ip, int min_port, 
                                                      int max_port, int tenant_id, int extmcastvnid)
{
    cli_service_extvip_t extvip;
    char *ip1, *ip2;
    uint32_t tmp;
    uint32_t min_ip_tmp, max_ip_tmp;

    ip1 = (char *)&min_ip_tmp;
    ip2 = (char *)&max_ip_tmp;

    memset(&extvip,0,sizeof(extvip));

    strcpy (extvip.bridge_name, name);
    extvip.domain = domain;
    extvip.port_min = (uint16_t)min_port;
    extvip.port_max = (uint16_t)max_port;
    extvip.tenant_id = tenant_id;
    extvip.extmcastvnid = extmcastvnid;

    ip1[0] = ((min_ip >> 24) & 0xff);
    ip1[1] = ((min_ip >> 16) & 0xff);
    ip1[2] = ((min_ip >> 8) & 0xff);
    ip1[3] = ((min_ip) & 0xff);

    ip2[0] = ((max_ip >> 24) & 0xff);
    ip2[1] = ((max_ip >> 16) & 0xff);
    ip2[2] = ((max_ip >> 8) & 0xff);
    ip2[3] = ((max_ip) & 0xff);
    
            
    for (tmp = min_ip_tmp; tmp <= max_ip_tmp; tmp++)
    {       
        ip1 = (char *)&extvip.vIPv4;
        
        ip1[0] = ((tmp >> 24) & 0xff);
        ip1[1] = ((tmp >> 16) & 0xff);
        ip1[2] = ((tmp >> 8) & 0xff);
        ip1[3] = ((tmp) & 0xff);

        if (DOVE_STATUS_OK != api_dgadmin_add_service_pubipv4(&extvip))
        {
            log_error(ServiceUtilLogLevel,"ERROR : Failed to configure IP:0x%x\r\n",tmp);
            /*return DOVE_STATUS_INVALID_PARAMETER;*/
        }
    }
            
    
    return DOVE_STATUS_OK;            
}

dove_status dgw_rest_api_del_service_external_vip (char *name, uint32_t min_ip, uint32_t max_ip)
{
    cli_service_extvip_t extvip;
    char *ip1, *ip2;
    uint32_t tmp;
    uint32_t min_ip_tmp, max_ip_tmp;

    memset(&extvip,0,sizeof(extvip));

    ip1 = (char *)&min_ip_tmp;
    ip2 = (char *)&max_ip_tmp;

    strcpy (extvip.bridge_name, name);

    ip1[0] = ((min_ip >> 24) & 0xff);
    ip1[1] = ((min_ip >> 16) & 0xff);
    ip1[2] = ((min_ip >> 8) & 0xff);
    ip1[3] = ((min_ip) & 0xff);

    ip2[0] = ((max_ip >> 24) & 0xff);
    ip2[1] = ((max_ip >> 16) & 0xff);
    ip2[2] = ((max_ip >> 8) & 0xff);
    ip2[3] = ((max_ip) & 0xff);
    
    for (tmp = min_ip_tmp; tmp <= max_ip_tmp; tmp++)
    {
        ip1 = (char *)&extvip.vIPv4;

        ip1[0] = ((tmp >> 24) & 0xff);
        ip1[1] = ((tmp >> 16) & 0xff);
        ip1[2] = ((tmp >> 8) & 0xff);
        ip1[3] = ((tmp) & 0xff);
    
        if (DOVE_STATUS_OK != api_dgadmin_del_service_pubipv4(&extvip))
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
    return DOVE_STATUS_OK;            
}

dove_status dgw_rest_api_create_service_internal_vip (char *name, int domain, uint32_t min_ip, uint32_t max_ip, int min_port, int max_port)
{
    cli_service_overlayip_t overlayip;
    char *ip1, *ip2;
        uint32_t tmp;
    uint32_t min_ip_tmp, max_ip_tmp;

    ip1 = (char *)&min_ip_tmp;
    ip2 = (char *)&max_ip_tmp;

    strcpy (overlayip.bridge_name, name);
    overlayip.domain = domain;
    overlayip.port_min = (uint16_t)min_port;
    overlayip.port_max = (uint16_t)max_port;

    ip1[0] = ((min_ip >> 24) & 0xff);
    ip1[1] = ((min_ip >> 16) & 0xff);
    ip1[2] = ((min_ip >> 8) & 0xff);
    ip1[3] = ((min_ip) & 0xff);

    ip2[0] = ((max_ip >> 24) & 0xff);
    ip2[1] = ((max_ip >> 16) & 0xff);
    ip2[2] = ((max_ip >> 8) & 0xff);
    ip2[3] = ((max_ip) & 0xff);
    
    for (tmp = min_ip_tmp; tmp <= max_ip_tmp; tmp++)
    {       
        ip1 = (char *)&overlayip.vIPv4;
        
        ip1[0] = ((tmp >> 24) & 0xff);
        ip1[1] = ((tmp >> 16) & 0xff);
        ip1[2] = ((tmp >> 8) & 0xff);
        ip1[3] = ((tmp) & 0xff);
        
        if (DOVE_STATUS_OK != (dove_status)api_dgadmin_add_service_overlayipv4(&overlayip))
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }
    return DOVE_STATUS_OK;            
}

dove_status dgw_rest_api_del_service_internal_vip (char *name, uint32_t min_ip, uint32_t max_ip)
{
    cli_service_overlayip_t overlayip;
    char *ip1, *ip2;
        uint32_t tmp;
    uint32_t min_ip_tmp, max_ip_tmp;

    ip1 = (char *)&min_ip_tmp;
    ip2 = (char *)&max_ip_tmp;

    strcpy (overlayip.bridge_name, name);

    ip1[0] = ((min_ip >> 24) & 0xff);
    ip1[1] = ((min_ip >> 16) & 0xff);
    ip1[2] = ((min_ip >> 8) & 0xff);
    ip1[3] = ((min_ip) & 0xff);

    ip2[0] = ((max_ip >> 24) & 0xff);
    ip2[1] = ((max_ip >> 16) & 0xff);
    ip2[2] = ((max_ip >> 8) & 0xff);
    ip2[3] = ((max_ip) & 0xff);
    
    for (tmp = min_ip_tmp; tmp <= max_ip_tmp; tmp++)
    {
        ip1 = (char *)&overlayip.vIPv4;
        ip1[0] = ((tmp >> 24) & 0xff);
        ip1[1] = ((tmp >> 16) & 0xff);
        ip1[2] = ((tmp >> 8) & 0xff);
        ip1[3] = ((tmp) & 0xff);
        
        if (DOVE_STATUS_OK != (dove_status)api_dgadmin_del_service_overlayipv4(&overlayip))
        {
            return DOVE_STATUS_INVALID_PARAMETER;
        }
    }

        return DOVE_STATUS_OK;            
}

dove_status dgw_rest_api_create_service_rule (char *name, int domain, 
                                              uint32_t ip, uint32_t real_ip, 
                                              int port, int real_port, int protocol,
                                              uint32_t pip_min, uint32_t pip_max)
{
    cli_service_fwdrule_t rule;

    strcpy (rule.bridge_name, name);
    rule.domain = domain;
    rule.IPv4 = ip;
    rule.IPv4_map = real_ip;
    rule.port = (uint16_t)port;
    rule.port_map = (uint16_t)real_port;
    rule.protocol = (uint16_t)protocol;
    rule.pip_min = pip_min;
    rule.pip_max = pip_max;

    if (DOVE_STATUS_OK == api_dgadmin_add_service_fwdrule(&rule))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }       
}

dove_status dgw_rest_api_del_service_rule (char *name, int domain, 
                                           uint32_t ip, uint32_t real_ip,
                                           int port, int real_port, int protocol)
{
    cli_service_fwdrule_t rule;

    strcpy (rule.bridge_name, name);
    rule.domain = domain;
    rule.IPv4 = ip;
    rule.IPv4_map = real_ip;
    rule.port = (uint16_t)port;
    rule.port_map = (uint16_t)real_port;
    rule.protocol = (uint16_t)protocol;

    if (DOVE_STATUS_OK == api_dgadmin_del_service_fwdrule(&rule))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }       
}

dove_status dgw_rest_api_create_service_vlan_map(char *name, int domain, int vlan)
{
    cli_service_dvlan_t vlanmap;

    strcpy (vlanmap.bridge_name, name);
    vlanmap.domain = domain;
    vlanmap.vlan = vlan;

    if (DOVE_STATUS_OK == api_dgadmin_add_domain_vlan(&vlanmap))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }           
}

dove_status dgw_rest_api_del_service_vlan_map(char *name, int domain, int vlan)
{
    cli_service_dvlan_t vlanmap;

    strcpy (vlanmap.bridge_name, name);
    vlanmap.domain = domain;
    vlanmap.vlan = vlan;

    log_debug(ServiceUtilLogLevel,
              "\nvlan map delete, name[%s], domain[%d], vlan[%d]\n", name, domain, vlan);

    if (DOVE_STATUS_OK == api_dgadmin_del_domain_vlan(&vlanmap))
    {
        return DOVE_STATUS_OK;
    }
    else
    {
        return DOVE_STATUS_INVALID_PARAMETER;
    }           
}


dove_status dgw_rest_api_set_service_subnet(char *name, 
                                            uint32_t vnid,
                                            uint32_t ip, 
                                            uint32_t mask, 
                                            uint32_t nexthop,
                                            char *type_str)
{
    cli_vnid_subnet_t vnid_subnet;
    dove_status status = DOVE_STATUS_OK;
    int len=strlen("shared");

    memset(&vnid_subnet,0,sizeof(vnid_subnet));

    vnid_subnet.vnid = vnid;
    vnid_subnet.IPv4 = ip;
    vnid_subnet.IPv4_mask = mask;
    vnid_subnet.IPv4_nexthop = nexthop;

    if (strncmp(type_str,"shared",len)==0)
    {
        vnid_subnet.subnet_mode=VNID_INFO_STATE_SHARED;
    }
    else
    {
        vnid_subnet.subnet_mode=VNID_INFO_STATE_DEDICATED;
    }

    status = api_dgadmin_add_vnid_subnetv4(&vnid_subnet);
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_del_service_subnet(char *name, 
                                            uint32_t vnid,
                                            uint32_t ip)
{
    cli_vnid_subnet_t vnid_subnet;
    dove_status status = DOVE_STATUS_OK;

    memset(&vnid_subnet,0,sizeof(vnid_subnet));

    vnid_subnet.vnid = vnid;
    vnid_subnet.IPv4 = ip;
    vnid_subnet.IPv4_mask = 0;
    vnid_subnet.IPv4_nexthop = 0;
    vnid_subnet.subnet_mode = 0;

    status = api_dgadmin_del_vnid_subnetv4(&vnid_subnet);
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_set_service_dmc(char *name, 
                                         uint32_t ip, 
                                         int port)
{
    cli_dmc_ipv4_t dmc_ipv4;
    dove_status status = DOVE_STATUS_OK;

    memset(&dmc_ipv4,0,sizeof(dmc_ipv4));
    dmc_ipv4.IPv4 = ip;
    dmc_ipv4.port = port;
    status = api_dgadmin_set_dmc_ipv4(&dmc_ipv4);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_show_all_vnid_stats(dgwy_vnid_stats_t *stats)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_show_all_vnid_stats(stats);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_show_all_stats(dgwy_stats_t *stats)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_show_all_stats(stats);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_show_all_ext_sessions(ext_sesion_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_show_all_ext_sessions(sessions);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_show_all_int_sessions(int_session_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_show_all_int_sessions(sessions);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_show_all_fwddyn_sessions(sessions);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


dove_status dgw_rest_api_set_ovl_port(int port)
{
    dove_status status = DOVE_STATUS_OK;
    
    status = api_set_ovl_port(port);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


dove_status dgw_rest_api_set_mcast_external(uint32_t domain_id, uint32_t ipv4)
{
    dove_status status = DOVE_STATUS_OK;

    status = api_dgadmin_set_ext_mcast_vnid(domain_id,ipv4);
    
    if(status != DOVE_STATUS_OK)
    {   
        return DOVE_STATUS_INVALID_PARAMETER;
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

dove_status dgw_rest_api_del_network(int netid, int domainid, int type)
{
    dove_status status = DOVE_STATUS_OK;

    if(type==0)
    {
        /* NON mcastvnid  */
        status = api_dgadmin_del_vnid(netid,domainid);
        if(status != DOVE_STATUS_OK)
        {   
            return DOVE_STATUS_INVALID_PARAMETER;
        }
        else
        {
            return (DOVE_STATUS_OK);
        }
    }
    else
    {
        /* mcastvnid  */
        status = api_dgadmin_del_extmcastvnid(netid,domainid);
        if(status != DOVE_STATUS_OK)
        {   
            return DOVE_STATUS_INVALID_PARAMETER;
        }
        else
        {
            return (DOVE_STATUS_OK);
        }
    }

}

dove_status dgwy_rest_api_ctrl_reset_stats(void)
{
    return dgwy_api_ctrl_reset_stats();
}

