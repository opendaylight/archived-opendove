/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#include "dgadmin_generic_api.h"
#include "cli_interface.h"
#include <ifaddrs.h>

extern pthread_mutex_t GLB_ROUTE_LOCK; /* used to serialize route operations */
extern void check_dregister_tunnel(uint32_t vnid);
extern uint8_t g_APPBRIDGE_type;
extern  dgwy_service_list_t GLB_SVC_TABLE[MAX_SERVICE_TABLES];

extern int update_dmc(uint32_t ip, int port);

dove_status api_dgadmin_unset_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                             uint32_t ipv4);

dove_status test_generic_api(void)
{
    dove_status status = DOVE_STATUS_OK;
    return status;
}

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
dove_status api_dgadmin_add_service_type(cli_service_type_t *type_add)
{
    dgwy_service_config_t svcp;
    dove_status status = DOVE_STATUS_OK;
    int ret_val;

    memset(&svcp,0,sizeof(dgwy_service_config_t));
    log_debug(CliLogLevel,
             "Service:%s, Type:%s ",
             type_add->bridge_name,
             type_add->type);
    do
    {
        /* Set the command type*/
        svcp.cmd = CMD_ADD_SVC;
        /* Copy the bridge name */
        memcpy(svcp.name, type_add->bridge_name,
               (strlen(type_add->bridge_name)<SVC_NAME)?
               (strlen(type_add->bridge_name)):(SVC_NAME-1));

        /* Make sure the bridge is ready */
        if (chk_start_svc(type_add->bridge_name) == 0)
        {
            /* No service */
            log_error(ServiceUtilLogLevel,
                        "Error: Failed to set service %s\n", svcp.name);
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
            break;
        }
        else
        {
            set_noicmp_redirect(type_add->bridge_name);
        }

        svcp.type.cmd = CMD_TYPE;
        if(!(strncmp(type_add->type, "EXTVLAN", 7)))
        {
            svcp.type.dgtype = DGWY_TYPE_EXT_VLAN;
            g_APPBRIDGE_type = DGWY_TYPE_EXT_VLAN;
        }
        else if(!(strncmp(type_add->type, "VLAN", 4)))
        {
            svcp.type.dgtype = DGWY_TYPE_VLAN;
            g_APPBRIDGE_type = DGWY_TYPE_VLAN;
        }
        else if(!(strncmp(type_add->type, "EXT", 3)))
        {
            svcp.type.dgtype = DGWY_TYPE_EXTERNAL;
            g_APPBRIDGE_type = DGWY_TYPE_EXTERNAL;
        }
        else
        {
            svcp.type.dgtype = DGWY_TYPE_NONE;
            g_APPBRIDGE_type = DGWY_TYPE_NONE;

            /*
            log_error(ServiceUtilLogLevel,
                        "Error: Invalid Service Type: must be EXT (or) VLAN\n");
            status = DOVE_SERVICE_TYPE_STATUS_INVALID;
            break;
            */
        }

        /* Add the service with given type 
         * Send the Command to Kernel Module */
        ret_val = dgwy_ctrl_set_svc_type(svcp.cmd, svcp.name, svcp.type.dgtype);
        if(ret_val == -1)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }

    }while(0);

    return status;
}


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
dove_status api_dgadmin_set_service_type(cli_service_type_t *type_set)
{
    dgwy_service_config_t svcp;
    dove_status status = DOVE_STATUS_OK;
    int ret_val;

    memset(&svcp,0,sizeof(dgwy_service_config_t));
    log_debug(ServiceUtilLogLevel,
             "Service:%s, Type:%s ",
             type_set->bridge_name,
             type_set->type);
    do
    {
        /* Set the command type*/
        svcp.cmd = CMD_SET_SVC;
        /* Copy the bridge name */
        memcpy(svcp.name, type_set->bridge_name,
               (strlen(type_set->bridge_name)<SVC_NAME)?
               (strlen(type_set->bridge_name)):(SVC_NAME-1));

        /* Make sure the bridge is ready */
        if (chk_start_svc(type_set->bridge_name) == 0)
        {
            /* No service */
            log_error(ServiceUtilLogLevel,
                        "Error: Failed to set service %s\n", svcp.name);
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
            break;
        }
        else
        {
            set_noicmp_redirect(type_set->bridge_name);
        }

        svcp.type.cmd = CMD_TYPE;

        if(!(strncmp(type_set->type, "EXTVLAN", 7)))
        {
            svcp.type.dgtype = DGWY_TYPE_EXT_VLAN;
            log_info(ServiceUtilLogLevel,
                     "Type:%d ", 
                     svcp.type.dgtype);
            g_APPBRIDGE_type = DGWY_TYPE_EXT_VLAN;
        }
        else if(!(strncmp(type_set->type, "EXT", 3)))
        {
            svcp.type.dgtype = DGWY_TYPE_EXTERNAL;
            log_info(ServiceUtilLogLevel,
                     "Type:%d ", 
                     svcp.type.dgtype);
            g_APPBRIDGE_type = DGWY_TYPE_EXTERNAL;
        }
        else if(!(strncmp(type_set->type, "VLAN", 4)))
        {
            svcp.type.dgtype = DGWY_TYPE_VLAN;
            log_info(ServiceUtilLogLevel,
                     "Type:%d ", 
                     svcp.type.dgtype);
            g_APPBRIDGE_type = DGWY_TYPE_VLAN;
        }
        else
        {
            svcp.type.dgtype = DGWY_TYPE_NONE;
            g_APPBRIDGE_type = DGWY_TYPE_NONE;
            /*
            log_error(ServiceUtilLogLevel,
                        "Error: Invalid Service Type: must be EXT (or) VLAN\n");
            status = DOVE_SERVICE_TYPE_STATUS_INVALID;
            break;
            */
        }

        /* Add the service with given type 
         * Send the Command to Kernel Module */
        ret_val = dgwy_ctrl_set_svc_type(svcp.cmd, svcp.name, svcp.type.dgtype);
        if(ret_val == -1)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }

    }while(0);

    return status;
}


/*
 ******************************************************************************
 * api_dgadmin_add_service_nic_auto                                       *//**
 *
 * \brief - Add NIC (port) to Gateway Service 
 *
 * \param[in] cli_service_ifmac_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_add_service_nic_auto(cli_service_ifmac_t *ifmac_add)
{
	dgwy_service_config_t svcp;
    dove_status status = DOVE_STATUS_OK;
    log_debug(CliLogLevel,"%s: Service:%s",__FUNCTION__,
             ifmac_add->bridge_name);

    memset(&svcp,0,sizeof(dgwy_service_config_t));
	do
	{
		/* Set the command type */
		svcp.cmd = CMD_SET_SVC;
		/* Copy the bridge name */
		memcpy(svcp.name, ifmac_add->bridge_name,
                (strlen(ifmac_add->bridge_name)<SVC_NAME)?\
                (strlen(ifmac_add->bridge_name)):(SVC_NAME-1));

		/* Make sure the bridge is ready */
		if (chk_start_svc(ifmac_add->bridge_name) == 0)
		{
			/* No service */
			log_error(ServiceUtilLogLevel,
			            "Error: Failed to set service %s\n", svcp.name);
			status = DOVE_SERVICE_TYPE_STATUS_INVALID;
			break;
		}

		svcp.ifmac_config.cmd = CMD_ADD_IFMAC;
		memcpy(svcp.ifmac_config.ifmac.mac, ifmac_add->mac, 6);

		/* Send the Command to Kernel Module */
		dgwy_ctrl_svc_macs(&svcp);
	} while(0);

    return status;
}



dove_status api_dgadmin_add_allmacs(void)
{
    struct ifaddrs *ifaddr=NULL;
    struct ifaddrs *ifa=NULL;
    cli_service_ifmac_t addifmac;
    int family;
    
    if (getifaddrs(&ifaddr) == -1) 
    {
        return DOVE_SERVICE_TYPE_STATUS_INVALID;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr != NULL) 
        {
            if((ifa->ifa_name[0]=='e') &&
               (ifa->ifa_name[1]=='t') &&
               (ifa->ifa_name[2]=='h'))
            {
                family = ifa->ifa_addr->sa_family;
                if ((family == AF_INET) || (family==AF_PACKET))
                {
                    struct ifreq buffer;
                    int s=0;

                    s = socket(PF_INET, SOCK_DGRAM, 0);
                    if(s<0)
                    {
                        break;
                    }
                    memset(&buffer, 0x00, sizeof(buffer));
                    strncpy(buffer.ifr_name,ifa->ifa_name, strlen(ifa->ifa_name));
                    ioctl(s, SIOCGIFHWADDR, &buffer);
                    close(s);
                    if(buffer.ifr_hwaddr.sa_data)
                    {
                        memset(&addifmac,0, sizeof(addifmac));
                        memcpy(addifmac.bridge_name,"APBR",strlen("APBR"));
                        memcpy(addifmac.mac, buffer.ifr_hwaddr.sa_data, 6);
                        log_info(ServiceUtilLogLevel,
                                "Add MAC %x:%x:%x:%x:%x:%x to APBR",
                                addifmac.mac[0]&0xff,addifmac.mac[1]&0xff,addifmac.mac[2]&0xff,
                                addifmac.mac[3]&0xff,addifmac.mac[4]&0xff,addifmac.mac[5]&0xff);
                        
                        api_dgadmin_add_service_nic_auto(&addifmac);
                    }
                }
            }
        }
    }
    return DOVE_STATUS_OK;
}



/*
 * This setup a service bridge of type "NONE"
 * Find the MAC address of all interfaces and
 * add them as ports of the bridge.
 * */
dove_status api_dgadmin_init_appliance(void)
{
    cli_service_type_t svctype;
    cli_service_mtu_t svcmtu;

    memset(&svctype,0,sizeof(svctype));
    memset(&svcmtu,0,sizeof(svcmtu));
    memcpy(svctype.bridge_name, "APBR", strlen("APBR"));
    api_dgadmin_add_service_type(&svctype);

    api_dgadmin_add_allmacs();

    memcpy(svcmtu.bridge_name, "APBR", strlen("APBR"));
    svcmtu.mtu = 9000;
    api_dgadmin_set_service_mtu(&svcmtu);

    dgwy_ctrl_set_global(CMD_START);
    return DOVE_STATUS_OK;
}


int check_nic_in_service(char * bridge_name)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int found = 0;

    while(1)
    {
        svcp = dgwy_svc_table_get(bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_IF_MACS; i++)
        {
            if((svcp->ifmacList[i].mac[0]| svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]| svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]| svcp->ifmacList[i].mac[5]))
            {
                /* port/Nic exist in service */
                found = 1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return found;
}

int reset_service(char * bridge_name)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    FILE *fp = NULL;
    char buf[1024];
    int svcid=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(bridge_name, &result, __FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int j=0;
        svcid  = svcp->index;
        for(j=0; j<MAX_DOMIANS; j++)
        {
            svcp->domainList[j] = INVALID_DOMAIN_ID;
        }
        for(j=0; j<MAX_DVMAP; j++)
        {
            svcp->dvList[j].domain = INVALID_DOMAIN_ID;
        }
        memset(svcp->name,0,SVC_NAME);
        svcp->mtu = 0;
        svcp->type = DGWY_TYPE_NONE;
        svcp->state = 0;
        memset(svcp->ifmacList,0,sizeof(svcp->ifmacList));
        memset(svcp->ifipv4List,0,sizeof(svcp->ifipv4List));
        memset(svcp->extipv4List,0,sizeof(svcp->extipv4List));
        memset(svcp->intipv4List,0,sizeof(svcp->intipv4List));
        memset(svcp->fwdruleList,0,sizeof(svcp->fwdruleList));

        SVC_UNLOCK(svcp->lock);

        memset(buf,0,1024);
        sprintf(buf,"cat /etc/iproute2/rt_tables > /etc/iproute2/rt_tables_temp; "
                    "sed '/%d\\t%s/d' /etc/iproute2/rt_tables_temp > "
                    "/etc/iproute2/rt_tables" ,svcid,bridge_name);

        SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
    
        fp = popen(buf,"r");
        if(fp == NULL)
        {   
            log_error(ServiceUtilLogLevel,
                    "Failed to delete route table");
        }   
        else
        {
            fclose(fp);
        }
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */

        return 0;
    }
    return -1;
}



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
dove_status api_dgadmin_del_service_type(cli_service_type_t *type_rem)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
    int ret_val;

    log_debug(CliLogLevel,"%s: Service:%s ",__FUNCTION__,
             type_rem->bridge_name);

    if(check_nic_in_service(type_rem->bridge_name) != 0)
    {
        log_error(ServiceUtilLogLevel,
                  "Can't delete service %s : Nic(s) present",
                  type_rem->bridge_name);
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    dgwy_ctrl_set_global(CMD_STOP);
    memset(&svcp,0,sizeof(dgwy_service_config_t));
    do
    {
        /* Set the command type*/
        svcp.cmd = CMD_DEL_SVC;
        /* Copy the bridge name */
        memcpy(svcp.name, type_rem->bridge_name,
               (strlen(type_rem->bridge_name)<SVC_NAME)?
               (strlen(type_rem->bridge_name)):(SVC_NAME-1));

        /* Delete service with given type 
         * Send the Command to Kernel Module */
        ret_val = dgwy_ctrl_set_svc_type(svcp.cmd, svcp.name, DGWY_TYPE_NONE);
        if(ret_val == -1)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        /* Delete bridge */
        chk_stop_svc(type_rem->bridge_name);

        reset_service(type_rem->bridge_name);

    }while(0);

    dgwy_ctrl_set_global(CMD_START);
    /* XXX TODO */
    return status;
}

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
dove_status api_dgadmin_add_service_nic(cli_service_ifmac_t *ifmac_add)
{
	int ret_val;
	dgwy_service_config_t svcp;
    dove_status status = DOVE_STATUS_OK;
    log_debug(CliLogLevel,"%s: Service:%s",__FUNCTION__,
             ifmac_add->bridge_name);

    memset(&svcp,0,sizeof(dgwy_service_config_t));
	do
	{
		/* Set the command type */
		svcp.cmd = CMD_SET_SVC;
		/* Copy the bridge name */
		memcpy(svcp.name, ifmac_add->bridge_name,
                (strlen(ifmac_add->bridge_name)<SVC_NAME)?\
                (strlen(ifmac_add->bridge_name)):(SVC_NAME-1));

		/* Make sure the bridge is ready */
		if (chk_start_svc(ifmac_add->bridge_name) == 0)
		{
			/* No service */
			log_error(ServiceUtilLogLevel,
			            "Error: Failed to set service %s\n", svcp.name);
			status = DOVE_SERVICE_TYPE_STATUS_INVALID;
			break;
		}

		svcp.ifmac_config.cmd = CMD_ADD_IFMAC;
		memcpy(svcp.ifmac_config.ifmac.mac, ifmac_add->mac, 6);

		/* Add the MAC address to the bridge */
		ret_val = chk_mac_add_bridge(svcp.name, svcp.ifmac_config.ifmac.mac);
		if (ret_val)
		{
			status = DOVE_SERVICE_ADD_STATUS_FAIL;
		}

		/* Send the Command to Kernel Module */
		dgwy_ctrl_svc_macs(&svcp);
	} while(0);

    return status;
}


int delete_svc_nic(cli_service_ifmac_t *ifmac_rem)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;

    while(1)
    {
        svcp = dgwy_svc_table_get(ifmac_rem->bridge_name, &result, __FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_IF_MACS; i++)
        {
            log_debug(ServiceUtilLogLevel,"%d: %x %x",i,
                      svcp->ifmacList[i].mac[5], ifmac_rem->mac[5]);
            if((svcp->ifmacList[i].mac[0]| svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]| svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]| svcp->ifmacList[i].mac[5]))
            {
                if(((ifmac_rem->mac[0]&0xff) == svcp->ifmacList[i].mac[0]) &&
                   ((ifmac_rem->mac[1]&0xff) == svcp->ifmacList[i].mac[1]) &&
                   ((ifmac_rem->mac[2]&0xff) == svcp->ifmacList[i].mac[2]) &&
                   ((ifmac_rem->mac[3]&0xff) == svcp->ifmacList[i].mac[3]) &&
                   ((ifmac_rem->mac[4]&0xff) == svcp->ifmacList[i].mac[4]) &&
                   ((ifmac_rem->mac[5]&0xff) == svcp->ifmacList[i].mac[5]))
                {
                    memset(&svcp->ifmacList[i],0,sizeof(svcp->ifmacList[i]));
                    break;
                }
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return 0;
}
 
int check_nic_in_ipv4(cli_service_ifmac_t *ifmac_rem)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int found=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(ifmac_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }
    
    if(svcp)
    {
        int i=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].ipv4)
            {
                /* found one active ip */
                found=1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return found; 
}



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
dove_status api_dgadmin_del_service_nic(cli_service_ifmac_t *ifmac_rem)
{
	int ret_val;
	dgwy_service_config_t svcp;
    dove_status status = DOVE_STATUS_OK;
    log_debug(CliLogLevel,"%s: Service:%s",__FUNCTION__,
             ifmac_rem->bridge_name);

    if(check_nic_in_ipv4(ifmac_rem) != 0)
    {
        /* there is active IPV4 in this service 
         * Should delete svc IPV4 first */
	    log_error(CliLogLevel, "Can't delete NIC %02x:%02x:%02x:%02x:%02x:%02x "
                  "Inuse by service %s", 
                  ifmac_rem->mac[0]&0xff,
                  ifmac_rem->mac[1]&0xff,
                  ifmac_rem->mac[2]&0xff,
                  ifmac_rem->mac[3]&0xff,
                  ifmac_rem->mac[4]&0xff,
                  ifmac_rem->mac[5]&0xff,
                  ifmac_rem->bridge_name);

        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    dgwy_ctrl_set_global(CMD_STOP);
    memset(&svcp,0,sizeof(dgwy_service_config_t));
	do
	{
		/* Set the command type */
		svcp.cmd = CMD_SET_SVC;
		/* Copy the bridge name */
		memcpy(svcp.name, ifmac_rem->bridge_name,
                (strlen(ifmac_rem->bridge_name)<SVC_NAME)?\
                (strlen(ifmac_rem->bridge_name)):(SVC_NAME-1));

		svcp.ifmac_config.cmd = CMD_DEL_IFMAC;
		memcpy(svcp.ifmac_config.ifmac.mac, ifmac_rem->mac, 6);

		/* Remove the MAC address from the bridge */
		ret_val = chk_mac_rem_bridge(svcp.name, svcp.ifmac_config.ifmac.mac);
		if (ret_val)
		{
			status = DOVE_SERVICE_ADD_STATUS_FAIL;
		}

		/* Send the Command to Kernel Module */
		dgwy_ctrl_svc_macs(&svcp);
        
        delete_svc_nic(ifmac_rem);
	} while(0);

    dgwy_ctrl_set_global(CMD_START);
    return status;
}


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
dove_status api_dgadmin_add_service_ipv4(cli_service_addressv4_t *ipv4_add)
{
    dove_status status = DOVE_STATUS_OK;
	char IP_string[INET6_ADDRSTRLEN];
    char mask_string[INET6_ADDRSTRLEN],nexthop_string[INET6_ADDRSTRLEN];
    dgwy_service_config_t svcp;
	int ret_val;

	inet_ntop(AF_INET, &ipv4_add->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ipv4_add->IPv4_netmask, mask_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ipv4_add->IPv4_nexthop, nexthop_string, INET_ADDRSTRLEN);

	log_debug(CliLogLevel, "%s: Service:%s, IP %s, NetMask %s",__FUNCTION__,
	         ipv4_add->bridge_name,
	         IP_string,
	         mask_string);

    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.ipv4_config.cmd = CMD_ADD_IPV4;
    memcpy(svcp.name, ipv4_add->bridge_name, 
           (strlen(ipv4_add->bridge_name) < SVC_NAME)? 
           (strlen(ipv4_add->bridge_name)):(SVC_NAME-1)); 
    svcp.ipv4_config.ifipv4 = ipv4_add->IPv4;
    svcp.ipv4_config.mask = ipv4_add->IPv4_netmask;
    svcp.ipv4_config.nexthop = ipv4_add->IPv4_nexthop;
    svcp.ipv4_config.vlan_id = ipv4_add->vlan_id;
    
    //printf("-> %x %x <-\n", svcp.ipv4_config.ifipv4, svcp.ipv4_config.mask);

    if(ipv4_add->vlan_id)
    {
       chk_add_svc_ipv4_with_vlan(svcp.name,IP_string,mask_string,nexthop_string,IP_TYPE_EXTERNAL,ipv4_add->vlan_id);
    }
    else
    {
        chk_add_svc_ipv4(svcp.name,IP_string,mask_string,nexthop_string,IP_TYPE_EXTERNAL);
    }

    ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

dove_status api_dgadmin_add_dovenet_svc_ipv4(cli_service_addressv4_t *ipv4_add)
{
    dove_status status = DOVE_STATUS_OK;
	char IP_string[INET6_ADDRSTRLEN];
    char mask_string[INET6_ADDRSTRLEN],nexthop_string[INET6_ADDRSTRLEN];
    dgwy_service_config_t svcp;
	int ret_val;

	inet_ntop(AF_INET, &ipv4_add->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ipv4_add->IPv4_netmask, mask_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ipv4_add->IPv4_nexthop, nexthop_string, INET_ADDRSTRLEN);

	log_debug(CliLogLevel, "%s: Service:%s, IP %s, NetMask %s",__FUNCTION__,
	         ipv4_add->bridge_name,
	         IP_string,
	         mask_string);

    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.ipv4_config.cmd = CMD_ADD_IPV4;
    memcpy(svcp.name, ipv4_add->bridge_name, 
           (strlen(ipv4_add->bridge_name) < SVC_NAME)? 
           (strlen(ipv4_add->bridge_name)):(SVC_NAME-1)); 
    svcp.ipv4_config.ifipv4 = ipv4_add->IPv4;
    svcp.ipv4_config.mask = ipv4_add->IPv4_netmask;
    svcp.ipv4_config.nexthop = ipv4_add->IPv4_nexthop;
    svcp.ipv4_config.vlan_id = ipv4_add->vlan_id;
    
    //printf("-> %x %x <-\n", svcp.ipv4_config.ifipv4, svcp.ipv4_config.mask);
    if(ipv4_add->vlan_id)
    {
       chk_add_svc_ipv4_with_vlan(svcp.name,IP_string,mask_string,nexthop_string,IP_TYPE_DOVE_TEP,ipv4_add->vlan_id);
    }
    else
    {
        chk_add_svc_ipv4(svcp.name,IP_string,mask_string,nexthop_string,IP_TYPE_DOVE_TEP);
    }

    ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

dove_status internal_dgadmin_add_service_ipv4(char *name, uint32_t ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	int ret_val;

    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.ipv4_config.cmd = CMD_ADD_IPV4;
    memcpy(svcp.name, name, 
           (strlen(name) < SVC_NAME)? 
           (strlen(name)):(SVC_NAME-1)); 
    svcp.ipv4_config.ifipv4 = ipv4;
    
    ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

dove_status internal_dgadmin_rem_service_ipv4(char *name, uint32_t ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	int ret_val;

    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.ipv4_config.cmd = CMD_DEL_IPV4;
    memcpy(svcp.name, name, 
           (strlen(name) < SVC_NAME)? 
           (strlen(name)):(SVC_NAME-1)); 
    svcp.ipv4_config.ifipv4 = ipv4;
    
    ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

/*
 * -1 : ERROR
 *  0 : Not found
 *  1 : found
 *  */
int get_svc_ipv4_matching_vlan(int vlan_id, int *retipv4)
{
    int result  = 0;
    int retry   = 0;
    dgwy_service_list_t *svcp = NULL;
    int ret=-1;

    while(1)
    {
        svcp = dgwy_svc_table_get((char*)"APBR",
                                  &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return ret;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        ret=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].vlan_id == vlan_id)
            {
                if(retipv4)
                {
                    *retipv4 = svcp->ifipv4List[i].ipv4;
                }
                ret=1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return ret;
}



int get_svc_ipv4_vlan(cli_service_addressv4_t *ipv4)
{
    int vlan_id = 0 ;
    int result  = 0;
    int retry   = 0;
    dgwy_service_list_t *svcp = NULL;

    while(1)
    {
        svcp = dgwy_svc_table_get(ipv4->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
	        log_debug(CliLogLevel, "%s: %x %x", __FUNCTION__,
                     svcp->ifipv4List[i].ipv4,
                     ipv4->IPv4);

            if(svcp->ifipv4List[i].ipv4 == ipv4->IPv4)
            {
                vlan_id = svcp->ifipv4List[i].vlan_id;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
        return vlan_id;
    }
    return -1;
}


int delete_svc_ipv4(cli_service_addressv4_t *ipv4_rem)
{
    int result = 0;
    int retry = 0;
    dgwy_service_list_t *svcp = NULL;

    while(1)
    {
        svcp = dgwy_svc_table_get(ipv4_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
	        log_debug(CliLogLevel, "%s: %x %x", __FUNCTION__,
                     svcp->ifipv4List[i].ipv4,
                     ipv4_rem->IPv4);

            if(svcp->ifipv4List[i].ipv4 == ipv4_rem->IPv4)
            {
                /* found a matching fwdrule */
                memset(&svcp->ifipv4List[i],0,sizeof(svcp->ifipv4List[i]));
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    return -1;
}

int check_ipv4_for_vlanmap(cli_service_addressv4_t *ipv4_rem)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int found=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(ipv4_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }
    
    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(svcp->dvList[i].domain != INVALID_DOMAIN_ID)
            {
                /* found a vlan map */
                found=1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return found; 
}


int check_ipv4_in_extvip(cli_service_addressv4_t *ipv4_rem)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int found=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(ipv4_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }
    
    if(svcp)
    {
        int i=0;
        uint32_t network=0;
        uint32_t mask;

        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
	        log_debug(CliLogLevel, "%s: %x %x", __FUNCTION__,
                     svcp->ifipv4List[i].ipv4,
                     ipv4_rem->IPv4);

            if(svcp->ifipv4List[i].ipv4 == ipv4_rem->IPv4)
            {
                network = svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;
                mask =  svcp->ifipv4List[i].mask;
                break;
            }
        }
        if(!network || !mask)
        {
            SVC_UNLOCK(svcp->lock);
            log_error(ServiceUtilLogLevel,"Can not be 0:network %x mask %x",
                      network, mask);
            return 1;
        }

        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(network == (svcp->extipv4List[i].ipv4&mask))
            {
                found=1;
                break;
            }
            if(ipv4_rem->IPv4 == svcp->extipv4List[i].ipv4)
            {
                /* found a matching extvip */
                found=1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return found; 
}


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
dove_status api_dgadmin_del_service_ipv4(cli_service_addressv4_t *ipv4_rem)
{
    dove_status status = DOVE_STATUS_OK;
	char IP_string[INET6_ADDRSTRLEN], mask_string[INET6_ADDRSTRLEN];
    dgwy_service_config_t svcp;
	int ret_val=-1;
    int vlan_id=0;

#if 0 
    if(check_ipv4_for_vlanmap(ipv4_rem) != 0)
    {
        /* there is a vlan map
         * Should delete vlanmap  */
	    log_error(CliLogLevel, "Can't delete IPV4 %x: VLAN MAP in use",
                  ipv4_rem->IPv4);
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }

#endif
    if(check_ipv4_in_extvip(ipv4_rem) != 0)
    {
        /* there is a extvip with given ipV4
         * Should delete extvip first */
	    log_error(CliLogLevel, "Can't delete IPV4 %x: Inuse by EXTVIP",
                  ipv4_rem->IPv4);
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    dgwy_ctrl_set_global(CMD_STOP);
	inet_ntop(AF_INET, &ipv4_rem->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &ipv4_rem->IPv4_netmask, mask_string, INET_ADDRSTRLEN);
	log_debug(CliLogLevel, "%s: Service:%s, IP %s, NetMask %s",__FUNCTION__,
	         ipv4_rem->bridge_name,
	         IP_string,
	         mask_string);

    vlan_id = get_svc_ipv4_vlan(ipv4_rem);

    if(vlan_id)
    {
        if(is_ipv4_on_vlan(ipv4_rem->bridge_name, IP_string, vlan_id))
        {
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            
            svcp.ipv4_config.cmd = CMD_DEL_IPV4;
            memcpy(svcp.name, ipv4_rem->bridge_name, 
                   (strlen(ipv4_rem->bridge_name) < SVC_NAME)? 
                   (strlen(ipv4_rem->bridge_name)):(SVC_NAME-1)); 
            svcp.ipv4_config.ifipv4 = ipv4_rem->IPv4;
            
            chk_rem_svc_ipv4_with_vlan(svcp.name,IP_string,vlan_id);
            
            ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);
            
            delete_svc_ipv4(ipv4_rem);
            if(get_svc_ipv4_matching_vlan(vlan_id,NULL)==0)
            {
                /* no more IPs with this vlan 
                 * bring vlan interface down*/
                 FILE *fp;
                 char command[64];
                 memset(command,0,64);
                 sprintf(command,"ifconfig %s.%d down;vconfig rem %s.%d",
                         svcp.name,vlan_id,svcp.name,vlan_id);
                 fp = popen(command, "r");
                 if(fp)
                 {
                     pclose(fp);
                 }
            }
        }
    }
    else if(is_ipv4_on(ipv4_rem->bridge_name, IP_string))
    {
        memset(&svcp,0, sizeof(dgwy_service_config_t));

        svcp.ipv4_config.cmd = CMD_DEL_IPV4;
        memcpy(svcp.name, ipv4_rem->bridge_name, 
               (strlen(ipv4_rem->bridge_name) < SVC_NAME)? 
               (strlen(ipv4_rem->bridge_name)):(SVC_NAME-1)); 
        svcp.ipv4_config.ifipv4 = ipv4_rem->IPv4;
        
        chk_rem_svc_ipv4(svcp.name,IP_string);

        ret_val = dgwy_ctrl_svc_ipv4(&svcp,1);

        delete_svc_ipv4(ipv4_rem);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "IP %s Not enabled: DEL IP: %s\n",IP_string,__FUNCTION__);
        /* Handle error XXX TODO */
    }

	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    dgwy_ctrl_set_global(CMD_START);
    return status;
}

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
dove_status api_dgadmin_add_service_pubipv4(cli_service_extvip_t *extvipv4_add)
{
    dove_status status = DOVE_STATUS_OK;
	dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN];
    int ret_val=-1;
    int vlan_id = 0;

    if((extvipv4_add->port_max-extvipv4_add->port_min)>PORT_POOL_SZ)
    {
        log_error(ServiceUtilLogLevel,
                    "Range Exceeds Maximum Allowed\n");
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	inet_ntop(AF_INET, &extvipv4_add->vIPv4, IP_string, INET_ADDRSTRLEN);
	log_debug(CliLogLevel, "%s: Service:%s, Domain=%d IP %s, Ports %d-%d",
             __FUNCTION__, extvipv4_add->bridge_name,
             extvipv4_add->domain,
	         IP_string, extvipv4_add->port_min,
	         extvipv4_add->port_max);

    vlan_id = get_vlan_of_pubalias(extvipv4_add->bridge_name, IP_string);

    if(vlan_id < 0)
    {
        log_error(ServiceUtilLogLevel,
                    "Failed to get vlan info of extvip\n");
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    else if(vlan_id == 0)
    {
        if(-1 == check_add_pubipv4_alias(extvipv4_add->bridge_name, IP_string))
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add EXT IP and subnet first \n");
            return DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        internal_dgadmin_add_service_ipv4(extvipv4_add->bridge_name,
                                          extvipv4_add->vIPv4);

        if(is_ipv4_on(extvipv4_add->bridge_name, IP_string))
        {
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
            memcpy(svcp.name, extvipv4_add->bridge_name, 
                   (strlen(extvipv4_add->bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_add->bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_add->vIPv4;
            svcp.extipv4_config.extip.domain = extvipv4_add->domain;
            svcp.extipv4_config.extip.port_start = extvipv4_add->port_min;
            svcp.extipv4_config.extip.port_end = extvipv4_add->port_max;
            svcp.extipv4_config.extip.tenant_id = extvipv4_add->tenant_id;
            svcp.extipv4_config.extip.extmcastvnid = extvipv4_add->extmcastvnid;
            ret_val = dgwy_ctrl_svc_extipv4(&svcp);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add IP: add_addressv4 \n");
            /* Handle error XXX TODO */
        }
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }
        return status;
    }
    else
    {
        if(-1 == check_add_pubipv4_alias_with_vlan(extvipv4_add->bridge_name, IP_string))
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add EXT IP and subnet first \n");
            return DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        internal_dgadmin_add_service_ipv4(extvipv4_add->bridge_name,
                                          extvipv4_add->vIPv4);

        if(is_ipv4_on_vlan(extvipv4_add->bridge_name, IP_string, vlan_id))
        {
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
            memcpy(svcp.name, extvipv4_add->bridge_name, 
                   (strlen(extvipv4_add->bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_add->bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_add->vIPv4;
            svcp.extipv4_config.extip.domain = extvipv4_add->domain;
            svcp.extipv4_config.extip.port_start = extvipv4_add->port_min;
            svcp.extipv4_config.extip.port_end = extvipv4_add->port_max;
            svcp.extipv4_config.extip.tenant_id = extvipv4_add->tenant_id;
            svcp.extipv4_config.extip.extmcastvnid = extvipv4_add->extmcastvnid;
            ret_val = dgwy_ctrl_svc_extipv4(&svcp);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add IP: add_addressv4 \n");
            /* Handle error XXX TODO */
        }
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }
        return status;


    }
}


int delete_svc_extipv4(cli_service_extvip_t *extvipv4_rem, int *tenant_id, int *extmcastvnid)
{
    int result = 0;
    int retry = 0;
    dgwy_service_list_t *svcp = NULL;

    while(1)
    {
        svcp = dgwy_svc_table_get(extvipv4_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        uint32_t vnid = 0;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
	        log_debug(CliLogLevel, "%s: %x %x", __FUNCTION__,
                     svcp->extipv4List[i].ipv4,
                     extvipv4_rem->vIPv4);
        
            if(svcp->extipv4List[i].ipv4 == extvipv4_rem->vIPv4)
            {
                vnid = svcp->extipv4List[i].domain;
                *extmcastvnid  = svcp->extipv4List[i].extmcastvnid;
                *tenant_id     = svcp->extipv4List[i].tenant_id;
                /* found a matching fwdrule */
                memset(&svcp->extipv4List[i],0,sizeof(svcp->extipv4List[i]));
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
        if(vnid)
        {
            check_dregister_tunnel(vnid);
        }
        return 0;
    }
    return -1;
}

/* Return
 * 1 - Empty
 * 0 - Non-Empty
 * */
int is_vnid_empty_intenant(uint32_t tenantid)
{
    int result  = 0;
    int retry   = 0;
    int empty   = 1;
    dgwy_service_list_t *svcp = NULL;

    while(1)
    {
        svcp = dgwy_svc_table_get((char*)"APBR", &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        uint32_t vnid = svcp->extipv4List[i].domain;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(svcp->extipv4List[i].tenant_id == tenantid)
            {
                /* found matching domain */
                empty=0;
                break;
            }
        }

        if(empty!=0)
        {
            for(i=0; i<MAX_DOMIANS; i++)
            {
                if(svcp->extsharedVnidList[i].tenant_id == tenantid)
                {
                    /* found matching domain */
                    empty=0;
                    break;
                }
            }
        }

        SVC_UNLOCK(svcp->lock);
        check_dregister_tunnel(vnid);
    }
    return empty;
}



int check_extvip_in_fwd_rule(cli_service_extvip_t *extvipv4_rem)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int found=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(extvipv4_rem->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }
    
    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if(extvipv4_rem->vIPv4 == svcp->fwdruleList[i].fwdipv4)
            {
                /* found a matching fwdrule */
                found=1;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return found; 
}


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
dove_status api_dgadmin_del_service_pubipv4(cli_service_extvip_t *extvipv4_rem)
{
    dove_status status = DOVE_STATUS_OK;
	dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN];
    int ret_val=-1;
    int tenant_id=0;
    int extmcastvnid=0;
    int vlan_id=0;

    if(check_extvip_in_fwd_rule(extvipv4_rem) != 0)
    {
        /* there is a fwdrule with given extvip
         * Should delete svc rules first */
	    log_error(CliLogLevel, "Can't delete Extern VIP %x: Inuse Service Rule",
                  extvipv4_rem->vIPv4);
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
	
    inet_ntop(AF_INET, &extvipv4_rem->vIPv4, IP_string, INET_ADDRSTRLEN);

    vlan_id = get_vlan_of_pubalias(extvipv4_rem->bridge_name,IP_string);

    if(vlan_id<0)
    {
	    log_error(CliLogLevel, "Can't delete Extern VIP %x: failed to get vlan info",
                  extvipv4_rem->vIPv4);
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    else if(vlan_id==0)
    {

        dgwy_ctrl_set_global(CMD_STOP);

        log_debug(CliLogLevel, "%s: Service:%s, IP %x",
                 __FUNCTION__, extvipv4_rem->bridge_name, extvipv4_rem->vIPv4);

        if(is_ipv4_on(extvipv4_rem->bridge_name, IP_string))
        {
            internal_dgadmin_rem_service_ipv4(extvipv4_rem->bridge_name,
                                              extvipv4_rem->vIPv4);
            check_rem_pubipv4_alias(extvipv4_rem->bridge_name, IP_string);

            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem->bridge_name, 
                   (strlen(extvipv4_rem->bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem->bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem->vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            ret_val = dgwy_ctrl_svc_extipv4(&svcp);

            delete_svc_extipv4(extvipv4_rem, &tenant_id, &extmcastvnid);
            if(is_vnid_empty_intenant(tenant_id))
            {
                /* empty: delete extmcast vnid */
                if(tenant_id || extmcastvnid)
                {
                    api_dgadmin_unset_ext_mcast_vnid(extmcastvnid,0/* ipv4 0 */);
                }
            }
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add IP: %s\n",__FUNCTION__);
            /* Handle error XXX TODO */
        }
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        dgwy_ctrl_set_global(CMD_START);
        return status;
    }
    else
    {
        dgwy_ctrl_set_global(CMD_STOP);

        log_debug(CliLogLevel, "%s: Service:%s, IP %x",
                 __FUNCTION__, extvipv4_rem->bridge_name, extvipv4_rem->vIPv4);

        if(is_ipv4_on_vlan(extvipv4_rem->bridge_name, IP_string, vlan_id))
        {
            internal_dgadmin_rem_service_ipv4(extvipv4_rem->bridge_name,
                                              extvipv4_rem->vIPv4);
            check_rem_pubipv4_alias_with_vlan(extvipv4_rem->bridge_name, IP_string,vlan_id);

            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem->bridge_name, 
                   (strlen(extvipv4_rem->bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem->bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem->vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            ret_val = dgwy_ctrl_svc_extipv4(&svcp);

            delete_svc_extipv4(extvipv4_rem, &tenant_id, &extmcastvnid);
            if(is_vnid_empty_intenant(tenant_id))
            {
                /* empty: delete extmcast vnid */
                if(tenant_id || extmcastvnid)
                {
                    api_dgadmin_unset_ext_mcast_vnid(extmcastvnid,0/* ipv4 0 */);
                }

            }
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add IP: %s\n",__FUNCTION__);
            /* Handle error XXX TODO */
        }
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        dgwy_ctrl_set_global(CMD_START);
        return status;
    }
}


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
dove_status api_dgadmin_add_service_overlayipv4(cli_service_overlayip_t *overlayipv4_add)
{
    dove_status status = DOVE_STATUS_OK;

    return status;
}

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
dove_status api_dgadmin_del_service_overlayipv4(cli_service_overlayip_t *overlayipv4_rem)
{
    dove_status status = DOVE_STATUS_OK;

    return status;
}



dove_status dgadmin_add_service_fwdrule(cli_service_fwdrule_t *fwdrl_add)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN], IPmap_string[INET6_ADDRSTRLEN];
    int ret_val=-1;

	inet_ntop(AF_INET, &fwdrl_add->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &fwdrl_add->IPv4_map, IPmap_string, INET_ADDRSTRLEN);


	log_debug(CliLogLevel,
	         "%s: Service %s, Domain %d, Protocol %d, "
	         "IP %s, Port %d, IP map %s, Port map %d",__FUNCTION__,
	         fwdrl_add->bridge_name,
	         fwdrl_add->domain,
	         fwdrl_add->protocol,
	         IP_string,
	         fwdrl_add->port,
	         IPmap_string,
	         fwdrl_add->port_map);

    if(is_ipv4_on(fwdrl_add->bridge_name, IP_string))
    {
        if((fwdrl_add->port==0) &&
           (fwdrl_add->protocol==0))
        {
            /* Adding a fwd rule with no port and protocol: 
             * The param is only IP :
             * This can not be added on an exiting IP
             * */
            log_error(ServiceUtilLogLevel,
                      "IP Only Fwd rule Error: %s already present\n", 
                      IP_string);
            status=DOVE_SERVICE_ADD_STATUS_FAIL; 
            goto out;
        }
        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
        memcpy(svcp.name, fwdrl_add->bridge_name, 
               (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_add->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_add->port;
        svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_add->IPv4_map;
        svcp.fwdrule_config.fwdrule.realport = fwdrl_add->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_add->domain;
        svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
        svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;

        dgwy_ctrl_fwd_rule(&svcp);
    }
    else
    {
        if(fwdrl_add->port || fwdrl_add->protocol)
        {
#if 0
            /* don't know this code was added
             * I can't remember ; disable for now
             * */
            /* Handle error XXX TODO */
            log_error(ServiceUtilLogLevel,
                      "IP Not enabled: Add IP: add_addressv4 \n");
            status=DOVE_SERVICE_ADD_STATUS_FAIL; 
            goto out;
#endif
            if(-1 == check_add_pubipv4_alias(fwdrl_add->bridge_name,
                                             IP_string))
            {
                log_error(ServiceUtilLogLevel,
                          "IP Subnet Not enabled: "
                          "Add EXT IP subnet first \n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }
            
            internal_dgadmin_add_service_ipv4(fwdrl_add->bridge_name,
                                              fwdrl_add->IPv4);
            
            if(is_ipv4_on(fwdrl_add->bridge_name, IP_string))
            {
                memset(&svcp,0, sizeof(dgwy_service_config_t));
                svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
                memcpy(svcp.name, fwdrl_add->bridge_name, 
                       (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                       (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
                svcp.extipv4_config.extip.ipv4 = fwdrl_add->IPv4;
                svcp.extipv4_config.extip.domain = fwdrl_add->domain;
                svcp.extipv4_config.extip.port_start = 0;
                svcp.extipv4_config.extip.port_end = 0;
                ret_val = dgwy_ctrl_svc_extipv4(&svcp);
                if(ret_val)
                {
                    log_error(ServiceUtilLogLevel,
                              "Failed to add extvip \n");
                    status=DOVE_SERVICE_ADD_STATUS_FAIL;
                    goto out;
                }
            }
            else
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to add IPV4 for fwd rule\n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }

            log_debug(ServiceUtilLogLevel,
                      "Add IP only fwd rule \n");
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
            memcpy(svcp.name, fwdrl_add->bridge_name, 
                   (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                   (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
            svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
            svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_add->protocol;
            svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_add->port;
            svcp.fwdrule_config.fwdrule.realport = fwdrl_add->port_map;
            svcp.fwdrule_config.fwdrule.realipv4=fwdrl_add->IPv4_map;
            svcp.fwdrule_config.fwdrule.domain = fwdrl_add->domain;
            svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
            svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;
            dgwy_ctrl_fwd_rule(&svcp);
        }
        else
        {
            if(-1 == check_add_pubipv4_alias(fwdrl_add->bridge_name,
                                             IP_string))
            {
                log_error(ServiceUtilLogLevel,
                          "IP Subnet Not enabled: "
                          "Add EXT IP subnet first \n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }
            
            internal_dgadmin_add_service_ipv4(fwdrl_add->bridge_name,
                                              fwdrl_add->IPv4);
            
            if(is_ipv4_on(fwdrl_add->bridge_name, IP_string))
            {
                memset(&svcp,0, sizeof(dgwy_service_config_t));
                svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
                memcpy(svcp.name, fwdrl_add->bridge_name, 
                       (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                       (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
                svcp.extipv4_config.extip.ipv4 = fwdrl_add->IPv4;
                svcp.extipv4_config.extip.domain = fwdrl_add->domain;
                svcp.extipv4_config.extip.port_start = 0;
                svcp.extipv4_config.extip.port_end = 0;
                ret_val = dgwy_ctrl_svc_extipv4(&svcp);
                if(ret_val)
                {
                    log_error(ServiceUtilLogLevel,
                              "Failed to add extvip \n");
                    status=DOVE_SERVICE_ADD_STATUS_FAIL;
                    goto out;
                }
            }
            else
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to add IPV4 for fwd rule\n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }

            log_debug(ServiceUtilLogLevel,
                      "Add IP only fwd rule \n");
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
            memcpy(svcp.name, fwdrl_add->bridge_name, 
                   (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                   (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
            svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
            svcp.fwdrule_config.fwdrule.fwdproto = 0;
            svcp.fwdrule_config.fwdrule.fwdport  = 0;
            svcp.fwdrule_config.fwdrule.realport = 0;
            svcp.fwdrule_config.fwdrule.realipv4=fwdrl_add->IPv4_map;
            svcp.fwdrule_config.fwdrule.domain = fwdrl_add->domain;
            svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
            svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;
            dgwy_ctrl_fwd_rule(&svcp);
        }
    }
out:
    return status;
}

dove_status dgadmin_add_service_fwdrule_with_vlan(cli_service_fwdrule_t *fwdrl_add,
                                                  int vlan_id)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN], IPmap_string[INET6_ADDRSTRLEN];
    int ret_val=-1;

	inet_ntop(AF_INET, &fwdrl_add->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &fwdrl_add->IPv4_map, IPmap_string, INET_ADDRSTRLEN);


	log_debug(CliLogLevel,
	         "%s: Service %s, Domain %d, Protocol %d, "
	         "IP %s, Port %d, IP map %s, Port map %d",__FUNCTION__,
	         fwdrl_add->bridge_name,
	         fwdrl_add->domain,
	         fwdrl_add->protocol,
	         IP_string,
	         fwdrl_add->port,
	         IPmap_string,
	         fwdrl_add->port_map);

    if(is_ipv4_on_vlan(fwdrl_add->bridge_name, IP_string, vlan_id))
    {
        if((fwdrl_add->port==0) &&
           (fwdrl_add->protocol==0))
        {
            /* Adding a fwd rule with no port and protocol: 
             * The param is only IP :
             * This can not be added on an exiting IP
             * */
            log_error(ServiceUtilLogLevel,
                      "IP Only Fwd rule Error: %s already present\n", 
                      IP_string);
            status=DOVE_SERVICE_ADD_STATUS_FAIL; 
            goto out;
        }
        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
        memcpy(svcp.name, fwdrl_add->bridge_name, 
               (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_add->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_add->port;
        svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_add->IPv4_map;
        svcp.fwdrule_config.fwdrule.realport = fwdrl_add->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_add->domain;
        svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
        svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;

        dgwy_ctrl_fwd_rule(&svcp);
    }
    else
    {
        if(fwdrl_add->port || fwdrl_add->protocol)
        {
            if(-1 == check_add_pubipv4_alias_with_vlan(fwdrl_add->bridge_name,
                                                       IP_string))
            {
                log_error(ServiceUtilLogLevel,
                          "IP Subnet Not enabled: "
                          "Add EXT IP subnet first \n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }
            
            internal_dgadmin_add_service_ipv4(fwdrl_add->bridge_name,
                                              fwdrl_add->IPv4);
            
            if(is_ipv4_on_vlan(fwdrl_add->bridge_name, IP_string, vlan_id))
            {
                memset(&svcp,0, sizeof(dgwy_service_config_t));
                svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
                memcpy(svcp.name, fwdrl_add->bridge_name, 
                       (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                       (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
                svcp.extipv4_config.extip.ipv4 = fwdrl_add->IPv4;
                svcp.extipv4_config.extip.domain = fwdrl_add->domain;
                svcp.extipv4_config.extip.port_start = 0;
                svcp.extipv4_config.extip.port_end = 0;
                ret_val = dgwy_ctrl_svc_extipv4(&svcp);
                if(ret_val)
                {
                    log_error(ServiceUtilLogLevel,
                              "Failed to add extvip \n");
                    status=DOVE_SERVICE_ADD_STATUS_FAIL;
                    goto out;
                }
            }
            else
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to add IPV4 for fwd rule\n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }

            log_debug(ServiceUtilLogLevel,
                      "Add IP only fwd rule \n");
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
            memcpy(svcp.name, fwdrl_add->bridge_name, 
                   (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                   (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
            svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
            svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_add->protocol;
            svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_add->port;
            svcp.fwdrule_config.fwdrule.realport = fwdrl_add->port_map;
            svcp.fwdrule_config.fwdrule.realipv4=fwdrl_add->IPv4_map;
            svcp.fwdrule_config.fwdrule.domain = fwdrl_add->domain;

            svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
            svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;
            dgwy_ctrl_fwd_rule(&svcp);
        }
        else
        {
            if(-1 == check_add_pubipv4_alias_with_vlan(fwdrl_add->bridge_name,
                                                       IP_string))
            {
                log_error(ServiceUtilLogLevel,
                          "IP Subnet Not enabled: "
                          "Add EXT IP subnet first \n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }
            
            internal_dgadmin_add_service_ipv4(fwdrl_add->bridge_name,
                                              fwdrl_add->IPv4);
            
            if(is_ipv4_on_vlan(fwdrl_add->bridge_name, IP_string, vlan_id))
            {
                memset(&svcp,0, sizeof(dgwy_service_config_t));
                svcp.extipv4_config.cmd = CMD_ADD_EXTVIP;
                memcpy(svcp.name, fwdrl_add->bridge_name, 
                       (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                       (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
                svcp.extipv4_config.extip.ipv4 = fwdrl_add->IPv4;
                svcp.extipv4_config.extip.domain = fwdrl_add->domain;
                svcp.extipv4_config.extip.port_start = 0;
                svcp.extipv4_config.extip.port_end = 0;
                ret_val = dgwy_ctrl_svc_extipv4(&svcp);
                if(ret_val)
                {
                    log_error(ServiceUtilLogLevel,
                              "Failed to add extvip \n");
                    status=DOVE_SERVICE_ADD_STATUS_FAIL;
                    goto out;
                }
            }
            else
            {
                log_error(ServiceUtilLogLevel,
                          "Failed to add IPV4 for fwd rule\n");
                status=DOVE_SERVICE_ADD_STATUS_FAIL;
                goto out;
            }

            log_debug(ServiceUtilLogLevel,
                      "Add IP only fwd rule \n");
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.fwdrule_config.cmd = CMD_ADD_FWD_RULE;
            memcpy(svcp.name, fwdrl_add->bridge_name, 
                   (strlen(fwdrl_add->bridge_name) < SVC_NAME)? 
                   (strlen(fwdrl_add->bridge_name)):(SVC_NAME-1)); 
            svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_add->IPv4;
            svcp.fwdrule_config.fwdrule.fwdproto = 0;
            svcp.fwdrule_config.fwdrule.fwdport  = 0;
            svcp.fwdrule_config.fwdrule.realport = 0;
            svcp.fwdrule_config.fwdrule.realipv4=fwdrl_add->IPv4_map;
            svcp.fwdrule_config.fwdrule.domain = fwdrl_add->domain;

            svcp.fwdrule_config.fwdrule.pip_min  = fwdrl_add->pip_min;
            svcp.fwdrule_config.fwdrule.pip_max  = fwdrl_add->pip_max;
            dgwy_ctrl_fwd_rule(&svcp);
        }
    }
out:
    return status;
}


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
dove_status api_dgadmin_add_service_fwdrule(cli_service_fwdrule_t *fwdrl_add)
{
	char IP_string[INET6_ADDRSTRLEN];
    int vlan_id = 0;

	inet_ntop(AF_INET, &fwdrl_add->IPv4, IP_string, INET_ADDRSTRLEN);

    vlan_id = get_vlan_of_pubalias(fwdrl_add->bridge_name, IP_string);
    if(vlan_id < 0)
    {
        log_error(ServiceUtilLogLevel,
                    "Failed to get vlan info of extvip\n");
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    else if(vlan_id==0)
    {
        return (dgadmin_add_service_fwdrule(fwdrl_add));
    }
    else
    {
        return (dgadmin_add_service_fwdrule_with_vlan(fwdrl_add,
                                                      vlan_id));
    }

}

uint32_t get_svc_fwd_rule_mapIP(cli_service_fwdrule_t *rule)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    uint32_t retIP=0;

    while(1)
    {
        svcp = dgwy_svc_table_get(rule->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return retIP;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if((rule->domain == svcp->fwdruleList[i].domain) &&
               (rule->IPv4 == svcp->fwdruleList[i].fwdipv4)  &&
               (rule->port == svcp->fwdruleList[i].fwdport) &&
               (rule->protocol == svcp->fwdruleList[i].fwdproto))
            {
                /* found a matching fwdrule */
                 retIP=svcp->fwdruleList[i].realipv4;
                 break;
            }
        }
        SVC_UNLOCK(svcp->lock);

    }
    return retIP;
}



/*
 ***************************************************************************
 * delete_svc_fwd_rule                                                 *//**
 *
 * \brief - Delete the matching Forward rule entry: 
 *
 * \param[in] cli_service_fwdrule_t
 *
  \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int delete_svc_fwd_rule(cli_service_fwdrule_t *rule)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;

    while(1)
    {
        svcp = dgwy_svc_table_get(rule->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if((rule->domain == svcp->fwdruleList[i].domain) &&
               (rule->IPv4 == svcp->fwdruleList[i].fwdipv4)  &&
               /*(rule->IPv4_map == svcp->fwdruleList[i].realipv4) &&*/
               (rule->port == svcp->fwdruleList[i].fwdport) &&
               /*(rule->port_map == svcp->fwdruleList[i].realport) &&*/
               (rule->protocol == svcp->fwdruleList[i].fwdproto))
            {
                /* found a matching fwdrule */
                fwdrule_pip_deregister(svcp->fwdruleList[i].pip_min,svcp->fwdruleList[i].pip_max,rule->domain);
                memset(&svcp->fwdruleList[i],0,sizeof(svcp->fwdruleList[i]));
                break;
            }
        }

        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    return -1; 
}

/* -1 error;
 * 0  No matching rule
 * 1  Yes matching rule
 * */
int is_there_fwd_rule_matching(cli_service_fwdrule_t *rule)
{
    int result = 0;
    int match = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;

    while(1)
    {
        svcp = dgwy_svc_table_get(rule->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if((rule->domain == svcp->fwdruleList[i].domain) &&
               (rule->IPv4 == svcp->fwdruleList[i].fwdipv4)  &&
               (rule->port == svcp->fwdruleList[i].fwdport) &&
               (rule->protocol == svcp->fwdruleList[i].fwdproto))
            {
                /* found a matching fwdrule */
                match = 1;
                break;
            }
        }

        SVC_UNLOCK(svcp->lock);
        return match;
    }
    return -1; 
}


int get_fwd_rule_count(cli_service_fwdrule_t *rule)
{
    dgwy_service_list_t *svcp = NULL;
    int result = 0;
    int retry = 0;
    int count = 0;

    while(1)
    {
        svcp = dgwy_svc_table_get(rule->bridge_name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if((rule->domain == svcp->fwdruleList[i].domain) &&
               (rule->IPv4 == svcp->fwdruleList[i].fwdipv4) )
            {
                /* found a matching fwdrule 
                 * with same ip*/
                count++;
            }
        }

        SVC_UNLOCK(svcp->lock);
        return count;
    }
    return -1; 
}


dove_status dgadmin_del_service_fwdrule(cli_service_fwdrule_t *fwdrl_rem)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN], IPmap_string[INET6_ADDRSTRLEN];

	inet_ntop(AF_INET, &fwdrl_rem->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &fwdrl_rem->IPv4_map, IPmap_string, INET_ADDRSTRLEN);

    if(is_there_fwd_rule_matching(fwdrl_rem) != 1)
    {
        /* matching rule NOT exist */
        log_error(ServiceUtilLogLevel,
                  "NO Matching FWD rule SKIP\n");
        return status;
    }

    dgwy_ctrl_set_global(CMD_STOP);

	log_debug(CliLogLevel,
	         "%s: Service %s, Domain %d, Protocol %d, "
	         "IP %s, Port %d, IP map %s, Port map %d",__FUNCTION__,
	         fwdrl_rem->bridge_name,
	         fwdrl_rem->domain,
	         fwdrl_rem->protocol,
	         IP_string,
	         fwdrl_rem->port,
	         IPmap_string,
	         fwdrl_rem->port_map);

    if(is_ipv4_on(fwdrl_rem->bridge_name, IP_string))
    {
        int tenid,extmcast;
        cli_service_extvip_t extvipv4_rem;
        int fwdrules = get_fwd_rule_count(fwdrl_rem);

        log_debug (ServiceUtilLogLevel,
                   "fwdrules = %d\n",fwdrules);
                    
        if(fwdrules == -1 )
        {
            log_error (ServiceUtilLogLevel,
                       "Failed to delete fwd rule \n");
            goto end;
        }
        else if(fwdrules == 1 )
        {
            /* only one rule 
             * delete the extvip
             * */
            memset(&extvipv4_rem,0,sizeof(extvipv4_rem));
            extvipv4_rem.domain = fwdrl_rem->domain;
            extvipv4_rem.vIPv4 = fwdrl_rem->IPv4;
            extvipv4_rem.port_min = 0;
            extvipv4_rem.port_max = 0;
            memcpy(extvipv4_rem.bridge_name,"APBR",
                   strlen("APBR"));

            internal_dgadmin_rem_service_ipv4(fwdrl_rem->bridge_name,
                                              fwdrl_rem->IPv4);
            check_rem_pubipv4_alias(fwdrl_rem->bridge_name, IP_string);
        }

        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_DEL_FWD_RULE;
        memcpy(svcp.name, fwdrl_rem->bridge_name, 
               (strlen(fwdrl_rem->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_rem->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_rem->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_rem->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_rem->port;
        if(fwdrl_rem->IPv4_map)
        {
            svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_rem->IPv4_map;
        }
        else
        {
            svcp.fwdrule_config.fwdrule.realipv4 = get_svc_fwd_rule_mapIP(fwdrl_rem);
        }
        svcp.fwdrule_config.fwdrule.realport = fwdrl_rem->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_rem->domain;

        dgwy_ctrl_fwd_rule(&svcp);
        delete_svc_fwd_rule(fwdrl_rem);

        if(fwdrules <= 1 )
        {
            /* now delete the extvip */
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem.bridge_name, 
                   (strlen(extvipv4_rem.bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem.bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem.vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            dgwy_ctrl_svc_extipv4(&svcp);

            delete_svc_extipv4(&extvipv4_rem, &tenid, &extmcast);
        }

    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "IP Not enabled: Add IP: add_addressv4 \n");
        /* Handle error XXX TODO */
    }

end:
    dgwy_ctrl_set_global(CMD_START);
    return status;
}



dove_status dgadmin_del_service_fwdrule_with_vlan(cli_service_fwdrule_t *fwdrl_rem,
                                                  int vlan_id)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN], IPmap_string[INET6_ADDRSTRLEN];

	inet_ntop(AF_INET, &fwdrl_rem->IPv4, IP_string, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &fwdrl_rem->IPv4_map, IPmap_string, INET_ADDRSTRLEN);

    if(is_there_fwd_rule_matching(fwdrl_rem) != 1)
    {
        /* matching rule NOT exist */
        log_error(ServiceUtilLogLevel,
                  "NO Matching FWD rule SKIP\n");
        return status;
    }

    dgwy_ctrl_set_global(CMD_STOP);

	log_debug(ServiceUtilLogLevel,
	          "%s: Service %s, Domain %d, Protocol %d, "
	          "IP %s, Port %d, IP map %s, Port map %d",__FUNCTION__,
	          fwdrl_rem->bridge_name,
	          fwdrl_rem->domain,
	          fwdrl_rem->protocol,
	          IP_string,
	          fwdrl_rem->port,
	          IPmap_string,
	          fwdrl_rem->port_map);

    if(is_ipv4_on_vlan(fwdrl_rem->bridge_name, IP_string, vlan_id))
    {
        int tenid,extmcast;
        cli_service_extvip_t extvipv4_rem;
        int fwdrules = get_fwd_rule_count(fwdrl_rem);
                    
        log_debug (ServiceUtilLogLevel,
                   "fwdrules = %d\n",fwdrules);
        if(fwdrules == -1 )
        {
            log_error (ServiceUtilLogLevel,
                       "Failed ro delete fwd rule \n");
            goto end;
        }
        else if(fwdrules == 1 )
        {
            memset(&extvipv4_rem,0,sizeof(extvipv4_rem));
            extvipv4_rem.domain = fwdrl_rem->domain;
            extvipv4_rem.vIPv4 = fwdrl_rem->IPv4;
            extvipv4_rem.port_min = 0;
            extvipv4_rem.port_max = 0;
            memcpy(extvipv4_rem.bridge_name,"APBR",
                   strlen("APBR"));

            internal_dgadmin_rem_service_ipv4(fwdrl_rem->bridge_name,
                                              fwdrl_rem->IPv4);
            check_rem_pubipv4_alias_with_vlan(fwdrl_rem->bridge_name, IP_string, vlan_id);
        }

        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_DEL_FWD_RULE;
        memcpy(svcp.name, fwdrl_rem->bridge_name, 
               (strlen(fwdrl_rem->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_rem->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_rem->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_rem->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_rem->port;
        if(fwdrl_rem->IPv4_map)
        {
            svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_rem->IPv4_map;
        }
        else
        {
            svcp.fwdrule_config.fwdrule.realipv4 = get_svc_fwd_rule_mapIP(fwdrl_rem);
        }
        svcp.fwdrule_config.fwdrule.realport = fwdrl_rem->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_rem->domain;

        dgwy_ctrl_fwd_rule(&svcp);
        delete_svc_fwd_rule(fwdrl_rem);

        if(fwdrules <= 1 )
        {
            /* now delete the extvip */
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem.bridge_name, 
                   (strlen(extvipv4_rem.bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem.bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem.vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            dgwy_ctrl_svc_extipv4(&svcp);

            delete_svc_extipv4(&extvipv4_rem, &tenid, &extmcast);
        }

    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "IP Not enabled: Add IP: add_addressv4 \n");
        /* Handle error XXX TODO */
    }

end:
    dgwy_ctrl_set_global(CMD_START);
    return status;
}




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
dove_status api_dgadmin_del_service_fwdrule(cli_service_fwdrule_t *fwdrl_rem)
{
	char IP_string[INET6_ADDRSTRLEN];
    int vlan_id=0;

	inet_ntop(AF_INET, &fwdrl_rem->IPv4, IP_string, INET_ADDRSTRLEN);

    vlan_id = get_vlan_of_pubalias(fwdrl_rem->bridge_name, IP_string);

    if(vlan_id < 0)
    {
        log_error(ServiceUtilLogLevel,
                    "Failed to get vlan info of fwdrule ip\n");
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    else if(vlan_id == 0)
    {
        return dgadmin_del_service_fwdrule(fwdrl_rem);
    }
    else
    {
        return dgadmin_del_service_fwdrule_with_vlan(fwdrl_rem,vlan_id);
    }
}

dove_status api_dgadmin_del_service_fwdrule_force(cli_service_fwdrule_t *fwdrl_rem)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_service_config_t svcp;
	char IP_string[INET6_ADDRSTRLEN];
    int tenid,extmcast;
    cli_service_extvip_t extvipv4_rem;
    int vlan_id =0;

	inet_ntop(AF_INET, &fwdrl_rem->IPv4, IP_string, INET_ADDRSTRLEN);

    vlan_id = get_vlan_of_pubalias(fwdrl_rem->bridge_name, IP_string);

    if(vlan_id<=0)
    {
        int fwdrules = get_fwd_rule_count(fwdrl_rem);
                    
        memset(&extvipv4_rem,0,sizeof(extvipv4_rem));

        extvipv4_rem.domain = fwdrl_rem->domain;
        extvipv4_rem.vIPv4 = fwdrl_rem->IPv4;
        extvipv4_rem.port_min = 0;
        extvipv4_rem.port_max = 0;
        memcpy(extvipv4_rem.bridge_name,"APBR",
               strlen("APBR"));

        dgwy_ctrl_set_global(CMD_STOP);

        
        if(is_ipv4_on(fwdrl_rem->bridge_name, IP_string))
        {
            if(fwdrules <= 1 )
            {
                internal_dgadmin_rem_service_ipv4(fwdrl_rem->bridge_name,
                                                  fwdrl_rem->IPv4);
                check_rem_pubipv4_alias(fwdrl_rem->bridge_name, IP_string);
            }
        }

        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_DEL_FWD_RULE;
        memcpy(svcp.name, fwdrl_rem->bridge_name, 
               (strlen(fwdrl_rem->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_rem->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_rem->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_rem->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_rem->port;
        if(fwdrl_rem->IPv4_map)
        {
            svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_rem->IPv4_map;
        }
        else
        {
            svcp.fwdrule_config.fwdrule.realipv4 = get_svc_fwd_rule_mapIP(fwdrl_rem);
        }

        svcp.fwdrule_config.fwdrule.realport = fwdrl_rem->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_rem->domain;

        dgwy_ctrl_fwd_rule(&svcp);
        delete_svc_fwd_rule(fwdrl_rem);

        if(fwdrules <= 1 )
        {
            /* now delete the extvip */
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem.bridge_name, 
                   (strlen(extvipv4_rem.bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem.bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem.vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            dgwy_ctrl_svc_extipv4(&svcp);

            delete_svc_extipv4(&extvipv4_rem, &tenid, &extmcast);
        }


        dgwy_ctrl_set_global(CMD_START);
        return status;
    }
    else
    {
        int fwdrules = get_fwd_rule_count(fwdrl_rem);

        memset(&extvipv4_rem,0,sizeof(extvipv4_rem));

        extvipv4_rem.domain = fwdrl_rem->domain;
        extvipv4_rem.vIPv4 = fwdrl_rem->IPv4;
        extvipv4_rem.port_min = 0;
        extvipv4_rem.port_max = 0;
        memcpy(extvipv4_rem.bridge_name,"APBR",
               strlen("APBR"));

        dgwy_ctrl_set_global(CMD_STOP);

        
        if(is_ipv4_on_vlan(fwdrl_rem->bridge_name, IP_string, vlan_id))
        {
            if(fwdrules <= 1 )
            {
                internal_dgadmin_rem_service_ipv4(fwdrl_rem->bridge_name,
                                                  fwdrl_rem->IPv4);
                check_rem_pubipv4_alias_with_vlan(fwdrl_rem->bridge_name, 
                                                  IP_string, vlan_id);
            }
        }

        memset(&svcp,0, sizeof(dgwy_service_config_t));
        svcp.fwdrule_config.cmd = CMD_DEL_FWD_RULE;
        memcpy(svcp.name, fwdrl_rem->bridge_name, 
               (strlen(fwdrl_rem->bridge_name) < SVC_NAME)? 
               (strlen(fwdrl_rem->bridge_name)):(SVC_NAME-1)); 

        svcp.fwdrule_config.fwdrule.fwdipv4 = fwdrl_rem->IPv4;
        svcp.fwdrule_config.fwdrule.fwdproto = fwdrl_rem->protocol;
        svcp.fwdrule_config.fwdrule.fwdport  = fwdrl_rem->port;
        if(fwdrl_rem->IPv4_map)
        {
            svcp.fwdrule_config.fwdrule.realipv4 = fwdrl_rem->IPv4_map;
        }
        else
        {
            svcp.fwdrule_config.fwdrule.realipv4 = get_svc_fwd_rule_mapIP(fwdrl_rem);
        }

        svcp.fwdrule_config.fwdrule.realport = fwdrl_rem->port_map;
        svcp.fwdrule_config.fwdrule.domain   = fwdrl_rem->domain;

        dgwy_ctrl_fwd_rule(&svcp);
        delete_svc_fwd_rule(fwdrl_rem);

        if(fwdrules <= 1 )
        {
            /* now delete the extvip */
            memset(&svcp,0, sizeof(dgwy_service_config_t));
            svcp.extipv4_config.cmd = CMD_DEL_EXTVIP;
            memcpy(svcp.name, extvipv4_rem.bridge_name, 
                   (strlen(extvipv4_rem.bridge_name) < SVC_NAME)? 
                   (strlen(extvipv4_rem.bridge_name)):(SVC_NAME-1)); 
            svcp.extipv4_config.extip.ipv4 = extvipv4_rem.vIPv4;
            svcp.extipv4_config.extip.domain = 0; /* no meaning */
            svcp.extipv4_config.extip.port_start = 0; /* no meaning */
            svcp.extipv4_config.extip.port_end = 0; /* no meaning */ 
            dgwy_ctrl_svc_extipv4(&svcp);
            delete_svc_extipv4(&extvipv4_rem, &tenid, &extmcast);
        }

        dgwy_ctrl_set_global(CMD_START);
        return status;

    }
}

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
dove_status api_dgadmin_set_service_mtu(cli_service_mtu_t *svc_mtu)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: Service %s, MTU %u ",__FUNCTION__,
	         svc_mtu->bridge_name,
	         svc_mtu->mtu);

    ret_val = dgwy_ctrl_set_svc_mtu(CMD_MTU,
                                    svc_mtu->bridge_name,
                                    svc_mtu->mtu);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

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
dove_status api_dgadmin_add_domain_vlan(cli_service_dvlan_t *dvlan_add)
{
    int ret_val=0;
    dove_status status = DOVE_STATUS_OK;
	dgwy_service_config_t svcp;
	log_debug(CliLogLevel,
	         "%s:Service %s, Domain %d, vlan %u ",__FUNCTION__,
	         dvlan_add->bridge_name,
	         dvlan_add->domain,
	         dvlan_add->vlan);

    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.dv_config.cmd = CMD_SET_DOMAIN_VLAN;
    memcpy(svcp.name, dvlan_add->bridge_name, 
           (strlen(dvlan_add->bridge_name) < SVC_NAME)? 
           (strlen(dvlan_add->bridge_name)):(SVC_NAME-1)); 
    svcp.dv_config.domain = dvlan_add->domain;
    svcp.dv_config.vlans[0] = dvlan_add->vlan; /* one at a time */

    ret_val = dgwy_ctrl_svc_domain_vlans(&svcp);

    if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

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
dove_status api_dgadmin_del_domain_vlan(cli_service_dvlan_t *dvlan_rem)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	dgwy_service_config_t svcp;
	log_debug(CliLogLevel,
	         "%s:Service %s, Domain %d, vlan %u ",__FUNCTION__,
	         dvlan_rem->bridge_name,
	         dvlan_rem->domain,
	         dvlan_rem->vlan);

    dgwy_ctrl_set_global(CMD_STOP);
    memset(&svcp,0, sizeof(dgwy_service_config_t));

    svcp.dv_config.cmd = CMD_DEL_DOMAIN_VLAN;
    memcpy(svcp.name, dvlan_rem->bridge_name, 
           (strlen(dvlan_rem->bridge_name) < SVC_NAME)? 
           (strlen(dvlan_rem->bridge_name)):(SVC_NAME-1)); 
    svcp.dv_config.domain = dvlan_rem->domain;
    svcp.dv_config.vlans[0] = dvlan_rem->vlan; /* one at a time */

    ret_val = dgwy_ctrl_svc_domain_vlans(&svcp);

    if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }


    dgwy_ctrl_set_global(CMD_START);
    return status;
}

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
dove_status api_dgadmin_add_dps_server(cli_dps_server_t *dps)
{
    int ret_val=0;
    dove_status status = DOVE_STATUS_OK;
	char IP_dps[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET, &dps->dps_IP,
	          IP_dps, INET_ADDRSTRLEN);

    log_debug(CliLogLevel,
             "%s: Add DPS IP %s "
             "Port %u DVG %u DOMAIN %u",__FUNCTION__,
             IP_dps, dps->port, dps->domain);
    /*
    ret_val = dgwy_del_dps_server(dps->dps_IP);
    */

    ret_val = dgwy_add_dps_server(dps->domain,dps->dps_IP,dps->port);
    if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

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
dove_status api_dgadmin_del_dps_server(cli_dps_server_t *dps)
{
    int ret_val=0;
    dove_status status = DOVE_STATUS_OK;
	char IP_dps[INET6_ADDRSTRLEN];

    /* ?? XXX TODO Remove Later ??
     * JUST ignore [WORK AROUND FOR DPS CLIENT BUG]
     * If you delete and add new IP ; 
     *  - causes problem in dps client protocol
     * we can set address only once 
     * */
    return status;

	inet_ntop(AF_INET, &dps->dps_IP,
	          IP_dps, INET_ADDRSTRLEN);

    log_debug(CliLogLevel,
             "%s: Delete DPS IP %s ",__FUNCTION__,IP_dps);

    ret_val = dgwy_del_dps_server(dps->dps_IP);
    if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}

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
dove_status api_dgadmin_start_gateway(void)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_ctrl_set_global(CMD_START);
    return status;
}

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
dove_status api_dgadmin_stop_gateway(void)
{
    dove_status status = DOVE_STATUS_OK;
    dgwy_ctrl_set_global(CMD_STOP);
    return status;
}

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
dove_status api_dgadmin_set_dove_net_ipv4(cli_dove_net_ipv4_t *dove_net_ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: Service %s, MTU %u ",__FUNCTION__,
	         dove_net_ipv4->bridge_name,
	         dove_net_ipv4->IPv4);

    ret_val = dgwy_ctrl_set_dovenet_ipv4(dove_net_ipv4->bridge_name, 
                                         dove_net_ipv4->IPv4,
                                         dove_net_ipv4->IPv4_nexthop);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}

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
dove_status api_dgadmin_set_ext_net_ipv4(cli_dove_net_ipv4_t *dove_net_ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: Service %s, MTU %u ",__FUNCTION__,
	         dove_net_ipv4->bridge_name,
	         dove_net_ipv4->IPv4);

    ret_val = dgwy_ctrl_set_external_ipv4 (dove_net_ipv4->bridge_name, 
                                           dove_net_ipv4->IPv4,
                                           dove_net_ipv4->IPv4_nexthop);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}


/*
 ******************************************************************************
 * api_dgadmin_set_mgmt_ipv4                                              *//**
 *
 * \brief - Set Gateway MGMT IPV4
 *
 * \param[in] cli_mgmt_ipv4_t
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status api_dgadmin_set_mgmt_ipv4(cli_mgmt_ipv4_t *mgmt_ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: MGMT %u ",__FUNCTION__,
	         mgmt_ipv4->IPv4);

    ret_val = dgwy_ctrl_set_mgmt_ipv4(mgmt_ipv4->IPv4, 
                                      mgmt_ipv4->IPv4_mask,
                                      mgmt_ipv4->IPv4_nexthop);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}


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
dove_status api_dgadmin_set_dmc_ipv4(cli_dmc_ipv4_t *dmc_ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: dmc %x port %u ",__FUNCTION__,
	         dmc_ipv4->IPv4,
	         dmc_ipv4->port);

    update_dmc(dmc_ipv4->IPv4,dmc_ipv4->port);

    ret_val = dgwy_ctrl_set_dmc_ipv4(dmc_ipv4->IPv4,
                                     dmc_ipv4->port);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}


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
                                           uint32_t ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
    char IP_string[INET6_ADDRSTRLEN];
    int vlan_id = 0;

	inet_ntop(AF_INET, &ipv4, IP_string, INET_ADDRSTRLEN);

    vlan_id = get_vlan_of_pubalias((char*)"APBR", IP_string);

    if(vlan_id < 0)
    {
        log_error(ServiceUtilLogLevel,
                    "Failed to get vlan info of extvip\n");
        return DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    else if(vlan_id == 0)
    {
        if(-1 == check_add_pubipv4_alias((char*)"APBR", IP_string))
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add EXT IP and subnet first %s\n", IP_string);
            return DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        log_debug(CliLogLevel,
                  "%s: EXT MCAST VNID %x ",__FUNCTION__,
                  ext_mcast_vnid);

        ret_val = dgwy_ctrl_set_ext_mcast_vnid(ext_mcast_vnid,ipv4);
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }
        return status;
    }
    else
    {
        if(-1 == check_add_pubipv4_alias_with_vlan((char*)"APBR", IP_string))
        {
            log_error(ServiceUtilLogLevel,
                        "IP Not enabled: Add EXT IP and subnet first %s\n", IP_string);
            return DOVE_SERVICE_ADD_STATUS_FAIL;
        }

        log_debug(CliLogLevel,
                  "%s: EXT MCAST VNID %x ",__FUNCTION__,
                  ext_mcast_vnid);

        ret_val = dgwy_ctrl_set_ext_mcast_vnid(ext_mcast_vnid,ipv4);
        if(ret_val)
        {
            status = DOVE_SERVICE_ADD_STATUS_FAIL;
        }
        return status;
    }
}


dove_status api_dgadmin_unset_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                             uint32_t ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;

	log_debug(CliLogLevel,
	         "%s: EXT MCAST VNID %u ",__FUNCTION__,
             ext_mcast_vnid);

    ret_val = dgwy_ctrl_unset_ext_mcast_vnid(ext_mcast_vnid,ipv4);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}




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
dove_status api_dgadmin_set_peer_ipv4(cli_peer_ipv4_t *peer_ipv4)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: Service %s, peer IPV4 %u ",__FUNCTION__,
	         peer_ipv4->bridge_name,
	         peer_ipv4->IPv4);

    ret_val = dgwy_ctrl_set_peer_ipv4(peer_ipv4->bridge_name,
                                      peer_ipv4->IPv4);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}


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
dove_status api_dgadmin_add_vnid_subnetv4(cli_vnid_subnet_t *vnid_subnet)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
              "%s: VNID %x, IPV4 0x%x shared %d ",
              __FUNCTION__,
              vnid_subnet->vnid,
              vnid_subnet->IPv4,
              vnid_subnet->subnet_mode);

    ret_val = dgwy_ctrl_add_vnid_info(vnid_subnet->vnid,
                                      vnid_subnet->IPv4,
                                      vnid_subnet->IPv4_mask,
                                      vnid_subnet->IPv4_nexthop,
                                      vnid_subnet->subnet_mode);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}

dove_status api_dgadmin_add_extshared_vnids(int vnid, int domain, int extmcast)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;

    ret_val = dgwy_ctrl_extshared_vnids(vnid, domain, 
                                        extmcast, CMD_ADD_EXT_SHARED);

	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}

dove_status api_dgadmin_del_extshared_vnids(int vnid)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;

    ret_val = dgwy_ctrl_extshared_vnids(vnid, 0, 0,
                                        CMD_DEL_EXT_SHARED);

	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }
    return status;
}

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
dove_status api_dgadmin_del_vnid_subnetv4(cli_vnid_subnet_t *vnid_subnet)
{
    dove_status status = DOVE_STATUS_OK;
    int ret_val=0;
	log_debug(CliLogLevel,
	         "%s: VNID %d, IPV4 0x%x shared %d ",
             __FUNCTION__,
             vnid_subnet->vnid,
             vnid_subnet->IPv4,
	         vnid_subnet->subnet_mode);

    ret_val = dgwy_ctrl_del_vnid_info(vnid_subnet->vnid,
                                      vnid_subnet->IPv4,
                                      vnid_subnet->IPv4_mask,
                                      vnid_subnet->IPv4_nexthop,
                                      vnid_subnet->subnet_mode);
	if(ret_val)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

    return status;
}


dove_status api_show_all_ext_sessions(ext_sesion_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    memset(sessions,0,sizeof(ext_sesion_dump_t));
    if(show_all_ext_sessions(sessions) < 0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}

dove_status api_show_all_int_sessions(int_session_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    memset(sessions,0,sizeof(int_session_dump_t));
    if(show_all_int_sessions(sessions) < 0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}


dove_status api_show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    memset(sessions,0,sizeof(fwddyn_session_dump_t));
    if(show_all_fwddyn_sessions(sessions) < 0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}

dove_status api_show_all_vnid_stats(dgwy_vnid_stats_t *stats)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    memset(stats,0,sizeof(dgwy_vnid_stats_t));
    if(show_all_vnid_stats(stats) < 0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}

dove_status api_show_all_stats(dgwy_stats_t *stats)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    memset(stats,0,sizeof(dgwy_stats_t));
    if(show_all_stats(stats) < 0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}


dove_status api_set_ovl_port(int port)
{
    dove_status status = DOVE_STATUS_OK;

	log_debug(ServiceUtilLogLevel,"Enter");

    if(dgw_ctrl_set_ovl_port(port)<0)
    {
        status = DOVE_SERVICE_ADD_STATUS_FAIL;
    }

	log_debug(ServiceUtilLogLevel,"Exit");
    return status;
}


void check_del_all_vlanmap(int vnid)
{
    int n=0;
    for(n=SVC_INDX_START; n<MAX_SERVICE_TABLES; n++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[n];
        if(svcp)
        {
            int i=0;
            for(i=0; i<MAX_DVMAP; i++)
            {
                if(svcp->dvList[i].domain==(uint32_t)vnid)
                {
                    dgwy_service_config_t cfg;

                    memset(&cfg,0,sizeof(cfg));
                    cfg.dv_config.cmd = CMD_DEL_DOMAIN_VLAN;
                    memcpy(cfg.name,(char*)"APBR",
                           strlen((char*)"APBR"));
                    cfg.dv_config.domain = vnid;
                    cfg.dv_config.vlans[0] = svcp->dvList[i].vlans[0];
                    dgwy_ctrl_svc_domain_vlans(&cfg);
                }
            }
        }
    }
}

void check_del_all_extvip(int vnid)
{
    int n=0;
    for(n=SVC_INDX_START; n<MAX_SERVICE_TABLES; n++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[n];
        if(svcp)
        {
            int i=0;
            for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
            {
                if((svcp->extipv4List[i].domain==(uint32_t)vnid)&&
                   (svcp->extipv4List[i].extmcastvnid))
                {   
                    char IP_string[INET6_ADDRSTRLEN];
                    dgwy_service_config_t cfg;
                    int tenid=0;int emcast=0;
                    cli_service_extvip_t extvipv4_rem;
                    int vlan_id=0;

                    memset(&extvipv4_rem,0,sizeof(extvipv4_rem));

                    extvipv4_rem.domain = (uint32_t)vnid;
                    extvipv4_rem.vIPv4 = svcp->extipv4List[i].ipv4;
                    extvipv4_rem.port_min = svcp->extipv4List[i].port_start;
                    extvipv4_rem.port_max = svcp->extipv4List[i].port_end;
                    memcpy(extvipv4_rem.bridge_name,"APBR",
                           strlen("APBR"));

	                inet_ntop(AF_INET, &svcp->extipv4List[i].ipv4, 
                              IP_string, INET_ADDRSTRLEN);
                    
                    vlan_id = get_vlan_of_pubalias((char*)"APBR", IP_string);

                    if(vlan_id<=0)
                    {
                        if(is_ipv4_on((char*)"APBR", IP_string))
                        {
                            internal_dgadmin_rem_service_ipv4((char*)"APBR",
                                                              svcp->extipv4List[i].ipv4);

                            check_rem_pubipv4_alias((char*)"APBR", IP_string);
                        }
                    }
                    else
                    {
                        if(is_ipv4_on_vlan((char*)"APBR", IP_string,vlan_id))
                        {
                            internal_dgadmin_rem_service_ipv4((char*)"APBR",
                                                              svcp->extipv4List[i].ipv4);

                            check_rem_pubipv4_alias_with_vlan((char*)"APBR", IP_string, vlan_id);
                        }
                   
                    }

                    memset(&cfg,0,sizeof(cfg));
                    cfg.extipv4_config.cmd = CMD_DEL_EXTVIP;
                    memcpy(cfg.name, (char*)"APBR", strlen((char*)"APBR"));
                    cfg.extipv4_config.extip.ipv4 = svcp->extipv4List[i].ipv4;
                    cfg.extipv4_config.extip.domain = 0; /* no meaning */
                    cfg.extipv4_config.extip.port_start = 0; /* no meaning */
                    cfg.extipv4_config.extip.port_end = 0; /* no meaning */ 
                    dgwy_ctrl_svc_extipv4(&cfg);

                    delete_svc_extipv4(&extvipv4_rem, &tenid, &emcast);
                    if(is_vnid_empty_intenant(tenid))
                    {
                        /* empty: delete extmcast vnid */
                        if(tenid || emcast)
                        {
                            api_dgadmin_unset_ext_mcast_vnid(emcast,0/* ipv4 0 */);
                        }
                    }
                }
            }
        }
    }
}

void check_del_all_fwdrules(int vnid)
{
    int n=0;
    for(n=SVC_INDX_START; n<MAX_SERVICE_TABLES; n++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[n];
        if(svcp)
        {
            cli_service_fwdrule_t fwdrl_rem;
            int i=0;
            for(i=0; i<MAX_FWDRULE; i++)
            {
                if(svcp->fwdruleList[i].domain == (uint32_t)vnid)
                {

                    memset(&fwdrl_rem,0,sizeof(fwdrl_rem));

                    memcpy(fwdrl_rem.bridge_name,
                           "APBR",strlen("APBR"));
                    fwdrl_rem.domain = (uint32_t)vnid;
                    fwdrl_rem.IPv4 = svcp->fwdruleList[i].fwdipv4;
                    fwdrl_rem.IPv4_map = svcp->fwdruleList[i].realipv4;
                    fwdrl_rem.port = svcp->fwdruleList[i].fwdport;
                    fwdrl_rem.port_map = svcp->fwdruleList[i].realport;
                    fwdrl_rem.protocol = svcp->fwdruleList[i].fwdproto;

                    api_dgadmin_del_service_fwdrule_force(&fwdrl_rem);
                
                }
            }
        }
    }
}

void check_del_all_subnets(int vnid)
{
    int n=0;
    for(n=SVC_INDX_START; n<MAX_SERVICE_TABLES; n++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[n];
        if(svcp)
        {
            int i=0;
            for(i=0; i<MAX_VNID_SUBNETS; i++)
            {
                if(svcp->vnid_subnet[i].vnid == (uint32_t)vnid)
                {
                    cli_vnid_subnet_t vnid_subnet;
                    memset(&vnid_subnet,0,sizeof(vnid_subnet));
                    vnid_subnet.vnid = (uint32_t)vnid;
                    vnid_subnet.IPv4 = svcp->vnid_subnet[i].IPv4;
                    vnid_subnet.IPv4_mask = 0;
                    vnid_subnet.IPv4_nexthop = 0;
                    vnid_subnet.subnet_mode = 0;
                    api_dgadmin_del_vnid_subnetv4(&vnid_subnet);
                }
            }
        }
    }
}


void check_del_all_extshared(int vnid)
{
    int n=0;
    for(n=SVC_INDX_START; n<MAX_SERVICE_TABLES; n++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[n];
        if(svcp)
        {
            int i=0;
            for(i=0; i<MAX_DOMIANS; i++)
            {
                if(svcp->extsharedVnidList[i].vnid == (uint32_t)vnid)
                {
                    dgwy_ctrl_extshared_vnids(svcp->extsharedVnidList[i].vnid,
                                              0,0, CMD_DEL_EXT_SHARED);
                    return;
                }
            }
        }
    }
}


dove_status api_dgadmin_del_vnid(int vnid, int domainid)
{
    dove_status status = DOVE_STATUS_OK;

    check_del_all_fwdrules(vnid);
    check_del_all_extshared(vnid);
    check_del_all_extvip(vnid);
    check_del_all_subnets(vnid);
    check_del_all_vlanmap(vnid);

    return status;
}

dove_status api_dgadmin_del_extmcastvnid(int vnid, int domainid)
{
    dove_status status = DOVE_STATUS_OK;

    if(is_vnid_empty_intenant(domainid))
    {
        dgwy_ctrl_unset_ext_mcast_vnid(vnid,0);
    }

    return status;
}

dove_status dgwy_api_ctrl_reset_stats(void)
{
    dove_status status = DOVE_STATUS_OK;
    if(dgwy_ctrl_reset_stats() < 0)
    {
        status = DOVE_STATUS_ERROR;
    }
    return status;
}



