/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Source File:
 *      interface.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DPS Server
 *
 */


#include "dgadmin_generic_api.h"
#include "cli_interface.h"

extern uint8_t GLB_STATE;
/*
 ******************************************************************************
 * CLI (Service) Configuration Handling                                   *//**
 *
 * \ingroup DOVEGatewayCLIService
 * @{
 */

/**
 * \brief The callback function for CLI_CONFIG Codes
 */

typedef dove_status (*cli_service_callback_func)(cli_service_t *);

/**
 * \brief An Array of Callbacks for every CLI_CONFIG Code
 */
static cli_service_callback_func cli_callback_array[CLI_SERVICE_MAX];

/*
 ******************************************************************************
 * log_level                                                              *//**
 *
 * \brief - Changes the Log Level of the DOVE Gateway Utilities Module
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status log_level(cli_service_t *cli_service)
{
	ServiceUtilLogLevel = (int)cli_service->log_level.log_level;
	log_info(CliLogLevel, "ServiceUtilLogLevel set to %d", ServiceUtilLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * add_type                                                               *//**
 *
 * \brief - Adds service type
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_type(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_service_type(&cli_service->type_add);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

/*
 ******************************************************************************
 * set_type                                                               *//**
 *
 * \brief - Adds service type
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_type(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_service_type(&cli_service->type_add);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_add_type(char *name, dgwy_type_t type)
{
    cli_service_t svcp;

    memset(&svcp,0,sizeof(cli_service_t));
    memcpy(svcp.type_add.bridge_name, name, strlen(name));
    if(type == DGWY_TYPE_EXTERNAL)
    {
        memcpy(svcp.type_add.type,"EXT",3);
    }
    else if(type == DGWY_TYPE_VLAN)
    {
        memcpy(svcp.type_add.type,"VLAN",4);
    }
    else if(type == DGWY_TYPE_EXT_VLAN)
    {
        memcpy(svcp.type_add.type,"EXTVLAN",7);
    }
    set_type(&svcp);
}

/*
 ******************************************************************************
 * set_mtu                                                               *//**
 *
 * \brief - Set service mtu
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_mtu(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_service_mtu(&cli_service->svc_mtu);
    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_set_mtu(char *name, uint16_t mtu)
{
    cli_service_t svcp;

    memset(&svcp,0,sizeof(cli_service_t));

    memcpy(svcp.svc_mtu.bridge_name, name, strlen(name));
    svcp.svc_mtu.mtu = mtu;

    set_mtu(&svcp);

}

/*
 ******************************************************************************
 * set_start                                                              *//**
 *
 * \brief - Set start
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_start(cli_service_t *cli_service)
{
	//dove_status status = DOVE_STATUS_OK;
	//status = api_dgadmin_start_gateway();
	api_dgadmin_start_gateway();
	return DOVE_STATUS_OK;
}

void call_svc_set_start(cli_service_t *cli_service)
{
	set_start(cli_service);
}

/*
 ******************************************************************************
 * set_stop                                                               *//**
 *
 * \brief - Set service stop
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_stop(cli_service_t *cli_service)
{
	//dove_status status = DOVE_STATUS_OK;
	//status = api_dgadmin_stop_gateway();
	api_dgadmin_stop_gateway();
	return DOVE_STATUS_OK;
}

void call_svc_set_stop(cli_service_t *cli_service)
{
    set_stop(cli_service);
}

/*
 ******************************************************************************
 * show_all                                                               *//**
 *
 * \brief - show service  *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status show_all(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;

    show_svc_all();
	return status;
}


/*
 ******************************************************************************
 * show_ext_sessions                                                      *//**
 *
 * \brief - show service  *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status show_ext_sessions(cli_service_t *cli_service)
{
    ext_sesion_dump_t sessions;
    int_session_dump_t int_sessions;
    fwddyn_session_dump_t fwd_sessions;
	dove_status status = DOVE_STATUS_OK;
    api_show_all_ext_sessions(&sessions);
    api_show_all_int_sessions(&int_sessions);
    api_show_all_fwddyn_sessions(&fwd_sessions);
	return status;
}



/*
 ******************************************************************************
 * show_ovl_entry                                                              *//**
 *
 * \brief - SHow ovl entry
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status show_ovl_entry(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;

    show_ovl_all();
	return status;
}


/*
 ******************************************************************************
 * add_ifmac                                                              *//**
 *
 * \brief - Adds a MAC Interface
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_ifmac(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;

	log_info(CliLogLevel,
	         "Service:%s, MAC " MAC_FMT,
	         cli_service->ifmac_add.bridge_name,
	         MAC_OCTETS(cli_service->ifmac_add.mac));

    status = api_dgadmin_add_service_nic(&cli_service->ifmac_add);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

/*
 ******************************************************************************
 * add_ipv4                                                               *//**
 *
 * \brief - Adds a IPV4 Interface
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_ipv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_service_ipv4(&cli_service->addressv4_add);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_svc_addressv4(char *name , dgwy_service_list_t *svcp)
{
    cli_service_t clip;

    if(svcp)
    {
        int i=0;
                
        memset(&clip,0,sizeof(cli_service_t));
        memcpy(clip.addressv4_add.bridge_name, name, strlen(name));
        for(i=0; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].ipv4)
            {
                clip.addressv4_add.IPv4 = svcp->ifipv4List[i].ipv4;
                clip.addressv4_add.IPv4_netmask = svcp->ifipv4List[i].mask;
                clip.addressv4_add.IPv4_nexthop = svcp->ifipv4List[i].nexthop;
                add_ipv4(&clip);
            }
        }
    }
}

void call_svc_ifmacs(char *name, dgwy_service_list_t *svcp)
{
    cli_service_t clip;
    if(svcp)
    {
        int i=0;
        memset(&clip,0,sizeof(cli_service_t));
        memcpy(clip.ifmac_add.bridge_name, name, strlen(name));
        for(i=0; i<MAX_IF_MACS; i++)
        {
            if((svcp->ifmacList[i].mac[0]| svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]| svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]| svcp->ifmacList[i].mac[5]))
            {
                clip.ifmac_add.mac[0]=svcp->ifmacList[i].mac[0];
                clip.ifmac_add.mac[1]=svcp->ifmacList[i].mac[1];
                clip.ifmac_add.mac[2]=svcp->ifmacList[i].mac[2];
                clip.ifmac_add.mac[3]=svcp->ifmacList[i].mac[3];
                clip.ifmac_add.mac[4]=svcp->ifmacList[i].mac[4];
                clip.ifmac_add.mac[5]=svcp->ifmacList[i].mac[5];
                add_ifmac(&clip);
            }
        }
    }
}

/*
 ******************************************************************************
 * add_extvip                                                             *//**
 *
 * \brief - Adds a External Virtual IPV4 Address
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_extvip(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_service_pubipv4(&cli_service->extvip_add);
    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_svc_extipv4(char *name, dgwy_service_list_t *svcp)
{
    cli_service_t clip;
    if(svcp)
    {
        int i=0;

        memset(&clip,0,sizeof(cli_service_t));
        memcpy(clip.extvip_add.bridge_name, name, strlen(name));
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if((svcp->extipv4List[i].ipv4) &&
               (svcp->extipv4List[i].port_start))
            {
                clip.extvip_add.domain = svcp->extipv4List[i].domain;
                clip.extvip_add.vIPv4 = svcp->extipv4List[i].ipv4 ;
                clip.extvip_add.port_min = svcp->extipv4List[i].port_start;
                clip.extvip_add.port_max = svcp->extipv4List[i].port_end;
                add_extvip(&clip);
            }
        }
    }
}


/*
 ******************************************************************************
 * add_overlayip                                                          *//**
 *
 * \brief - Adds a Overlay IPV4 Address
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_overlayip(cli_service_t *cli_service)
{
	dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &cli_service->overlayvip_add.vIPv4, IP_string, INET_ADDRSTRLEN);
	log_info(CliLogLevel, "Bridge %s, IP %s, Ports %d-%d",
	         cli_service->overlayvip_add.bridge_name,
	         IP_string,
	         cli_service->overlayvip_add.port_min,
	         cli_service->overlayvip_add.port_max);

    memset(&svcp,0, sizeof(dgwy_service_config_t));
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * add_fwdrule                                                            *//**
 *
 * \brief - Adds a Forwarding Rule
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_fwdrule(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_service_fwdrule(&cli_service->fwdrule_add);
    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_svc_fwdrule(char *name, dgwy_service_list_t *svcp)
{
    cli_service_t clip;
    if(svcp)
    {
        int i=0;
        
        memset(&clip,0,sizeof(cli_service_t));
        memcpy(clip.fwdrule_add.bridge_name, name, strlen(name));
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if(svcp->fwdruleList[i].fwdipv4)
            {
                clip.fwdrule_add.IPv4 = svcp->fwdruleList[i].fwdipv4;
                clip.fwdrule_add.IPv4_map = svcp->fwdruleList[i].realipv4;
                clip.fwdrule_add.domain = svcp->fwdruleList[i].domain;
                clip.fwdrule_add.port = svcp->fwdruleList[i].fwdport;
                clip.fwdrule_add.port_map = svcp->fwdruleList[i].realport;
                clip.fwdrule_add.protocol = svcp->fwdruleList[i].fwdproto;
                add_fwdrule(&clip);
            }
        }
    }
}

/*
 ******************************************************************************
 * add_location                                                           *//**
 *
 * \brief - Adds a Location
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_location(cli_service_t *cli_service)
{
	char IP_location[INET6_ADDRSTRLEN];
	char IP_end_location[INET6_ADDRSTRLEN];
	char IP_end_src_location[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &cli_service->location_add.location_IP,
	          IP_location, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &cli_service->location_add.end_location_IP,
	          IP_end_location, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &cli_service->location_add.end_src_location_IP,
	          IP_end_src_location, INET_ADDRSTRLEN);
	log_info(CliLogLevel,
	         "Bridge %s, Domain %d, Location %s, "
	         "End Location %s, End Source Location %s",
	         cli_service->location_add.bridge_name,
	         cli_service->location_add.domain,
	         IP_location,
	         IP_end_location,
	         IP_end_src_location);
	log_info(CliLogLevel,
	         "Overlay Protocol %d, Overlay Source Port %d, "
	         "Overlay Destination Port %d, Overlay Destination MAC MAC " MAC_FMT,
	         cli_service->location_add.ovl_proto,
	         cli_service->location_add.ovl_src_port,
	         cli_service->location_add.ovl_dst_port,
	         MAC_OCTETS(cli_service->location_add.ovl_dst_mac));
	return DOVE_STATUS_OK;
}


/*
 ******************************************************************************
 * register_location                                                      *//**
 *
 * \brief - Register a Location
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status register_location(cli_service_t *cli_service)
{
	char IP_vm_location[INET6_ADDRSTRLEN];
	char IP_phy_location[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &cli_service->location_register.vm_location_IP,
	          IP_vm_location, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &cli_service->location_register.phy_location_IP,
	          IP_phy_location, INET_ADDRSTRLEN);

    log_info(CliLogLevel,
	         "Location register IP %s "
             "physical ip %s DVG %u MAC " MAC_FMT,
             IP_vm_location, IP_phy_location,
             cli_service->location_register.dvg,
	         MAC_OCTETS(cli_service->location_register.ovl_dst_mac));

    dgwy_register_vip_location(cli_service->location_register.domain,
                           ntohl(cli_service->location_register.vm_location_IP),
                           ntohl(cli_service->location_register.phy_location_IP),
                           cli_service->location_register.dvg,
                           (uint8_t*)cli_service->location_register.ovl_dst_mac);

	return DOVE_STATUS_OK;
}


/*
 ******************************************************************************
 * add_dps                                                                *//**
 *
 * \brief - Add DPS Server
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_dps(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_dps_server(&cli_service->dps_server);
    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


/*
 ******************************************************************************
 * dps_lookup                                                             *//**
 *
 * \brief - DPS Lookup
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dps_lookup(cli_service_t *cli_service)
{
	char IP_vm[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &cli_service->dps_lookup.vm_IP,
	          IP_vm, INET_ADDRSTRLEN);

    log_debug(CliLogLevel,
	         "DPS Lookup for IP %s[0x%x] domain %d",
             IP_vm, cli_service->dps_lookup.vm_IP,
             cli_service->dps_lookup.domain);

    dgwy_dps_cli_lookup(cli_service->dps_lookup.domain, 
                        cli_service->dps_lookup.vm_IP);

	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * cli_service_callback                                                 *//**
 *
 * \brief - The callback for CLI_SERVICE
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_service_callback(void *cli)
{
	cli_service_t *cli_service = (cli_service_t *)cli;
	cli_service_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;
    dove_status dv_status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter");

	if (cli_service->service_code < CLI_SERVICE_MAX)
	{
		func = cli_callback_array[cli_service->service_code];
		if (func)
		{
			status = func(cli_service);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

    if(status != DOVE_STATUS_OK)
    {
        dv_status = DOVE_STATUS_UNKNOWN;
    }

	return dv_status;

}



/*
 ******************************************************************************
 * set_dvlan                                                              *//**
 *
 * \brief - Adds a DOMAIN VLAN MAPPING
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_dvlan(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_domain_vlan(&cli_service->dvlan);
    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_svc_domain_vlans(char *name, dgwy_service_list_t *svcp)
{
    cli_service_t clip;
    if(svcp)
    {
        int i=0;

        memset(&clip,0,sizeof(cli_service_t));
        memcpy(clip.dvlan.bridge_name, name, strlen(name));
        for(i=0; i<MAX_DVMAP; i++)
        {
            
            if(svcp->dvList[i].domain!=INVALID_DOMAIN_ID)
            {
                int j=0;

                clip.dvlan.domain = svcp->dvList[i].domain;
                for(j=0; j<MAX_VLANS; j++)
                {
                    if(svcp->dvList[i].vlans[j])
                    {
                        clip.dvlan.vlan = svcp->dvList[i].vlans[j];
                        set_dvlan(&clip);
                    }
                }
            }
        }
    }
}


/*
 ******************************************************************************
 * save_tofile                                                            *//**
 *
 * \brief - Save conf to file
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status save_tofile(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
	int ret_val;

	log_info(CliLogLevel,
	         "Config %s  ",
	         cli_service->cfg_save.file_name);

    ret_val = dgwy_cfg_save_tofile(cli_service->cfg_save.file_name);

    if(ret_val < 0)
    {
        log_error(CliLogLevel, "Error: Save config [%s] to file failed",
                                cli_service->cfg_save.file_name);
    }
	return status;
}


/*
 ******************************************************************************
 * show_config                                                            *//**
 *
 * \brief - Save conf to file
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status show_config(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
	int ret_val;

	log_info(CliLogLevel,
	         "Config %s  ",
	         cli_service->cfg_save.file_name);

    ret_val = dgwy_show_cfg_file(cli_service->cfg_save.file_name);

    if(ret_val < 0)
    {
        log_error(CliLogLevel, "Error: Show config [%s] failed",
                                cli_service->cfg_save.file_name);
    }
	return status;
}



/*
 ******************************************************************************
 * save_startup                                                           *//**
 *
 * \brief - Save conf to file
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status save_startup(cli_service_t *cli_service)
{
	dove_status status = DOVE_STATUS_OK;
	return status;
}

/*
 ******************************************************************************
 * set_dove_net_ipv4                                                      *//**
 *
 * \brief - Set DOVE NET IPV4 
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_dove_net_ipv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_dove_net_ipv4(&cli_service->dove_net_ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

void call_set_dove_net_ipv4(char *name, uint32_t IPv4)
{
    cli_service_t svcp;

    memset(&svcp,0,sizeof(cli_service_t));

    memcpy(svcp.svc_mtu.bridge_name, name, strlen(name));
    svcp.dove_net_ipv4.IPv4=IPv4;

    set_dove_net_ipv4(&svcp);

}

/*
 ******************************************************************************
 * set_mgmt_dhcp                                                          *//**
 *
 * \brief - Set MGMT IPV4 
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_mgmt_dhcp(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    //strncpy(cli_service->mgmt_ipv4.bridge_name,"APBR",strlen("APBR"));
    cli_service->mgmt_ipv4.IPv4=0;
    cli_service->mgmt_ipv4.IPv4_mask=0;
    cli_service->mgmt_ipv4.IPv4_nexthop=0;

    status = api_dgadmin_set_mgmt_ipv4(&cli_service->mgmt_ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

/*
 ******************************************************************************
 * set_mgmt_ipv4                                                          *//**
 *
 * \brief - Set MGMT IPV4 
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_mgmt_ipv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;

    //strncpy(cli_service->mgmt_ipv4.bridge_name,"APBR",strlen("APBR"));
    status = api_dgadmin_set_mgmt_ipv4(&cli_service->mgmt_ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


void call_set_mgmt_ipv4(char *name, uint32_t IPv4)
{
    cli_service_t svcp;

    memset(&svcp,0,sizeof(cli_service_t));
    memcpy(svcp.svc_mtu.bridge_name, name, strlen(name));
    svcp.mgmt_ipv4.IPv4=IPv4;
    set_mgmt_ipv4(&svcp);
}


/*
 ******************************************************************************
 * set_peer_ipv4                                                          *//**
 *
 * \brief - Set PEER IPV4 
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_peer_ipv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_peer_ipv4(&cli_service->peer_ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

/*
 ******************************************************************************
 * set_dmc_ipv4                                                          *//**
 *
 * \brief - Set PEER IPV4 
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_dmc_ipv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_dmc_ipv4(&cli_service->dmc_ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


void call_set_dmc_ipv4(char *name, uint32_t IPv4,uint16_t port)
{
    cli_service_t svcp;

    memset(&svcp,0,sizeof(cli_service_t));
    svcp.dmc_ipv4.IPv4=IPv4;
    svcp.dmc_ipv4.port=port;
    set_dmc_ipv4(&svcp);
}


/*
 ******************************************************************************
 * set_ext_mcast_vnid                                                     *//**
 *
 * \brief - Set EXT MCAST VNID
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status set_ext_mcast_vnid(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_set_ext_mcast_vnid(cli_service->ext_mcast_vnid.vnid,
                                            cli_service->ext_mcast_vnid.ipv4);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}

static dove_status call_add_ext_mcast_vnid(dgwy_service_list_t *svcp)
{
    int i = 0;
    for(i=0; i<MAX_EXT_VNIDS; i++)
    {
        if(svcp->ext_mcast_vnids[i].vnid)
        {
            cli_service_t cli_service;
            cli_service.ext_mcast_vnid.vnid = svcp->ext_mcast_vnids[i].vnid;
            cli_service.ext_mcast_vnid.ipv4 = svcp->ext_mcast_vnids[i].ipv4;
            set_ext_mcast_vnid(&cli_service);
        }
    }
    return (DOVE_STATUS_OK);
}

 

/*
 ******************************************************************************
 * add_vnid_subnetv4                                                      *//**
 *
 * \brief - Add subnet  
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status add_vnid_subnetv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_add_vnid_subnetv4(&cli_service->vnid_subnet);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


/*
 ******************************************************************************
 * call_add_vnid_subnetv4                                                      *//**
 *
 * \brief - Add subnet  
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status call_add_vnid_subnetv4(dgwy_service_list_t *svcp)
{
    int i = 0;

    for(i=0; i<MAX_VNID_SUBNETS; i++)
    {
        if(svcp->vnid_subnet[i].IPv4)
        {
            cli_service_t cli_service;
            dove_status status = DOVE_STATUS_OK;
            
            cli_service.vnid_subnet.vnid=svcp->vnid_subnet[i].vnid;
            cli_service.vnid_subnet.IPv4=svcp->vnid_subnet[i].IPv4;
            cli_service.vnid_subnet.IPv4_mask=svcp->vnid_subnet[i].IPv4_mask;
            cli_service.vnid_subnet.subnet_mode=svcp->vnid_subnet[i].subnet_mode;
            cli_service.vnid_subnet.IPv4_nexthop=svcp->vnid_subnet[i].IPv4_nexthop;
            
            status = add_vnid_subnetv4(&cli_service);
            
            if(status != DOVE_STATUS_OK)
            {
                log_error(ServiceUtilLogLevel,
                          "Error add vnid subnet ");
            }
        }
    }
    return (DOVE_STATUS_OK);
}


/*
 ******************************************************************************
 * del_vnid_subnetv4                                                      *//**
 *
 * \brief - Add subnet  
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status del_vnid_subnetv4(cli_service_t *cli_service)
{
    dove_status status = DOVE_STATUS_OK;
    status = api_dgadmin_del_vnid_subnetv4(&cli_service->vnid_subnet);

    if(status != DOVE_STATUS_OK)
    {
        return (DOVE_STATUS_UNKNOWN);
    }
    else
    {
        return (DOVE_STATUS_OK);
    }
}


#if 0
/*
 ******************************************************************************
 * call_del_vnid_subnetv4                                                      *//**
 *
 * \brief - Add subnet  
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status call_del_vnid_subnetv4(dgwy_service_list_t *svcp)
{
    int i = 0;

    for(i=0; i<MAX_VNID_SUBNETS; i++)
    {
        if(svcp->vnid_subnet[i].IPv4)
        {
            cli_service_t cli_service;
            dove_status status = DOVE_STATUS_OK;
            
            cli_service.vnid_subnet.vnid=svcp->vnid_subnet[i].vnid;
            cli_service.vnid_subnet.IPv4=svcp->vnid_subnet[i].IPv4;
            cli_service.vnid_subnet.IPv4_mask=svcp->vnid_subnet[i].IPv4_mask;
            cli_service.vnid_subnet.subnet_mode=svcp->vnid_subnet[i].subnet_mode;
            cli_service.vnid_subnet.IPv4_nexthop=svcp->vnid_subnet[i].IPv4_nexthop;
            
            status = add_vnid_subnetv4(&cli_service);
            
            if(status != DOVE_STATUS_OK)
            {
                log_error(ServiceUtilLogLevel,
                          "Error add vnid subnet ");
            }
        }
    }
    return (DOVE_STATUS_OK);
}

#endif

dove_status gdb_load_config(char *fname)
{
    int ret_val=0;
    int i=0;
	dove_status status = DOVE_STATUS_OK;
    dgwy_service_cfg_t *bufCfg = (dgwy_service_cfg_t*)
                                    malloc(sizeof(dgwy_service_cfg_t));
    if(!bufCfg)
    {
        log_error(ServiceUtilLogLevel,"Error Get config from file");
        return status;
    }
    
    ret_val = dgwy_get_cfg_from_file(fname, 
                                     bufCfg);
    if(ret_val<0)
    {
        log_error(ServiceUtilLogLevel,"Error Get config from file");
        free(bufCfg);
        return status;
    }

    reset_global_svctable();

    GLB_STATE = bufCfg->gState;

    dgwy_add_dps_server(bufCfg->dps.domain,
                        bufCfg->dps.dpsIp,
                        bufCfg->dps.port);

    for(i=0; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(bufCfg->cfgList[i].name);
        if(len)
        {
            call_add_type(bufCfg->cfgList[i].name, bufCfg->cfgList[i].type);
            call_svc_addressv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_ifmacs(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_extipv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_fwdrule(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_domain_vlans(bufCfg->cfgList[i].name,
                                  &bufCfg->cfgList[i]);

            /*
             * TODO
            call_svc_intipv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            */
            call_set_mtu(bufCfg->cfgList[i].name,bufCfg->cfgList[i].mtu);
            call_set_dove_net_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].dovenet_ipv4);
            call_set_mgmt_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].mgmt_ipv4);
            call_set_dmc_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].dmc_ipv4,
                              bufCfg->cfgList[i].dmc_port);
            call_add_vnid_subnetv4(&bufCfg->cfgList[i]);
            call_add_ext_mcast_vnid(&bufCfg->cfgList[i]);

            if(GLB_STATE)
            {
                cli_service_t clip;
                call_svc_set_start(&clip);
            }
            else
            {
                cli_service_t clip;
                call_svc_set_stop(&clip);
            }
        }
    }
        
    free(bufCfg);
	return status;
}

/*
 ******************************************************************************
 * load_config                                                            *//**
 *
 * \brief - Load conf to running
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status load_config(cli_service_t *cli_service)
{
    int ret_val=0;
    int i=0;
	dove_status status = DOVE_STATUS_OK;
    dgwy_service_cfg_t *bufCfg = (dgwy_service_cfg_t*)
                                    malloc(sizeof(dgwy_service_cfg_t));
    if(!bufCfg)
    {
        log_error(ServiceUtilLogLevel,"Error Get config from file");
        return status;
    }
    
    ret_val = dgwy_get_cfg_from_file(cli_service->cfg_save.file_name, 
                                     bufCfg);
    if(ret_val<0)
    {
        log_error(ServiceUtilLogLevel,"Error Get config from file");
        free(bufCfg);
        return status;
    }

    reset_global_svctable();

    GLB_STATE = bufCfg->gState;

    dgwy_add_dps_server(bufCfg->dps.domain,
                        bufCfg->dps.dpsIp,
                        bufCfg->dps.port);

    for(i=0; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(bufCfg->cfgList[i].name);
        if(len)
        {
            call_add_type(bufCfg->cfgList[i].name, bufCfg->cfgList[i].type);
            call_svc_addressv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_ifmacs(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_extipv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_fwdrule(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            call_svc_domain_vlans(bufCfg->cfgList[i].name,
                                  &bufCfg->cfgList[i]);

            /*
             * TODO
            call_svc_intipv4(bufCfg->cfgList[i].name, &bufCfg->cfgList[i]);
            */
            call_set_mtu(bufCfg->cfgList[i].name,bufCfg->cfgList[i].mtu);
            call_set_dove_net_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].dovenet_ipv4);
            call_set_mgmt_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].mgmt_ipv4);
            call_set_dmc_ipv4(bufCfg->cfgList[i].name,bufCfg->cfgList[i].dmc_ipv4,
                              bufCfg->cfgList[i].dmc_port);
            call_add_vnid_subnetv4(&bufCfg->cfgList[i]);
            call_add_ext_mcast_vnid(&bufCfg->cfgList[i]);

            if(GLB_STATE)
            {
                cli_service_t clip;
                call_svc_set_start(&clip);
            }
            else
            {
                cli_service_t clip;
                call_svc_set_stop(&clip);
            }
        }
    }
        
    free(bufCfg);
	return status;
}


/*
 ******************************************************************************
 * cli_service_init                                                       *//**
 *
 * \brief - Initializes the DOVE Gateway CLI_SERVICE related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_service_init(void)
{

	log_debug(CliLogLevel, "Enter");

    dove_status dv_status = DOVE_STATUS_OK;

	// Initialize the CLI_MAIN_MENU callbacks here

	cli_callback_array[CLI_SERVICE_LOG_LEVEL] = log_level;
	cli_callback_array[CLI_SERVICE_ADD_TYPE] = add_type;
	cli_callback_array[CLI_SERVICE_ADD_IFMAC] = add_ifmac;
	cli_callback_array[CLI_SERVICE_ADD_IPV4] = add_ipv4;
	cli_callback_array[CLI_SERVICE_ADD_EXTVIP] = add_extvip;
	cli_callback_array[CLI_SERVICE_ADD_OVERLAYVIP] = add_overlayip;
	cli_callback_array[CLI_SERVICE_ADD_FWRULE] = add_fwdrule;
	cli_callback_array[CLI_SERVICE_ADD_LOCATION] = add_location;
	cli_callback_array[CLI_SERVICE_REGISTER_LOCATION] = register_location;
	cli_callback_array[CLI_SERVICE_ADD_DPS] = add_dps;
	cli_callback_array[CLI_SERVICE_DPS_LOOKUP] = dps_lookup;
	cli_callback_array[CLI_SERVICE_SET_MTU] = set_mtu;
	cli_callback_array[CLI_SERVICE_SET_START] = set_start;
	cli_callback_array[CLI_SERVICE_SET_STOP] = set_stop;
	cli_callback_array[CLI_SERVICE_DVLAN] = set_dvlan;
	cli_callback_array[CLI_SERVICE_SHOW_ALL] = show_all;
	cli_callback_array[CLI_SERVICE_SAVE_TOFILE] = save_tofile;
	cli_callback_array[CLI_SERVICE_SHOW_CONFIG] = show_config;
	cli_callback_array[CLI_SERVICE_SAVE_STARTUP] = save_startup;
	cli_callback_array[CLI_SERVICE_LOAD_CONFIG] = load_config;
	cli_callback_array[CLI_SERVICE_SET_TYPE] = set_type;
	cli_callback_array[CLI_DOVE_NET_IPV4] = set_dove_net_ipv4;
	cli_callback_array[CLI_MGMT_IPV4] = set_mgmt_ipv4;
	cli_callback_array[CLI_PEER_IPV4] = set_peer_ipv4;
	cli_callback_array[CLI_DMC_IPV4] = set_dmc_ipv4;
	cli_callback_array[CLI_ADD_VNID_SUBNET] = add_vnid_subnetv4;
	cli_callback_array[CLI_DEL_VNID_SUBNET] = del_vnid_subnetv4;
	cli_callback_array[CLI_SHOW_OVL_SYS] = show_ovl_entry;
	cli_callback_array[CLI_EXT_MCAST_VNID] = set_ext_mcast_vnid;
	cli_callback_array[CLI_SHOW_EXT_SESSIONS] = show_ext_sessions;
	cli_callback_array[CLI_MGMT_DHCP] = set_mgmt_dhcp;

	log_debug(CliLogLevel, "Exit");

	return dv_status;
}
/** @} */

