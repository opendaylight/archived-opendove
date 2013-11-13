/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#ifdef DGAMIN_EXECUTABLE

#include "include.h"

extern int dgwy_ctrl_set_global(uint8_t cmd);
extern int dgwy_getinfo(void);
/*
extern int dgwy_ctrl_listener(void);
extern int dgwy_ctrl_listener_test(void);
*/
extern int chk_start_svc(char *name);
extern int chk_stop_svc(char *name);
extern int chk_add_port(char *brname,char *port_name);
extern int chk_rem_port(char *brname,char *port_name);
extern int dgwy_nl_send_message(struct nl_msg *msg, nl_recvmsg_msg_cb_t func, void *arg);
extern int dgwy_ctrl_set_svc_type(uint8_t cmd, char *name, dgwy_type_t svc_type);
extern int dgwy_ctrl_set_svc_mtu(uint8_t cmd, char *name, uint16_t mtu);
extern int dgwy_ctrl_svc_domains(dgwy_ext_service_t *svcp);
extern int dgwy_ctrl_svc_domain_vlans(dgwy_ext_service_t *svcp);
extern int dgwy_ctrl_svc_macs(dgwy_ext_service_t *svcp);
extern int dgwy_ctrl_svc_ipv4(dgwy_ext_service_t *svcp);
extern int chk_mac_rem_bridge(char *brname, uint8_t *hwaddr);
extern int chk_add_svc_ipv4(char *name, char *svc_ifaddr, char *svc_addr_mask);
extern int chk_rem_svc_ipv4(char *name, char *svc_ifaddr);
extern int dgwy_ctrl_svc_extipv4(dgwy_ext_service_t *svcp);
extern int dgwy_ctrl_svc_intipv4(dgwy_ext_service_t *svcp);
extern int is_ipv4_on(char *name, char *svc_ifaddr);
extern int dgwy_ctrl_fwd_rule(dgwy_ext_service_t *svcp);
extern int dgwy_ctrl_location(dgwy_ext_service_t *svcp);
extern dove_status api_dgadmin_init_appliance(void);

extern unsigned char* mactobyte (const char *macaddr, unsigned char* byteaddr);
extern void* dgwy_cb_func;


extern struct nl_handle *listener_sock;
extern int char_dev_exit(void);
extern int dgwy_chardev_fd;

uint16_t g_domain_index=0;
uint16_t g_ifmac_index=0;
uint16_t g_ifipv4_index=0;
uint16_t g_extvip_index=0;
uint16_t g_intvip_index=0;
uint16_t g_fwdrule_index=0;

typedef struct cmdfmt {
    char command[32];
    char help[128];
} cmdfmt_t;

static const cmdfmt_t cmdlist[] = {
/* 0*/    {"add-service",   " Add new gateway service "},
/* 1*/    {"set-service",   " Set gateway service properties"},
/* 2*/    {"delete-service"," Delete gateway service"},
/* 3*/    {"type",          " Gatway service type external (or) vlan"},
/* 4*/    {"mtu",           " Gatway service mtu"},
/* 5*/    {"set-domain",    " Add gateway to Domains"},
/* 6*/    {"delete-domain", " Delete gateway from domains"},
/* 7*/    {"add-ifmac",     " Add interface macs to service"},
/* 8*/    {"delete-ifmac",  " Delete interface macs from service"},
/* 9*/    {"add-addressv4", " Add IPV4 address of service"},
/*10*/    {"delete-addressv4"," Delete IPV4 address of service"},
/*11*/    {"add-extvip",    " Add external virtual IPV4 address "},
/*12*/    {"delete-extvip", " Delete external virtual IPV4 address "},
/*13*/    {"add-fwdrule",   " Add forward rule to export services"},
/*14*/    {"set-fwdrule",   " Set forward rule to export services"},
/*15*/    {"delete-fwdrule"," Delete forward rule "},
/*16*/    {"CMD_ADD_INFO_RESP", " Internal command"},
/*17*/    {"add-overlayvip"," Add overlay virtual IPV4 address "},
/*18*/    {"delete-overlayvip"," Delete overlay virtual IPV4 address "},
/*19*/    {"set-location"," Set overlay endpoint location IPV4 address "},
/*20*/    {"delete-location"," Delete overlay endpoint location IPV4 address "},
/*21*/    {"start"," Start packet Processing "},
/*22*/    {"stop"," Stop packet Processing "},
/*23*/    {"set-vlan-domain", "Set domain vlan mapping"},
/*24*/    {"delete-vlan-domain", "Delete domain vlan mapping"},
/*25*/    {"none"," NONE"},
};

cmds_t get_cmd_type(char *argStr)
{
    int i=0;
    for(i=0; i<CMD_NONE; i++)
    {
        if(strlen(argStr) >= strlen(cmdlist[i].command))
        {
            if(!(memcmp(cmdlist[i].command, argStr, strlen(cmdlist[i].command))))
            {
                return i;
            }
        }
    }
    return CMD_NONE;
}

int handle_add_svc(void);

int g_arg_pos=0;
char *get_next(char **argv)
{
    char *argStr = (char *) *(argv+g_arg_pos);
    if(argStr)
    {
        g_arg_pos++;
        return argStr;
    }
    return NULL;
}


int process_svc_mtu(dgwy_ext_service_t *svcp,
                    char *svc_mtu)
{
    if(!svc_mtu)
    {
        printf("Error: invalid svc mtu \n");
        return 0;
    }
    else if(strlen(svc_mtu) < 4)
    {
        printf("Error: invalid svc mtu \n");
        return 0;
    }
    else
    {
        svcp->mtu = atoi(svc_mtu);

        if((svcp->mtu < 512)||
           (svcp->mtu > 64*1024))
        {
            printf("Error: invalid svc mtu \n");
            return 0;
        }

        dgwy_ctrl_set_svc_mtu(svcp->cmd, svcp->name, svcp->mtu);
    }
    return 1;

}


int process_svc_type(dgwy_ext_service_t *svcp,
                     char *svc_type)
{
    if(!svc_type)
    {
        printf("Error: invalid svc type \n");
        return 0;
    }
    else if(strlen(svc_type) < 4)
    {
        printf("Error: invalid svc type \n");
        return 0;
    }
    else
    {
        int ret=0;
        if(!(memcmp(svc_type,"gw-e",4)))
        {
            svcp->type = DGWY_TYPE_EXTERNAL;
        }
        else if(!(memcmp(svc_type,"gw-v",4)))
        {
            svcp->type = DGWY_TYPE_VLAN;
        }
        else
        {
            svcp->type = 0;
            printf("Error: invalid svc type \n");
            return 0;
        }
        ret = dgwy_ctrl_set_svc_type(svcp->cmd, svcp->name, svcp->type);
        printf("ctrl_set_svc_type ret=%d\n",ret);
    }
    return 1;

}


int process_svc_domain(dgwy_ext_service_t *svcp,
                       char *svc_domain)
{
    int ret=0;
    if(!svc_domain)
    {
        printf("Error: invalid svc domain\n");
        return 0;
    }
    else
    {
        char *p = strtok(svc_domain,",");
        while(p)
        {
            svcp->dlist.domainid[g_domain_index] = atoi(p);
            p = strtok(NULL,",");        
            g_domain_index++;
        }    
        ret = dgwy_ctrl_svc_domains(svcp);
        printf("ctrl_set_svc_domain ret=%d\n",ret);
    }
    return 1;
}


int process_svc_set_domain_vlan(dgwy_ext_service_t *svcp,
                                char *svc_domain,
                                char *svc_vlans)
{
    char *p = strtok(svc_vlans,",");
    int i = 0;
    
    svcp->dvlist.domain = atoi(svc_domain);

    while(p)
    {
        svcp->dvlist.vlans[i] = atoi(p);
        p = strtok(NULL,",");
        i++;
    }

    dgwy_ctrl_svc_domain_vlans(svcp);

    return 1;
}


int process_svc_ifmacs(dgwy_ext_service_t *svcp,
                       char *svc_ifmacs)
{
    if(!svc_ifmacs)
    {
        printf("Error: invalid svc domain\n");
        return 0;
    }
    else
    {
        char *p = strtok(svc_ifmacs,",");
        while(p)
        {
            if(mactobyte(p, svcp->ifmaclist.ifmac[g_ifmac_index].mac))
            {
                if(svcp->ifmaclist.ifmac[g_ifmac_index].mac[0] |
                   svcp->ifmaclist.ifmac[g_ifmac_index].mac[1] |
                   svcp->ifmaclist.ifmac[g_ifmac_index].mac[2] |
                   svcp->ifmaclist.ifmac[g_ifmac_index].mac[3] |
                   svcp->ifmaclist.ifmac[g_ifmac_index].mac[4] |
                   svcp->ifmaclist.ifmac[g_ifmac_index].mac[5] )
                {
                    if(svcp->ifmaclist.cmd == CMD_ADD_IFMAC)
                    {
                        chk_mac_add_bridge(svcp->name, 
                                           svcp->ifmaclist.ifmac[g_ifmac_index].mac);
                    }
                    else
                    {
                        chk_mac_rem_bridge(svcp->name, 
                                           svcp->ifmaclist.ifmac[g_ifmac_index].mac);
                    }
                }

                p = strtok(NULL,",");        
                g_ifmac_index++;
            }
            else
            {
                printf("Error: invalid svc domain\n");
                return 0;
            }
            
        }    
        dgwy_ctrl_svc_macs(svcp);
    }
    return 1;
}

int process_svc_ipv4addr(dgwy_ext_service_t *svcp,
                         char *svc_ifaddr, char *svc_addr_mask)
{
    if(!svc_ifaddr)
    {
        printf("Error: invalid svc IPV4 Address\n");
        return 0;
    }
    else
    {
        char *p = strtok(svc_ifaddr,",");
        while(p)
        {
            struct sockaddr_in ipv4addr;
            inet_aton(p, &ipv4addr.sin_addr);
            svcp->ipv4list.ifipv4[g_ifipv4_index] = ipv4addr.sin_addr.s_addr;
            if(svcp->ipv4list.cmd == CMD_ADD_IPV4)
            {
                chk_add_svc_ipv4(svcp->name, p, svc_addr_mask);
            }
            else
            {
                chk_rem_svc_ipv4(svcp->name, p);
            }
            p = strtok(NULL,",");        
            g_ifipv4_index++;
        }    
        dgwy_ctrl_svc_ipv4(svcp);
    }
    return 1;
}
 
int process_svc_extvip(dgwy_ext_service_t *svcp,
                       char *svc_extvip, char *port_range)
{
    if(!svc_extvip)
    {
        printf("Error: invalid svc IPV4 Address\n");
        return 0;
    }
    else
    {
        char *p = strtok(svc_extvip,",");
        while(p)
        {
            if(is_ipv4_on(svcp->name, p))
            {
                struct sockaddr_in ipv4addr;
                inet_aton(p, &ipv4addr.sin_addr);
                svcp->extipv4list.extip[g_extvip_index].ipv4 = ipv4addr.sin_addr.s_addr;
                svcp->extipv4list.extip[g_extvip_index].port_start = 0;
                svcp->extipv4list.extip[g_extvip_index].port_end = 0;
                g_extvip_index++;
            }
            else
            {
                printf("Error: Add %s to interface-address list first\n",p);
            }
            p = strtok(NULL,",");        
        }    
        if(port_range)
        {
            int i=0;
            int cnt = g_extvip_index;
            p = strtok(port_range,"-");
            if(p)
            {
                uint16_t start = atoi(p);
                uint16_t end   = 0;
                p = strtok(NULL,"-"); 
                if(p)
                {
                    end = atoi(p);
                }
                else
                {
                    end = start;
                }
                for(i=0; i<cnt; i++)
                {
                    svcp->extipv4list.extip[i].port_start = start;
                    svcp->extipv4list.extip[i].port_end   = end;
                    printf("Add range %d-%d\n",start,end);
                }
            }
        }
        dgwy_ctrl_svc_extipv4(svcp);
    }
    return 1;
}

int process_svc_intvip(dgwy_ext_service_t *svcp,
                       char *svc_intvip, char *port_range)
{
    if(!svc_intvip)
    {
        printf("Error: invalid svc IPV4 Address\n");
        return 0;
    }
    else
    {
        char *p = strtok(svc_intvip,",");
        while(p)
        {
            struct sockaddr_in ipv4addr;
            inet_aton(p, &ipv4addr.sin_addr);
            svcp->intipv4list.intip[g_intvip_index].ipv4 = ipv4addr.sin_addr.s_addr;
            svcp->intipv4list.intip[g_intvip_index].port_start = 0;
            svcp->intipv4list.intip[g_intvip_index].port_end = 0;
            g_intvip_index++;
        
            p = strtok(NULL,",");        
        }    
        if(port_range)
        {
            int i=0;
            int cnt = g_intvip_index;
            p = strtok(port_range,"-");
            if(p)
            {
                uint16_t start = atoi(p);
                uint16_t end   = 0;
                p = strtok(NULL,"-"); 
                if(p)
                {
                    end = atoi(p);
                }
                else
                {
                    end = start;
                }
                for(i=0; i<cnt; i++)
                {
                    svcp->intipv4list.intip[i].port_start = start;
                    svcp->intipv4list.intip[i].port_end   = end;
                    printf("Add range %d-%d\n",start,end);
                }
            }
        }
        dgwy_ctrl_svc_intipv4(svcp);
    }
    return 1;
}


int process_fwd_rule(dgwy_ext_service_t *svcp,char *svc_fwdaddr,
                     char *svc_proto, char *svc_fwdport,
                     char *svc_mapto_ipv4, char *svc_mapport,
                     char *svc_fwddomain)
{
    if(!svc_fwdaddr)
    {
        printf("Error: invalid fwdrule  IPV4 Address\n");
        return 0;
    }
    else
    {
        struct sockaddr_in ipv4addr;
        if(is_ipv4_on(svcp->name, svc_fwdaddr))
        {
            inet_aton(svc_fwdaddr, &ipv4addr.sin_addr);
            svcp->fwdrule.fwdipv4 = ipv4addr.sin_addr.s_addr;
        }
        else
        {
            printf("Error: Add %s to interface-address list first\n",svc_fwdaddr);
            goto error;
        }
        svcp->fwdrule.fwdproto = atoi(svc_proto);
        if(svcp->fwdrule.fwdproto > 255)
        {
            printf("Error: Invalid protocol: expected <= 255 \n");
            goto error;
        }
        svcp->fwdrule.fwdport = atoi(svc_fwdport);
        inet_aton(svc_mapto_ipv4,&ipv4addr.sin_addr);
        svcp->fwdrule.realipv4 = ipv4addr.sin_addr.s_addr;
        svcp->fwdrule.realport = atoi(svc_mapport);
        svcp->fwdrule.domain   = atoi(svc_fwddomain);

        dgwy_ctrl_fwd_rule(svcp);
        return 1;
    }
error:
    return 0;
}


void dump_service(dgwy_ext_service_t *svcp)
{
    int i=0;
   
    if(strlen(svcp->name)==0)
    {
        printf("INVALID SERVICE\n");
        return;
    }
    printf("SVC CMD  : %d\n", svcp->cmd);
    printf("SVC Name : %s\n", svcp->name);
    printf("SVC type : %d\n", svcp->type);
    printf("SVC DOMAIN:\n");
    printf("DOMAIN CMD: %d\n",svcp->dlist.cmd);
    for(i=0; i<MAX_DOMIANS; i++)
    {
        if(svcp->dlist.domainid[i])
        {
            printf("\tDOMAIN ID: %d\n",svcp->dlist.domainid[i]);
        }
    }

    printf("IF MAC CMD: %d\n",svcp->ifmaclist.cmd);
    for(i=0; i<MAX_IF_MACS; i++)
    {
        printf("\tIF MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
               svcp->ifmaclist.ifmac[i].mac[0],
               svcp->ifmaclist.ifmac[i].mac[1],
               svcp->ifmaclist.ifmac[i].mac[2],
               svcp->ifmaclist.ifmac[i].mac[3],
               svcp->ifmaclist.ifmac[i].mac[4],
               svcp->ifmaclist.ifmac[i].mac[5]);
    }

    printf("IF IPV4 Address CMD: %d\n", svcp->ipv4list.cmd);
    for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
    {
        if(svcp->ipv4list.ifipv4[i])
        {
            printf("\tIPV4 Address: %x\n",svcp->ipv4list.ifipv4[i]);
        }
    }

    printf("EXT-VIP CMD: %d\n", svcp->extipv4list.cmd);
    for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
    {
        if(svcp->extipv4list.extip[i].ipv4)
        {
            printf("\tEXTVIP IPV4: %x\n",svcp->extipv4list.extip[i].ipv4);
        }
    }
}



void parse_add_fwd_rule(char **argv, dgwy_ext_service_t *svcp, cmds_t command)
{
    char *svc_fwdaddr       =  get_next(argv);
    char *svc_fwdprot_str   = get_next(argv);
    char *svc_proto         = NULL;
    char *svc_fwdport_str   = NULL;
    char *svc_fwdport       = NULL;
    char *svc_mapto_str     = NULL;
    char *svc_mapto_ipv4    = NULL;
    char *svc_mapport_str   = NULL;
    char *svc_mapport       = NULL;
    char *svc_fwddomain_str = NULL;

    if((svc_fwdprot_str==NULL)||
       (strlen(svc_fwdprot_str) < 5) ||
       (memcmp(svc_fwdprot_str,"proto",5)))
    {
        printf("Syntax error: need [proto <proto number>]\n"); 
        goto error;
    }

    svc_proto = get_next(argv);
    if(svc_proto == NULL)
    {
        printf("Syntax error: need [proto <proto number>]\n"); 
        goto error;
    }
    svc_fwdport_str = get_next(argv);
    if((svc_fwdport_str==NULL)||
       (strlen(svc_fwdport_str)<4) ||
       (memcmp(svc_fwdport_str,"port",4)))
    {
        printf("Syntax error: need [port <port number>]\n"); 
        goto error;
    }
    svc_fwdport = get_next(argv);
    if(svc_fwdport==NULL)
    {
        printf("Syntax error: need [port <port number>]\n"); 
        goto error;
    }
    svc_mapto_str = get_next(argv);
    if((svc_mapto_str==NULL)||
       (strlen(svc_mapto_str)<5)||
       (memcmp(svc_mapto_str,"mapto",5)))
    {
        printf("Syntax error: need [mapto <ip> port <port number> ]\n"); 
        goto error;
    }
    svc_mapto_ipv4 = get_next(argv);
    if(svc_mapto_ipv4 == NULL)
    {
        printf("Syntax error: need "
               "[mapto <ip> port <port number> ]\n");
        goto error;
    }
    svc_mapport_str = get_next(argv);
    if((svc_mapport_str==NULL)||
       (strlen(svc_mapport_str)<4) ||
       (memcmp(svc_mapport_str,"port",4)))
    {
        printf("Syntax error: need "
               "[mapto <ip> port <port number> ]\n");
        goto error;
    }
    svc_mapport = get_next(argv);
    if(svc_mapport==NULL)
    {
        printf("Syntax error: need [port <port number>]\n"); 
        goto error;
    }
    svc_fwddomain_str = get_next(argv);
    if((svc_fwddomain_str==NULL)||
       (strlen(svc_fwddomain_str)<6)||
       (memcmp(svc_fwddomain_str,"domain",6)))
    {
        printf("Syntax error: need [domain <domain id>]\n"); 
    }
    else
    {
        char *svc_fwddomain = get_next(argv);
        if(svc_fwddomain==NULL)
        {
            printf("Syntax error: need [domain <domain id>]\n"); 
        }
        else
        {
            svcp->fwdrule.cmd = command;
            process_fwd_rule(svcp,svc_fwdaddr,svc_proto,
                             svc_fwdport, svc_mapto_ipv4,
                             svc_mapport, svc_fwddomain);                                                     
        }
    }
error:
    return;
}

void parse_del_fwd_rule(char **argv, dgwy_ext_service_t *svcp, cmds_t command)
{
    char *svc_fwdaddr       = get_next(argv);
    char *svc_fwdprot_str   = get_next(argv);
    char *svc_proto         = NULL;
    char *svc_fwdport_str   = NULL;
    char *svc_fwdport       = NULL;
    /*char *svc_fwddomain_str = NULL;*/

    if((svc_fwdprot_str==NULL)||
       (strlen(svc_fwdprot_str) < 5) ||
       (memcmp(svc_fwdprot_str,"proto",5)))
    {
        printf("Syntax error: need [proto <proto number>]\n"); 
        goto error;
    }

    svc_proto = get_next(argv);
    if(svc_proto == NULL)
    {
        printf("Syntax error: need [proto <proto number>]\n"); 
        goto error;
    }
    svc_fwdport_str = get_next(argv);
    if((svc_fwdport_str==NULL)||
       (strlen(svc_fwdport_str)<4) ||
       (memcmp(svc_fwdport_str,"port",4)))
    {
        printf("Syntax error: need [port <port number>]\n"); 
        goto error;
    }
    svc_fwdport = get_next(argv);
    if(svc_fwdport==NULL)
    {
        printf("Syntax error: need [port <port number>]\n"); 
        goto error;
    }
    
#if 0
    svc_fwddomain_str = get_next(argv);
    if((svc_fwddomain_str==NULL)||
       (strlen(svc_fwddomain_str)<6)||
       (memcmp(svc_fwddomain_str,"domain",6)))
    {
        printf("Syntax error: need [domain <domain id>]\n"); 
    }
    else
    {
        char *svc_fwddomain = get_next(argv);
        if(svc_fwddomain==NULL)
        {
            printf("Syntax error: need [domain <domain id>]\n"); 
        }
        else
        {
            svcp->fwdrule.cmd = command;
            process_fwd_rule(svcp,svc_fwdaddr,svc_proto,
                             svc_fwdport, "0", "0",
                             svc_fwddomain);                                                     
        }
    }
#endif

    svcp->fwdrule.cmd = command;
    process_fwd_rule(svcp,svc_fwdaddr,svc_proto,
                     svc_fwdport, "0", "0", "0");


error:
    return;
}

void process_ovl_location(dgwy_ext_service_t *svcp,
                          char *svc_endpoint, char *svc_domain, 
                          char *svc_end_location, char *svc_end_src_location, 
                          char *svc_ovl_proto, char *svc_ovl_src_port, 
                          char *svc_ovl_dst_port, char *svc_ovl_dst_mac,
                          cmds_t command)
{
    struct sockaddr_in ipv4addr;

    svcp->ovl_location.cmd = command;
    svcp->ovl_location.domain = atoi(svc_domain);

    inet_aton(svc_endpoint, &ipv4addr.sin_addr);
    svcp->ovl_location.endpoint_ip = ipv4addr.sin_addr.s_addr;

    inet_aton(svc_end_location, &ipv4addr.sin_addr);
    svcp->ovl_location.endpoint_location =  ipv4addr.sin_addr.s_addr;

    inet_aton(svc_end_src_location, &ipv4addr.sin_addr);
    svcp->ovl_location.ovl_src_location = ipv4addr.sin_addr.s_addr;

    svcp->ovl_location.ovl_header_proto = atoi(svc_ovl_proto);
    svcp->ovl_location.ovl_src_port = atoi(svc_ovl_src_port);
    svcp->ovl_location.ovl_dst_port = atoi(svc_ovl_dst_port);
    
    mactobyte(svc_ovl_dst_mac, svcp->ovl_location.ovl_dst_mac);

    dgwy_ctrl_location(svcp);
}

void parse_set_ovl_location(char **argv, dgwy_ext_service_t *svcp, cmds_t command)
{
    char *svc_endpoint          = get_next(argv);
    char *svc_domain_str        = get_next(argv);
    char *svc_domain            = NULL;
    char *svc_end_location_str  = NULL;
    char *svc_end_location      = NULL;
    char *svc_end_src_location_str = NULL;
    char *svc_end_src_location  = NULL;
    char *svc_ovl_proto_str     = NULL;
    char *svc_ovl_proto         = NULL;
    char *svc_ovl_src_port_str  = NULL;
    char *svc_ovl_src_port      = NULL;
    char *svc_ovl_dst_port_str  = NULL;
    char *svc_ovl_dst_port      = NULL;
    char *svc_ovl_dst_mac_str   = NULL;
    char *svc_ovl_dst_mac       = NULL;
    
    if((NULL == svc_domain_str)||
       (strlen(svc_domain_str) < 6) ||
       (memcmp(svc_domain_str,"domain",6)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }

    svc_domain = get_next(argv);
    if(NULL == svc_domain)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_end_location_str = get_next(argv);
    if((NULL == svc_end_location_str)||
       (strlen(svc_end_location_str) < 12) ||
       (memcmp(svc_end_location_str,"end-location",12)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_end_location = get_next(argv);
    if(NULL == svc_end_location)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_end_src_location_str = get_next(argv);
    if((NULL==svc_end_src_location_str)||
       (strlen(svc_end_src_location_str) < 16)||
       (memcmp(svc_end_src_location_str,"end-src-location",16)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_end_src_location = get_next(argv);
    if( NULL == svc_end_src_location)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_ovl_proto_str = get_next(argv);
    if((NULL == svc_ovl_proto_str)||
       (strlen(svc_ovl_proto_str)<9)||
       (memcmp(svc_ovl_proto_str,"ovl-proto",9)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_ovl_proto = get_next(argv);
    if(NULL == svc_ovl_proto)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }

    svc_ovl_src_port_str = get_next(argv);
    if((NULL == svc_ovl_src_port_str)||
       (strlen(svc_ovl_src_port_str)<12)||
       (memcmp(svc_ovl_src_port_str,"ovl-src-port",12)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_ovl_src_port = get_next(argv);
    if(NULL == svc_ovl_src_port)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }

    svc_ovl_dst_port_str = get_next(argv);
    if((NULL == svc_ovl_dst_port_str)||
       (strlen(svc_ovl_dst_port_str)<12)||
       (memcmp(svc_ovl_dst_port_str,"ovl-dst-port",12)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-dst-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_ovl_dst_port = get_next(argv);
    if(NULL == svc_ovl_dst_port)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }

    svc_ovl_dst_mac_str = get_next(argv);
    if((NULL == svc_ovl_dst_mac_str)||
       (strlen(svc_ovl_dst_mac_str) < 11)||
       (memcmp(svc_ovl_dst_mac_str,"ovl-dst-mac",11)))
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-dst-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }
    svc_ovl_dst_mac = get_next(argv);
    if(NULL == svc_ovl_dst_mac)
    {
        printf("Syntax error: [set-location <ip> domain <id>"
               " end-location <ip> end-src-location <ip> ovl-proto <tcp>"
               " ovl-src-port <1234> ovl-dst-port <1235> ovl-dst-mac <mac>\n");
        goto error;
    }

    process_ovl_location(svcp, svc_endpoint, svc_domain,
                         svc_end_location, svc_end_src_location,
                         svc_ovl_proto, svc_ovl_src_port, 
                         svc_ovl_dst_port, svc_ovl_dst_mac,
                         command);

error:
    return;
}

void parse_del_ovl_location(char **argv, dgwy_ext_service_t *svcp, cmds_t command)
{
    char *svc_endpoint          = get_next(argv);
    char *svc_domain_str        = get_next(argv);
    char *svc_domain            = NULL;
    
    if((NULL == svc_domain_str)||
       (strlen(svc_domain_str) < 6) ||
       (memcmp(svc_domain_str,"domain",6)))
    {
        printf("Syntax error: [delete-location <ip> domain <id>\n");
        goto error;
    }

    svc_domain = get_next(argv);
    if(NULL == svc_domain)
    {
        printf("Syntax error: [delete-location <ip> domain <id>\n");
        goto error;
    }

    process_ovl_location(svcp, svc_endpoint, svc_domain,
                         "0", "0", "0", "0","0","0",command);

error:
    return;
}



int process_main(int argc, char **argv)
{
    int ret=0;
    char *argStr = (char *) *argv;
    dgwy_ext_service_t svc, *svcp;
    printf("ARGC = %d svcsize=%zu\n", argc, sizeof(svc));

    svcp = &svc;
    memset(svcp, 0, sizeof(dgwy_ext_service_t));

    g_arg_pos=0;

    while(1)
    {
        argStr = get_next(argv);
        if(argStr == NULL)
        {
            break;
        }
        else
        {
            cmds_t cmdidx = get_cmd_type(argStr);
            printf("%d: %s : %s\n",cmdidx,
                   cmdlist[cmdidx].command, cmdlist[cmdidx].help);
            switch(cmdidx)
            {
                case CMD_ADD_SVC:
                {
                    printf("Handle SVC Add\n");
                    char *svc_name = get_next(argv);
                    if(!svc_name)
                    {
                        printf("Error: invalid svc name \n");
                    }
                    else
                    {
                        memcpy(svcp->name, svc_name,
                               (strlen(svc_name)<SVC_NAME)?
                                (strlen(svc_name)):(SVC_NAME-1));
                        svcp->cmd = CMD_ADD_SVC;
                        if(chk_start_svc(svcp->name) == 0)
                        {
                            /* No service */
                            printf("Error: Failed to start service %s\n",
                                   svcp->name);
                        }
                    }
                    break;
                }
                case CMD_SET_SVC:
                {
                    printf("Handle SVC SET\n");
                    char *svc_name = get_next(argv);
                    if(!svc_name)
                    {
                        printf("Error: invalid svc name \n");
                    }
                    else
                    {
                        memcpy(svcp->name, svc_name,
                               (strlen(svc_name)<SVC_NAME)?
                                (strlen(svc_name)):(SVC_NAME-1));
                        svcp->cmd = CMD_SET_SVC;
                        if(chk_start_svc(svcp->name) == 0)
                        {
                            /* No service */
                            printf("Error: Failed to set service %s\n",
                                   svcp->name);
                        }
                    }
                
                    break;
                }
                case CMD_DEL_SVC:
                {
                    printf("Handle SVC DEL\n");
                    char *svc_name = get_next(argv);
                    if(!svc_name)
                    {
                        printf("Error: invalid svc name \n");
                    }
                    else
                    {
                        memcpy(svcp->name, svc_name,
                               (strlen(svc_name)<SVC_NAME)?
                                (strlen(svc_name)):(SVC_NAME-1));
                        svcp->cmd = CMD_DEL_SVC;
                        chk_stop_svc(svcp->name);
                    }
                    break;
                }
                case CMD_TYPE:
                {
                    char *svc_type = get_next(argv);
                    printf("Handle SVC type\n");
                    svcp->cmd = CMD_SET_SVC;
                    process_svc_type(svcp, svc_type);
                    break;
                }
                case CMD_MTU:
                {
                    char *svc_mtu = get_next(argv);
                    printf("Handle SVC type\n");
                    svcp->cmd = CMD_MTU;
                    process_svc_mtu(svcp, svc_mtu);
                    break;
                }
                case CMD_SET_DOMAIN:
                {
                    char *svc_domain = get_next(argv);
                    printf("Handle SVC Set domain\n");
                    g_domain_index=0;
                    svcp->dlist.cmd = CMD_SET_DOMAIN;
                    process_svc_domain(svcp, svc_domain);
                    break;
                }
                case CMD_DEL_DOMAIN:
                {
                    char *svc_domain = get_next(argv);
                    printf("Handle SVC delete domain\n");
                    svcp->dlist.cmd = CMD_DEL_DOMAIN;
                    g_domain_index=0;
                    process_svc_domain(svcp, svc_domain);
                    break;
                }
                case CMD_ADD_IFMAC:
                {
                    char *svc_ifmac = get_next(argv);
                    g_ifmac_index=0;
                    printf("Handle SVC add mac %s\n", svc_ifmac);
                    svcp->ifmaclist.cmd = CMD_ADD_IFMAC;
                    process_svc_ifmacs(svcp, svc_ifmac);
                    break;
                }
                case CMD_DEL_IFMAC:
                {
                    char *svc_ifmac = get_next(argv);
                    g_ifmac_index=0;
                    printf("Handle SVC del mac\n");
                    svcp->ifmaclist.cmd = CMD_DEL_IFMAC;
                    process_svc_ifmacs(svcp, svc_ifmac);
                    break;
                }
                case CMD_ADD_IPV4:
                {
                    char *svc_ifaddr = get_next(argv);
                    char *svc_mask_str = get_next(argv);
                    if((svc_mask_str == NULL) ||
                       (memcmp(svc_mask_str,"mask",4)))
                    {
                        printf("Syntax error: need ipv4 mask\n");
                    }
                    else
                    {
                        char *svc_addr_mask = get_next(argv);
                        if(svc_addr_mask == NULL)
                        {
                            printf("Syntax error: need ipv4 mask\n");
                        }
                        else
                        {
                            printf("Handle Add IPV4\n");
                            g_ifipv4_index=0;
                            memset(svcp->ipv4list.ifipv4,0,sizeof(svcp->ipv4list.ifipv4));
                            svcp->ipv4list.cmd = CMD_ADD_IPV4;
                            process_svc_ipv4addr(svcp,svc_ifaddr,svc_addr_mask);
                        }
                    }
                    break;
                }
                case CMD_DEL_IPV4:
                {
                    char *svc_ifaddr = get_next(argv);
                    printf("Handle Delete IPV4\n");
                    g_ifipv4_index=0;
                    svcp->ipv4list.cmd = CMD_DEL_IPV4;
                    process_svc_ipv4addr(svcp,svc_ifaddr,NULL);
                    break;
                }
                case CMD_ADD_EXTVIP:
                {
                    char *svc_extaddr = get_next(argv);
                    char *port_range  = get_next(argv);
                    printf("Handle Add EXT IPV4\n");
                    g_extvip_index=0;
                    memset(svcp->extipv4list.extip, 0, sizeof(svcp->extipv4list.extip));
                    svcp->extipv4list.cmd = CMD_ADD_EXTVIP;
                    process_svc_extvip(svcp, svc_extaddr, port_range);
                    break;
                }
                case CMD_DEL_EXTVIP:
                {
                    char *svc_extaddr = get_next(argv);
                    printf("Handle Delete EXT IPV4\n");
                    g_extvip_index=0;
                    svcp->extipv4list.cmd = CMD_DEL_EXTVIP;
                    process_svc_extvip(svcp, svc_extaddr, NULL);
                    break;
                }
                case CMD_ADD_INTVIP:
                {
                    char *svc_intaddr = get_next(argv);
                    char *port_range  = get_next(argv);
                    printf("Handle Add OVERLAY IPV4\n");
                    g_intvip_index=0;
                    memset(svcp->intipv4list.intip, 0, sizeof(svcp->intipv4list.intip));
                    svcp->intipv4list.cmd = CMD_ADD_INTVIP;
                    process_svc_intvip(svcp, svc_intaddr, port_range);
                    break;
                }
                case CMD_DEL_INTVIP:
                {
                    char *svc_intaddr = get_next(argv);
                    printf("Handle Delete OVERLAY IPV4\n");
                    g_intvip_index=0;
                    svcp->intipv4list.cmd = CMD_DEL_INTVIP;
                    process_svc_intvip(svcp, svc_intaddr, NULL);
                    break;
                }
                case CMD_ADD_FWD_RULE:
                {
                    parse_add_fwd_rule(argv, svcp, CMD_ADD_FWD_RULE);
                    printf("Handle Add FWD Rule\n");
                    break;
                }
                case CMD_SET_FWD_RULE:
                {
                    printf("Handle SET FWD Rule: Not available.\n");
                    /*parse_add_fwd_rule(argv, svcp, CMD_SET_FWD_RULE);*/
                    break;
                }
                case CMD_DEL_FWD_RULE:
                {
                    parse_del_fwd_rule(argv, svcp, CMD_DEL_FWD_RULE);
                    printf("Handle Delete FWD Rule\n");
                    break;
                }
                case CMD_SET_OVL_IP:
                {
                    parse_set_ovl_location(argv, svcp, CMD_SET_OVL_IP);
                    printf("Handle set ovl location\n");
                    break;
                }
                case CMD_DEL_OVL_IP:
                {
                    parse_del_ovl_location(argv, svcp, CMD_DEL_OVL_IP);
                    printf("Handle delete ovl location\n");
                    break;
                }
                case CMD_START:
                {
                    dgwy_ctrl_set_global(CMD_START);
                    break;
                }
                case CMD_STOP:
                {
                    dgwy_ctrl_set_global(CMD_STOP);
                    break;
                }
                case CMD_SET_DOMAIN_VLAN:
                {
                    char *svc_domain = get_next(argv);
                    char *svc_vlans  = get_next(argv);
                    printf("Handle SVC Set domain Vlans\n");
                    g_domain_index=0;
                    svcp->dvlist.cmd = CMD_SET_DOMAIN_VLAN;
                    process_svc_set_domain_vlan(svcp, 
                                                svc_domain,
                                                svc_vlans);
                    break;
                }
                case CMD_DEL_DOMAIN_VLAN:
                {
                    char *svc_domain = get_next(argv);
                    char *svc_vlans  = get_next(argv);
                    printf("Handle SVC Del domain Vlans\n");
                    g_domain_index=0;
                    svcp->dvlist.cmd = CMD_DEL_DOMAIN_VLAN;
                    process_svc_set_domain_vlan(svcp, 
                                                svc_domain,
                                                svc_vlans);

                    break;
                }
                case CMD_NONE:
                {
                    printf("Handle NONE\n");
                
                    break;
                }
                default:
                {
                    printf("Error : Un known input\n");
                }
            }/* switch */

        }
    }/* while 1 */

    if(1)
    {
        int i =0;
        struct pollfd ufds[1];
        int rv =0;
        
        ufds[0].fd = dgwy_chardev_fd;
        ufds[0].events = POLLIN;

        for(i=0; i<5; i++)
        {
            if(dgwy_chardev_fd >= 0)
            {
                printf("polling %d\n", i);
                rv = poll(ufds, 1, 1000);
                if (rv == -1) 
                {
                    perror("poll"); // error occurred in poll()
                }
                else if (rv == 0) 
                {
                    printf("Timeout occurred!  No data after 3.5 seconds.\n");
                }
                else 
                {
                    int ret = 0;
                    int sdf = 0;
	                char buf[DGWY_IOCTL_MAX_SZ];
                    /*
                    if((ret=ioctl(dgwy_chardev_fd, DGWY_READ_IOCTL, buf)) < 0)
                    {
                        perror("second ioctl");
                    }
                    */
                    ret = read(dgwy_chardev_fd,buf,DGWY_IOCTL_MAX_SZ);
                    printf("message: %d\n", ret);
                    while(ret>0)
                    {
                        dgwy_dps_query_list_info_t *info=
                            (dgwy_dps_query_list_info_t*)(&buf[sdf]);
                        printf("info>ip %d domain %d\n", info->ip4, 
                                info->domain);
                        sdf += sizeof(dgwy_dps_query_list_info_t);
                        ret -= sizeof(dgwy_dps_query_list_info_t);
                    }
                }
                sleep(5);
            }
        }
    }
    dump_service(svcp);
    return ret;
}

#if 0
/*
 ******************************************************************************
 * dps_protocol_send_to_client --                                         *//**
 *
 * \brief This routine MUST be called for (calculation purposes) for every
 *        random test answer sent from the DPS Client
 *
 * \param[in] dps_obj The dps_client_data_t associated with the reply/request
 *
 * \retval None
 */

void dps_protocol_send_to_client(dps_client_data_t *msg)
{
	return;
}
#endif

int dgwy_ctrl_init(void)
{
    dgwy_cb_func = dgwy_ctrl_init;

    if(dgwy_nl_send_message(NULL,NULL,NULL)==0)
    {
		return dgwy_getinfo();
	}
	return -1;
}
		
int main(int argc, char **argv)
{
    int result;

    if(dgwy_ctrl_init() != 0)
    {
        printf(" Error: Netlink Init error \n");
        return 1;
    }

    log_info(ServiceUtilLogLevel,
             "INIT APPLIANCE ");
    api_dgadmin_init_appliance();
    result = process_main(argc, argv);

	char_dev_exit();
    return result;
}



#endif /* (DGAMIN_EXECUTABLE) */



