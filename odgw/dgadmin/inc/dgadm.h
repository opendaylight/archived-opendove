/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef  __DGADM_H_ 
#define __DGADM_H_
#include "include.h"

#define DGWYCTRLTASK_TIMER_EVENT        0x00000001
#define DGWYCTRLTASK_MSG_EVENT          0x00000002

#define DGWY_HA_TASK_TIMER_EVENT        0x00000001
#define DGWY_HA_TASK_MSG_EVENT          0x00000002

#define DGWY_HA_LISTEN_TASK_TIMER_EVENT 0x00000001
#define DGWY_HA_LISTEN_TASK_MSG_EVENT   0x00000002

#define IP_TYPE_EXTERNAL    0
#define IP_TYPE_EXT_VIP     1
#define IP_TYPE_DOVE_TEP    2

typedef struct dgadm_dps_context {
    uint32_t svnid;
    uint32_t quid;
    uint8_t  shared_lookup;
} dgadm_dps_context_t;
#define VNID_CTX_SZ sizeof(dgadm_dps_context_t)                  

typedef enum {
	DGWY_GET_INFO 	= 1,
	DGWY_INFO_RESP 	= 2,
    DGWY_PEER_UP    = 3,
    DGWY_PEER_DOWN  = 4,
} dgwy_ctrl_msg_type_t;

typedef enum {
	DGWY_HA_START 	            = 1,
	DGWY_HA_CTRL_DSNET_IPV4 	= 2,
	DGWY_HA_CTRL_VNID_VLAN   	= 3,
	DGWY_HA_CTRL_PEER_IPV4   	= 4,
	DGWY_HA_LEGACY_STATE       	= 5,
	DGWY_HA_ARP_PROXY_STATE     = 6,
    DGWY_HA_SKT_ACCEPTED        = 7,
    DGWY_HA_LISTEN_START        = 8,
}dgwy_ha_task_msg_type_t;

typedef struct dgwy_ctrl_msg_info {
	dgwy_ctrl_msg_type_t  type;
	int64_t reqTskId;
	int64_t reqMsgId;
}dgwy_ctrl_msg_info_t;

typedef struct dgwy_ctrl_peer_info {
    dgwy_ctrl_msg_info_t msg_info;
    uint32_t peer_ipv4;
    uint8_t state;
} dgwy_ctrl_peer_info_t;

typedef struct dgwy_ha_task_msg_info {
	dgwy_ha_task_msg_type_t  type;
	int64_t reqTskId;
	int64_t reqMsgId;
}dgwy_ha_task_msg_info_t;

typedef struct dgwy_ha_task_msg_connect{
    dgwy_ha_task_msg_info_t msg_info;
    int fdConnected;
}dgwy_ha_task_msg_connect_t;

typedef struct dgwy_ha_task_msg_peer{
    dgwy_ha_task_msg_info_t msg_info;
    uint32_t peer_ipv4;
    uint32_t local_ipv4;
}dgwy_ha_task_msg_peer_t;

typedef enum dgwy_command {
    DGWY_ADD=1,
    DGWY_SET,
    DGWY_DEL,
}dgwy_command_t;

typedef struct domain_cfg {
    cmds_t      cmd;      /* CMD_SET_DOMAIN (or) CMD_DEL_DOMAIN */
    uint32_t    domainid; /* domain id */
} domain_cfg_t;

typedef struct domain_vlan_map {
    uint32_t domain;            /* domain id */
    uint16_t vlans[MAX_VLANS];  /* Mapped VLAN list: a vlan mapped to single domain */
} domain_vlan_map_t;

typedef struct vnid_subnet_s{
	/**
	 * \brief The IP Address being added
	 */
    uint32_t vnid;
	uint32_t IPv4;
	uint32_t IPv4_mask;
	uint8_t  subnet_mode; /* 1 - shared */
    uint32_t IPv4_nexthop;
} vnid_subnet_t;



struct domain_vlan_cfg {
    cmds_t   cmd;               /* CMD_SET_DOMAIN_VLAN (or) CMD_DEL_DOMAIN_VLAN */ 
    uint32_t domain;            /* domain id */
    uint16_t vlans[MAX_VLANS];  /* Mapped VLAN list: a vlan mapped to single domain */
}__attribute__((packed));
typedef struct domain_vlan_cfg domain_vlan_cfg_t;

struct broadcast_lst {
    cmds_t   cmd;               /* CMD_SET_DOMAIN_VLAN (or) CMD_DEL_DOMAIN_VLAN */ 
    uint32_t    domain_id;
    uint16_t    num_v4_switch;
    uint16_t    num_v6_switch;
    uint32_t    switch_list[MAX_BROADCAST_MEMBERS_PER_DOMAIN];
    uint32_t    source_ip_list[MAX_BROADCAST_MEMBERS_PER_DOMAIN];
    uint16_t    sport;
    uint16_t    dport;
}__attribute__((packed));
typedef struct broadcast_lst broadcast_lst_t;

struct ifmac {
    uint8_t mac[6];
    uint16_t vlan;
}__attribute__((packed));
typedef struct ifmac ifmac_t;

typedef struct ifmac_cfg {
    cmds_t  cmd;        /* CMD_ADD_IFMAC (or) CMD_DEL_IFMAC */
    ifmac_t ifmac;
} ifmac_cfg_t;

typedef struct ifipv4_cfg {
    cmds_t  cmd;        /* CMD_ADD_IPV4 (or) CMD_DEL_IPV4 */  
    uint32_t ifipv4;
    uint32_t mask;
    uint32_t nexthop;
    int vlan_id;
} ifipv4_cfg_t;


typedef struct extipv4_cfg {
    cmds_t      cmd;    /* CMD_ADD_EXTVIP (or) CMD_DEL_EXTVIP */
    extipv4_t   extip;
} extipv4_cfg_t;

typedef struct intipv4_cfg{
    cmds_t      cmd;    /* CMD_ADD_INTVIP (or) CMD_DEL_INTVIP */
    intipv4_t   intip;
} intipv4_cfg_t;

struct fwd_rule {
    uint32_t    fwdipv4; 
    uint8_t     fwdproto;
    uint16_t    fwdport;
    uint32_t    realipv4;
    uint16_t    realport;
    uint32_t    domain;
    uint32_t    pip_min;
    uint32_t    pip_max;
}__attribute__((packed));
typedef struct fwd_rule fwd_rule_t;

typedef struct fwd_rule_cfg {
    cmds_t  cmd;    /* CMD_ADD_FWD_RULE (or) 
                       CMD_SET_FWD_RULE (or) CMD_DEL_FWD_RULE */
    fwd_rule_t fwdrule;
} fwd_rule_cfg_t;

typedef struct ovl_location
{
    uint32_t    domain;
    uint32_t    dst_domain;
    uint32_t    endpoint_ip;
    uint32_t    endpoint_location;
    uint32_t    ovl_src_location;
    uint8_t     ovl_header_proto;
    uint16_t    ovl_src_port;
    uint16_t    ovl_dst_port;
    uint8_t     ovl_dst_mac[6];
    uint8_t     is_self; /* 1 = location is self */
} ovl_location_t;

typedef struct ovl_location_cfg
{
    cmds_t  cmd;    /* CMD_UPDATE_OVL_IP */
    uint16_t index;
    ovl_location_t location;
} ovl_location_cfg_t;


typedef struct ovl_tunnel_info
{
    uint32_t    dst_domain;
    uint32_t    endpoint_location;
} ovl_tunnel_info_t;

typedef struct ovl_tunnel_cfg
{
    cmds_t  cmd;    /* CMD_UPDATE_OVL_IP */
    uint8_t     num_entry;
    uint32_t    domain;
    uint32_t    endpoint_ip;
    uint8_t     ovl_dst_mac[6];
    ovl_tunnel_info_t location[MAX_TUNNEL_INFO];
} ovl_tunnel_cfg_t;

typedef struct dgwy_type_cfg
{
    cmds_t  cmd;    /* CMD_TYPE */
    dgwy_type_t dgtype;
}dgwy_type_cfg_t;

typedef struct dgwy_svc_mtu_cfg
{
    cmds_t  cmd;
    uint16_t            mtu;
}dgwy_svc_mtu_cfg_t;

typedef struct dgwy_service_config
{
    cmds_t          cmd;
    char            name[SVC_NAME];
    union {    
        dgwy_type_cfg_t     type;
        dgwy_svc_mtu_cfg_t  mtu;
        domain_cfg_t        domain_config;
        ifmac_cfg_t         ifmac_config;
        ifipv4_cfg_t        ipv4_config;
        extipv4_cfg_t       extipv4_config;
        intipv4_cfg_t       intipv4_config;
        fwd_rule_cfg_t      fwdrule_config;
        ovl_location_cfg_t  ovl_location;
        domain_vlan_cfg_t   dv_config;
        broadcast_lst_t     bcast_lst;
    };
} dgwy_service_config_t;

typedef struct dgwy_vlangw_ha_table
{
    uint32_t            peer_ipv4;
    uint8_t             state;
} dgwy_vlangw_ha_table_t;

typedef struct dgwy_ha_peer_table
{
    uint32_t            peer_ipv4;
    uint32_t            local_ipv4;
    uint8_t             state;
    int                 fdRead;
    int                 fdWrite;
    clock_t             last;
} dgwy_ha_peer_table_t;

typedef struct ext_mcast_vnid_s{
	/**
	 * \brief ext mcast vnid
	 */
	uint32_t ipv4;
	uint32_t vnid;
    uint32_t master; /* 1 Master */
}ext_mcast_vnid_t;



#define SVC_LOCK_INIT(lock) pthread_mutex_init(&(lock),0)
#define SVC_LOCK(lock) pthread_mutex_lock(&(lock))
#define SVC_UNLOCK(lock) pthread_mutex_unlock(&(lock))

#define SVC_ROUTE_LOCK_INIT(lock) pthread_mutex_init(&(lock),0)
#define SVC_ROUTE_LOCK(lock) pthread_mutex_lock(&(lock))
#define SVC_ROUTE_UNLOCK(lock) pthread_mutex_unlock(&(lock))

/* GLOBAL service list */
struct dgwy_service_list
{
    int                 index;
    char                name[SVC_NAME];
    char                caller_name[SVC_NAME];
    pthread_mutex_t     lock;
    uint8_t             state; /* 0 - Free
                                  1 - Disabled
                                  2 - Enabled */
    dgwy_type_t         type;
    uint16_t            mtu;
    uint32_t            dovenet_ipv4;
    uint32_t            mgmt_ipv4;
    uint32_t            dmc_ipv4;
    uint16_t            dmc_port;
    uint32_t            domainList[MAX_DOMIANS];
    ifmac_t             ifmacList[MAX_IF_MACS];
    ifipv4_t            ifipv4List[MAX_IFIPV4_ADDRESS];
    extipv4_t           extipv4List[MAX_EXTIPV4_ADDRESS];
    intipv4_t           intipv4List[MAX_EXTIPV4_ADDRESS];
    fwd_rule_t          fwdruleList[MAX_FWDRULE];
    domain_vlan_map_t   dvList[MAX_DVMAP];
    vnid_subnet_t       vnid_subnet[MAX_VNID_SUBNETS];
    ext_mcast_vnid_t    ext_mcast_vnids[MAX_EXT_VNIDS];
    extshared_vnids_t   extsharedVnidList[MAX_DOMIANS];
}__attribute__((packed));
typedef struct dgwy_service_list dgwy_service_list_t;

struct dgwy_dps_server
{
    uint32_t domain;
    uint32_t dpsIp;
    uint16_t port;
}__attribute__((packed));
typedef struct dgwy_dps_server dgwy_dps_server_t;

struct dgwy_service_cfg
{
    uint8_t gState;
    dgwy_dps_server_t dps;
    dgwy_service_list_t cfgList[MAX_SERVICE_TABLES];
}__attribute__((packed));
typedef struct dgwy_service_cfg dgwy_service_cfg_t;

/*
 ******************************************************************************
 * dgadmin_initialize                                                     *//**
 *
 * \brief - Initializes the DOVE Gateway Administrative Service
 *
 * \param[in] fExitOnCtrlC - Whether the Server Process should exit on
 *                           CTRL+C being pressed. In a development environment
 *                           this should be TRUE (1) while in a Production
 *                           Build this should be FALSE (0).
 * \param[in] fDebugCli - Whether the DebugCli should be started.
 *
 * \retval -1: Should never happen - This is an infinite loop.
 *
 ******************************************************************************
 */

int dgadmin_initialize(int fExitOnCtrlC, int fDebugCli);

/*
 ******************************************************************************
 * print_console --                                                       *//**
 *
 * \brief This routine prints the message to the console. This routine should
 *        be called to print messages to the console.
 *
 * \param output_string - The String To Print
 *
 * \remarks DO NOT log in this routine. Will cause infinite loop :)
 *
 *****************************************************************************/

void print_console(const char *output_string);




/*
 **************************************************************************
 * dgwy_svc_table_get                                                 *//**
 *
 * \brief - Check for existing svc table entry.
 *
 *
 * \param[in] service name 
 * \param[in] result [0 not found]
 *                   [1 busy]
 *                   [2 success]   
 *
 * \retval -(* dgwy_service_list_t)
 * \retval -[NULL] error/not-found 
 *
 **************************************************************************
 */
dgwy_service_list_t *dgwy_svc_table_get(char *svcname, int *result, const char * caller);


void fwdrule_pip_deregister(uint32_t ipmin, uint32_t ipmax, uint32_t domain);

#define VLAN_HA_MSG_LEN ((MAX_IF_MACS*6)+2)

#endif /* __DGADM_H_ */
