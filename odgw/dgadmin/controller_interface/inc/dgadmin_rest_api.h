/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


#ifndef _DPS_REST_API_H_
#define _DPS_REST_API_H_

#include <service.h>

#define SERVICE_TYPE_MAX_LEN (15)
#define SERVICE_NAME_MAX_LEN (BRIDGE_NAME_MAX - 1)
#define MAC_STR_LEN 17
#define VIP_ID_LEN 24
#define IP_ID_LEN 12
#define VLAN_MAP_ID_LEN 10
#define MAC_ID_LEN 18

#define DPS_REST_LISTEN_DEFAULT_IP "0.0.0.0"
#define DPS_REST_LISTEN_DEFAULT_PORT 1889

#define DGW_SERVICES_URI        "/api/dove/dgw/service"
#define DGW_SERVICE_URI         "/api/dove/dgw/service/*"
#define DGW_SERVICE_NICS_URI    "/api/dove/dgw/service/*/nic"
#define DGW_SERVICE_NIC_URI     "/api/dove/dgw/service/*/nic/*"
#define DGW_DPSES_URI           "/api/dove/dgw/service/dps"
#define DGW_DPS_URI             "/api/dove/dgw/service/dps/*"
#define DGW_SERVICE_IPV4S_URI   "/controller/sb/v2/opendove/odmc/odgw/ipv4"   //"/api/dove/dgw/service/ipv4"
#define DGW_SERVICE_IPV4_URI    "/controller/sb/v2/opendove/odmc/odgw/ipv4/*" //"/api/dove/dgw/service/ipv4/*"
#define DGW_SERVICE_EXTERNAL_VIPS_URI "/controller/sb/v2/opendove/odmc/ext-gws"  //"/api/dove/dgw/service/ext-gw"
#define DGW_SERVICE_EXTERNAL_VIP_URI  "/controller/sb/v2/opendove/odmc/ext-gws/*" //"/api/dove/dgw/service/ext-gw/*"
#define DGW_SERVICE_INTERNAL_VIPS_URI "/api/dove/dgw/service/*/internal-vip"
#define DGW_SERVICE_INTERNAL_VIP_URI "/api/dove/dgw/service/*/internal-vip/*"
#define DGW_SERVICE_RULES_URI "/controller/sb/v2/opendove/odmc/ext-fwd-rules" //"/api/dove/dgw/service/rule"
#define DGW_SERVICE_RULE_URI "/controller/sb/v2/opendove/odmc/ext-fwd-rules/*" //"/api/dove/dgw/service/rule/*"
#define DGW_SERVICE_VLANMAPS_URI "/controller/sb/v2/opendove/odmc/vlan-gws"   //"/api/dove/dgw/service/vlan-gw"
#define DGW_SERVICE_VLANMAP_URI  "/controller/sb/v2/opendove/odmc/vlan-gws/*" //"/api/dove/dgw/service/vlan-gw/*"
#define DGW_SERVICE_START "/api/dove/dgw/service/*/start/*"
#define DGW_APPLIANCE_REGISTRATION_URI "/controller/sb/v2/opendove/odmc/odgw"  //"/api/dove/dgw/service/registration"
#define DGW_APPLIANCE_DMC_HEARTBEAT_URI "/controller/sb/v2/opendove/odmc/odgw" //"/api/dove/dgw/service/heartbeat"
#define DGW_APPLIANCE_DMC_INFO_URI "/api/dove/dgw/dmc" 
#define DGW_SERVICE_SUBNETS_DMC_URI "/controller/sb/v2/opendove/odmc/networks/*/networkSubnets/*" //"/api/dove/dgw/service/subnet"
#define DGW_SERVICE_SUBNETS_URI "/controller/sb/v2/opendove/odmc/networks/*/networkSubnets" //"/api/dove/dgw/service/subnet"
#define DGW_SERVICE_SUBNET_URI "/controller/sb/v2/opendove/odmc/networks/*/networkSubnets/*" //"/api/dove/dgw/service/subnet/*"
#define DGW_SERVICE_SUBNET_URI_PARAM "/controller/sb/v2/opendove/odmc/networks/%d/networkSubnets" //"/api/dove/dgw/service/subnet/*"
#define DGW_SERVICE_EXT_SESSION_URI "/api/dove/dgw/service/extsessions"
#define DGW_SERVICE_INT_SESSION_URI "/api/dove/dgw/service/intsessions"
#define DGW_SERVICE_FWDDYN_SESSION_URI "/api/dove/dgw/service/fwddynsessions"
#define DGW_OVERLAY_PORT_NUMBER "/api/dove/dgw/service/ovlport"
#define DGW_SERVICE_EXTERNAL_MCASTS_URI "/api/dove/dgw/service/mcast-ext"
#define DGW_SERVICE_EXTERNAL_MCAST_URI "/api/dove/dgw/service/mcast-ext/*"
#define DGW_SERVICE_VNID_STATS_URI "/api/dove/dgw/service/vnid_stats"
#define DGW_SERVICE_STATS_URI "/api/dove/dgw/service/stats"
#define DGW_SERVICE_NETWORKS  "/controller/sb/v2/opendove/odmc/networks/"  //"/api/dove/dgw/service/network"
#define DGW_SERVICE_NETWORK   "/controller/sb/v2/opendove/odmc/networks/*" //"/api/dove/dgw/service/network/*"
#define DGW_APPLIANCE_DCS_REQUEST_URI "/controller/sb/v2/opendove/odmc/odcs/leader" //"/api/dove/dps/cluster/leader"
#define DGW_SERVICE_RESETSTATS_URI "/api/dove/dgw/service/resetstat"
#define DGW_VXLAN_REQUEST_URI "/api/dove/sys/vxlan-port"

#define DGW_URI_MAX_LEN 128
#define DGW_JSON_OBJECT_LEN 64

typedef struct
{
    char uri[DGW_URI_MAX_LEN];
    char json_object_name[DGW_JSON_OBJECT_LEN];
} dgw_uri_list_t;

typedef struct
{
    unsigned int id;
	char name[SERVICE_NAME_MAX_LEN + 1];
} service_id_key_map;

dove_status dgw_rest_vxlan_port(int port);
dove_status dgw_rest_api_create_dps (uint32_t ip, int port);
dove_status dgw_rest_api_del_dps (uint32_t ip);
dove_status dgw_rest_api_create_service (char *name, char *type);
dove_status dgw_rest_api_del_service (char *name);
dove_status dgw_rest_api_set_service (char *name, int mtu, int start);
dove_status dgw_rest_api_create_service_nic(char *name, char *mac);
dove_status dgw_rest_api_del_service_nic(char *name, char *mac);
dove_status dgw_rest_api_create_service_ipv4(char *name, uint32_t ip, uint32_t mask,uint32_t nexthop,char* type, int vlan_id);
dove_status dgw_rest_api_del_service_ipv4(char *name, uint32_t ip);
dove_status dgw_rest_api_set_mcast_external(uint32_t domain_id, uint32_t ipv4);
dove_status dgw_rest_api_create_service_external_vip (char *name, int domain, uint32_t min_ip, 
                                                      uint32_t max_ip, int min_port, 
                                                      int max_port, int tenant_id, int extmcastvnid);

dove_status dgw_rest_api_del_network(int netid, int domainid, int type);

dove_status dgw_rest_api_set_ipv4_type(char *name, uint32_t ip, char *type, uint32_t nexthop);
dove_status dgw_rest_api_del_service_external_vip (char *name, uint32_t ip_start, uint32_t ip_end);
dove_status dgw_rest_api_create_service_internal_vip (char *name, int domain, uint32_t ip_start, uint32_t ip_end, int port_start, int port_end);
dove_status dgw_rest_api_del_service_internal_vip (char *name, uint32_t ip_start, uint32_t ip_end);
dove_status dgw_rest_api_create_service_rule (char *name, int domain, uint32_t ip, uint32_t real_ip, int port, 
                                              int real_port, int protocol, uint32_t pip_min, uint32_t pip_max);
dove_status dgw_rest_api_del_service_rule (char *name, int domain, uint32_t ip, uint32_t real_ip, int port, int real_port, int protocol);
dove_status dgw_rest_api_create_service_vlan_map(char *name, int domain, int vlan);
dove_status dgw_rest_api_del_service_vlan_map(char *name, int domain, int vlan);
dove_status dgw_rest_api_set_service_subnet(char *name, 
                                            uint32_t vnid,
                                            uint32_t ip, 
                                            uint32_t mask, 
                                            uint32_t nexthop,
                                            char *type_str);

dove_status dgw_rest_api_del_service_subnet(char *name, 
                                            uint32_t vnid,
                                            uint32_t ip);

dove_status dgw_rest_api_set_service_dmc(char *name, 
                                         uint32_t ip, 
                                         int port);

dove_status dgw_rest_api_show_all_ext_sessions(ext_sesion_dump_t *sessions);
dove_status dgw_rest_api_show_all_int_sessions(int_session_dump_t *sessions);
dove_status dgw_rest_api_show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions);
dove_status dgw_rest_api_show_all_vnid_stats(dgwy_vnid_stats_t *stats);
dove_status dgw_rest_api_show_all_stats(dgwy_stats_t *stats);

dove_status dgw_rest_api_set_ovl_port(int port);

dove_status dgwy_rest_api_ctrl_reset_stats(void);

extern UINT4 ip_id_to_v4_addr (char *str, UINT4 *addr);
#endif
