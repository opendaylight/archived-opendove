/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef __DGWY_GENERIC_API_H__
#define __DGWY_GENERIC_API_H__
#include "include.h"
#include "main_menu.h"
#include "service.h"


/*
 ******************************************************************************
 * test_generic_api                                                       *//**
 *                                                                              
 * \brief - Test  API 
 * 
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status test_generic_api(void);


/*
 ******************************************************************************
 * api_dgadmin_add_service_type                                          *//**
 *
 * \brief - Adds Gateway Service With Given Type [ External / VLAN ]
 *
 * \param[in] cli_service_type_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_type(cli_service_type_t *type_add);


/*
 ******************************************************************************
 * api_dgadmin_set_service_type                                          *//**
 *
 * \brief - Adds Gateway Service With Given Type [ External / VLAN ]
 *
 * \param[in] cli_service_type_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_service_type(cli_service_type_t *type_set);



/*
 ******************************************************************************
 * api_dgadmin_del_service_type                                          *//**
 *
 * \brief - Delete Gateway Service With Given Name
 *
 * \param[in] cli_service_type_t
 * 
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_type(cli_service_type_t *type_rem);

/*
 ******************************************************************************
 * api_dgadmin_add_service_nic                                            *//**
 *
 * \brief - Add NIC (port) to Gateway Service 
 *
 * \param[in] cli_service_ifmac_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_nic(cli_service_ifmac_t *ifmac_add);

/*
 ******************************************************************************
 * api_dgadmin_del_service_nic                                            *//**
 *
 * \brief - Delete NIC (port) from Gateway Service 
 *
 * \param[in] cli_service_ifmac_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_nic(cli_service_ifmac_t *ifmac_rem);

/*
 ******************************************************************************
 * api_dgadmin_add_service_ipv4                                           *//**
 *
 * \brief - Add IPV4 Address to Gateway Service 
 *
 * \param[in] cli_service_addressv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_ipv4(cli_service_addressv4_t *ipv4_add);
dove_status api_dgadmin_add_dovenet_svc_ipv4(cli_service_addressv4_t *ipv4_add);

/*
 ******************************************************************************
 * api_dgadmin_del_service_ipv4                                           *//**
 *
 * \brief - Delete IPV4 Address From Gateway Service 
 *
 * \param[in] cli_service_addressv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_ipv4(cli_service_addressv4_t *ipv4_rem);

/*
 ******************************************************************************
 * api_dgadmin_add_service_pubipv4                                        *//**
 *
 * \brief - Add Domain Public IPV4 Address to Gateway Service 
 *
 * \param[in] cli_service_extvip_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_pubipv4(cli_service_extvip_t *extvipv4_add);

/*
 ******************************************************************************
 * api_dgadmin_del_service_pubipv4                                        *//**
 *
 * \brief - Delete Domain Public IPV4 Address From Gateway Service 
 *
 * \param[in] cli_service_extvip_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_pubipv4(cli_service_extvip_t *extvipv4_rem);

/*
 ******************************************************************************
 * api_dgadmin_add_service_overlayipv4                                        *//**
 *
 * \brief - Add Domain Overlay IPV4 Address to Gateway Service 
 *
 * \param[in] cli_service_extvip_t
 *
 * \return dgadmin_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_overlayipv4(cli_service_overlayip_t *overlayipv4_add);

/*
 ******************************************************************************
 * api_dgadmin_del_service_overlayipv4                                        *//**
 *
 * \brief - Delete Domain Overlay IPV4 Address From Gateway Service 
 *
 * \param[in] cli_service_extvip_t
 *
 * \return dgadmin_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_overlayipv4(cli_service_overlayip_t *overlayipv4_rem);

/*
 ******************************************************************************
 * api_dgadmin_add_service_fwdrule                                        *//**
 *
 * \brief - Add Gateway Service Forward Rule
 *
 * \param[in] cli_service_fwdrule_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_fwdrule(cli_service_fwdrule_t *fwdrl_add);

/*
 ******************************************************************************
 * api_dgadmin_del_service_fwdrule                                        *//**
 *
 * \brief - Delete Gateway Service Forward Rule
 *
 * \param[in] cli_service_fwdrule_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_service_fwdrule(cli_service_fwdrule_t *fwdrl_rem);

/*
 ******************************************************************************
 * api_dgadmin_set_service_mtu                                            *//**
 *
 * \brief - Set Gateway Service MTU
 *
 * \param[in] cli_service_mtu_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_service_mtu(cli_service_mtu_t *svc_mtu);

/*
 ******************************************************************************
 * api_dgadmin_add_domain_vlan                                            *//**
 *
 * \brief - Add Gateway Service Domain VLAN Mapping
 *
 * \param[in] cli_service_dvlan_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_domain_vlan(cli_service_dvlan_t *dvlan_add);

/*
 ******************************************************************************
 * api_dgadmin_del_domain_vlan                                            *//**
 *
 * \brief - Remove Gateway Service Domain VLAN Mapping
 *
 * \param[in] cli_service_dvlan_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_domain_vlan(cli_service_dvlan_t *dvlan_add);

/*
 ******************************************************************************
 * api_dgadmin_add_dps_server                                             *//**
 *
 * \brief - Add DPS Server
 *
 * \param[in] cli_dps_server_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_dps_server(cli_dps_server_t *dps);

/*
 ******************************************************************************
 * api_dgadmin_del_dps_server                                             *//**
 *
 * \brief - Delete DPS Server
 *
 * \param[in] cli_dps_server_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_dps_server(cli_dps_server_t *dps);

/*
 ******************************************************************************
 * api_dgadmin_start_gateway                                              *//**
 *
 * \brief - Start Gateway Packet Processing
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_start_gateway(void);

/*
 ******************************************************************************
 * api_dgadmin_stop_gateway                                              *//**
 *
 * \brief - STOP Gateway Packet Processing
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_stop_gateway(void);

/*
 ******************************************************************************
 * api_dgadmin_set_dove_net_ipv4                                          *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] cli_dove_net_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_dove_net_ipv4(cli_dove_net_ipv4_t *dove_net_ipv4);

/*
 ******************************************************************************
 * api_dgadmin_set_ext_net_ipv4                                          *//**
 *
 * \brief - Set Gateway EXT NET IPV4
 *
 * \param[in] cli_dove_net_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_ext_net_ipv4(cli_dove_net_ipv4_t *dove_net_ipv4);


/*
 ******************************************************************************
 * api_dgadmin_set_mgmt_ipv4                                              *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] cli_mgmt_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_mgmt_ipv4(cli_mgmt_ipv4_t *mgmt_ipv4);

/*
 ******************************************************************************
 * api_dgadmin_set_dove_net_ipv4                                          *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] cli_dove_net_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_dove_net_ipv4(cli_dove_net_ipv4_t *dove_net_ipv4);

/*
 ******************************************************************************
 * api_dgadmin_set_mgmt_ipv4                                              *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] cli_mgmt_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_mgmt_ipv4(cli_mgmt_ipv4_t *mgmt_ipv4);


/*
 ******************************************************************************
 * api_dgadmin_set_peer_ipv4                                              *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] cli_peer_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_peer_ipv4(cli_peer_ipv4_t *peer_ipv4);


/*
 ******************************************************************************
 * api_dgadmin_set_dmc_ipv4                                              *//**
 *
 * \brief - Set Gateway DMC IPV4
 *
 * \param[in] cli_dmc_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_dmc_ipv4(cli_dmc_ipv4_t *dmc_ipv4);

/*
 ******************************************************************************
 * api_dgadmin_add_vnid_subnetv4                                          *//**
 *
 * \brief - Add IPV4 Subnet to Gateway Service 
 *
 * \param[in] cli_vnid_subnet_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_vnid_subnetv4(cli_vnid_subnet_t *vnid_subnet);

/*
 ******************************************************************************
 * api_dgadmin_add_vnid_subnetv4                                          *//**
 *
 * \brief - Set IPV4 Subnet to Gateway Service 
 *
 * \param[in] cli_vnid_subnet_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_vnid_subnetv4(cli_vnid_subnet_t *vnid_subnet);

/*
 ******************************************************************************
 * api_dgadmin_del_vnid_subnetv4                                          *//**
 *
 * \brief - Set IPV4 Subnet to Gateway Service 
 *
 * \param[in] cli_vnid_subnet_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_del_vnid_subnetv4(cli_vnid_subnet_t *vnid_subnet);

dove_status api_dgadmin_set_ext_mcast_vnid(uint32_t ext_mcast_vnid, uint32_t ipv4);

dove_status api_dgadmin_add_extshared_vnids(int vnid, int domain, int extmcast);
dove_status api_dgadmin_del_extshared_vnids(int vnid);

dove_status api_show_all_ext_sessions(ext_sesion_dump_t *sessions);
dove_status api_show_all_int_sessions(int_session_dump_t *sessions);
dove_status api_show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions);
dove_status api_show_all_vnid_stats(dgwy_vnid_stats_t *stats);
dove_status api_show_all_stats(dgwy_stats_t *stats);
dove_status api_set_ovl_port(int port);

/*
 ******************************************************************************
 * api_dgadmin_set_ext_mcast_vnid                                         *//**
 *
 * \brief - Set Gateway DOVE NET IPV4
 *
 * \param[in] ext_mcast_vnid
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                           uint32_t ipv4);


dove_status api_dgadmin_del_vnid(int vnid, int domainid);
dove_status api_dgadmin_del_extmcastvnid(int vnid, int domainid);
dove_status dgwy_api_ctrl_reset_stats(void);

#endif /* __DGWY_GENERIC_API_H__ */
