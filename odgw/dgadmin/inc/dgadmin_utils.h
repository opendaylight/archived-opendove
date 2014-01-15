/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef _DGADM_UTILS_H_
#define _DGADM_UTILS_H_
#include "include.h"

/*
 ******************************************************************************
 * DOVEGatewayUtils                                                       *//**
 *
 * \addtogroup DOVEGateway
 * @{
 * \defgroup DOVEGatewayUtils DOVE Gateway Utilities
 * @{
 *
 * This module describes the DOVE Gateway Utilities.
 */

#define SYSFS_CLASS_NET "/sys/class/net/"
#define SYSFS_PATH_MAX 256
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_OCTETS(_mac)					\
	(_mac)[0], (_mac)[1], (_mac)[2], (_mac)[3], (_mac)[4], (_mac)[5]

extern int ServiceUtilLogLevel;

/*
 ******************************************************************************
 * chk_start_svc                                                          *//**
 *
 * \brief - Checks if service has been started
 *
 * \param[in] name The Name of the Bridge
 *
 * \retval 0 Bridge exists
 * \retval Non-Zero Bridge does not exist
 *
 ******************************************************************************
 */
int chk_start_svc(char *name);
int chk_stop_svc(char *name);

/*
 ******************************************************************************
 * chk_mac_add_bridge                                                     *//**
 *
 * \brief - Add a mac address to the bridge
 *
 * \param[in] brname The Name of the Bridge
 *
 * \retval 0 Bridge exists
 * \retval Non-Zero Bridge does not exist
 *
 ******************************************************************************
 */
int chk_mac_add_bridge(char *brname, uint8_t *hwaddr);
int chk_mac_rem_bridge(char *brname, uint8_t *hwaddr);

/*
 ******************************************************************************
 * dgwy_ctrl_svc_macs                                                     *//**
 *
 * \brief - Send Control Message to the DOVE Gateway Kernel Module to Add
 *          MAC to the interface
 *
 * \param[in] svcp Structure of type dgwy_service_config_t
 *
 * \retval -1
 *
 ******************************************************************************
 */
int dgwy_ctrl_svc_macs(dgwy_service_config_t *svcp);

/*
 ******************************************************************************
 * dgwy_ctrl_set_svc_type                                                 *//**
 *
 * \brief - Send Control Message to the DOVE Gateway Kernel Module to Add
 *          service with given type
 *
 * \param[in] svc command
 * \param[in] svc name 
 * \param[in] svc type
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_ctrl_set_svc_type(uint8_t cmd, char *name, dgwy_type_t svc_type);


/*
 ******************************************************************************
 * dgwy_register_vip_location                                                 *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 *
 * \param[in] domain
 * \param[in] vm IP 
 * \param[in] physical IP
 * \param[in] DVG
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_register_vip_location(uint32_t domain, uint32_t vIP, uint32_t pIP, uint16_t dvg, uint8_t *mac);


/*
 ******************************************************************************
 * dgwy_add_dps_server                                                 *//**
 *
 * \brief -Add DPS server.
 *
 *
 * \param[in] IP 
 * \param[in] port
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_add_dps_server(uint32_t domain, uint32_t dpsIp, uint16_t port);
int dgwy_del_dps_server(uint32_t dpsIp);

/*
 ***************************************************************************
 * dgwy_dps_cli_lookup                                                 *//**
 *
 * \brief - DPS lookup.
 *
 *
 * \param[in] domain 
 * \param[in] ip
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_dps_cli_lookup(uint32_t domain, uint32_t vmIp);

/*
 ***************************************************************************
 * dgwy_ctrl_svc_ipv4                                                  *//**
 *
 * \brief - Manage Service IP.
 *
 *
 * \param[in] dgwy_service_config_t *  
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_svc_ipv4(dgwy_service_config_t *svcp, int flags);

/*
 ***************************************************************************
 * chk_add_svc_ipv4                                                    *//**
 *
 * \brief - Manage Service IP.
 *
 *
 * \param[in] service name  
 * \param[in] service ipv4 address  
 * \param[in] service ipv4 address netmask 
 * \param[in] service nexthop address  
 * \param[in] withroute [ 1 - insert route ]  
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int chk_add_svc_ipv4(char *name, char *svc_ifaddr, 
                     char *svc_addr_mask, char *svc_addr_nexthop, int withroute);
int chk_add_svc_ipv4_with_vlan(char *name, char *svc_ifaddr, 
                               char *svc_addr_mask, char *svc_addr_nexthop, 
                               int withroute, int vlan_id);
int chk_rem_svc_ipv4(char *name, char *svc_ifaddr);
int chk_rem_svc_ipv4_with_vlan(char *name, char *svc_ifaddr, int vlan_id);


/*
 ***************************************************************************
 * is_ipv4_on                                                          *//**
 *
 * \brief - Check IP(v4) Enabled.
 *
 *
 * \param[in] service name  
 * \param[in] ipv4 address  
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int is_ipv4_on(char *name, char *svc_ifaddr);
int is_ipv4_on_vlan(char *name, char *svc_ifaddr, int vlan_id);



/*
 ***************************************************************************
 * check_add_pubipv4_alias                                             *//**
 *
 * \brief - Check IP(v4) in subnet enabled then add given as alias 
 *
 *
 * \param[in] service name  
 * \param[in] ipv4 address  
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int check_add_pubipv4_alias(char *name, char *svc_ifaddr);
int check_add_pubipv4_alias_with_vlan(char *name, char *svc_ifaddr);
int check_rem_pubipv4_alias(char *name, char *svc_ifaddr);
int check_rem_pubipv4_alias_with_vlan(char *name, char *svc_ifaddr, int vlan_id);


/*
 ***************************************************************************
 * dgwy_ctrl_svc_extipv4                                               *//**
 *
 * \brief - Manage EXTERNAL VIPS (IPv4s).
 *
 *
 * \param[in] dgwy_service_config_t *  
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_svc_extipv4(dgwy_service_config_t *svcp);

/*
 ***************************************************************************
 * dgwy_ctrl_fwd_rule                                                  *//**
 *
 * \brief - Manage PORT Forward Rule for svc hosting.
 *
 *
 * \param[in] dgwy_service_config_t *  
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_fwd_rule(dgwy_service_config_t *svcp);


/*
 ***************************************************************************
 * dgwy_ctrl_set_svc_mtu                                               *//**
 *
 * \brief - Manage service MTU.
 *
 *
 * \param[in] cmd : CMD_MTU
 * \param[in] svc name
 * \param[in] svc MTU
 *
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_svc_mtu(uint8_t cmd, char *name, uint16_t mtu);

/*
 ***************************************************************************
 * dgwy_ctrl_set_mgmt_ipv4                                             *//**
 *
 * \brief - MGMT IPV4
 *
 * \param[in] MGMT ipv4
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_mgmt_ipv4(uint32_t IPV4, uint32_t mask, uint32_t nexthop);

/*
 ***************************************************************************
 * dgwy_ctrl_set_dovenet_ipv4                                          *//**
 *
 * \brief - DOVE NET IPV4
 *
 * \param[in] dove net ipv4
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_dovenet_ipv4(char *name, uint32_t IPV4, uint32_t nexthop);

/*
 ***************************************************************************
 * dgwy_ctrl_set_external_ipv4                                         *//**
 *
 * \brief - EXTERNAL INTERFACE IPV4
 *
 * \param[in] ext net ipv4
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_external_ipv4(char *name, uint32_t IPV4, uint32_t nexthop);

/*
 ***************************************************************************
 * dgwy_ctrl_set_dmc_ipv4                                             *//**
 *
 * \brief - DMC IPV4
 *
 * \param[in] DMC ipv4
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_dmc_ipv4(uint32_t IPV4, uint16_t port);


/*
 ***************************************************************************
 * dgwy_ctrl_set_global                                               *//**
 *
 * \brief - Manage service .
 *
 *
 * \param[in] cmd : CMD_START (or) CMD_STOP
 *
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_global(uint8_t cmd);

/*
 ***************************************************************************
 * dgwy_ctrl_svc_domain_vlans                                          *//**
 *
 * \brief - Manage service .
 *
 *
 * \param[in] (dgwy_service_config_t *) 
 *
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_svc_domain_vlans(dgwy_service_config_t *svcp);

/*
 ***************************************************************************
 * dgwy_ctrl_set_peer_ipv4                                             *//**
 *
 * \brief - PEER IPV4
 *
 * \param[in] PEER ipv4
 *
 * \retval -[ 0] success
 * \retval -[-1] error
 *
 **************************************************************************
 */
int dgwy_ctrl_set_peer_ipv4(char *name, uint32_t IPV4);

int dgwy_ctrl_add_vnid_info(uint32_t vnid, 
                            uint32_t IPv4,
                            uint32_t IPv4_mask,
                            uint32_t IPv4_nexthop,
                            uint8_t state);

int dgwy_ctrl_del_vnid_info(uint32_t vnid, 
                            uint32_t IPv4,
                            uint32_t IPv4_mask,
                            uint32_t IPv4_nexthop,
                            uint8_t state);

int dgwy_ctrl_extshared_vnids(int vnid, int domain, 
                              int extmcast, cmds_t cmd);

int dgwy_ctrl_reset_stats(void);


/****************************************************************************
 * show_svc_all
 **************************************************************************
 */
void show_svc_all(void);
int show_ovl_all(void);
int dgwy_cfg_save_tofile(char *cfgname);
int dgwy_show_cfg_file(char *cfgname);
int dgwy_get_cfg_from_file(char *cfgname, dgwy_service_cfg_t *cfg);
void reset_global_svctable(void);

/*
 ******************************************************************************
 * set_noicmp_redirect                                                    *//**
 *
 * \brief - Set service interface icmp no redirect
 *
 * \param[in] name The Name of the Bridge
 *
 * \retval none
 *
 ******************************************************************************
 */
void set_noicmp_redirect(char *name);


int dgwy_ctrl_set_ext_mcast_vnid(uint32_t ext_mcast_vnid, uint32_t ipv4);
int dgwy_ctrl_unset_ext_mcast_vnid(uint32_t ext_mcast_vnid, uint32_t ipv4);

int show_all_ext_sessions(ext_sesion_dump_t *sessions);
int show_all_int_sessions(int_session_dump_t *sessions);
int show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions);
int show_all_vnid_stats(dgwy_vnid_stats_t *stats);
int show_all_stats(dgwy_stats_t *stats);
int dgw_ctrl_set_ovl_port(int port);

int get_vlan_of_pubalias(char *name, char *svc_ifaddr);

/** @} */
/** @} */

#endif /* _DGADM_UTILS_H_ */
