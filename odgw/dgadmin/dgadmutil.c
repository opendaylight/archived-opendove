/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */
 


#include "include.h"
#include "dgwy_extern.h"

/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 **
 * \ingroup DPSServerCLIConfig
 * @{
 */

#define BRIDGE_NAME_MAX 16
#define mcast_dps_resp_offsetof(_type, _member) ((size_t) &((_type *)0)->_member)

int ServiceUtilLogLevel = DPS_SERVER_LOGLEVEL_INFO;

void* dgwy_cb_func = NULL;
static struct nl_handle *get_info_sock = NULL;
struct nl_handle *listener_sock = NULL;
static int family;
uint8_t g_target_mac[6];
uint8_t g_target_ifname[32];
uint16_t g_svc_mtu=64;
uint8_t init_done=0;
int dgwy_chardev_fd = -1;
int stat_ctx_alloc = 0;
uint32_t dps_ctx_quid = 0;
uint8_t g_tunnel_reg_status=0;
uint32_t g_config_version=0;
/*uint8_t gMAC[6]={0x0,0x18,0xb1,0xaa,0xaa,0x01};*/
uint8_t gMAC[6]={0x0,0x0,0x0,0x0,0x0,0x0};

int dgwy_ctrl_set_dmc_ipv4(uint32_t IPV4, uint16_t port);
int dgwy_ctrl_location(dgwy_service_config_t *svcp);
int update_svc_table_type(char *name, dgwy_type_t svc_type);
int update_svc_table_domain(char *name, uint32_t domain);
int update_svc_table_macs(char *name, ifmac_t *ifmac);
int update_svc_table_ifipv4(char *name, uint32_t ipv4, uint32_t mask, uint32_t nexthop, int vlan_id);
int update_svc_table_extipv4(char *name, extipv4_t *extip);
int update_svc_table_intipv4(char *name, intipv4_t *intip);
int update_svc_table_fwdrule(char *name, fwd_rule_t *fwdrule);
int update_svc_table_dvlist(char *name, domain_vlan_cfg_t *dvlcfg);
int update_svc_table_mtu(char *name, uint16_t mtu);
void check_dregister_tunnel(uint32_t vnid);
int chk_self_ipv4(char *ipchk);
int chk_self_ipv4_getmask(char *ipchk, uint32_t *mask);
int dgwy_del_dps_server(uint32_t dpsIp);
void check_register_tunnel(void);
void check_del_tep(void);
int dgwy_ctrl_ext_mcast_master_state(uint32_t ext_mcast_vnid,
                                     uint32_t ipv4, uint32_t master);

/* GLOBAL service table */
dgwy_service_list_t GLB_SVC_TABLE[MAX_SERVICE_TABLES];
pthread_mutex_t GLB_SVC_TABLE_LOCK;
pthread_mutex_t GLB_ROUTE_LOCK; /* used to serialize route operations */
uint8_t GLB_STATE = 0; /* OFF */
uint16_t OVL_PROTO_DST_PORT = 8472 ;  /* OVL DST PORT */ 

dgwy_service_cfg_t fileCfg;
dgwy_dps_server_t dpsCfg;
uint32_t g_dovenet_ipv4=0;
uint32_t g_dovenet_ipv4_nexthop=0;
uint32_t g_extif_ipv4=0;
uint32_t g_extif_ipv4_nexthop=0;
uint32_t g_mgmt_ipv4=0;
uint32_t g_dmc_ipv4=0;
uint16_t g_dmc_port=0;
char g_node_uuid[40];
char g_built_version[64];
uint16_t g_dgwy_rest_port=1889;
uint8_t g_dmc_register_done=0;
uint8_t g_APPBRIDGE_type=0;
uint8_t g_got_dcs_info=0;

extern void dgwy_dmc_registration(void);
extern void dgwy_get_dcs_seed_info(void);
extern dove_status dgadm_read_svc_app_uuid(void);
extern int g_dgwconfig_curversion;
extern int is_mac_found_in_peer(char * mac);
extern int is_vnid_empty_intenant(uint32_t tenantid);
extern
dove_status api_dgadmin_unset_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                             uint32_t ipv4);

extern int
update_mgmt_ipv4(int mode, uint32_t ipv4, 
                 uint32_t ipv4_mask, uint32_t nexthop);

extern void show_dmc_info(void);

static uint32_t nexthop_list[250];

/*
static unsigned long long get_timestamp ()
{
    struct timeval now;
    gettimeofday (&now, NULL);
    return  now.tv_usec + ( unsigned long long)now.tv_sec * 1000000;
}
*/


static inline unsigned compare_ether_addr(const uint8_t *addr1, const uint8_t *addr2)
{
    const uint16_t *a = (const uint16_t *) addr1;
    const uint16_t *b = (const uint16_t *) addr2;

    return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) != 0;
}

unsigned char* mactobyte 
(const char *macaddr, unsigned char* byteaddr)
{
    char macsep='-';
    char macsep1=':';

    int i=0;
    for (i = 0; i < 6; ++i)
    {
        unsigned int num = 0;
        char ch;

        ch = tolower (*macaddr++);

        if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
        {
            return NULL;
        }

        num = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
        ch = tolower (*macaddr);

        if ((i < 5 && ((ch != macsep) && (ch != macsep1))) || 
            (i == 5 && ch != '\0' && !isspace (ch)))
        {
            ++macaddr;

            if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
            {
                return NULL;
            }

            num <<= 4;
            num += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
            ch = *macaddr;

            if ((i < 5 && ((ch != macsep) && (ch != macsep1))))
            {
                return NULL;
            }
        }
        byteaddr[i] = (unsigned char) num;
        ++macaddr;
    }

    log_debug(ServiceUtilLogLevel," > %02x:%02x:%02x:%02x:%02x:%02x <\n",
           byteaddr[0],byteaddr[1],byteaddr[2],
           byteaddr[3],byteaddr[4],byteaddr[5]);


    return byteaddr;
}


#if defined(SEPRATE_SVC_MODE)
static int is_match_mac(const struct dirent *entry)
{
    char path[SYSFS_PATH_MAX];
    char path_br[SYSFS_PATH_MAX];
    struct stat st;
    FILE *fp = NULL;

    snprintf(path_br, SYSFS_PATH_MAX, SYSFS_CLASS_NET "%s/bridge", entry->d_name);
    if((stat(path_br, &st) != 0))
    {
        snprintf(path, SYSFS_PATH_MAX, SYSFS_CLASS_NET "%s/address", entry->d_name);
        fp = fopen(path,"r");
        if(fp)
        {   
            int lSize = 0;
            char buf[32];
            uint8_t mac_byte[6]={0,0,0,0,0,0};
            fseek (fp, 0 , SEEK_END);
            lSize = ftell (fp);
            rewind (fp);
            memset(buf,0,32);
            fread (buf,1,((lSize>=32)?31:lSize),fp);
            mactobyte(buf,mac_byte);
            if((compare_ether_addr(mac_byte,g_target_mac)) == 0)
            {
                memset(g_target_ifname,0,32);
                memcpy(g_target_ifname, entry->d_name, 
                       (strlen(entry->d_name)>=32)?31:strlen(entry->d_name));
                log_debug(ServiceUtilLogLevel,"** >> matched mac %s << **\n",g_target_ifname);
            }
        }
    }
    return 0;

}
#endif



static int isbridge(const char *name)
{
    char path[SYSFS_PATH_MAX];
    struct stat st;

    snprintf(path, SYSFS_PATH_MAX, SYSFS_CLASS_NET "%s", name);
    return ((stat(path, &st) == 0) && S_ISDIR(st.st_mode));
}

static int isbridge_port(const char *brname,char *port_name)
{
    char path[SYSFS_PATH_MAX];
    struct stat st;

    snprintf(path, SYSFS_PATH_MAX, SYSFS_CLASS_NET "%s/brif/%s", 
             brname, port_name);
    log_debug(ServiceUtilLogLevel,">> PATH %s <<\n",path);
    return ((stat(path, &st) == 0) && S_ISDIR(st.st_mode));
}

uint32_t get_extipv4_for_extmcast_vnid(uint32_t ipv4)
{
    dgwy_service_list_t *svcp = NULL;
    int result=0, retry=0;
    uint32_t ret_ipv4=0;
    /* get svc */
    while(1)
    {
        svcp = dgwy_svc_table_get((char *)"APBR", &result,__FUNCTION__);
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

        /* forst check is there an IP 
         * in this subnet configured
         * */

        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            uint32_t network = 
                svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;

            if(!network)
            {
                continue;
            }

            if(network == (ipv4 & svcp->ifipv4List[i].mask))
            {
                /* got matching network */
                ret_ipv4 = svcp->ifipv4List[i].ipv4;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
    return ret_ipv4;
}

int check_add_pubipv4_alias(char *name, char *svc_ifaddr)
{
    dgwy_service_list_t *svcp = NULL;
    int result=0, retry=0;
    /* get svc */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        int found=-1;
        struct sockaddr_in pubipv4;
        /* forst check is there an IP 
         * in this subnet configured
         * */

        inet_aton(svc_ifaddr, &pubipv4.sin_addr);
            
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            uint32_t network = 
                svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;

            if(!network)
            {
                continue;
            }

            if(network == (pubipv4.sin_addr.s_addr &
                           svcp->ifipv4List[i].mask))
            {
                /* matching inteface */
                log_debug(ServiceUtilLogLevel,"matching if %x %x %x network %x pub %x",
                          svcp->ifipv4List[i].ipv4,
                          svcp->ifipv4List[i].mask,
                          svcp->ifipv4List[i].nexthop, 
                          network, pubipv4.sin_addr.s_addr);
                found=i;
                break;
            }
        }
        if(found == -1)
        {
            SVC_UNLOCK(svcp->lock);
            log_error(ServiceUtilLogLevel,"No IPv4 with matching subnet");
            return -1;
        }
        else
        {
            char svc_addr_mask[INET_ADDRSTRLEN];
            char svc_addr_nexthop[INET_ADDRSTRLEN];
            struct sockaddr_in mask;
            struct sockaddr_in nexthop;

            mask.sin_addr.s_addr = svcp->ifipv4List[found].mask;
            nexthop.sin_addr.s_addr = svcp->ifipv4List[found].nexthop;

            memset(svc_addr_mask,0,sizeof(svc_addr_mask));
            memset(svc_addr_nexthop,0,sizeof(svc_addr_nexthop));

            inet_ntop(AF_INET, &nexthop.sin_addr, svc_addr_nexthop,
                      sizeof(svc_addr_nexthop));
            inet_ntop(AF_INET, &mask.sin_addr, svc_addr_mask,
                      sizeof(svc_addr_mask));

            SVC_UNLOCK(svcp->lock);
            chk_add_svc_ipv4(name, svc_ifaddr, svc_addr_mask,
                             svc_addr_nexthop,IP_TYPE_EXT_VIP);
        }
            
        return 0;
    }
    return -1;

}

int check_rem_pubipv4_alias(char *name, char *svc_ifaddr)
{
    if(isbridge(name))
    {
        int i=0;
        int found=0;
        struct sockaddr_in ipv4addr;
        char buf[128];
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s:%d",name,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                log_debug(ServiceUtilLogLevel,
                          "Found %s with ip %s\n",buf,svc_ifaddr);
                found=1;
                break;
            }
        }
        if(found)
        {
            /* bring down the alias ip */ 
            char command[128];
            FILE *fp=NULL;
            
            log_debug(ServiceUtilLogLevel,
                      "%s found ip %s\n",buf, svc_ifaddr);
            
            memset(command,0,128);
            sprintf(command,"/sbin/ifconfig %s down",buf);
            log_debug(ServiceUtilLogLevel, "Command %s\n",command);
            fp = popen(command, "r");
            
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                          "Error popen:%s\n",command); 
                return -1;
            }
            pclose(fp);
        }
    }
    return 0;
}

int check_add_pubipv4_alias_with_vlan(char *name, char *svc_ifaddr)
{
    dgwy_service_list_t *svcp = NULL;
    int result=0, retry=0;
    int vlan_id = 0;

    /* get svc */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        int found=-1;
        struct sockaddr_in pubipv4;
        /* forst check is there an IP 
         * in this subnet configured
         * */

        inet_aton(svc_ifaddr, &pubipv4.sin_addr);
            
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            uint32_t network = 
                svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;

            if(!network)
            {
                continue;
            }

            if(network == (pubipv4.sin_addr.s_addr &
                           svcp->ifipv4List[i].mask))
            {
                /* matching inteface */
                log_debug(ServiceUtilLogLevel,"matching if %x %x %x network %x pub %x",
                          svcp->ifipv4List[i].ipv4,
                          svcp->ifipv4List[i].mask,
                          svcp->ifipv4List[i].nexthop, 
                          network, pubipv4.sin_addr.s_addr);
                found=i;
                vlan_id = svcp->ifipv4List[i].vlan_id;
                break;
            }
        }
        if((found == -1) || (vlan_id==0))
        {
            SVC_UNLOCK(svcp->lock);
            log_error(ServiceUtilLogLevel,"No IPv4 with matching subnet");
            return -1;
        }
        else
        {
            char svc_addr_mask[INET_ADDRSTRLEN];
            char svc_addr_nexthop[INET_ADDRSTRLEN];
            struct sockaddr_in mask;
            struct sockaddr_in nexthop;

            mask.sin_addr.s_addr = svcp->ifipv4List[found].mask;
            nexthop.sin_addr.s_addr = svcp->ifipv4List[found].nexthop;

            memset(svc_addr_mask,0,sizeof(svc_addr_mask));
            memset(svc_addr_nexthop,0,sizeof(svc_addr_nexthop));

            inet_ntop(AF_INET, &nexthop.sin_addr, svc_addr_nexthop,
                      sizeof(svc_addr_nexthop));
            inet_ntop(AF_INET, &mask.sin_addr, svc_addr_mask,
                      sizeof(svc_addr_mask));

            SVC_UNLOCK(svcp->lock);
            chk_add_svc_ipv4_with_vlan(name, svc_ifaddr, svc_addr_mask,
                                       svc_addr_nexthop,IP_TYPE_EXT_VIP, vlan_id);
        }
            
        return 0;
    }
    return -1;

}

int check_rem_pubipv4_alias_with_vlan(char *name, char *svc_ifaddr, int vlan_id)
{
    if(isbridge(name))
    {
        int i=0;
        int found=0;
        struct sockaddr_in ipv4addr;
        char buf[128];
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s.%d:%d",name,vlan_id,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                log_debug(ServiceUtilLogLevel,
                          "Found %s with ip %s\n",buf,svc_ifaddr);
                found=1;
                break;
            }
        }
        if(found)
        {
            /* bring down the alias ip */ 
            char command[128];
            FILE *fp=NULL;
            
            log_debug(ServiceUtilLogLevel,
                      "%s found ip %s\n",buf, svc_ifaddr);
            
            memset(command,0,128);
            sprintf(command,"/sbin/ifconfig %s down",buf);
            log_debug(ServiceUtilLogLevel, "Command %s\n",command);
            fp = popen(command, "r");
            
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                          "Error popen:%s\n",command); 
                return -1;
            }
            pclose(fp);
        }
    }
    return 0;
}


/* -1 Success
 * 0  No VLAN
 * >0 vlan_id 
 * */

int get_vlan_of_pubalias(char *name, char *svc_ifaddr)
{
    dgwy_service_list_t *svcp = NULL;
    int result=0, retry=0;
    int vlan_id=0;
    /* get svc */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        struct sockaddr_in pubipv4;
        /* forst check is there an IP 
         * in this subnet configured
         * */

        inet_aton(svc_ifaddr, &pubipv4.sin_addr);
            
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            uint32_t network = 
                svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;

            if(!network)
            {
                continue;
            }

            if(network == (pubipv4.sin_addr.s_addr &
                           svcp->ifipv4List[i].mask))
            {
                /* matching inteface */
                log_debug(ServiceUtilLogLevel,"matching if %x %x %x network %x pub %x",
                          svcp->ifipv4List[i].ipv4,
                          svcp->ifipv4List[i].mask,
                          svcp->ifipv4List[i].nexthop, 
                          network, pubipv4.sin_addr.s_addr);
                vlan_id = svcp->ifipv4List[i].vlan_id;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
        return vlan_id;
    }
    return -1;

}



/* chk ip configured 
 * 1-Yes, 0-No
 * */
int is_ipv4_on_vlan(char *name, char *svc_ifaddr, int vlan_id)
{
    if(isbridge(name))
    {
        int i=0;
        struct sockaddr_in ipv4addr;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s.%d:%d",name,vlan_id,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            /*printf("%d:%s:%x\n", i,buf,(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr));*/
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                return 1;
            }
        }
    }
    return 0;
}


/* chk ip configured 
 * 1-Yes, 0-No
 * */
int is_ipv4_on(char *name, char *svc_ifaddr)
{
    if(isbridge(name))
    {
        int i=0;
        struct sockaddr_in ipv4addr;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s:%d",name,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            /*printf("%d:%s:%x\n", i,buf,(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr));*/
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                return 1;
            }
        }
    }
    return 0;
}


/* input netmask in network order */
static int get_prefix(uint32_t netmask)
{
    uint32_t msk = 0x80000000;
    int ret      = 0;
    
    while (msk) 
    {   
        if (netmask & msk)
        {
            ret++;
        }
        msk >>= 1;
    }   
    return ret;
}


/* This function will setup the required routes for
 * each IPV4 added.
 * 
 * 1) Create a routing table by adding entry into
 *    /etc/iproute2/rt_tables with below
 *    svcid        svcname
 *
 * 2) Add route entry to the new table
 *    a) ip route add ipnet/prefix dev svcname src ip table svcname
 *       example:
 *       ip route add 5.5.5.0/24 dev ext src 5.5.5.201 table ext
 *    b) ip route add default via nexthop dev svcname table svcname
 *       example:
 *       ip route add default via 5.5.5.100 dev ext table ext
 *
 * 3) Add routing rule to use this table for
 *    a) All from given network 
 *    b) From all to given network
 *       example:
 *       ip rule add from 5.5.5.0/24 table ext
 *       ip rule add to 5.5.5.0/24 table ext
 *
 **/

int add_ipv4_nexthop(char *name, char *svc_ifaddr,
                     char *svc_addr_mask, char *svc_addr_nexthop)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in maskv4;
    struct sockaddr_in routeadd;
    int prefix = 0;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    int svcid=0;
    FILE *fp = NULL;
    char *radd_str = NULL;
    

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        svcid  = svcp->index;
        SVC_UNLOCK(svcp->lock);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get servicet!");
        return -1;
    }

    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    inet_aton(svc_addr_mask, &maskv4.sin_addr);

    prefix = get_prefix(maskv4.sin_addr.s_addr);
    routeadd.sin_addr.s_addr = (ipv4.sin_addr.s_addr & maskv4.sin_addr.s_addr);

    /* check table exist */
    memset(buf,0,1024);
    sprintf(buf,"sed -n '/%d\\t%s/=' /etc/iproute2/rt_tables",svcid,name);

    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
    
    fp = popen(buf,"r");
    if(fp == NULL)
    {   
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to check route table exist!");
        return -1;
    }   
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {
            /* add entry to last */
            memset(buf,0,1024);
            sprintf(buf,"cat /etc/iproute2/rt_tables > /etc/iproute2/rt_tables_temp; "
                        "sed '$ a %d\t%s' /etc/iproute2/rt_tables_temp > "
                        "/etc/iproute2/rt_tables", svcid, name);
            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            {
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new route table!");
                return -1;
            }
        }
    }
    memset(buf,0,1024);
    radd_str = inet_ntoa(routeadd.sin_addr);
    sprintf(buf,"ip route add %s/%d dev %s src %s table %s",
            radd_str,prefix,name,svc_ifaddr,name);
            
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip route add default via %s dev %s table %s",
            svc_addr_nexthop, name, name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip rule | grep %s/%d |grep -w \"%s\"",radd_str,prefix,name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {   
            memset(buf,0,1024);
            /*sprintf(buf,"ip rule add from %s/%d dev %s table %s; "
                        "ip rule add to %s/%d dev %s table %s",
                        radd_str,prefix,name,name,
                        radd_str,prefix,name,name);*/

            sprintf(buf,"ip rule add to %s/%d dev %s table %s",
                        radd_str,prefix,name,name);

            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            { 
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new ip rule");
                return -1;
            }
        }
    }

    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}

int add_ipv4_nexthop_table(char *name, char *svc_ifaddr,
                           char *svc_addr_mask, char *svc_addr_nexthop,
                           int iptype)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in maskv4;
    struct sockaddr_in nexthop;
    struct sockaddr_in routeadd;
    int prefix = 0;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    //int svcid=0;
    FILE *fp = NULL;
    char *radd_str = NULL;
    char route_table_name[16];
    int itr=0;
    int hopidx=0;

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        //svcid  = svcp->index;
        SVC_UNLOCK(svcp->lock);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get servicet!");
        return -1;
    }

    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    inet_aton(svc_addr_mask, &maskv4.sin_addr);
    inet_aton(svc_addr_nexthop, &nexthop.sin_addr);

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == nexthop.sin_addr.s_addr)
        {
            log_error(ServiceUtilLogLevel, "Nexthop table exit");
            return -1;
        }
    }

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == 0)
        {
            hopidx=itr;
            nexthop_list[itr]=nexthop.sin_addr.s_addr;
            break;
        }
    }
    if(!hopidx)
    {
        log_error(ServiceUtilLogLevel, "Error no free nexthop index");
        return -1;
    }



    prefix = get_prefix(maskv4.sin_addr.s_addr);
    routeadd.sin_addr.s_addr = (ipv4.sin_addr.s_addr & maskv4.sin_addr.s_addr);

    /* check table exist */
    memset(route_table_name,0,16);
    memset(buf,0,1024);

    sprintf(route_table_name,"%x",nexthop.sin_addr.s_addr);
    sprintf(buf,"sed -n '/%d\\t%s/=' /etc/iproute2/rt_tables",hopidx,route_table_name);

    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
    
    fp = popen(buf,"r");
    if(fp == NULL)
    {   
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to check route table exist!");
        return -1;
    }   
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {
            
            /* add entry to last */
            memset(buf,0,1024);
            sprintf(buf,"cat /etc/iproute2/rt_tables > /etc/iproute2/rt_tables_temp; "
                        "sed '$ a %d\t%s' /etc/iproute2/rt_tables_temp > "
                        "/etc/iproute2/rt_tables", hopidx, route_table_name);
            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            {
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new route table!");
                return -1;
            }
        }
        else
        {
            SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
            log_error(ServiceUtilLogLevel, "Nexthop table exit");
            return -1;
        }
    }
    memset(buf,0,1024);
    radd_str = inet_ntoa(routeadd.sin_addr);
    sprintf(buf,"ip route add %s/%d dev %s src %s table %s",
            radd_str,prefix,name,svc_ifaddr,route_table_name);
            
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip route add default via %s dev %s table %s",
            svc_addr_nexthop, name, route_table_name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip rule | grep %s/%d |grep -w \"%s\"",radd_str,prefix,route_table_name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {   
            memset(buf,0,1024);
            /*
            sprintf(buf,"ip rule add from %s/%d dev %s table %s; "
                        "ip rule add to %s/%d dev %s table %s",
                        radd_str,prefix,name,route_table_name,
                        radd_str,prefix,name,route_table_name);
                        */


            sprintf(buf,"ip rule add to %s/%d dev %s table %s",
                        radd_str,prefix,name,route_table_name);

            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            { 
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new ip rule");
                return -1;
            }

            if(iptype == IP_TYPE_DOVE_TEP)
            {
                memset(buf,0,1024);
                sprintf(buf,"ip rule add from %s/%d dev lo table %s",
                            radd_str,prefix,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }

                memset(buf,0,1024);
                sprintf(buf,"ip rule add from 169.254.0.252 dev %s table %s ;",
                        name,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }
            }
            else if(iptype == IP_TYPE_EXTERNAL)
            {
                memset(buf,0,1024);
                sprintf(buf,"ip rule add from %s/%d dev lo table %s",
                            radd_str,prefix,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }

                memset(buf,0,1024);
                sprintf(buf,"ip rule add from 169.254.0.253 dev %s table %s ;",
                        name,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }
            }
        }
    }

    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}

int add_ipv4_nexthop_table_with_vlan(char *name, char *svc_ifaddr,
                                     char *svc_addr_mask, char *svc_addr_nexthop,
                                     int iptype, int vlan_id)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in maskv4;
    struct sockaddr_in nexthop;
    struct sockaddr_in routeadd;
    int prefix = 0;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    //int svcid=0;
    FILE *fp = NULL;
    char *radd_str = NULL;
    char route_table_name[16];
    int itr=0;
    int hopidx=0;

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        //svcid  = svcp->index;
        SVC_UNLOCK(svcp->lock);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get servicet!");
        return -1;
    }

    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    inet_aton(svc_addr_mask, &maskv4.sin_addr);
    inet_aton(svc_addr_nexthop, &nexthop.sin_addr);

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == nexthop.sin_addr.s_addr)
        {
            log_error(ServiceUtilLogLevel, "Nexthop table exit");
            return -1;
        }
    }

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == 0)
        {
            hopidx=itr;
            nexthop_list[itr]=nexthop.sin_addr.s_addr;
            break;
        }
    }
    if(!hopidx)
    {
        log_error(ServiceUtilLogLevel, "Error no free nexthop index");
        return -1;
    }



    prefix = get_prefix(maskv4.sin_addr.s_addr);
    routeadd.sin_addr.s_addr = (ipv4.sin_addr.s_addr & maskv4.sin_addr.s_addr);

    /* check table exist */
    memset(route_table_name,0,16);
    memset(buf,0,1024);

    sprintf(route_table_name,"%x",nexthop.sin_addr.s_addr);
    sprintf(buf,"sed -n '/%d\\t%s/=' /etc/iproute2/rt_tables",hopidx,route_table_name);

    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
    
    fp = popen(buf,"r");
    if(fp == NULL)
    {   
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to check route table exist!");
        return -1;
    }   
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {
            
            /* add entry to last */
            memset(buf,0,1024);
            sprintf(buf,"cat /etc/iproute2/rt_tables > /etc/iproute2/rt_tables_temp; "
                        "sed '$ a %d\t%s' /etc/iproute2/rt_tables_temp > "
                        "/etc/iproute2/rt_tables", hopidx, route_table_name);
            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            {
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new route table!");
                return -1;
            }
        }
        else
        {
            SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
            log_error(ServiceUtilLogLevel, "Nexthop table exit");
            return -1;
        }
    }
    memset(buf,0,1024);
    radd_str = inet_ntoa(routeadd.sin_addr);
    sprintf(buf,"ip route add %s/%d dev %s.%d src %s table %s",
            radd_str,prefix,name,vlan_id,svc_ifaddr,route_table_name);
            
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip route add default via %s dev %s.%d table %s",
            svc_addr_nexthop, name, vlan_id, route_table_name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }

    memset(buf,0,1024);
    sprintf(buf,"ip rule | grep %s/%d |grep -w \"%s\"",radd_str,prefix,route_table_name);
    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {   
            memset(buf,0,1024);
            /*
            sprintf(buf,"ip rule add from %s/%d dev %s table %s; "
                        "ip rule add to %s/%d dev %s table %s",
                        radd_str,prefix,name,route_table_name,
                        radd_str,prefix,name,route_table_name);
                        */

            sprintf(buf,"ip rule add to %s/%d dev %s.%d table %s",
                        radd_str,prefix,name,vlan_id,route_table_name);

            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            { 
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new ip rule");
                return -1;
            }

            if(iptype == IP_TYPE_DOVE_TEP)
            {
                memset(buf,0,1024);
                sprintf(buf,"ip rule add from %s/%d dev lo table %s",
                            radd_str,prefix,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }

                memset(buf,0,1024);
                sprintf(buf,"ip rule add from 169.254.0.252 dev %s.%d table %s ;",
                        name,vlan_id,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }
            }
            else if(iptype == IP_TYPE_EXTERNAL)
            {
                memset(buf,0,1024);
                sprintf(buf,"ip rule add from %s/%d dev lo table %s",
                            radd_str,prefix,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }

                memset(buf,0,1024);
                sprintf(buf,"ip rule add from 169.254.0.253 dev %s.%d table %s ;",
                        name,vlan_id,route_table_name);

                fclose(fp);
                fp = popen(buf,"r");
                if(fp == NULL)
                { 
                    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                    log_error(ServiceUtilLogLevel,
                              "Failed to add new ip rule");
                    return -1;
                }
            }
        }
    }

    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}

int add_ipv4_nexthop_pub_iprule(char *name, char *svc_ifaddr,
                                char *svc_addr_mask, char *svc_addr_nexthop)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in maskv4;
    struct sockaddr_in nexthop;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    //int svcid=0;
    FILE *fp = NULL;
    char route_table_name[16];

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel, "In use %s: failed\n",name);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        //svcid  = svcp->index;
        SVC_UNLOCK(svcp->lock);
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get servicet!");
        return -1;
    }

    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    inet_aton(svc_addr_mask, &maskv4.sin_addr);
    inet_aton(svc_addr_nexthop, &nexthop.sin_addr);

    memset(buf,0,1024);
    memset(route_table_name,0,16);
    sprintf(route_table_name,"%x",nexthop.sin_addr.s_addr);
    sprintf(buf,"ip rule | grep 169.254.0.253 |grep -w \"%s\"",route_table_name);


    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */

    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to add new default route ");
        return -1;
    }
    else
    {
        int found=0;
        sleep(1);
        memset(buf,0,1);
        while (fread(buf, 1, 1, fp)) 
        {
            if(buf[0])
            {
                found++;
            }
            break;
        }
        
        if(!found)
        {   
            memset(buf,0,1024);
            sprintf(buf,"ip rule add from 169.254.0.253 dev %s table %s ;",
                    name,route_table_name);

            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            { 
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new ip rule");
                return -1;
            }

            memset(buf,0,1024);
            sprintf(buf,"ip route add 169.254.0.253 dev %s src %s table %s",
                    name,svc_ifaddr,route_table_name);
                    
            fclose(fp);
            fp = popen(buf,"r");
            if(fp == NULL)
            {
                SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
                log_error(ServiceUtilLogLevel,
                          "Failed to add new route ");
                return -1;
            }
        }
    }

    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}




/* This function will remove routes of
 * IPV4 .
 * 
 **/

int rem_ipv4_nexthop(char *name, char *svc_ifaddr)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in routedel;
    int prefix = 0;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    FILE *fp = NULL;
    char *rdel_str = NULL;
    char svc_addr_nexthop[INET_ADDRSTRLEN];
    uint32_t route_table_name=0;
    int itr=0;
    int is_tep=0;
    
    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    //inet_aton(svc_addr_mask, &maskv4.sin_addr);
    //routedel.sin_addr.s_addr = (ipv4.sin_addr.s_addr & maskv4.sin_addr.s_addr);

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        int i=0;int found=0;

        /* check how many IPV4 
         * with this subnet.
         * Incase more than 1 then do nothing
         * */
        if(g_dovenet_ipv4 == ipv4.sin_addr.s_addr)
        {
            is_tep=1;
        }

        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].ipv4 == ipv4.sin_addr.s_addr)
            {
                /* entry with given network */
                prefix = get_prefix(svcp->ifipv4List[i].mask);
                routedel.sin_addr.s_addr = svcp->ifipv4List[i].nexthop;
                route_table_name = svcp->ifipv4List[i].nexthop;
                memset(svc_addr_nexthop,0,sizeof(svc_addr_nexthop));
                inet_ntop(AF_INET, &routedel.sin_addr, svc_addr_nexthop,
                          sizeof(svc_addr_nexthop));
                routedel.sin_addr.s_addr = svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;
                found++;
                break;
            }
        }
        if(found <=0)
        {
            SVC_UNLOCK(svcp->lock);
            return -1;
        }
        found=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            /* check for more than one IP with same network
             * if yes do not clean routes */
            if((svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask) ==
               (routedel.sin_addr.s_addr))
            {
                found++;
            }
        }

        SVC_UNLOCK(svcp->lock);
        if(found > 1)
        {
            log_error(ServiceUtilLogLevel,
                      "Routs not cleand : another interface exits !");
            return -1;
        }
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get service!");
        return -1;
    }

   
    memset(buf,0,1024);

    
    /* rem entry from table */
    memset(buf,0,1024);
    rdel_str = inet_ntoa(routedel.sin_addr);
    sprintf(buf,"ip route delete %s/%d dev %s src %s table %x",
            rdel_str,prefix,name,svc_ifaddr,route_table_name);

    log_debug(ServiceUtilLogLevel,"%s",buf);
    
    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
            
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove route ");
        return -1;
    }


    memset(buf,0,1024);
    sprintf(buf,"ip route delete default via %s dev %s table %x",
            svc_addr_nexthop, name, route_table_name);
    fclose(fp);
    
    log_debug(ServiceUtilLogLevel,"%s",buf);

    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to delete default route ");
        return -1;
    }


    memset(buf,0,1024);
    sprintf(buf,"sed -i '/%x/d' /etc/iproute2/rt_tables",route_table_name);
    fclose(fp);
    
    log_debug(ServiceUtilLogLevel,"%s",buf);

    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to delete default route ");
        return -1;
    }

        
    memset(buf,0,1024);
    sprintf(buf,"ip rule delete from %s/%d dev %s table %x; "
                "ip rule delete to %s/%d dev %s table %x",
                rdel_str,prefix,name,route_table_name,
                rdel_str,prefix,name,route_table_name);

    log_debug(ServiceUtilLogLevel,"%s",buf);

    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    { 
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove ip rule ");
        return -1;
    }

 
    memset(buf,0,1024);
    if(is_tep)
    {
        sprintf(buf,"ip rule delete from %s/%d dev lo table %x; "
                    "ip rule delete from 169.254.0.252 dev %s table %x;",
                    rdel_str,prefix,route_table_name,
                    name,route_table_name);
    }
    else
    {
        sprintf(buf,"ip rule delete from %s/%d dev lo table %x; "
                    "ip rule delete from 169.254.0.253 dev %s table %x;",
                    rdel_str,prefix,route_table_name,
                    name,route_table_name);
    }

    log_debug(ServiceUtilLogLevel,"%s",buf);

    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    { 
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove ip rule ");
        return -1;
    }

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == route_table_name)
        {
            log_debug(ServiceUtilLogLevel, "Free nexthop from nexthop_list");
            nexthop_list[itr]=0;
            break;
        }
    }


    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}

int rem_ipv4_nexthop_with_vlan(char *name, char *svc_ifaddr, int vlan_id)
{
    struct sockaddr_in ipv4;
    struct sockaddr_in routedel;
    int prefix = 0;
    char buf[1024];
    int result=0, retry=0;
    dgwy_service_list_t *svcp = NULL;
    FILE *fp = NULL;
    char *rdel_str = NULL;
    char svc_addr_nexthop[INET_ADDRSTRLEN];
    uint32_t route_table_name=0;
    int itr=0;
    int is_tep=0;
    
    inet_aton(svc_ifaddr, &ipv4.sin_addr);
    //inet_aton(svc_addr_mask, &maskv4.sin_addr);
    //routedel.sin_addr.s_addr = (ipv4.sin_addr.s_addr & maskv4.sin_addr.s_addr);

    /* get svc id */
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        int i=0;int found=0;

        /* check how many IPV4 
         * with this subnet.
         * Incase more than 1 then do nothing
         * */
        if(g_dovenet_ipv4 == ipv4.sin_addr.s_addr)
        {
            is_tep=1;
        }

        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].ipv4 == ipv4.sin_addr.s_addr)
            {
                /* entry with given network */
                prefix = get_prefix(svcp->ifipv4List[i].mask);
                routedel.sin_addr.s_addr = svcp->ifipv4List[i].nexthop;
                route_table_name = svcp->ifipv4List[i].nexthop;
                memset(svc_addr_nexthop,0,sizeof(svc_addr_nexthop));
                inet_ntop(AF_INET, &routedel.sin_addr, svc_addr_nexthop,
                          sizeof(svc_addr_nexthop));
                routedel.sin_addr.s_addr = svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask;
                found++;
                break;
            }
        }
        if(found <=0)
        {
            SVC_UNLOCK(svcp->lock);
            return -1;
        }
        found=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            /* check for more than one IP with same network
             * if yes do not clean routes */
            if((svcp->ifipv4List[i].ipv4 & svcp->ifipv4List[i].mask) ==
               (routedel.sin_addr.s_addr))
            {
                found++;
            }
        }

        SVC_UNLOCK(svcp->lock);
        if(found > 1)
        {
            log_error(ServiceUtilLogLevel,
                      "Routs not cleand : another interface exits !");
            return -1;
        }
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Failed to get service!");
        return -1;
    }

   
    memset(buf,0,1024);

    SVC_ROUTE_LOCK(GLB_ROUTE_LOCK); /* XXX LOCKED */
    
    /* rem entry from table */
    memset(buf,0,1024);
    rdel_str = inet_ntoa(routedel.sin_addr);
    sprintf(buf,"ip route delete %s/%d dev %s.%d src %s table %x",
            rdel_str,prefix,name,vlan_id,svc_ifaddr,route_table_name);

    log_debug(ServiceUtilLogLevel,"%s",buf);
            
    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove route ");
        return -1;
    }


    memset(buf,0,1024);
    sprintf(buf,"ip route delete default via %s dev %s.%d table %x",
            svc_addr_nexthop, name, vlan_id, route_table_name);
    fclose(fp);
    
    log_debug(ServiceUtilLogLevel,"%s",buf);

    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to delete default route ");
        return -1;
    }


    memset(buf,0,1024);
    sprintf(buf,"sed -i '/%x/d' /etc/iproute2/rt_tables",route_table_name);
    fclose(fp);
    
    log_debug(ServiceUtilLogLevel,"%s",buf);

    fp = popen(buf,"r");
    if(fp == NULL)
    {
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to delete default route ");
        return -1;
    }

        
    memset(buf,0,1024);
    sprintf(buf,"ip rule delete from %s/%d dev %s.%d table %x; "
                "ip rule delete to %s/%d dev %s.%d table %x",
                rdel_str,prefix,name,vlan_id,route_table_name,
                rdel_str,prefix,name,vlan_id,route_table_name);

    log_debug(ServiceUtilLogLevel,"%s",buf);

    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    { 
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove ip rule ");
        return -1;
    }

 
    memset(buf,0,1024);
    if(is_tep)
    {
        sprintf(buf,"ip rule delete from %s/%d dev lo table %x; "
                    "ip rule delete from 169.254.0.252 dev %s.%d table %x;",
                    rdel_str,prefix,route_table_name,
                    name,vlan_id,route_table_name);
    }
    else
    {
        sprintf(buf,"ip rule delete from %s/%d dev lo table %x; "
                    "ip rule delete from 169.254.0.253 dev %s.%d table %x;",
                    rdel_str,prefix,route_table_name,
                    name,vlan_id,route_table_name);
    }

    log_debug(ServiceUtilLogLevel,"%s",buf);

    fclose(fp);
    fp = popen(buf,"r");
    if(fp == NULL)
    { 
        SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
        log_error(ServiceUtilLogLevel,
                  "Failed to remove ip rule ");
        return -1;
    }

    for(itr=1; itr < 250; itr++)
    {
        if(nexthop_list[itr] == route_table_name)
        {
            log_debug(ServiceUtilLogLevel, "Free nexthop from nexthop_list");
            nexthop_list[itr]=0;
            break;
        }
    }


    SVC_ROUTE_UNLOCK(GLB_ROUTE_LOCK); /* XXX UNLOCKED */
    fclose(fp);

    return 0;
}



int chk_add_svc_ipv4_with_vlan(char *name, char *svc_ifaddr, 
                               char *svc_addr_mask, 
                               char *svc_addr_nexthop, int iptype, int vlan_id)
{
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(vlan_id)
    {
        FILE *fp; 
        char command[64];
        memset(command,0,64);
        sprintf(command,"vconfig add %s %d >/dev/null 2>&1; ifconfig %s.%d up",
                chk_name, vlan_id,
                chk_name, vlan_id );
        fp = popen(command, "r");
        
        if(fp)
        {
            pclose(fp);
        }
    }

    if(isbridge(chk_name))
    {
        int i=0;
        int found=0;
        struct sockaddr_in ipv4addr;
        int freeidx = -1;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s.%d:%d",chk_name,vlan_id,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);

            /*printf("%d:%s:%x\n", i,buf,(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr));*/
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                log_error(ServiceUtilLogLevel,
                        "%s already assigned ip %s/%s\n",buf,svc_ifaddr,
                        svc_addr_mask);
                found=1;
                break;
            }
            if((freeidx<0)&&(((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr)) == 0))
            {
                freeidx=i;
            }
            
        }
        if(!found && (freeidx>=0))
        {
            FILE *fp;
            char command[256];
            memset(command,0,256);
            sprintf(command,"/sbin/ifconfig %s.%d:%d %s netmask %s ",
                    chk_name, vlan_id, freeidx, svc_ifaddr, svc_addr_mask);
            log_debug(ServiceUtilLogLevel, "Command %s\n",command);
            fp = popen(command, "r");

            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
                return -1;
            }
            pclose(fp);
            if((iptype==IP_TYPE_EXTERNAL)|| (iptype==IP_TYPE_DOVE_TEP))
            {
                if(vlan_id)
                {
                    return add_ipv4_nexthop_table_with_vlan(chk_name,svc_ifaddr,
                                                            svc_addr_mask,svc_addr_nexthop,
                                                            iptype, vlan_id);
                }
                else
                {
                    return add_ipv4_nexthop_table(chk_name,svc_ifaddr,
                                                  svc_addr_mask,svc_addr_nexthop,iptype);
                }
            }
            else if(iptype==IP_TYPE_EXT_VIP)
            {
                return add_ipv4_nexthop_pub_iprule(chk_name,svc_ifaddr,
                                                   svc_addr_mask,svc_addr_nexthop);
            }
            return 0;
        }
    }
    return -1;
}

                

int chk_add_svc_ipv4(char *name, char *svc_ifaddr, 
                     char *svc_addr_mask, 
                     char *svc_addr_nexthop, int iptype)
{
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(isbridge(chk_name))
    {
        int i=0;
        int found=0;
        struct sockaddr_in ipv4addr;
        int freeidx = -1;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s:%d",chk_name,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            /*printf("%d:%s:%x\n", i,buf,(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr));*/
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                log_error(ServiceUtilLogLevel,
                        "%s already assigned ip %s/%s\n",buf,svc_ifaddr,
                        svc_addr_mask);
                found=1;
                break;
            }
            if((freeidx<0)&&(((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr)) == 0))
            {
                freeidx=i;
            }
            
        }
        if(!found && (freeidx>=0))
        {
            FILE *fp;
            char command[256];
            memset(command,0,256);
            sprintf(command,"/sbin/ifconfig %s:%d %s netmask %s ",
                    chk_name, freeidx, svc_ifaddr, svc_addr_mask);
            log_debug(ServiceUtilLogLevel, "Command %s\n",command);
            fp = popen(command, "r");

            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
                return -1;
            }
            pclose(fp);
            if((iptype==IP_TYPE_EXTERNAL)|| (iptype==IP_TYPE_DOVE_TEP))
            {
                return add_ipv4_nexthop_table(chk_name,svc_ifaddr,
                                              svc_addr_mask,svc_addr_nexthop,iptype);
            }
            else if(iptype==IP_TYPE_EXT_VIP)
            {
                return add_ipv4_nexthop_pub_iprule(chk_name,svc_ifaddr,
                                                   svc_addr_mask,svc_addr_nexthop);
            }
            return 0;
        }
    }
    return -1;
}

static void
send_ext_mcast_recv_join(uint32_t vnid, int type)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    uint32_t domain_horder = vnid;
    uint8_t mcast_mac[6]={0x1,0,0x5e,0,0,0};

    if((dpsCfg.dpsIp==0) || (init_done==0))
    {
        log_error(ServiceUtilLogLevel,
                  "Not ready to request \n");
        return;
    }

    log_debug(ServiceUtilLogLevel,
            "EXTIP 0x%x VNID %d type %d stype %d"
            "mac %02x:%02x:%02x:%02x:%02x:%02x \n", 
            g_dovenet_ipv4, domain_horder, type,
            g_APPBRIDGE_type,
            mcast_mac[0],mcast_mac[1],
            mcast_mac[2],mcast_mac[3],
            mcast_mac[4],mcast_mac[5]);

    /**
     * Populate the location request to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    if(type == DPS_MCAST_RECV_JOIN)
    {
        dps_client_data.hdr.type = DPS_MCAST_RECEIVER_JOIN;
    }
    else if(type == DPS_MCAST_RECV_LEAVE)
    {
        dps_client_data.hdr.type = DPS_MCAST_RECEIVER_LEAVE;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Invalid MSG Type\n");
        return;
    }

    if((g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)||
       (g_APPBRIDGE_type==DGWY_TYPE_EXT_VLAN))
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Invalid Service Type\n");
        return;
    }

    dps_client_data.hdr.vnid = domain_horder;

    dps_client_data.mcast_receiver.tunnel_endpoint.family = AF_INET;
    dps_client_data.mcast_receiver.tunnel_endpoint.ip4 = ntohl(g_dovenet_ipv4);

    /* TODO change 0 to MCAST_ADDR_V4_ICB_RANGE */
    dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.mcast_addr_type=MCAST_ADDR_MAC;
    dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.u.mcast_ip4=0;
    memcpy(dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.mcast_mac,
           mcast_mac, 6);

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        memset(pdomain,0,VNID_CTX_SZ);
        /* should free when reply comes back */
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data.context = pdomain;
    }
    else
    {
        dps_client_data.context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }
    return;
}


void dove_mcast_dcs_vgw_all_recv_join(void)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
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
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return ;
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
            if(g_dovenet_ipv4 &&
               (svcp->dvList[i].domain>0) &&
               (svcp->dvList[i].domain <= 0xFFFFFF))
            {
                int vnid=svcp->dvList[i].domain;
                send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_JOIN);
            }
        }

        SVC_UNLOCK(svcp->lock);
        return;
    }
}

void dove_mcast_dcs_vgw_all_recv_leave(void)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
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
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return ;
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
            if(g_dovenet_ipv4 &&
               (svcp->dvList[i].domain>0) &&
               (svcp->dvList[i].domain <= 0xFFFFFF))
            {
                int vnid=svcp->dvList[i].domain;
                send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_LEAVE);
            }
        }

        SVC_UNLOCK(svcp->lock);
        return;
    }
}

int chk_rem_svc_ipv4_with_vlan(char *name, char *svc_ifaddr, int vlan_id)
{
    int retval=0;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(isbridge(chk_name))
    {
        int i=0;
        struct sockaddr_in ipv4addr;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s.%d:%d",chk_name,vlan_id,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                 FILE *fp;
                 char command[128];

                 log_debug(ServiceUtilLogLevel,
                            "%s found ip %s\n",buf, svc_ifaddr);
                 
                 memset(command,0,128);
                 sprintf(command,"/sbin/ifconfig %s down",buf);
                 log_debug(ServiceUtilLogLevel, "Command %s\n",command);
                 fp = popen(command, "r");
                 
                 if(fp == NULL)
                 {
                     log_error(ServiceUtilLogLevel,
                                "Error popen:%s\n",command);
                     break;
                 }
                 pclose(fp);
                 rem_ipv4_nexthop_with_vlan(chk_name, svc_ifaddr, vlan_id);
                 retval=1;
            }
        }

        if(ipv4addr.sin_addr.s_addr == g_dovenet_ipv4)
        {
            dove_mcast_dcs_vgw_all_recv_leave();
            check_del_tep();
            dgwy_ctrl_set_dovenet_ipv4((char*)"APBR", 0 ,0);
            g_dovenet_ipv4=0;
            g_dovenet_ipv4_nexthop=0;
        }
        else if(ipv4addr.sin_addr.s_addr == g_extif_ipv4)
        {
            dgwy_ctrl_set_external_ipv4((char*)"APBR", 0 ,0);
            g_extif_ipv4=0;
            g_extif_ipv4_nexthop=0;
        }
    }
    return retval;
}


int chk_rem_svc_ipv4(char *name, char *svc_ifaddr)
{
    int retval=0;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(isbridge(chk_name))
    {
        int i=0;
        struct sockaddr_in ipv4addr;
            
        inet_aton(svc_ifaddr, &ipv4addr.sin_addr);
        
        for(i=1; i < MAX_IFIPV4_ADDRESS ; i++)
        {
            int fd;
            struct ifreq ifr;
            char buf[128];

            memset(&ifr,0,sizeof(ifr));
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            
            ifr.ifr_addr.sa_family = AF_INET;
            
            memset(buf,0,128);
            sprintf(buf,"%s:%d",chk_name,i);
            strncpy(ifr.ifr_name, buf, IFNAMSIZ-1);
            
            ioctl(fd, SIOCGIFADDR, &ifr);
            
            close(fd);
            if((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr) == ipv4addr.sin_addr.s_addr)
            {
                 FILE *fp;
                 char command[128];

                 log_debug(ServiceUtilLogLevel,
                            "%s found ip %s\n",buf, svc_ifaddr);
                 
                 memset(command,0,128);
                 sprintf(command,"/sbin/ifconfig %s down",buf);
                 log_debug(ServiceUtilLogLevel, "Command %s\n",command);
                 fp = popen(command, "r");
                 
                 if(fp == NULL)
                 {
                     log_error(ServiceUtilLogLevel,
                                "Error popen:%s\n",command);
                     break;
                 }
                 pclose(fp);
                 rem_ipv4_nexthop(chk_name, svc_ifaddr);
                 retval=1;
            }
        }

        if(ipv4addr.sin_addr.s_addr == g_dovenet_ipv4)
        {
            dove_mcast_dcs_vgw_all_recv_leave();
            check_del_tep();
            dgwy_ctrl_set_dovenet_ipv4((char*)"APBR", 0 ,0);
            g_dovenet_ipv4=0;
            g_dovenet_ipv4_nexthop=0;
        }
        else if(ipv4addr.sin_addr.s_addr == g_extif_ipv4)
        {
            dgwy_ctrl_set_external_ipv4((char*)"APBR", 0 ,0);
            g_extif_ipv4=0;
            g_extif_ipv4_nexthop=0;
        }
    }
    return retval;
}


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
void  set_noicmp_redirect(char *name)
{
    FILE *fp;
    char command[256];
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
    if(NULL==chk_name)
    {
        return;
    }
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif
    memset(command,0,256);
    sprintf(command,"echo 0 > /proc/sys/net/ipv4/conf/all/send_redirects;"
                    "echo 0 > /proc/sys/net/ipv4/conf/%s/send_redirects", chk_name);
    log_debug(ServiceUtilLogLevel, "Command %s\n",command);
    fp = popen(command, "r");
    
    if(fp == NULL)
    {
        log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
        return;
    }
    pclose(fp);
    return;
}


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
int chk_start_svc(char *name)
{
    int ret_val = 0;
#if defined(SEPRATE_SVC_MODE)
    FILE *fp;
    char command[64];
    do{
        if (isbridge(name))
        {
            /* service does exist */
            ret_val = 1;
            break;
        }

        memset(command, 0, 64);
        sprintf(command, "brctl addbr %s", name);
        log_info(ServiceUtilLogLevel, "Command %s\n", command);

        fp = popen(command, "r");
        if (fp == NULL)
        {
            log_warn(ServiceUtilLogLevel, "Error popen:%s\n", command);
            break;
        }
        pclose(fp);

        if (isbridge(name))
        {
            memset(command, 0, 64);
            sprintf(command, "/sbin/ifconfig %s up", name);
            log_info(ServiceUtilLogLevel, "Command %s\n", command);

            fp = popen(command, "r");
            if (fp == NULL)
            {
                log_warn(ServiceUtilLogLevel, "Error popen:%s\n", command);
                return 0;
            }
            pclose(fp);
            ret_val = 1;
            break;
        }
    } while(0);
#else
    ret_val=1;
#endif

    return ret_val;
}

int chk_add_port(char *brname,char *port_name)
{

#if defined(SEPRATE_SVC_MODE)
    if(isbridge_port(brname,port_name))
    {
        return 1;
    }
    else
    {
        FILE *fp;
        char command[64];
        memset(command,0,64);
        sprintf(command,"brctl addif %s %s",brname,port_name);
        log_error(ServiceUtilLogLevel, 
                    "Command %s\n",command);
        fp = popen(command, "r");

        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel, 
                        "Error popen:%s\n",command);
            return 0;
        }
        pclose(fp);
        if(isbridge_port(brname,port_name))
        {
            memset(command,0,64);
            sprintf(command,"/sbin/ifconfig %s 0.0.0.0 up; ethtool -K %s rx off",
                    port_name, port_name);

            log_error(ServiceUtilLogLevel, 
                        "Command %s\n",command);
            fp = popen(command, "r");
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel, 
                            "Error popen:%s\n",command);
                return 0;
            }
            pclose(fp);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 0;
#else
    return 1;
#endif
}

int chk_rem_port(char *brname,char *port_name)
{
#if defined(SEPRATE_SVC_MODE)
    if(isbridge_port(brname,port_name))
    {
        FILE *fp;
        char command[64];
        memset(command,0,64);
        sprintf(command,"brctl delif %s %s",brname,port_name);
        log_error(ServiceUtilLogLevel, 
                    "Command %s\n",command);
        fp = popen(command, "r");

        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                        "Error popen:%s\n",command);
            return 0;
        }
        pclose(fp);
        return 1;
    }
    return 0;
#else
    return 1;
#endif
}

/*
 ******************************************************************************
 * chk_mac_add_bridge                                                     *//**
 *
 * \brief - Add a mac address to the bridge
 *
 * \param[in] brname The Name of the Bridge
 *
 * \retval 0 Success
 * \retval Non-Zero Error
 *
 ******************************************************************************
 */
int chk_mac_add_bridge(char *brname, uint8_t *hwaddr)
{
#if defined(SEPRATE_SVC_MODE)
    struct dirent **namelist;
    int i, count = 0;

    memcpy(g_target_mac, hwaddr, 6);
    memset(g_target_ifname,0,32);

    count = scandir(SYSFS_CLASS_NET, &namelist, is_match_mac, alphasort);
    if (count < 0)
    {
        return -1;
    }

    if((strlen((char*)g_target_ifname)) > 0)
    {
        chk_add_port(brname,(char*)g_target_ifname);
    }

    for (i = 0; i < count; i++)
    {
        free(namelist[i]);
    }
    free(namelist);

    return 0;
#else
    return 0;
#endif
}


int chk_mac_rem_bridge(char *brname, uint8_t *hwaddr)
{
#if defined(SEPRATE_SVC_MODE)
    struct dirent **namelist;
    int i, count = 0;

    memcpy(g_target_mac, hwaddr, 6);
    memset(g_target_ifname,0,32);

    count = scandir(SYSFS_CLASS_NET, &namelist, is_match_mac, alphasort);
    if (count < 0)
        return -1;

    if((strlen((char*)g_target_ifname)) > 0)
    {
        chk_rem_port(brname,(char*)g_target_ifname);
    }
    
    for (i = 0; i < count; i++)
    {
        free(namelist[i]);
    }
    free(namelist);

    return 0;
#else
    return 0;
#endif
}


int chk_stop_svc(char *name)
{
#if defined(SEPRATE_SVC_MODE)
    if(isbridge(name))
    {
        FILE *fp;
        char command[64];
        memset(command,0,64);
        sprintf(command,"ifconfig %s down;brctl delbr %s",name,name);
        fp = popen(command, "r");

        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                        "Error popen:%s\n",command);
            return 0;
        }
        pclose(fp);
    }
    return 1;
#else
    return 1;
#endif

}

int dgwy_nl_init_listener(struct nl_msg *msg, nl_recvmsg_msg_cb_t func, void *arg)
{
    int err = EINVAL;

    listener_sock = nl_handle_alloc();
    if (!listener_sock) 
    {
        log_error(ServiceUtilLogLevel,
                    "Error : failed to alloc listener handle \n");
        nlmsg_free(msg);
        return -1;
    }    

    if (genl_connect(listener_sock) < 0) 
    {
        log_error(ServiceUtilLogLevel,
                    "Error : failed to connect listener handle \n");
        goto fail_genl;
    }

    family = genl_ctrl_resolve(listener_sock, DGWY_GENL_NAME);
    if (family < 0) 
    {
        goto fail_genl;
    }

    /* To test connections and set the family */
    if (msg == NULL) 
    {
        nl_handle_destroy(listener_sock);
        listener_sock = NULL;
        return 0;
    }    

    if (nl_socket_modify_cb(listener_sock, NL_CB_VALID, 
                NL_CB_CUSTOM, func, arg) != 0)
    {
        goto fail_genl;
    }

    if (nl_send_auto_complete(listener_sock, msg) < 0) 
    {
        goto fail_genl;
    }

    if ((err = -nl_recvmsgs_default(listener_sock)) > 0) 
    {
        goto fail_genl;
    }

    nlmsg_free(msg);

    return 0;

fail_genl:
    log_error(ServiceUtilLogLevel,
                "||||| LISTEN SOCKET ERROR |||||\n");
    nl_handle_destroy(listener_sock);
    listener_sock = NULL;
    nlmsg_free(msg);
    errno = err; 
    return -1;
}


int dgwy_nl_send_get_info_message(struct nl_msg *msg, nl_recvmsg_msg_cb_t func, void *arg)
{
    int err = EINVAL;

    get_info_sock = nl_handle_alloc();
    if (!get_info_sock) {
        nlmsg_free(msg);
        return -1;
    }    

    /* To test connections and set the family */
    if (msg == NULL) {
        nl_handle_destroy(get_info_sock);
        get_info_sock = NULL;
        return 0;
    }

    if (genl_connect(get_info_sock) < 0) 
        goto fail_genl;

    family = genl_ctrl_resolve(get_info_sock, DGWY_GENL_NAME);
    if (family < 0) 
        goto fail_genl;

    if (nl_socket_modify_cb(get_info_sock, NL_CB_VALID, NL_CB_CUSTOM, func, arg) != 0)
        goto fail_genl;

    if (nl_send_auto_complete(get_info_sock, msg) < 0) 
        goto fail_genl;

    if ((err = -nl_recvmsgs_default(get_info_sock)) > 0) 
        goto fail_genl;

    nlmsg_free(msg);

    nl_handle_destroy(get_info_sock);

    return 0;

fail_genl:
    nlmsg_free(msg);
    nl_handle_destroy(get_info_sock);
    get_info_sock = NULL;
    errno = err; 
    return -1;
}

int dgwy_nl_send_message(struct nl_msg *msg, nl_recvmsg_msg_cb_t func, void *arg)
{
    int err = EINVAL;
    struct nl_handle *sock = NULL;

    sock = nl_handle_alloc();
    if (!sock) {
        nlmsg_free(msg);
        return -1;
    }    

    /* To test connections and set the family */
    if (msg == NULL) {
        nl_handle_destroy(sock);
        sock = NULL;
        return 0;
    }

    if (genl_connect(sock) < 0) 
        goto fail_genl;

    family = genl_ctrl_resolve(sock, DGWY_GENL_NAME);
    if (family < 0) 
        goto fail_genl;

        

    if (nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, func, arg) != 0)
        goto fail_genl;

    if (nl_send_auto_complete(sock, msg) < 0) 
        goto fail_genl;

    if ((err = -nl_recvmsgs_default(sock)) > 0) 
        goto fail_genl;

    nlmsg_free(msg);

    nl_handle_destroy(sock);

    return 0;

fail_genl:
    nlmsg_free(msg);
    nl_handle_destroy(sock);
    sock = NULL;
    errno = err; 
    return -1;
}


struct nl_msg *dgwy_nl_message(int cmd, int flags)
{
    struct nl_msg *msg;

    msg = nlmsg_alloc();
    if (!msg)
    {
        return NULL;
    }
    
    genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, flags,
                cmd, DGWY_VERSION_NR);

    return msg;
}

typedef struct {
        struct nlmsghdr n;
        struct genlmsghdr g;
        char buf[256];
} msg_payload_t;


#define GENLMSG_DATA(glh) ((void *)(((char*)(NLMSG_DATA(glh))) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char*)(na) + NLA_HDRLEN))

static int dgwy_noop_cb(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}


int char_dev_init(int major)
{
    char command[64];
    //char buf[DGWY_IOCTL_MAX_SZ];
    FILE *fp; 

    if(dgwy_chardev_fd != -1)
    {
        printf("%s:Inited..\n",__FUNCTION__);
        return 0;
    }

    memset(&dpsCfg,0,sizeof(dpsCfg));
    memset(nexthop_list,0,sizeof(nexthop_list));
    /* cleanup if exist */
    memset(command,0,64);
    sprintf(command,"unlink /dev/dgwy_ctrl_device");
    fp = popen(command, "r");
    pclose(fp);

    memset(command,0,64);
    sprintf(command,"mknod /dev/dgwy_ctrl_device c %d 0",major);

    fp = popen(command, "r");
                
    if(fp == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Error popen:%s\n",command);
        pclose(fp);
        return -1;
    }
    pclose(fp);

    if ((dgwy_chardev_fd = open("/dev/dgwy_ctrl_device", O_RDWR)) < 0) 
    {
        perror("open");
        return -1;
    }

    /*
    if(ioctl(dgwy_chardev_fd, DGWY_READ_IOCTL, buf) < 0)
    {
        perror("second ioctl");
    }
    printf("message: %s\n", buf);
    */
    return 0;
}

int char_dev_exit(void)
{
    if(dgwy_chardev_fd >= 0)
    {
        FILE *fp; 
        char command[64];

        close(dgwy_chardev_fd);

        dgwy_chardev_fd=-1;

        memset(command,0,64);
        sprintf(command,"unlink /dev/dgwy_ctrl_device");

        fp = popen(command, "r");
                    
        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                        "Error popen:%s\n",command);
            pclose(fp);
            return -1;
        }
        pclose(fp);
    }
    return 0;
}


static int dgwy_getinfo_parse_cb(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[DGWY_CTRL_ATTR_MAX + 1];
    dgwy_ctrl_info_resp_t *info_resp;

    if(nlh)
    {
        msg_payload_t *resp = (msg_payload_t *)nlh;
        struct nlattr *na = (struct nlattr *) GENLMSG_DATA(resp);
        info_resp = (dgwy_ctrl_info_resp_t*) NLA_DATA(na);

        /*printf("nlhlen=%d[%zu] cmd=%d version=%d\n", 
               nlh->nlmsg_len, GENLMSG_PAYLOAD(&resp->n), 
               resp->g.cmd, resp->g.version);*/
        if(info_resp)
        {
            log_debug(ServiceUtilLogLevel,
                    "Got Info Response: version:%d cmd=%d major=%d len=%d\n",
                   info_resp->version, info_resp->cmd, 
                   info_resp->chardev_major,
                   nla_len(attrs[DGWY_CTRL_ATTR_MAX]));
            if(info_resp->chardev_major > 0)
            {
                char_dev_init(info_resp->chardev_major);
            }
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                    "Error: get info response \n");
        }
    }

    return NL_OK;
}

#if 0
static int dgwy_ctrl_listener_parse_cb(struct nl_msg *msg, void *arg)
{
    struct nlmsghdr *nlh = nlmsg_hdr(msg);
    struct nlattr *attrs[DGWY_CTRL_ATTR_MAX + 1];
    dgwy_ctrl_event_resp_t *event_resp;

    if(nlh)
    {
        msg_payload_t *resp = (msg_payload_t *)nlh;
        struct nlattr *na = (struct nlattr *) GENLMSG_DATA(resp);
        event_resp = (dgwy_ctrl_event_resp_t*) NLA_DATA(na);

        /*printf("nlhlen=%d[%zu] cmd=%d version=%d\n", 
               nlh->nlmsg_len, GENLMSG_PAYLOAD(&resp->n), 
               resp->g.cmd, resp->g.version);*/
        if(event_resp)
        {
            printf("Got EVENT Response: version:%d cmd=%d len=%d\n",
                   event_resp->version, event_resp->cmd, nla_len(attrs[DGWY_CTRL_ATTR_MAX]));
        }
        else
        {
            printf("Error: get info response \n");
        }
    }

    return NL_OK;
}


int dgwy_nl_test_listener(nl_recvmsg_msg_cb_t func, void *arg)
{
    int err = EINVAL;
    struct nl_msg *msg;

    if (listener_sock == NULL) 
    {
        printf("||||| LISTEN SOCKET ERROR |||||\n");
        return 0;
    }
    
    msg = dgwy_nl_message(DGWY_CTRL_CMD_LISTENER_TEST, 0);

    /* To test connections and set the family */
    if (msg == NULL) 
    {
        printf("||||| LISTEN SOCKET ERROR |||||\n");
        nl_handle_destroy(listener_sock);
        listener_sock = NULL;
        return 0;
    } 


    if (nl_socket_modify_cb(listener_sock, NL_CB_VALID, 
                NL_CB_CUSTOM, func, arg) != 0)
    {
        goto fail_genl;
    }


    if (nl_send_auto_complete(listener_sock, msg) < 0) 
    {
        goto fail_genl;
    }

    if ((err = -nl_recvmsgs_default(listener_sock)) > 0) 
    {
        goto fail_genl;
    }

    nlmsg_free(msg);

    return 0;

fail_genl:
    printf("||||| LISTEN SOCKET ERROR |||||\n");
    nl_handle_destroy(listener_sock);
    listener_sock = NULL;
    nlmsg_free(msg);
    errno = err; 
    return -1;
}


int dgwy_ctrl_listener_test(void)
{
    return dgwy_nl_test_listener(dgwy_ctrl_listener_parse_cb, NULL);
}

int dgwy_ctrl_listener(void)
{
    struct nl_msg *msg;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_LISTENER, 0);
    if (msg)
    {
        printf("INIT dgwy_nl_init_listener\n");
        return dgwy_nl_init_listener(msg, dgwy_ctrl_listener_parse_cb, NULL);
    }
    else
    {
        printf("INIT listener msg failed\n");
    }
    return -1;
}
#endif



int dgwy_getinfo(void)
{
    struct nl_msg *msg;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_GETINFO, 0);
    if (msg)
    {
        return dgwy_nl_send_get_info_message(msg, dgwy_getinfo_parse_cb, NULL);
    }
    return -1;
}

int dgwy_ctrl_nl_init(void)
{
    dgwy_cb_func = (void *)dgwy_ctrl_nl_init;

    nlmsg_set_default_size(2*getpagesize());

    if(dgwy_nl_send_message(NULL,NULL,NULL)==0)
    {
        return dgwy_getinfo();
    }
    return -1;
}

/*
 ******************************************************************************
 * dgwy_request_bcast_lst                                                 *//**
 *
 * \brief - Request bcast list from DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_request_bcast_lst(uint32_t domain, uint16_t dvg)
{
    dps_client_data_t       dps_client_data;

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_BCAST_LIST_REQ;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.resp_status = 0;

    /*
     * Send request to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send bcast request to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent domain %d bcast request to DPS Server",domain);
        return 0;
    }
}



/*
 ******************************************************************************
 * dgwy_request_extgw_lst                                                   *//**
 *
 * \brief - Request EXT GW list from DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_request_extgw_lst(uint32_t vnid)
{
    dps_client_data_t       dps_client_data;

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_EXTERNAL_GW_LIST_REQ;
    dps_client_data.hdr.vnid = vnid;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.resp_status = 0;

    /*
     * Send request to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send ExtGW LIST request to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VNID %d ExtGW LIST request to DPS Server",vnid);
        return 0;
    }
}


/*
 ******************************************************************************
 * dgwy_request_vlangw_lst                                                   *//**
 *
 * \brief - Request VLAN GW list from DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_request_vlangw_lst(uint32_t vnid)
{
    dps_client_data_t       dps_client_data;

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_VLAN_GW_LIST_REQ;
    dps_client_data.hdr.vnid = vnid;
    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.resp_status = 0;

    /*
     * Send request to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send vgwy list request to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent vnid %d vgwy list request to DPS Server",vnid);
        return 0;
    }
}




#if 0
/*
 ******************************************************************************
 * dgwy_request_intgw_lst                                                 *//**
 *
 * \brief - Request internal gw list from DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_request_intgw_lst(uint32_t domain, uint16_t dvg)
{
    dps_client_data_t       dps_client_data;

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_INTERNAL_GW_REQ;
    dps_client_data.hdr.vnid = domain;
    dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    dps_client_data.hdr.resp_status = 0;

    /*
     * Send request to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send intgw request to DPS Client");
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent domain %d intgw request to DPS Server",domain);
        return 0;
    }
}
#endif

/*
 ******************************************************************************
 * dgwy_register_mac_location                                                 *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 * \param[in] vm IP  [HOST BYTE ORDER]
 * \param[in] physical IP [HOST BYTE ORDER}
 * \param[in] DVG [HOST BYTE ORDER]
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_register_mac_location(uint32_t domain, uint32_t vIP, 
                           uint32_t pIP, uint16_t dvg, uint8_t *mac)
{
    dps_endpoint_update_t   dps_endpoint_update;
    dps_client_data_t       dps_client_data;

    log_debug(ServiceUtilLogLevel,"Received VM Registration Request \n");

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&dps_endpoint_update, 0, sizeof(dps_endpoint_update_t));

    dps_endpoint_update.vnid = domain;

    dps_endpoint_update.vm_ip_addr.ip4 = vIP;
    dps_endpoint_update.tunnel_info.num_of_tunnels=1;
    dps_endpoint_update.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN; 
    dps_endpoint_update.tunnel_info.tunnel_list[0].ip4 = pIP;
    dps_endpoint_update.tunnel_info.tunnel_list[0].vnid = domain;
    dps_endpoint_update.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_endpoint_update.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;
    if(mac)
    {
        memcpy(dps_endpoint_update.mac, mac, 6);
    }

    memcpy((void *)&dps_client_data.endpoint_update, 
           (void *)&dps_endpoint_update, 
           sizeof(dps_client_data.endpoint_update));

    dps_client_data.hdr.type = DPS_ENDPOINT_UPDATE;
    dps_client_data.hdr.sub_type = DPS_ENDPOINT_UPDATE_ADD;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }


    dps_client_data.hdr.resp_status = 0;

    log_debug(ServiceUtilLogLevel,
            "Sending VM Registration Request to DPS: \
            Domain Id: 0x%x \
            VM IP: 0x%x \
            Dove Switch IP: 0x%x \
            VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            dps_endpoint_update.vnid,
            dps_endpoint_update.vm_ip_addr.ip4,
            dps_endpoint_update.tunnel_info.tunnel_list[0].ip4,
            dps_endpoint_update.mac[0],
            dps_endpoint_update.mac[1],
            dps_endpoint_update.mac[2],
            dps_endpoint_update.mac[3],
            dps_endpoint_update.mac[4],
            dps_endpoint_update.mac[5] );

    /*
     * Send EndPoint Update message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send VM Registration Information to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VM Registration Information to DPS Server");
        return 0;
    }
}


/*
 ******************************************************************************
 * dgwy_register_vip_location                                             *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 * \param[in] vm IP  [HOST BYTE ORDER]
 * \param[in] physical IP [HOST BYTE ORDER}
 * \param[in] DVG [HOST BYTE ORDER]
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_register_vip_location(uint32_t domain, uint32_t vIP, 
                           uint32_t pIP, uint16_t dvg, uint8_t *mac)
{
    dps_endpoint_update_t   dps_endpoint_update;
    dps_client_data_t       dps_client_data;

    log_debug(ServiceUtilLogLevel,"Received VM Registration Request \n");

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&dps_endpoint_update, 0, sizeof(dps_endpoint_update_t));

    dps_endpoint_update.vnid = domain;

    dps_endpoint_update.vm_ip_addr.ip4 = vIP;
    dps_endpoint_update.tunnel_info.num_of_tunnels=1;
    dps_endpoint_update.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN; 
    dps_endpoint_update.tunnel_info.tunnel_list[0].ip4 = pIP;
    dps_endpoint_update.tunnel_info.tunnel_list[0].vnid = domain;
    dps_endpoint_update.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_endpoint_update.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;

    if(mac)
    {
        memcpy(dps_endpoint_update.mac, mac, 6);
    }

    memcpy((void *)&dps_client_data.endpoint_update, 
           (void *)&dps_endpoint_update, 
           sizeof(dps_client_data.endpoint_update));

    dps_client_data.hdr.type = DPS_ENDPOINT_UPDATE;
    dps_client_data.hdr.sub_type = DPS_ENDPOINT_UPDATE_VIP_ADD;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.resp_status = 0;

    log_debug(ServiceUtilLogLevel,
              "Sending VM Registration Request to DPS: \
              Domain Id: 0x%x \
              VM IP: 0x%x \
              Dove Switch IP: 0x%x \
              VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
              dps_endpoint_update.vnid,
              dps_endpoint_update.vm_ip_addr.ip4,
              dps_endpoint_update.tunnel_info.tunnel_list[0].ip4,
              dps_endpoint_update.mac[0],
              dps_endpoint_update.mac[1],
              dps_endpoint_update.mac[2],
              dps_endpoint_update.mac[3],
              dps_endpoint_update.mac[4],
              dps_endpoint_update.mac[5] );

    /*
     * Send EndPoint Update message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send VM Registration Information to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VM Registration Information to DPS Server");
        return 0;
    }
}



/*
 ******************************************************************************
 * dgwy_deregister_mac_location                                           *//**
 *
 * \brief - Send VM De-Registartion to DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 * \param[in] vm IP  [HOST BYTE ORDER]
 * \param[in] physical IP [HOST BYTE ORDER}
 * \param[in] DVG [HOST BYTE ORDER]
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_deregister_mac_location(uint32_t domain, uint32_t vIP, 
                             uint32_t pIP, uint16_t dvg, uint8_t *mac)
{
    dps_endpoint_update_t   dps_endpoint_update;
    dps_client_data_t       dps_client_data;

    log_debug(ServiceUtilLogLevel,"Received VM De-Registration Request \n");

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&dps_endpoint_update, 0, sizeof(dps_endpoint_update_t));

    dps_endpoint_update.vnid = domain;

    dps_endpoint_update.vm_ip_addr.ip4 = vIP;
    dps_endpoint_update.tunnel_info.num_of_tunnels=1;
    dps_endpoint_update.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN; 
    dps_endpoint_update.tunnel_info.tunnel_list[0].ip4 = pIP;
    dps_endpoint_update.tunnel_info.tunnel_list[0].vnid = domain;
    dps_endpoint_update.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_endpoint_update.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;

    if(mac)
    {
        memcpy(dps_endpoint_update.mac, mac, 6);
    }

    memcpy((void *)&dps_client_data.endpoint_update, 
           (void *)&dps_endpoint_update, 
           sizeof(dps_client_data.endpoint_update));

    dps_client_data.hdr.type = DPS_ENDPOINT_UPDATE;
    dps_client_data.hdr.sub_type = DPS_ENDPOINT_UPDATE_DELETE;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }


    dps_client_data.hdr.resp_status = 0;

    log_debug(ServiceUtilLogLevel,
            "Sending VM De-Registration Request to DPS: \
            Domain Id: 0x%x \
            VM IP: 0x%x \
            Dove Switch IP: 0x%x \
            VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            dps_endpoint_update.vnid,
            dps_endpoint_update.vm_ip_addr.ip4,
            dps_endpoint_update.tunnel_info.tunnel_list[0].ip4,
            dps_endpoint_update.mac[0],
            dps_endpoint_update.mac[1],
            dps_endpoint_update.mac[2],
            dps_endpoint_update.mac[3],
            dps_endpoint_update.mac[4],
            dps_endpoint_update.mac[5] );

    /*
     * Send EndPoint Update message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send VM De-Registration Information to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VM De-Registration Information to DPS Server");
        return 0;
    }
}

/*
 ******************************************************************************
 * dgwy_deregister_vip_location                                           *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 * \param[in] vm IP  [HOST BYTE ORDER]
 * \param[in] physical IP [HOST BYTE ORDER}
 * \param[in] DVG [HOST BYTE ORDER]
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_deregister_vip_location(uint32_t domain, uint32_t vIP, 
                             uint32_t pIP, uint16_t dvg, uint8_t *mac)
{
    dps_endpoint_update_t   dps_endpoint_update;
    dps_client_data_t       dps_client_data;

    log_debug(ServiceUtilLogLevel,"Received VM De-Registration Request \n");

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&dps_endpoint_update, 0, sizeof(dps_endpoint_update_t));

    dps_endpoint_update.vnid = domain;

    dps_endpoint_update.vm_ip_addr.ip4 = vIP;
    dps_endpoint_update.tunnel_info.num_of_tunnels=1;
    dps_endpoint_update.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN; 
    dps_endpoint_update.tunnel_info.tunnel_list[0].ip4 = pIP;
    dps_endpoint_update.tunnel_info.tunnel_list[0].vnid = domain;
    dps_endpoint_update.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_endpoint_update.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;

    if(mac)
    {
        memcpy(dps_endpoint_update.mac, mac, 6);
    }

    memcpy((void *)&dps_client_data.endpoint_update, 
           (void *)&dps_endpoint_update, 
           sizeof(dps_client_data.endpoint_update));

    dps_client_data.hdr.type = DPS_ENDPOINT_UPDATE;
    dps_client_data.hdr.sub_type = DPS_ENDPOINT_UPDATE_VIP_DEL;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    //dps_client_data.hdr.resp_status = DPS_ENDPOINT_UPDATE_VIP_DEL;
    dps_client_data.hdr.resp_status = 0;

    log_debug(ServiceUtilLogLevel,
            "Sending VM Registration Request to DPS: \
            Domain Id: 0x%x \
            VM IP: 0x%x \
            Dove Switch IP: 0x%x \
            VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            dps_endpoint_update.vnid,
            dps_endpoint_update.vm_ip_addr.ip4,
            dps_endpoint_update.tunnel_info.tunnel_list[0].ip4,
            dps_endpoint_update.mac[0],
            dps_endpoint_update.mac[1],
            dps_endpoint_update.mac[2],
            dps_endpoint_update.mac[3],
            dps_endpoint_update.mac[4],
            dps_endpoint_update.mac[5] );

    /*
     * Send EndPoint Update message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send VM Registration Information to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VM Registration Information to DPS Server");
        return 0;
    }
}

/*
 ******************************************************************************
 * dgwy_location_unavailable                                              *//**
 *
 * \brief - Send VM unavailable to DPS server.
 *
 **
 * \param[in] domain [HOST BYTE ORDER]
 * \param[in] vm IP  [HOST BYTE ORDER]
 * \param[in] physical IP [HOST BYTE ORDER}
 * \param[in] DVG [HOST BYTE ORDER]
 * \param[in] VM MAC
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ******************************************************************************
 */
int dgwy_location_unavailable(uint32_t domain, uint32_t dst_vIP, 
                              uint32_t src_vIP, uint32_t pIP, 
                              uint8_t *dst_vMAC, uint8_t *src_vMAC,
                              uint16_t dvg)
{
    dps_vm_migration_event_t    vm_migration_event;
    dps_client_data_t           dps_client_data;

    log_debug(ServiceUtilLogLevel,"Received VM Unavailable Request \n");

    /**
     * Populate the registration record to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));
    memset(&vm_migration_event, 0, sizeof(dps_vm_migration_event_t));

    vm_migration_event.migrated_vm_info.vm_ip_addr.ip4 = dst_vIP;
    if(dst_vMAC)
    {
        memcpy(vm_migration_event.migrated_vm_info.mac, dst_vMAC, 6);
    }

    vm_migration_event.src_vm_loc.vnid = domain;
    vm_migration_event.src_vm_loc.tunnel_info.num_of_tunnels=1;
    vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN;
    vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].ip4 = pIP;
    vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].vnid = domain;
    vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].family = AF_INET;
    vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;

    vm_migration_event.src_vm_loc.vm_ip_addr.ip4 = src_vIP;
    if(src_vMAC)
    {
        memcpy(vm_migration_event.src_vm_loc.mac, src_vMAC, 6);
    }


    memcpy((void *)&dps_client_data.vm_migration_event, 
           (void *)&vm_migration_event, 
           sizeof(dps_client_data.vm_migration_event));

    dps_client_data.hdr.type = DPS_VM_MIGRATION_EVENT;
    dps_client_data.hdr.vnid = domain;

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.resp_status = 0;

    log_debug(ServiceUtilLogLevel,
            "Sending VM MigrationEvent to DPS: \
            Domain Id: 0x%x \
            DVG: 0x%x \
            VM IP: 0x%x \
            Src Dove Switch IP: 0x%x \n",
            domain, dvg, dst_vIP, pIP);

    /*
     * Send EndPoint Update message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                  "Failed to send VM Migration Event to DPS Client");
        g_got_dcs_info=0;
        return -1;
    } 
    else 
    {
        log_error(ServiceUtilLogLevel,
                  "Sent VM Unavailable Information to DPS Server");
        return 0;
    }
}

/*
 ***************************************************************************
 * find_vnid_subnet_nexthop                                            *//**
 *
 * \brief - Find nexthop IP for the VNID given an IP
 *
 * \param[in] svc vnid 
 * \param[in] ipv4
 *
 * 
 * \retval - nexthop ip
 * \retval -[-1] error
 *
 ***************************************************************************
 */
uint32_t find_vnid_subnet_nexthop(uint32_t vnid,  
                                  uint32_t IPv4)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    uint32_t nexthop=0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get((char *)"APBR", &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
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

       /* add operation */
       for(i=0; i<MAX_VNID_SUBNETS; i++)
       {
           if(svcp->vnid_subnet[i].IPv4)
           {
               log_info(ServiceUtilLogLevel,
                        "vnid 0x%x[0x%x] ip 0x%x[0x%x]\n",
                        svcp->vnid_subnet[i].vnid,
                        vnid, 
                        svcp->vnid_subnet[i].IPv4&svcp->vnid_subnet[i].IPv4_mask,
                        IPv4&svcp->vnid_subnet[i].IPv4_mask);
           }
           if((svcp->vnid_subnet[i].IPv4) &&
              (svcp->vnid_subnet[i].vnid == vnid))
           {
               if((svcp->vnid_subnet[i].IPv4&
                   svcp->vnid_subnet[i].IPv4_mask)==
                  (IPv4&svcp->vnid_subnet[i].IPv4_mask))
               {
                   /* given vnid and ip's network-id
                    * matches with subnet configuration
                    * */
                   nexthop = svcp->vnid_subnet[i].IPv4_nexthop;
                   break;
               }
           }
       }

       SVC_UNLOCK(svcp->lock);
       return nexthop;
    }
    else
    {
        return 0;
    }
}

int dgwy_ctrl_set_legacy_nexthop(uint32_t vnid, uint32_t ipv4, uint32_t nexthop)
{
    dgwy_cb_func = (void*)dgwy_ctrl_set_legacy_nexthop;
    struct nl_msg *msg;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_legacy_nexthop_info_t svcp;
        svcp.cmd = CMD_LEGACY_NXTHOP;
        svcp.vnid = vnid;
        svcp.legacy_ip4 = ipv4;
        svcp.legacy_nexthop = nexthop;

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_legacy_nexthop_info_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

/*
 ******************************************************************************
 * dove_location_vip_update                                                   *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_vip_update(dgwy_dps_query_list_info_t *info)
{
    uint32_t nexthop=0;
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    if(is_mac_found_in_peer((char*)info->endpoint_mac))
    {
        /* mac found in peer vlan gw
         * don't register them
         * */
        return;
    }

    log_info(ServiceUtilLogLevel,
            "info->ip 0x%x phyip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, info->phyip4,
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    dgwy_register_vip_location(domain_horder, ntohl(info->ip4), 
                               ntohl(info->phyip4), 0, info->endpoint_mac);

    nexthop = find_vnid_subnet_nexthop(domain_horder, info->ip4);
    if(nexthop)
    {
        /* notify nexthop of legacy to datapath */
        dgwy_ctrl_set_legacy_nexthop(info->domain,info->ip4,nexthop);
    }
    else
    {
        log_info(ServiceUtilLogLevel,
                 "Legacy IP's[0x%x] nexthop info not found in config\n",
                 info->ip4);
    }
}



/*
 ******************************************************************************
 * dove_location_mac_update                                                   *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_mac_update(dgwy_dps_query_list_info_t *info)
{
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    log_info(ServiceUtilLogLevel,
            "info->ip 0x%x phyip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, info->phyip4,
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    dgwy_register_mac_location(domain_horder, ntohl(info->ip4), 
                           ntohl(info->phyip4), 0, info->endpoint_mac);
}


/*
 ******************************************************************************
 * dove_location_mac_delete                                                   *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_mac_delete(dgwy_dps_query_list_info_t *info)
{
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    log_info(ServiceUtilLogLevel,
            "info->ip 0x%x phyip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, info->phyip4,
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    dgwy_deregister_mac_location(domain_horder, ntohl(info->ip4), 
                             ntohl(info->phyip4), 0, info->endpoint_mac);
}


/*
 ******************************************************************************
 * dove_location_legacy_unavailable                                       *//**
 *
 * \brief - Send Location unavailable to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_legacy_unavailable (dgwy_dps_query_list_info_t *info)
{
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    log_info(ServiceUtilLogLevel,
            "info->ip 0x%x phyip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, info->phyip4,
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    dgwy_location_unavailable(domain_horder, 
                              ntohl(info->ip4),         /* dst vm IPV4 */
                              ntohl(info->src_ipv4),     /* src vm IPV4 */
                              ntohl(info->phyip4),      /* src dove IPV4*/
                              info->endpoint_mac,       /* dst vm mac */
                              info->src_endpoint_mac,   /* dst vm mac */
                              0                         /* dvg */
                              );

}

/*
 ******************************************************************************
 * dove_location_vip_delete                                                   *//**
 *
 * \brief - Send VM Registartion to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_vip_delete(dgwy_dps_query_list_info_t *info)
{
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    log_info(ServiceUtilLogLevel,
            "info->ip 0x%x phyip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, info->phyip4,
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    dgwy_deregister_vip_location(domain_horder, ntohl(info->ip4), 
                             ntohl(info->phyip4), 0, info->endpoint_mac);
}

#if 0
/*
 ******************************************************************************
 * dove_location_request_shared                                           *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_request_shared(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    int i=0,j=0;
    struct sockaddr_in dip;
    char *dst_address = NULL;
    uint32_t mask=0;
    uint32_t nexthop=0;

    /* validate self ip */
    dip.sin_addr.s_addr = info->ip4;
    dst_address = inet_ntoa(dip.sin_addr);

    log_debug(ServiceUtilLogLevel, "Enter %s\n",dst_address);

    if(dst_address && (chk_self_ipv4_getmask(dst_address, &mask) == 1))
    {
        dgwy_service_config_t svcp;
        FILE *fp; 
        char command[64];      
        char popen_result[16];
        memset(popen_result,0,16);
        memset(command,0,64);
        /* XXX TODO
         * if we know the serivice name 
         * we can format this lookup as
         * ip route get <dst-ip> oif <svc-name>
         * */
        sprintf(command,"ip route show | busybox awk '/default via/{print $3}'");
        log_debug(ServiceUtilLogLevel,
                 "Command:[%s]\n",command);
        fp = popen(command, "r");
        
        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                      "Error popen:%s\n",command);
        }
        else
        {
            if(fgets(popen_result, 16, fp))
            {
                nexthop =  inet_addr(popen_result);
            }
            fclose(fp);
        }


        svcp.ipv4_config.cmd = CMD_ADD_IPV4;
        svcp.ipv4_config.ifipv4 = info->ip4;
        svcp.ipv4_config.mask = mask;
        svcp.ipv4_config.nexthop = info->ip4;
        dgwy_ctrl_svc_ipv4(&svcp, 0);

        log_info(ServiceUtilLogLevel,
                  "Add self IP AUTO 0x%x mask %x nexthop %x",
                  info->ip4, mask, nexthop);
        return;
    }
                        

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    for(j=SVC_INDX_START; j<MAX_SERVICE_TABLES; j++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[j];
        
        if(svcp)
        {
            for(i=0; i<MAX_VNID_SUBNETS; i++)
            {
                if(svcp->vnid_subnet[i].IPv4)
                {
                    if(svcp->vnid_subnet[i].subnet_mode)
                    {
                        uint32_t domain_horder = svcp->vnid_subnet[i].vnid;
                        log_info(ServiceUtilLogLevel,
                                "info->ip 0x%x domain %d[%d] "
                                "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
                                info->domain, domain_horder,info->endpoint_mac[0],
                                info->endpoint_mac[1],info->endpoint_mac[2],
                                info->endpoint_mac[3],info->endpoint_mac[4],
                                info->endpoint_mac[5]);

                        /**
                         * Populate the location request to be sent to DPS
                         */
                        memset(&dps_client_data, 0, sizeof(dps_client_data_t));

                        dps_client_data.hdr.type = DPS_ENDPOINT_LOC_REQ;

                        if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
                        {
                            dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                        }
                        else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
                        {
                            dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
                        }
                        else
                        {
                            dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
                        }

                        dps_client_data.hdr.vnid = domain_horder;
                        dps_client_data.endpoint_loc_req.vnid = domain_horder;
                        dps_client_data.endpoint_loc_req.vm_ip_addr.family=AF_INET;
                        dps_client_data.endpoint_loc_req.vm_ip_addr.ip4 = 
                                                                ntohl(info->ip4);
                        memcpy(dps_client_data.endpoint_loc_req.mac,
                               info->endpoint_mac, 6);
                        pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
                        if(pdomain)
                        {
                            /* should free when reply comes back */
                            memset(pdomain,0,VNID_CTX_SZ);
                            stat_ctx_alloc++;
                            pdomain->svnid = domain_horder;
                            pdomain->quid  = dps_ctx_quid;
                            pdomain->shared_lookup=1;
                            dps_client_data.context = pdomain;
                        }
                        else
                        {
                            dps_client_data.context = NULL;
                        }
                        

                        /*
                         * Send Location Information Request message to DPS (via DPS Client)
                         */
                        dps_ctx_quid++;
                        if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
                        {
                            /**
                             * Log error message
                             */
                            log_error(ServiceUtilLogLevel,
                                        "Failed to send Location Request to DPS Client\n");
                            g_got_dcs_info=0;
                        } 
                        else 
                        {
                            log_debug(ServiceUtilLogLevel,
                                     "Sent Location Request to DPS Client\n");
                        }
                    }
                }
            }
        }
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

    log_debug(ServiceUtilLogLevel, "Exit\n");
    return;
}
#endif

/*
 ******************************************************************************
 * dove_policy_request_shared                                           *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_policy_request_shared(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    int i=0,j=0;
    struct sockaddr_in dip;
    char *dst_address = NULL;
    uint32_t mask=0;
    uint32_t nexthop=0;

    /* validate self ip */
    dip.sin_addr.s_addr = info->ip4;
    dst_address = inet_ntoa(dip.sin_addr);

    log_debug(ServiceUtilLogLevel, "Enter %s\n",dst_address);

    if(dst_address && (chk_self_ipv4_getmask(dst_address, &mask) == 1))
    {
        dgwy_service_config_t svcp;
        FILE *fp; 
        char command[64];      
        char popen_result[16];
        memset(popen_result,0,16);
        memset(command,0,64);
        /* XXX TODO
         * if we know the serivice name 
         * we can format this lookup as
         * ip route get <dst-ip> oif <svc-name>
         * */
        sprintf(command,"ip route show | busybox awk '/default via/{print $3}'");
        log_debug(ServiceUtilLogLevel,
                 "Command:[%s]\n",command);
        fp = popen(command, "r");
        
        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                      "Error popen:%s\n",command);
        }
        else
        {
            if(fgets(popen_result, 16, fp))
            {
                nexthop =  inet_addr(popen_result);
            }
            fclose(fp);
        }


        svcp.ipv4_config.cmd = CMD_ADD_IPV4;
        svcp.ipv4_config.ifipv4 = info->ip4;
        svcp.ipv4_config.mask = mask;
        svcp.ipv4_config.nexthop = info->ip4;
        dgwy_ctrl_svc_ipv4(&svcp, 0);

        log_info(ServiceUtilLogLevel,
                  "Add self IP AUTO 0x%x mask %x nexthop %x",
                  info->ip4, mask, nexthop);
        return;
    }
                        

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    for(j=SVC_INDX_START; j<MAX_SERVICE_TABLES; j++)
    {
        dgwy_service_list_t *svcp = &GLB_SVC_TABLE[j];
        
        if(svcp)
        {
            for(i=0; i<MAX_VNID_SUBNETS; i++)
            {
                if(svcp->vnid_subnet[i].IPv4)
                {
                    if((svcp->vnid_subnet[i].subnet_mode) &&
                       (info->ip4&svcp->vnid_subnet[i].IPv4_mask) ==  /* qury ip match subnet id */
                       (svcp->vnid_subnet[i].IPv4_mask&svcp->vnid_subnet[i].IPv4))
                    {
                        uint32_t domain_horder = svcp->vnid_subnet[i].vnid;
                        log_info(ServiceUtilLogLevel,
                                "info->ip 0x%x domain %d[%d] "
                                "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
                                info->domain, domain_horder,info->endpoint_mac[0],
                                info->endpoint_mac[1],info->endpoint_mac[2],
                                info->endpoint_mac[3],info->endpoint_mac[4],
                                info->endpoint_mac[5]);


                        /**
                         * Populate the location request to be sent to DPS
                         */
                        memset(&dps_client_data, 0, sizeof(dps_client_data_t));

                        dps_client_data.hdr.type = DPS_POLICY_REQ;

                        if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
                        {
                            dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                        }
                        else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
                        {
                            dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
                        }
                        else
                        {
                            dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
                        }

                        dps_client_data.hdr.vnid = domain_horder;
                        //dps_client_data.policy_req.dst_endpoint.vnid = domain_horder;
                        dps_client_data.policy_req.src_endpoint.vnid = domain_horder;
                        dps_client_data.policy_req.dst_endpoint.vm_ip_addr.family=AF_INET;
                        dps_client_data.policy_req.dst_endpoint.vm_ip_addr.ip4 = ntohl(info->ip4);
                        memcpy(dps_client_data.policy_req.dst_endpoint.mac,
                               info->endpoint_mac, 6);
                        pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
                        if(pdomain)
                        {
                            /* should free when reply comes back */
                            memset(pdomain,0,VNID_CTX_SZ);
                            stat_ctx_alloc++;
                            pdomain->svnid = domain_horder;
                            pdomain->quid  = dps_ctx_quid;
                            pdomain->shared_lookup=1;
                            dps_client_data.context = pdomain;
                        }
                        else
                        {
                            dps_client_data.context = NULL;
                        }
                        

                        /*
                         * Send Location Information Request message to DPS (via DPS Client)
                         */
                        dps_ctx_quid++;
                        if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
                        {
                            /**
                             * Log error message
                             */
                            log_error(ServiceUtilLogLevel,
                                        "Failed to send Location Request to DPS Client\n");
                            g_got_dcs_info=0;
                        } 
                        else 
                        {
                            log_debug(ServiceUtilLogLevel,
                                     "Sent Location Request to DPS Client\n");
                        }
                    }
                }
            }
        }
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

    log_debug(ServiceUtilLogLevel, "Exit\n");
    return;
}




/*
 ******************************************************************************
 * dove_policy_request                                                  *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_policy_request(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    if((dpsCfg.dpsIp==0) || (init_done==0))
    {
        log_error(ServiceUtilLogLevel,
                  "Not ready to request \n");
        return;
    }

    if(info->endpoint_mac)
    {
        if(is_mac_found_in_peer((char*)info->endpoint_mac))
        {
            log_error(ServiceUtilLogLevel,
                      "PEER VLANGW LOOKUP Query dropped \n");
            return;
        }
    }

    if((ntohl(info->domain)) == SHARED_VNID_MARK)
    {
        /* look for shared space */
        dove_policy_request_shared(info); 
        return;
    }

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    /**
     * Populate the location request to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_POLICY_REQ;
    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    dps_client_data.hdr.vnid = domain_horder;

    //dps_client_data.policy_req.dst_endpoint.vnid = domain_horder;//info->domain;
    dps_client_data.policy_req.src_endpoint.vnid = domain_horder;//info->domain;
    dps_client_data.policy_req.dst_endpoint.vm_ip_addr.family = AF_INET;
    dps_client_data.policy_req.dst_endpoint.vm_ip_addr.ip4 = ntohl(info->ip4);
    memcpy(dps_client_data.policy_req.dst_endpoint.mac,
           info->endpoint_mac, 6);

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        memset(pdomain,0,VNID_CTX_SZ);
        /* should free when reply comes back */
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data.context = pdomain;
    }
    else
    {
        dps_client_data.context = NULL;
        return;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_debug (ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
        if(pdomain)
        {
            free(pdomain);
        }
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }
    return;
}

#if 0
/*
 ******************************************************************************
 * dove_location_request                                                  *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_location_request(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    if((dpsCfg.dpsIp==0) || (init_done==0))
    {
        log_error(ServiceUtilLogLevel,
                  "Not ready to request \n");
        return;
    }

    if((ntohl(info->domain)) == SHARED_VNID_MARK)
    {
        /* look for shared space */
        dove_location_request_shared(info); 
        return;
    }

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    /**
     * Populate the location request to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    dps_client_data.hdr.type = DPS_ENDPOINT_LOC_REQ;
    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    dps_client_data.hdr.vnid = domain_horder;

    dps_client_data.endpoint_loc_req.vnid = domain_horder;//info->domain;
    dps_client_data.endpoint_loc_req.vm_ip_addr.family = AF_INET;
    dps_client_data.endpoint_loc_req.vm_ip_addr.ip4 = ntohl(info->ip4);
    memcpy(dps_client_data.endpoint_loc_req.mac,
           info->endpoint_mac, 6);

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        memset(pdomain,0,VNID_CTX_SZ);
        /* should free when reply comes back */
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data.context = pdomain;
    }
    else
    {
        dps_client_data.context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }
    return;
}
#endif


/*
 ******************************************************************************
 * dove_mcast_dcs_rcvr_notify                                                  *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_mcast_dcs_rcvr_notify(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    if((dpsCfg.dpsIp==0) || (init_done==0))
    {
        log_error(ServiceUtilLogLevel,
                  "Not ready to request \n");
        return;
    }

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    /**
     * Populate the location request to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    if(info->type == DPS_MCAST_RECV_JOIN)
    {
        dps_client_data.hdr.type = DPS_MCAST_RECEIVER_JOIN;
    }
    else if(info->type == DPS_MCAST_RECV_LEAVE)
    {
        dps_client_data.hdr.type = DPS_MCAST_RECEIVER_LEAVE;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Invalid MSG Type\n");
        return;
    }

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.vnid = domain_horder;

    dps_client_data.mcast_receiver.tunnel_endpoint.family = AF_INET;
    dps_client_data.mcast_receiver.tunnel_endpoint.ip4 = ntohl(info->phyip4);

    dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.mcast_addr_type=MCAST_ADDR_V4;
    dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.u.mcast_ip4=ntohl(info->ip4);
    memcpy(dps_client_data.mcast_receiver.mcast_group_rec.mcast_addr.mcast_mac,
           info->endpoint_mac, 6);

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        memset(pdomain,0,VNID_CTX_SZ);
        /* should free when reply comes back */
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data.context = pdomain;
    }
    else
    {
        dps_client_data.context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }
    return;
}




/*
 ******************************************************************************
 * dove_mcast_dcs_sender_notify                                           *//**
 *
 * \brief - Send VM location request to DPS server.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_mcast_dcs_sender_notify(dgwy_dps_query_list_info_t *info)
{
    dps_client_data_t       dps_client_data;
    dgadm_dps_context_t     *pdomain = NULL;
    //uint32_t domain_horder = (ntohl(info->domain&0xffffff00))>>8;
    uint32_t domain_horder = (ntohl(info->domain<<8));

    if((dpsCfg.dpsIp==0) || (init_done==0))
    {
        log_error(ServiceUtilLogLevel,
                  "Not ready to request \n");
        return;
    }

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x domain %d[%d] "
            "mac %02x:%02x:%02x:%02x:%02x:%02x\n", info->ip4, 
            info->domain, domain_horder,info->endpoint_mac[0],
            info->endpoint_mac[1],info->endpoint_mac[2],
            info->endpoint_mac[3],info->endpoint_mac[4],
            info->endpoint_mac[5]);

    /**
     * Populate the location request to be sent to DPS
     */
    memset(&dps_client_data, 0, sizeof(dps_client_data_t));

    if(info->type == DPS_MCAST_SENDER_REGISTER)
    {
        dps_client_data.hdr.type = DPS_MCAST_SENDER_REGISTRATION;
    }
    else if(info->type == DPS_MCAST_SENDER_LEAVE)
    {
        dps_client_data.hdr.type = DPS_MCAST_SENDER_DEREGISTRATION;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "Invalid MSG Type\n");
        return;
    }

    if(g_APPBRIDGE_type==DGWY_TYPE_EXTERNAL)
    {
        dps_client_data.hdr.client_id = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
    }
    else if(g_APPBRIDGE_type==DGWY_TYPE_VLAN)
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }
    else
    {
        dps_client_data.hdr.client_id = DOVE_VLAN_GATEWAY_AGENT_ID;
    }

    dps_client_data.hdr.vnid = domain_horder;

    dps_client_data.mcast_sender.tunnel_endpoint.family = AF_INET;
    dps_client_data.mcast_sender.tunnel_endpoint.ip4 = ntohl(info->phyip4);

    dps_client_data.mcast_sender.mcast_addr.mcast_addr_type=MCAST_ADDR_V4;
    dps_client_data.mcast_sender.mcast_addr.u.mcast_ip4=ntohl(info->ip4);
    memcpy(dps_client_data.mcast_sender.mcast_addr.mcast_mac,
           info->endpoint_mac, 6);

    dps_client_data.mcast_sender.mcast_src_vm.vnid = domain_horder;
    dps_client_data.mcast_sender.mcast_src_vm.vm_ip_addr.family = AF_INET;
    dps_client_data.mcast_sender.mcast_src_vm.vm_ip_addr.ip4 = ntohl(info->src_ipv4);
    memcpy(dps_client_data.mcast_sender.mcast_src_vm.mac,
           info->src_endpoint_mac, 6);

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        memset(pdomain,0,VNID_CTX_SZ);
        /* should free when reply comes back */
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data.context = pdomain;
    }
    else
    {
        dps_client_data.context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(&dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
        if(pdomain)
        {
            free(pdomain);
            stat_ctx_alloc--;
        }
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }
    return;
}




/*
 ******************************************************************************
 * dove_dps_tunnel_register                                               *//**
 *
 * \brief - Send gateway tunnel register.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_dps_tunnel_register(dgwy_tunnel_info_t *info)
{
    dps_client_data_t       *dps_client_data=NULL;
    dgadm_dps_context_t     *pdomain = NULL;
    uint32_t domain_horder = info->vnid;
    int data_size = sizeof(dps_client_data_t) + 
                    sizeof(dps_tunnel_endpoint_t);

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x vnid %d type %d",
            info->dovenet_ipv4, info->vnid,
            info->type); 

    dps_client_data = (dps_client_data_t*)malloc(data_size);
    if(dps_client_data == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "Alloc failed\n");
        return;
    }
    /**
     * Populate the location request to be sent to DPS
     */
    memset(dps_client_data, 0, data_size);

    dps_client_data->hdr.type = DPS_TUNNEL_REGISTER;
    dps_client_data->hdr.client_id = info->type;
    dps_client_data->hdr.vnid = domain_horder;

    dps_client_data->tunnel_reg_dereg.tunnel_info.num_of_tunnels = 1;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].ip4= ntohl(info->dovenet_ipv4);
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].vnid = domain_horder;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN;

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        /* should free when reply comes back */
        memset(pdomain,0,VNID_CTX_SZ);
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data->context = pdomain;
    }
    else
    {
        dps_client_data->context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send tunnel register to DPS\n");
        g_got_dcs_info=0;

        if(pdomain)
        {
            free(pdomain);
        }
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }

    free(dps_client_data);

    return;
}

/*
 ******************************************************************************
 * dove_dps_tunnel_deregister                                               *//**
 *
 * \brief - Send gateway tunnel register.
 *
 *
 * \param[in] query info
 *   :info->domain [NETWORK BYTE ORDER]
 *   :info->ipv4 [NETWORK BYTE ORDER]
 *
 ******************************************************************************
 */
static void
dove_dps_tunnel_deregister(dgwy_tunnel_info_t *info)
{
    dps_client_data_t       *dps_client_data=NULL;
    dgadm_dps_context_t     *pdomain = NULL;
    uint32_t domain_horder = info->vnid;
    int data_size = sizeof(dps_client_data_t) + 
                    sizeof(dps_tunnel_endpoint_t);

    log_debug(ServiceUtilLogLevel,
            "info->ip 0x%x vnid %d type %d",
            info->dovenet_ipv4, info->vnid,
            info->type); 

    dps_client_data = (dps_client_data_t*)malloc(data_size);
    if(dps_client_data == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "Alloc failed\n");
        return;
    }
    /**
     * Populate the location request to be sent to DPS
     */
    memset(dps_client_data, 0, data_size);

    dps_client_data->hdr.type = DPS_TUNNEL_DEREGISTER;
    dps_client_data->hdr.sub_type = DPS_TUNNEL_DEREG;
    dps_client_data->hdr.client_id = info->type;
    dps_client_data->hdr.vnid = domain_horder;

    dps_client_data->tunnel_reg_dereg.tunnel_info.num_of_tunnels = 1;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].ip4= ntohl(info->dovenet_ipv4);
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].vnid = domain_horder;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN;

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        /* should free when reply comes back */
        memset(pdomain,0,VNID_CTX_SZ);
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data->context = pdomain;
    }
    else
    {
        dps_client_data->context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
        if(pdomain)
        {
            free(pdomain);
        }
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }

    free(dps_client_data);

    return;
}

void
gdb_dove_dps_tunnel_deregister(int type, uint32_t vnid)
{
    dps_client_data_t       *dps_client_data=NULL;
    dgadm_dps_context_t     *pdomain = NULL;
    uint32_t domain_horder = vnid;
    int data_size = sizeof(dps_client_data_t) + 
                    sizeof(dps_tunnel_endpoint_t);

    dps_client_data = (dps_client_data_t*)malloc(data_size);
    if(dps_client_data == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "Alloc failed\n");
        return;
    }
    /**
     * Populate the location request to be sent to DPS
     */
    memset(dps_client_data, 0, data_size);

    dps_client_data->hdr.type = DPS_TUNNEL_DEREGISTER;
    dps_client_data->hdr.sub_type = DPS_TUNNEL_DEREG;
    dps_client_data->hdr.client_id = type;
    dps_client_data->hdr.vnid = domain_horder;

    dps_client_data->tunnel_reg_dereg.tunnel_info.num_of_tunnels = 1;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].family = AF_INET;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].port = OVL_PROTO_DST_PORT;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].ip4= ntohl(g_dovenet_ipv4);
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].vnid = domain_horder;
    dps_client_data->tunnel_reg_dereg.tunnel_info.tunnel_list[0].tunnel_type = TUNNEL_TYPE_VXLAN;

    pdomain = (dgadm_dps_context_t *) malloc (VNID_CTX_SZ);
    if(pdomain)
    {
        /* should free when reply comes back */
        memset(pdomain,0,VNID_CTX_SZ);
        stat_ctx_alloc++;
        pdomain->svnid = domain_horder;
        pdomain->quid  = dps_ctx_quid;
        dps_client_data->context = pdomain;
    }
    else
    {
        dps_client_data->context = NULL;
    }
    

    /*
     * Send Location Information Request message to DPS (via DPS Client)
     */
    dps_ctx_quid++;
    if (dps_protocol_client_send(dps_client_data) != DPS_SUCCESS)
    {
        /**
         * Log error message
         */
        log_error(ServiceUtilLogLevel,
                    "Failed to send Location Request to DPS Client\n");
        g_got_dcs_info=0;
        if(pdomain)
        {
            free(pdomain);
        }
    } 
    else 
    {
        //printf("Sent Location Request to DPS Client\n");
    }

    free(dps_client_data);

    return;
}



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
int dgwy_dps_cli_lookup(uint32_t domain, uint32_t vmIp)
{
    dgwy_dps_query_list_info_t info;

    memset(&info, 0, sizeof(dgwy_dps_query_list_info_t));
    info.ip4 = (vmIp);
    //info.domain = htonl(domain<<8);
    info.domain = htonl(domain)>>8;

    log_debug(ServiceUtilLogLevel,"info.ip 0x%x[%x] domain %d[%d]\n", info.ip4,vmIp,
            domain, 
            info.domain);

    dove_policy_request(&info);
    return 0;
}



int dgwy_device_poll(void)
{
    struct pollfd ufds[1];
    int rv =0;
    
    ufds[0].fd = dgwy_chardev_fd;
    ufds[0].events = POLLIN;

    if(dgwy_chardev_fd >= 0)
    {
        rv = poll(ufds, 1, 500 /* 500 ms wait*/);
        if (rv == -1) 
        {
            perror("poll"); // error occurred in poll()
        }
        else if (rv == 0) 
        {

        }
        else 
        {
            int ret = 0;
            int sdf = 0;
            char buf[DGWY_IOCTL_MAX_SZ];
            ret = read(dgwy_chardev_fd,buf,DGWY_IOCTL_MAX_SZ);
            log_debug(ServiceUtilLogLevel, "message: %d\n", ret);
            while(ret>0)
            {
                dgwy_dps_query_list_info_t *info=
                    (dgwy_dps_query_list_info_t*)(&buf[sdf]);

                sdf += sizeof(dgwy_dps_query_list_info_t);
                ret -= sizeof(dgwy_dps_query_list_info_t);

                if(info->type == DPS_LOCATION_QUERY) 
                {
                    log_debug(ServiceUtilLogLevel,
                             "Lookup ip %x in domain 0x%x[network byte]\n", 
                             info->ip4, info->domain);
                    dove_policy_request(info);
                }
                else if(info->type == DPS_LOCATION_MAC_UPDATE)
                {
                    log_info(ServiceUtilLogLevel,
                             "Update ip %x in domain %d [network byte]\n", 
                             info->ip4, info->domain);
                    dove_location_mac_update(info);
                }
                else if(info->type == DPS_LOCATION_MAC_DELETE)
                {
                    log_info(ServiceUtilLogLevel,
                             "LOC_MAC_DEL ip %x in domain %d [network byte]\n", 
                             info->ip4, info->domain);
                    dove_location_mac_delete(info); 
                }
                else if(info->type == DPS_LOCATION_VIP_UPDATE)
                {
                    log_info(ServiceUtilLogLevel,
                             "Update ip %x in domain %d [network byte]\n", 
                             info->ip4, info->domain);
                    dove_location_vip_update(info);
                }
                else if(info->type == DPS_LOCATION_VIP_DELETE)
                {
                    log_info(ServiceUtilLogLevel,
                             "VIP DELETE ip %x in domain %d [network byte]\n", 
                             info->ip4, info->domain);
                    dove_location_vip_delete(info); 
                }
                else if(info->type == DPS_LOCATION_UNAVAILABLE)
                {
                    log_info(ServiceUtilLogLevel,
                             "Update ip %x in domain %d [network byte]\n", 
                             info->ip4, info->domain);
                    dove_location_legacy_unavailable(info); 
                }
                else if(info->type == DPS_MCAST_RECV_JOIN)
                {
                    log_info(ServiceUtilLogLevel,
                             "RECV JOIN mgip %x in domain %d "
                             "[network byte] tunip %x\n",
                             info->ip4, info->domain,
                             info->phyip4);
                    dove_mcast_dcs_rcvr_notify(info);
                }
                else if(info->type == DPS_MCAST_RECV_LEAVE)
                {
                    log_info(ServiceUtilLogLevel,
                             "RECV Leave mgip %x in domain %d "
                             "[network byte] tunip %x\n",
                             info->ip4, info->domain,
                             info->phyip4);
                    dove_mcast_dcs_rcvr_notify(info);
                }
                else if(info->type == DPS_MCAST_SENDER_REGISTER)
                {
                    log_info(ServiceUtilLogLevel,
                             "SENDR REG mgip %x in domain %d "
                             "[network byte] \n",
                             info->ip4, info->domain);
                    dove_mcast_dcs_sender_notify(info);
                }
                else if(info->type == DPS_MCAST_SENDER_LEAVE)
                {
                    log_info(ServiceUtilLogLevel,
                             "SENDR Leave  mgip %x in domain %d "
                             "[network byte] \n",
                             info->ip4, info->domain);
                    dove_mcast_dcs_sender_notify(info);
                }
                else if(info->type == DPS_MCAST_VGW_ALL_RECV_JOIN)
                {
                    log_info(ServiceUtilLogLevel,
                             "SENDR Leave  mgip %x in domain %d "
                             "[network byte] \n",
                             info->ip4, info->domain);
                    dove_mcast_dcs_vgw_all_recv_join();
                }
                else if(info->type == DPS_MCAST_VGW_ALL_RECV_LEAVE)
                {
                    log_info(ServiceUtilLogLevel,
                             "SENDR Leave  mgip %x in domain %d "
                             "[network byte] \n",
                             info->ip4, info->domain);
                    dove_mcast_dcs_vgw_all_recv_leave();
                }
            }
        }
    }
    return 0;
}

/* check ip match self
 * 1  -Yes 
 * 0  -No
 * -1 -Error
 * */
int chk_self_ipv4(char *ipchk)
{
    int domain = AF_INET;   
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i,s;
    size_t len = strlen(ipchk);
    
    memset(ifr,0,sizeof(ifr));

    s = socket(domain, SOCK_STREAM, 0);
    if (s < 0) 
    {
        perror("socket");   
        return -1;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof(ifr);
    
    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) 
    {
        perror("ioctl");
        close(s);
        return -1;
    }
    
    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    for (i = 0; i < ifs; i++) 
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;
        
        memset(ip,0,sizeof(ip));
        if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) 
        {
            perror("inet_ntop");
            close(s);
            return -1;
        }
                
        log_debug(ServiceUtilLogLevel,
                  "%s - %s\n", ifr[i].ifr_name, ip);

        if(strlen(ip) == len)
        {
            if(memcmp(ipchk,ip,len) == 0)
            {
                /* match self ip */
                close(s);
                return 1;
            }
        }
    }
    close(s);

    return 0;
}

/* check ip match self
 * 1  -Yes 
 * 0  -No
 * -1 -Error
 * Get the mask & nexthop
 * */
int chk_self_ipv4_getmask(char *ipchk, uint32_t *mask)
{
    int domain = AF_INET;   
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i,s;
    size_t len = strlen(ipchk);
    
    memset(ifr,0,sizeof(ifr));

    s = socket(domain, SOCK_STREAM, 0);
    if (s < 0) 
    {
        perror("socket");   
        return -1;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof(ifr);
    
    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) 
    {
        perror("ioctl");
        close(s);
        return -1;
    }
    
    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    for (i = 0; i < ifs; i++) 
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;
        
        memset(ip,0,sizeof(ip));
        if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) 
        {
            perror("inet_ntop");
            close(s);
            return -1;
        }
                
        log_debug(ServiceUtilLogLevel,
                  "%s - %s\n", ifr[i].ifr_name, ip);

        if(strlen(ip) == len)
        {
            if(memcmp(ipchk,ip,len) == 0)
            {
                struct ifreq ifreq;
                /* match self ip */
                close(s);

                s = socket(domain, SOCK_STREAM, 0);
                if (s < 0) 
                {
                    perror("socket");   
                    return -1;
                }

                ifreq.ifr_addr.sa_family = AF_INET;
                strncpy(ifreq.ifr_name, ifr[i].ifr_name, IFNAMSIZ-1);

                if (ioctl(s, SIOCGIFNETMASK, &ifreq) == -1) 
                {
                    perror("ioctl");
                    close(s);
                    return -1;
                }

                *mask = 
                    ((struct sockaddr_in *)&ifreq.ifr_addr)->sin_addr.s_addr;

                close(s);
                return 1;
            }
        }
    }
    close(s);

    return 0;
}


int dgwy_ctrl_bcast_lst(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_bcast_lst;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_broadcast_lst_t blst;

        memset(&blst,0,sizeof(blst));
        blst.cmd            = svcp->bcast_lst.cmd;
        blst.domain_id      = svcp->bcast_lst.domain_id;
        blst.num_v4_switch  = svcp->bcast_lst.num_v4_switch;
        blst.num_v6_switch  = svcp->bcast_lst.num_v6_switch;
        blst.sport          = svcp->bcast_lst.sport;
        blst.dport          = svcp->bcast_lst.dport;
        memcpy(&blst.switch_list, 
               &svcp->bcast_lst.switch_list, 
               sizeof(blst.switch_list));
        memcpy(&blst.source_ip_list, 
               &svcp->bcast_lst.source_ip_list, 
               sizeof(blst.source_ip_list));


        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_broadcast_lst_t),&blst);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;

}

static inline 
dgwy_return_status handle_dps_bcast_reply_lst(void* context, 
                                              dps_pkd_tunnel_list_t* p_bcast_lst,
                                              uint32_t domain)
{
    dgwy_return_status retVal = DGWY_SUCCESS;
    dgwy_service_config_t svcp;
    int i=0,j=0;
    int cnt=0;

    if(p_bcast_lst == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Bast Reply is NULL\n");
        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    if((p_bcast_lst->num_v4_tunnels + 
        p_bcast_lst->num_v6_tunnels) > MAX_BROADCAST_MEMBERS_PER_DOMAIN)
    {
        log_error(ServiceUtilLogLevel,
                    "Bcast List is too big!\n");

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    log_info(ServiceUtilLogLevel,
             "%s: domain %d, %d %d\n",__FUNCTION__,
             domain, p_bcast_lst->num_v4_tunnels, p_bcast_lst->num_v6_tunnels);

    memset(&svcp, 0, sizeof(dgwy_service_config_t));
    for(i=0; i<p_bcast_lst->num_v4_tunnels;i++)
    {
        char *dst_address = NULL;
        struct sockaddr_in dip;

        if(!p_bcast_lst->tunnel_list[i])
        {
            continue;
        }
        dip.sin_addr.s_addr = htonl(p_bcast_lst->tunnel_list[i]);
        dst_address = inet_ntoa(dip.sin_addr);

        log_error(ServiceUtilLogLevel," %s %x ",
                dst_address, p_bcast_lst->tunnel_list[i]);

        if(dst_address && (chk_self_ipv4(dst_address) == 0))
        {
            FILE *fp; 
            char command[64];      
            char popen_result[16];
            memset(popen_result,0,16);
            memset(command,0,64);
            /* XXX TODO
             * if we know the serivice name 
             * we can format this lookup as
             * ip route get <dst-ip> oif <svc-name>
             * */
            sprintf(command,"ip route get %s | busybox awk -F\" src \" \'{print $2}\'",
                    dst_address);
            log_debug(ServiceUtilLogLevel,
                      "Command:[%s]\n",command);
            fp = popen(command, "r");
            
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                          "Error popen:%s\n",command);
            }
            else
            {
                if(fgets(popen_result, 16, fp))
                {
                    svcp.bcast_lst.switch_list[cnt] = htonl(p_bcast_lst->tunnel_list[i]);
                    svcp.bcast_lst.source_ip_list[cnt] = inet_addr(popen_result);
                    log_debug(ServiceUtilLogLevel,
                              "BCAST to %x SRC location %x\n", 
                              svcp.bcast_lst.switch_list[cnt],
                              svcp.bcast_lst.source_ip_list[cnt]);
                    cnt++;
                }
                fclose(fp);
            }
        }
    }
    svcp.bcast_lst.num_v4_switch = cnt;
    svcp.bcast_lst.sport = OVL_PROTO_SRC_PORT;
    svcp.bcast_lst.dport = OVL_PROTO_DST_PORT;

    for(j=0; j<p_bcast_lst->num_v6_tunnels;i++,j++)
    {
        /* XXX TODO handle IPV6 */
        log_error(ServiceUtilLogLevel,
                    "TODO handle IPV6\n");
    }
    svcp.bcast_lst.cmd = CMD_BCAST_LIST;
    svcp.bcast_lst.domain_id = domain;
    svcp.bcast_lst.num_v6_switch = p_bcast_lst->num_v6_tunnels;

    dgwy_ctrl_bcast_lst(&svcp);


    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
    return (retVal);
}


int dgwy_ctrl_mcast_lst(dgwy_ctrl_mcast_lst_t *mlst)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_mcast_lst;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        log_debug(ServiceUtilLogLevel,
                 "size of msg=%d\n",sizeof(dgwy_ctrl_mcast_lst_t));

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_mcast_lst_t),mlst);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

static inline 
dgwy_return_status handle_dps_mcast_recv_list(void* context, 
                                              dps_mcast_receiver_ds_list_t *mds_list,
                                              uint32_t domain)
{
    int indx=0;
    uint32_t num_vnid = 0;
    dgwy_ctrl_mcast_lst_t  mlst;
    dgwy_return_status retVal = DGWY_SUCCESS;
    int ret=0;
    dps_pkd_tunnel_list_t *mcast_tunlist = mds_list->mcast_recvr_list; 
    uint8_t *tmp_char_ptr;


    memset(&mlst,0,sizeof(dgwy_ctrl_mcast_lst_t));
    mlst.cmd = CMD_MCAST_LIST;
    //mlst.vnid =  htonl(domain<<8);
    mlst.vnid =  htonl(domain)>>8;

    mlst.mcast_ip = htonl(mds_list->mcast_addr.u.mcast_ip4);
    memcpy(mlst.mcast_mac, 
           mds_list->mcast_addr.mcast_mac,6);

    log_info(ServiceUtilLogLevel,
             "VNID %d mip %x numrec=%d mac=%x:%x:%x:%x:%x:%x\n", 
             mlst.vnid, mlst.mcast_ip,
             mds_list->num_of_rec,
             mlst.mcast_mac[0],mlst.mcast_mac[1],
             mlst.mcast_mac[2],mlst.mcast_mac[3],
             mlst.mcast_mac[4],mlst.mcast_mac[5]);
        
    for(num_vnid=0;
        num_vnid<mds_list->num_of_rec;num_vnid++)
    {
        int num_v4_tunls=mcast_tunlist->num_v4_tunnels;
        int num_v6_tunls=mcast_tunlist->num_v6_tunnels;
        tmp_char_ptr = (uint8_t *)mcast_tunlist;
        
        if(num_v4_tunls > 0)
        {
            int i=0;
            //uint32_t vnid =  htonl(mds_list->mcast_recvr_list[num_vnid].vnid<<8);
            uint32_t vnid =  htonl(mcast_tunlist->vnid)>>8;

            log_info(ServiceUtilLogLevel,"numtunnl %d\n", num_v4_tunls);
            if(MAX_MCAST_RCVER < num_v4_tunls)
            {
                log_error(ServiceUtilLogLevel,
                          "MCAST List too large!\n");
                tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[num_v4_tunls + (num_v6_tunls << 2)]);
                mcast_tunlist = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
                continue;
            }

            for(i=0; i<num_v4_tunls; i++)
            {
                struct sockaddr_in dip;
                char *dst_address = NULL;
                if(mcast_tunlist->tunnel_list[i]==0)
                {
                    tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[num_v4_tunls + (num_v6_tunls << 2)]);
                    mcast_tunlist = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
                    continue;
                }

                dip.sin_addr.s_addr = htonl(mcast_tunlist->tunnel_list[i]);
                dst_address = inet_ntoa(dip.sin_addr);

                if(dst_address /*&& (chk_self_ipv4(dst_address) == 0)*/)
                {

                    log_info(ServiceUtilLogLevel,
                             "tunnel ip %s vnid %d\n",
                             dst_address, vnid);
                    mlst.recv_list[indx].vnid = vnid;
                    mlst.recv_list[indx].tep_ip = 
                        htonl(mcast_tunlist->tunnel_list[i]);
                    indx++;
                    if(indx >= MAX_MCAST_RCVER)
                    {
                        log_error(ServiceUtilLogLevel,
                                  "MCAST List too big!\n");

                        if(context)
                        {
                            free(context);
                            stat_ctx_alloc--;
                        }
                        return DGWY_ERROR;
                    }
                }
            }
        }
        tmp_char_ptr += dps_offsetof(dps_pkd_tunnel_list_t, tunnel_list[num_v4_tunls + (num_v6_tunls << 2)]);
        mcast_tunlist = (dps_pkd_tunnel_list_t *)tmp_char_ptr;
    }

    mlst.num_v4_switch = indx;
    mlst.sport = OVL_PROTO_SRC_PORT;
    mlst.dport = OVL_PROTO_DST_PORT;

    if(indx > 0)
    {
        ret =  dgwy_ctrl_mcast_lst(&mlst);
    }

    log_info(ServiceUtilLogLevel,"dgwy_ctrl_mcast_lst ret=%d indx=%d\n",
             ret,indx);

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
    return (retVal);
}



static inline 
dgwy_return_status handle_dps_unsolicited_loc_reply(void* context, 
                                                    dps_endpoint_loc_reply_t* p_loc_reply)
{
    dgwy_service_config_t svcp;
    dgwy_return_status retVal = DGWY_SUCCESS;

    if (p_loc_reply == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Loc Reply is NULL\n");
        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
   
    memset(&svcp,0,sizeof(svcp));
    svcp.ovl_location.cmd = CMD_REFRESH_OVL_IP;
    svcp.ovl_location.location.endpoint_ip = htonl(p_loc_reply->vm_ip_addr.ip4);
    svcp.ovl_location.location.ovl_header_proto = OVL_PROTO;
    svcp.ovl_location.location.ovl_src_port = OVL_PROTO_SRC_PORT;
    svcp.ovl_location.location.ovl_dst_port = OVL_PROTO_DST_PORT;
    memcpy(svcp.ovl_location.location.ovl_dst_mac, p_loc_reply->mac, 6);
    
    dgwy_ctrl_location(&svcp);

    return (retVal);
}


static inline 
dgwy_return_status process_dps_loc_resp_shared(void* context, 
                                               dps_endpoint_loc_reply_t* p_loc_reply)
{
    dgwy_service_config_t svcp;
    ovl_tunnel_cfg_t tuninfo;
    dgwy_return_status retVal = DGWY_SUCCESS;
    struct sockaddr_in dip;
    int found_self_ip=-1;
    int numTunnel=0;
    int locIter=0;
    char *dst_address = NULL;

    memset(&tuninfo,0,sizeof(tuninfo));
    numTunnel = p_loc_reply->tunnel_info.num_of_tunnels;

    if(context)
    {
        int i=0;
        dgadm_dps_context_t *pdomain =
            (dgadm_dps_context_t*)context;
        tuninfo.cmd = CMD_UPDATE_OVL_IP_SHARED;
        tuninfo.domain = htonl(pdomain->svnid)>>8;
        tuninfo.num_entry = numTunnel;
        for(i=0;i<numTunnel;i++)
        {
            tuninfo.location[i].dst_domain = 
                htonl(p_loc_reply->vnid)>>8;

            tuninfo.location[i].endpoint_location = 
                htonl(p_loc_reply->tunnel_info.tunnel_list[i].ip4);

            log_info(ServiceUtilLogLevel,"CTX svnid 0x%x :"
                      " phyIP 0x%x dstVnid 0x%x \n",
                      tuninfo.domain,
                      tuninfo.location[i].endpoint_location,
                      tuninfo.location[i].dst_domain);
        }
    }


    tuninfo.endpoint_ip=htonl(p_loc_reply->vm_ip_addr.ip4);
    memcpy(tuninfo.ovl_dst_mac, p_loc_reply->mac, 6);

    if(numTunnel > 1)
    {
        /* sort the list decending order */
        int i=0,j=0;
        for(i=0; i < (numTunnel-1) ; ++i)
        {
            for(j=0; j<numTunnel-1-i; ++j)
            {
                if(tuninfo.location[j].endpoint_location <
                   tuninfo.location[j+1].endpoint_location)
                {
                    ovl_tunnel_info_t tmp_info = tuninfo.location[j+1];
                    tuninfo.location[j+1] = tuninfo.location[j];
                    tuninfo.location[j]=tmp_info;
                }
            }
        }
    }

    for(locIter=0;locIter<numTunnel;locIter++)
    {
        if(tuninfo.location[locIter].endpoint_location==0)
        {
            /* don't program entry with dst-loc 0 (implicit gws)
             * in shared vnid; This will cause us to respond to
             * ARPs in physical network. 
             * */
            continue;
        }
        memset(&svcp,0,sizeof(svcp));
        dip.sin_addr.s_addr=tuninfo.location[locIter].endpoint_location;
        dst_address = inet_ntoa(dip.sin_addr);
        if(dst_address != NULL)
        {
            /* check the destination ip given 
             * by dps is gateway own IP */
            found_self_ip = chk_self_ipv4(dst_address);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "Loc Reply : could not resolve src location\n");
            svcp.ovl_location.location.ovl_src_location = 0;
        }

        /*if(found_self_ip == 0)*/
        if(1)
        {
            FILE *fp; 
            char command[64];
            char popen_result[16];
            memset(popen_result,0,16);
            memset(command,0,64);
            sprintf(command,"ip route get %s | busybox awk -F\" src \" \'{print $2}\'",
                    dst_address);
            log_debug(ServiceUtilLogLevel,
                        "Command:[%s]\n",command);
            fp = popen(command, "r");

            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                            "Error popen:%s\n",command);
            }
            else
            {
                if(fgets(popen_result, 16, fp))
                {
                    if(found_self_ip)
                    {
                        svcp.ovl_location.location.is_self=1;
                    }
                    else
                    {
                        svcp.ovl_location.location.is_self=0;
                    }

                    svcp.ovl_location.cmd = tuninfo.cmd;
                    svcp.ovl_location.index = locIter;

                    svcp.ovl_location.location.domain = 
                                            tuninfo.domain;
                    svcp.ovl_location.location.dst_domain = 
                        tuninfo.location[locIter].dst_domain;
                    svcp.ovl_location.location.endpoint_ip = 
                        tuninfo.endpoint_ip;
                    svcp.ovl_location.location.endpoint_location =
                        tuninfo.location[locIter].endpoint_location;

                    if(g_dovenet_ipv4)
                    {
                        svcp.ovl_location.location.ovl_src_location = g_dovenet_ipv4;
                    }
                    else
                    {
                        svcp.ovl_location.location.ovl_src_location = 
                        inet_addr(popen_result);
                    }
                    
                    log_debug(ServiceUtilLogLevel,
                              "SRC location %x\n", 
                              svcp.ovl_location.location.ovl_src_location);
                    svcp.ovl_location.location.ovl_header_proto = OVL_PROTO;
                    svcp.ovl_location.location.ovl_src_port = OVL_PROTO_SRC_PORT;
                    svcp.ovl_location.location.ovl_dst_port = OVL_PROTO_DST_PORT;
                    memcpy(svcp.ovl_location.location.ovl_dst_mac,
                           tuninfo.ovl_dst_mac, 6);
                    
                    dgwy_ctrl_location(&svcp);

                }
                else
                {
                    log_error(ServiceUtilLogLevel,
                                "Error: popen:%s\n",command);
                }
                pclose(fp);
            }

            log_info(ServiceUtilLogLevel,
                   "Received Location Information Reply from DPS: "
                   "Domain Id: 0x%x [0x%x] "
                   "Dest Domain Id: 0x%x "
                   "Dest Dove Switch IP: %x "
                   "Dest VM IP: %x "
                   "Local-Source-IP: %x " 
                   "Dest VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   p_loc_reply->vnid,
                   svcp.ovl_location.location.domain,
                   svcp.ovl_location.location.dst_domain,
                   svcp.ovl_location.location.endpoint_location,
                   svcp.ovl_location.location.endpoint_ip,
                   svcp.ovl_location.location.ovl_src_location, 
                   svcp.ovl_location.location.ovl_dst_mac[0],
                   svcp.ovl_location.location.ovl_dst_mac[1],
                   svcp.ovl_location.location.ovl_dst_mac[2],
                   svcp.ovl_location.location.ovl_dst_mac[3],
                   svcp.ovl_location.location.ovl_dst_mac[4],
                   svcp.ovl_location.location.ovl_dst_mac[5]);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                      "Loc Reply : Self Location! VM IP:%x "
                      "MAC  %02x:%02x:%02x:%02x:%02x:%02x : Ignore\n",
                      tuninfo.endpoint_ip,
                      tuninfo.ovl_dst_mac[0],tuninfo.ovl_dst_mac[1],
                      tuninfo.ovl_dst_mac[2],tuninfo.ovl_dst_mac[3],
                      tuninfo.ovl_dst_mac[4],tuninfo.ovl_dst_mac[5]);
        }
    }
    return (retVal);
}


static inline 
dgwy_return_status handle_dps_loc_reply(void* context, 
                                        dps_endpoint_loc_reply_t* p_loc_reply)
{
    dgwy_service_config_t svcp;
    ovl_tunnel_cfg_t tuninfo;
    dgwy_return_status retVal = DGWY_SUCCESS;
    char *dst_address = NULL;
    struct sockaddr_in dip;
    int found_self_ip=-1;
    int shared_lookup_error=0;
    int numTunnel=0;
    int locIter=0;

    if (p_loc_reply == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Loc Reply is NULL\n");
        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    memset(&tuninfo,0,sizeof(tuninfo));

    numTunnel = p_loc_reply->tunnel_info.num_of_tunnels;
    if(numTunnel<= 0)
    {
        log_info(ServiceUtilLogLevel,
                 "Loc Reply Num Tunnel %d\n",
                 numTunnel);
    }

    if(numTunnel >= MAX_TUNNEL_INFO)
    {
        log_info(ServiceUtilLogLevel,
                 "Loc Reply Num Tunnel %d >= %d\n",
                 numTunnel,MAX_TUNNEL_INFO);

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    if(p_loc_reply->tunnel_info.tunnel_list[0].ip4 == 0)
    {
        log_info(ServiceUtilLogLevel,
                 "Loc Reply DST location empty for %x: internal gw\n",
                 p_loc_reply->vm_ip_addr.ip4);
    }

    if(context)
    {
        int i=0;
        dgadm_dps_context_t *pdomain =
            (dgadm_dps_context_t*)context;
        tuninfo.cmd = CMD_UPDATE_OVL_IP;
        //tuninfo.domain = htonl(pdomain->svnid<<8);
        tuninfo.domain = htonl(pdomain->svnid)>>8;
        tuninfo.num_entry = numTunnel;
        for(i=0;i<numTunnel;i++)
        {
            /* extepected dest vnid in tunnel_list
             * but dcs not returing it */
            /*tuninfo.location[i].dst_domain = 
                htonl(p_loc_reply->vnid<<8);*/

            tuninfo.location[i].dst_domain = 
                htonl(p_loc_reply->vnid)>>8;
            /*htonl(p_loc_reply->tunnel_info.tunnel_list[i].vnid<<8);*/

            tuninfo.location[i].endpoint_location = 
                htonl(p_loc_reply->tunnel_info.tunnel_list[i].ip4);

            log_info(ServiceUtilLogLevel,"CTX svnid 0x%x :"
                      " phyIP 0x%x dstVnid 0x%x \n",
                      tuninfo.domain,
                      tuninfo.location[i].endpoint_location,
                      tuninfo.location[i].dst_domain);
        }
        if(pdomain->shared_lookup)
        {                
            /* check for gatway mac */
            if(p_loc_reply->mac[0]|p_loc_reply->mac[1]|
               p_loc_reply->mac[2]|p_loc_reply->mac[3]|
               p_loc_reply->mac[4]|p_loc_reply->mac[5])
            {
                /* has valid vmmac */
                process_dps_loc_resp_shared(context, p_loc_reply);
                free(context);
                stat_ctx_alloc--;
                return retVal;
            }
            else
            {
                /* reply is gateway */
                shared_lookup_error=1;
            }
        }


        free(context);
        stat_ctx_alloc--;
    }
    else
    {
        int i=0;
        tuninfo.cmd = CMD_UPDATE_OVL_IP;
        /*tuninfo.domain = htonl(p_loc_reply->vnid<<8);*/
        tuninfo.domain = htonl(p_loc_reply->vnid)>>8;
        tuninfo.num_entry = numTunnel;
        for(i=0;i<numTunnel;i++)
        {

            /* extepected dest vnid in tunnel_list
             * but dcs not returing it */
            /*tuninfo.location[i].dst_domain = 
                htonl(p_loc_reply->vnid<<8);*/

            tuninfo.location[i].dst_domain = 
                htonl(p_loc_reply->vnid)>>8;
             /* htonl(p_loc_reply->tunnel_info.tunnel_list[i].vnid<<8);*/

            tuninfo.location[i].endpoint_location = 
                htonl(p_loc_reply->tunnel_info.tunnel_list[i].ip4);

            log_info(ServiceUtilLogLevel,"NOCTX svnid 0x%x :"
                      " phyIP 0x%x dstVnid 0x%x \n",
                      tuninfo.domain,
                      tuninfo.location[i].endpoint_location,
                      tuninfo.location[i].dst_domain);
        }
    }

    if(shared_lookup_error)
    {
        log_info(ServiceUtilLogLevel,
                 "Loc Reply DST MAC empty for %x: ext gw\n",
                 p_loc_reply->vm_ip_addr.ip4);
        return (DGWY_ERROR);
    }


    tuninfo.endpoint_ip=htonl(p_loc_reply->vm_ip_addr.ip4);
    memcpy(tuninfo.ovl_dst_mac, p_loc_reply->mac, 6);

    log_info(ServiceUtilLogLevel,
                "DPS Response: ctxalloc_stat %d numTunnel %d\n", 
                stat_ctx_alloc, numTunnel);

    if(numTunnel > 1)
    {
        /* sort the list decending order */
        int i=0,j=0;
        for(i=0; i < (numTunnel-1) ; ++i)
        {
            for(j=0; j<numTunnel-1-i; ++j)
            {
                if(tuninfo.location[j].endpoint_location <
                   tuninfo.location[j+1].endpoint_location)
                {
                    ovl_tunnel_info_t tmp_info = tuninfo.location[j+1];
                    tuninfo.location[j+1] = tuninfo.location[j];
                    tuninfo.location[j]=tmp_info;
                }
            }
        }
    }

    for(locIter=0;locIter<numTunnel;locIter++)
    {
        memset(&svcp,0,sizeof(svcp));
        dip.sin_addr.s_addr=tuninfo.location[locIter].endpoint_location;
        dst_address = inet_ntoa(dip.sin_addr);
        if(dst_address != NULL)
        {
            /* check the destination ip given 
             * by dps is gateway own IP */
            found_self_ip = chk_self_ipv4(dst_address);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                        "Loc Reply : could not resolve src location\n");
            svcp.ovl_location.location.ovl_src_location = 0;
        }

        /*if(found_self_ip == 0)*/
        if(1)
        {
            FILE *fp; 
            char command[64];
            char popen_result[16];
            memset(popen_result,0,16);
            memset(command,0,64);
            sprintf(command,"ip route get %s | busybox awk -F\" src \" \'{print $2}\'",
                    dst_address);
            log_debug(ServiceUtilLogLevel,
                        "Command:[%s]\n",command);
            fp = popen(command, "r");

            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                            "Error popen:%s\n",command);
            }
            else
            {
                if(fgets(popen_result, 16, fp))
                {
                    if(found_self_ip)
                    {
                        svcp.ovl_location.location.is_self=1;
                    }
                    else
                    {
                        svcp.ovl_location.location.is_self=0;
                    }

                    svcp.ovl_location.cmd = tuninfo.cmd;
                    svcp.ovl_location.index = locIter;

                    svcp.ovl_location.location.domain = 
                                            tuninfo.domain;
                    svcp.ovl_location.location.dst_domain = 
                        tuninfo.location[locIter].dst_domain;
                    svcp.ovl_location.location.endpoint_ip = 
                        tuninfo.endpoint_ip;
                    svcp.ovl_location.location.endpoint_location =
                        tuninfo.location[locIter].endpoint_location;

                    if(g_dovenet_ipv4)
                    {
                        svcp.ovl_location.location.ovl_src_location = g_dovenet_ipv4;
                    }
                    else
                    {
                        svcp.ovl_location.location.ovl_src_location = 
                        inet_addr(popen_result);
                    }
                    
                    log_debug(ServiceUtilLogLevel,
                              "SRC location %x\n", 
                              svcp.ovl_location.location.ovl_src_location);
                    svcp.ovl_location.location.ovl_header_proto = OVL_PROTO;
                    svcp.ovl_location.location.ovl_src_port = OVL_PROTO_SRC_PORT;
                    svcp.ovl_location.location.ovl_dst_port = OVL_PROTO_DST_PORT;
                    memcpy(svcp.ovl_location.location.ovl_dst_mac,
                           tuninfo.ovl_dst_mac, 6);
                    
                    dgwy_ctrl_location(&svcp);

                }
                else
                {
                    log_error(ServiceUtilLogLevel,
                                "Error: popen:%s\n",command);
                }
                pclose(fp);
            }

            log_info(ServiceUtilLogLevel,
                   "Received Location Information Reply from DPS: "
                   "Domain Id: 0x%x [0x%x] "
                   "Dest Domain Id: 0x%x "
                   "Dest Dove Switch IP: %x "
                   "Dest VM IP: %x "
                   "Local-Source-IP: %x " 
                   "Dest VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   p_loc_reply->vnid,
                   svcp.ovl_location.location.domain,
                   svcp.ovl_location.location.dst_domain,
                   svcp.ovl_location.location.endpoint_location,
                   svcp.ovl_location.location.endpoint_ip,
                   svcp.ovl_location.location.ovl_src_location, 
                   svcp.ovl_location.location.ovl_dst_mac[0],
                   svcp.ovl_location.location.ovl_dst_mac[1],
                   svcp.ovl_location.location.ovl_dst_mac[2],
                   svcp.ovl_location.location.ovl_dst_mac[3],
                   svcp.ovl_location.location.ovl_dst_mac[4],
                   svcp.ovl_location.location.ovl_dst_mac[5]);
        }
        else
        {
            log_error(ServiceUtilLogLevel,
                      "Loc Reply : Self Location! VM IP:%x "
                      "MAC  %02x:%02x:%02x:%02x:%02x:%02x : Ignore\n",
                      tuninfo.endpoint_ip,
                      tuninfo.ovl_dst_mac[0],tuninfo.ovl_dst_mac[1],
                      tuninfo.ovl_dst_mac[2],tuninfo.ovl_dst_mac[3],
                      tuninfo.ovl_dst_mac[4],tuninfo.ovl_dst_mac[5]);
        }
    }
    return (retVal);
}

static inline 
dgwy_return_status handle_dps_policy_reply(void* context,
                                           dps_policy_reply_t *pol_reply)
{
    dgadm_dps_context_t *pdomain = NULL;
    uint32_t             svnid   = 0;
    uint32_t             dvnid   = 0;
    int                  allowed = 0;
    dgwy_return_status   retval  = DGWY_ERROR;
    dps_endpoint_loc_reply_t *p_loc_reply = NULL;

    if (pol_reply == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Pol Reply is NULL\n");
        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        goto out;
    }

    if(context)
    {
        pdomain = (dgadm_dps_context_t*)context;
        svnid = pdomain->svnid;
    }
    else
    {
        log_debug (ServiceUtilLogLevel,
                  "Pol Reply CTX is NULL\n");
        goto out;
    }

    if(1/*pol_reply->dps_policy_info.dps_policy.policy_type == 1*/)
    {
        /* connectivity policy */
        int i=0;
        dps_policy_t *dps_pol= &pol_reply->dps_policy_info.dps_policy;
        int numpermits = dps_pol->vnid_policy.num_permit_rules;

        p_loc_reply = &pol_reply->dst_endpoint_loc_reply;
        dvnid       = p_loc_reply->vnid;

        log_debug(ServiceUtilLogLevel,
                 "numpermits=%d dvnid %d svnid %u %x:%x:%x:%x:%x:%x\n",
                 numpermits, dvnid, svnid, 
                 p_loc_reply->mac[0], p_loc_reply->mac[1],
                 p_loc_reply->mac[2], p_loc_reply->mac[3],
                 p_loc_reply->mac[4], p_loc_reply->mac[5]);

        for(i=0; i<numpermits; i++)
        {
            log_debug(ServiceUtilLogLevel,
                     "i=%d %u %u\n",
                     i, dps_pol->vnid_policy.src_dst_vnid[i].svnid,
                     dps_pol->vnid_policy.src_dst_vnid[i].dvnid);
            if((dps_pol->vnid_policy.src_dst_vnid[i].svnid == svnid) && 
               (dps_pol->vnid_policy.src_dst_vnid[i].dvnid == dvnid))
            {
                allowed=1;
                break;
            }
        }
    }
    else
    {
        /* unhandled policy type */
        log_error(ServiceUtilLogLevel,
                    "Unhandled Pol Type\n");
        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        goto out;
    }

    if(allowed)
    {
        retval = handle_dps_loc_reply(context,p_loc_reply);
    }
    else
    {
        log_debug(ServiceUtilLogLevel,
                  "Did not find allow policy\n ");
        free(context);
        stat_ctx_alloc--;
    }
out:
    return (retval);
}


static inline 
dgwy_return_status handle_dps_update_reply(void* context, 
                                dps_endpoint_update_reply_t* p_update_reply)
{
    dgwy_return_status retVal = DGWY_SUCCESS;

    if (p_update_reply == NULL)
    {
        log_error(ServiceUtilLogLevel,
                    "Update Reply is NULL");
        return (DGWY_ERROR);
    }

    log_debug(ServiceUtilLogLevel,
            "Received Update Reply from DPS: "
           "Domain Id: 0x%x "
           "Dest Dove Switch IP: %x "
           "Dest VM IP: %x "
           "Dest VM MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            p_update_reply->vnid,
            p_update_reply->tunnel_info.tunnel_list[0].ip4,
            p_update_reply->vm_ip_addr[0].ip4,
            p_update_reply->mac[0],
            p_update_reply->mac[1],
            p_update_reply->mac[2],
            p_update_reply->mac[3],
            p_update_reply->mac[4],
            p_update_reply->mac[5]);

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }

    return (retVal);
}



static inline 
dgwy_return_status handle_dps_vlangw_reply_lst(void* context, 
                                               dps_tunnel_list_t* p_tunnel_lst,
                                               uint32_t vnid)
{
    dgwy_return_status retVal = DGWY_SUCCESS;
    int i=0;

    if(p_tunnel_lst == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "VLANGWYList Reply is NULL\n");

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    if((p_tunnel_lst->num_of_tunnels) >= MAX_TUNNEL_INFO)
    {
        log_error(ServiceUtilLogLevel,
                  "VLANGWYList List is too big!\n");

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    log_debug(ServiceUtilLogLevel,
              "%s: vnid %d, %d \n",__FUNCTION__,
              vnid, p_tunnel_lst->num_of_tunnels);

    for(i=0; i<p_tunnel_lst->num_of_tunnels; i++)
    {
        char *dst_address = NULL;
        struct sockaddr_in dip;

        dip.sin_addr.s_addr = htonl(p_tunnel_lst->tunnel_list[i].ip4);
        dst_address = inet_ntoa(dip.sin_addr);

        log_debug(ServiceUtilLogLevel,"0x%x vnid %x\n",
                  p_tunnel_lst->tunnel_list[i].ip4,
                  p_tunnel_lst->tunnel_list[i].vnid);

        if(vnid != p_tunnel_lst->tunnel_list[i].vnid)
        {
            /* VGW supports only two peers:
             * The peers should support all the VLANs.
             * Do not support cases like:
             *  VGW1 : VLAN List -> 10,20,30{ Intent to have HA only for VLAN10}
             *  VGW2 : VLAN List -> 10,40,50{ We can't support mix             }
             * 
             * Only supports case:
             *  VGW1 : VLAN List -> 10,20,30
             *  VGW2 : VLAN List -> 10,20,30
             *
             * */
            continue;
        }

        if(dst_address && (chk_self_ipv4(dst_address) == 0))
        {
            dgwy_ctrl_set_peer_ipv4((char*)"APBR",
                                    htonl(p_tunnel_lst->tunnel_list[i].ip4));
            
        }
    }

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
    return (retVal);
}

static inline 
dgwy_return_status handle_dps_extgw_reply_lst(void* context, 
                                              dps_tunnel_list_t* p_tunnel_lst,
                                              uint32_t vnid)
{

    dgwy_return_status retVal = DGWY_SUCCESS;
    int i=0;

    if(p_tunnel_lst == NULL)
    {
        log_error(ServiceUtilLogLevel,
                  "EXTGWYList Reply is NULL\n");

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    if((p_tunnel_lst->num_of_tunnels) >= MAX_TUNNEL_INFO)
    {
        log_error(ServiceUtilLogLevel,
                  "EXTGWYList List is too big!\n");

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return (DGWY_ERROR);
    }

    log_info(ServiceUtilLogLevel,
             "%s: vnid %d, %d \n",__FUNCTION__,
             vnid, p_tunnel_lst->num_of_tunnels);

    if(p_tunnel_lst->num_of_tunnels > 0)
    {
        uint32_t tunipList[MAX_TUNNEL_INFO];
        uint32_t selfTunIPv4 = 0;
        uint32_t tmpIPv4 = 0;
        int numTunnel = p_tunnel_lst->num_of_tunnels;
        int j=0;

        memset(tunipList,0,MAX_TUNNEL_INFO);
        for(i=0; i<p_tunnel_lst->num_of_tunnels; i++)
        {
            char *dst_address = NULL;
            struct sockaddr_in dip;

            dip.sin_addr.s_addr = htonl(p_tunnel_lst->tunnel_list[i].ip4);
            dst_address = inet_ntoa(dip.sin_addr);

            tunipList[i] = p_tunnel_lst->tunnel_list[i].ip4;

            if(dst_address && (chk_self_ipv4(dst_address)))
            {
                selfTunIPv4 = p_tunnel_lst->tunnel_list[i].ip4;
            }
        }

        /* sort the list decending order */
        for(i=0; i<(numTunnel-1); ++i)
        {
            for(j=0; j<numTunnel-1-i; ++j)
            {
                if(tunipList[j] < tunipList[j+1])
                {
                    tmpIPv4 = tunipList[j+1];
                    tunipList[j+1] = tunipList[j];
                    tunipList[j]=tmpIPv4;
                }
            }
        }

        if(tunipList[0] == selfTunIPv4)
        {
            log_info(ServiceUtilLogLevel,
                     "ExtMcast Master for vnid %u\n",
                     vnid);
            for(j=SVC_INDX_START; j<MAX_SERVICE_TABLES; j++)
            {
                int len = strlen(GLB_SVC_TABLE[j].name);
                if(len)
                {
                    dgwy_service_list_t *svcp = &GLB_SVC_TABLE[j];
                    if(svcp)
                    {
                        int i=0;
                        SVC_LOCK(svcp->lock);
                        for(i=0;i<MAX_EXT_VNIDS;i++)
                        {
                            if(svcp->ext_mcast_vnids[i].vnid==vnid)
                            {
                                if(svcp->ext_mcast_vnids[i].master==0)
                                {
                                    /* self is master for macst vnid %/
                                     * inform data path */
                                    dgwy_ctrl_ext_mcast_master_state(vnid,
                                                                     svcp->ext_mcast_vnids[i].ipv4,
                                                                     1/* master*/);
                                    log_info(ServiceUtilLogLevel,
                                             "Inform DP:ExtMcast Master for vnid %u\n",
                                             vnid);
                                }
                                send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_JOIN);
                                svcp->ext_mcast_vnids[i].master=1;
                            }
                        }
                        SVC_UNLOCK(svcp->lock);
                    }
                }
            }
        }
        else
        {
            log_info(ServiceUtilLogLevel,
                     "ExtMcast SLAVE for vnid %u\n",
                     vnid);
            for(j=SVC_INDX_START; j<MAX_SERVICE_TABLES; j++)
            {
                int len = strlen(GLB_SVC_TABLE[j].name);
                if(len)
                {
                    dgwy_service_list_t *svcp = &GLB_SVC_TABLE[j];
                    if(svcp)
                    {
                        int i=0;
                        SVC_LOCK(svcp->lock);
                        for(i=0;i<MAX_EXT_VNIDS;i++)
                        {
                            if(svcp->ext_mcast_vnids[i].vnid==vnid)
                            {
                                if(svcp->ext_mcast_vnids[i].master==1)
                                {
                                    /* self is master for macst vnid %/
                                     * inform data path */
                                    dgwy_ctrl_ext_mcast_master_state(vnid,
                                                                     svcp->ext_mcast_vnids[i].ipv4,
                                                                     0/* slave */);
                                    
                                    log_info(ServiceUtilLogLevel,
                                             "Inform DP:ExtMcast SLAVE for vnid %u\n",
                                             vnid);

                                    send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_LEAVE);
                                }
                                svcp->ext_mcast_vnids[i].master=0;
                            }
                        }
                        SVC_UNLOCK(svcp->lock);
                    }
                }
            }
        }
    }

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
    return (retVal);
}

static inline 
dgwy_return_status handle_dps_address_resolve(void* context, 
                                              dps_endpoint_loc_req_t  *address_resolve,
                                              uint32_t vnid)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_location;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_ovl_location_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd          = CMD_ADDR_RESOLVE;
        svc_dp.endpoint_ip  = htonl(address_resolve->vm_ip_addr.ip4);

        log_debug (ServiceUtilLogLevel,
                   "Address RESOLVE Request from DPS Client IP:%x\n",
                   svc_dp.endpoint_ip);
        memcpy(svc_dp.ovl_dst_mac,address_resolve->mac,6);

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_ovl_location_t),&svc_dp);
        dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);

        if(context)
        {
            free(context);
            stat_ctx_alloc--;
        }
        return(DGWY_SUCCESS);
    }
nla_put_failure:

    if(context)
    {
        free(context);
        stat_ctx_alloc--;
    }
    return (DGWY_ERROR);
}

int dgw_ctrl_set_ovl_port(int port)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_location;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_ovl_port_t ovl_port;

        memset(&ovl_port,0,sizeof(ovl_port));
        ovl_port.cmd        = CMD_SET_OVL_PORT;
        ovl_port.port       = port;

        OVL_PROTO_DST_PORT = port;

        log_info(ServiceUtilLogLevel,
                 "Set OVL PORT :%d\n",port);

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_ovl_port_t),&ovl_port);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}



/*
 **************************************************************************
 * dgwy_handle_dps_response --                                        *//**
 *
 * \brief This routine called to process response recieved from DPS server
 *
 * \param[in] The dps_client_data_t associated with the reply
 *
 * \retval 0 -Success
 *         1 - Fail 
 */
static int dgwy_handle_dps_response(void* rsp)
{
    int   retVal = 0;
    int   rspType;
    void*   p_context;
    uint32_t domain_id;
    uint32_t resp_status=0;

    rspType = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).type;
    domain_id = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).vnid;
    p_context = (void*)((dps_client_data_t*) rsp)->context;
    resp_status = ((dps_client_hdr_t)((dps_client_data_t*) rsp)->hdr).resp_status;

    if((DPS_NO_RESPONSE == resp_status)    ||
       (DPS_ERROR_RETRY == resp_status)    ||
       (DPS_ERROR_NO_ROUTE == resp_status))
    {
        /* current dps node down: 
         * Ask for new node if any */
        g_got_dcs_info=0;
    }

    if (rspType >= DPS_MAX_MSG_TYPE)
    {
        /**
         * Log error
         */
        retVal = 1;
        goto dgwy_response_exit;
    }

    log_debug(ServiceUtilLogLevel,
              "Response from DPS Client type:%d\n",
              rspType);

    switch (rspType)
    {
        case DPS_POLICY_REPLY:
        {
            if(handle_dps_policy_reply(p_context, 
               &((dps_client_data_t*)rsp)->policy_reply)!=DGWY_SUCCESS)
            {
                /**
                 * Log error
                 */
                retVal = 1;
            } 
            break;
        }
        case DPS_ENDPOINT_LOC_REPLY:
        {
            if(handle_dps_loc_reply(p_context, 
               &((dps_client_data_t*)rsp)->endpoint_loc_reply)!=DGWY_SUCCESS)
            {
                /**
                 * Log error
                 */
                retVal = 1;
            } 
            break;
        }
        case DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
        {
            if(handle_dps_unsolicited_loc_reply(p_context, 
               &((dps_client_data_t*)rsp)->endpoint_loc_reply)!=DGWY_SUCCESS)
            {
                /**
                 * Log error
                 */
                retVal = 1;
            } 
            break;
        }
        case DPS_ENDPOINT_UPDATE_REPLY:
        {
            if(handle_dps_update_reply(p_context,
               &((dps_client_data_t*)rsp)->endpoint_update_reply)!=DGWY_SUCCESS)
            {
                /**
                 * Log error
                 */
                retVal = 1;
            }
            break;
        }
        case DPS_BCAST_LIST_REPLY:
        {
            if(handle_dps_bcast_reply_lst(p_context, 
               &(((dps_client_data_t*)rsp)->dove_switch_list), 
               domain_id)!=DGWY_SUCCESS)
            {
                /* Log error */
                retVal = 1;
            }
        
            break;
        }
        case DPS_MCAST_RECEIVER_DS_LIST:
        {
            if(handle_dps_mcast_recv_list(p_context,
               &(((dps_client_data_t*)rsp)->mcast_receiver_ds_list),
               domain_id) != DGWY_SUCCESS)
            {
                retVal = 1;
            }
            break;
        }
        case DPS_REG_DEREGISTER_ACK:
        {
            log_info(ServiceUtilLogLevel,
                     "Got Tunnel Register ACK \n"); 
            g_tunnel_reg_status=1;
            break;
        }
#if 0
        case DPS_EXTERNAL_GW_LIST_REPLY:
        {
             if(handle_dps_extgw_reply_lst(p_context, 
               &(((dps_client_data_t*)rsp)->tunnel_info), 
               domain_id)!=DGWY_SUCCESS)
            {
                /* Log error */
                retVal = 1;
            }

            break;
        }
#endif
        /*case DPS_UNSOLICITED_VLAN_GW_LIST:*/
        case DPS_VLAN_GW_LIST_REPLY:
        {
             if(handle_dps_vlangw_reply_lst(p_context, 
               &(((dps_client_data_t*)rsp)->tunnel_info), 
               domain_id)!=DGWY_SUCCESS)
            {
                /* Log error */
                retVal = 1;
            }

            break;
        }
        case DPS_UNSOLICITED_EXTERNAL_GW_LIST:
        case DPS_EXTERNAL_GW_LIST_REPLY:
        {
             if(handle_dps_extgw_reply_lst(p_context, 
                                           &(((dps_client_data_t*)rsp)->tunnel_info), 
                                           domain_id)!=DGWY_SUCCESS)
            {
                /* Log error */
                retVal = 1;
            }
            break;
        }
        case DPS_POLICY_INVALIDATE:
        case DPS_ADDR_RESOLVE:
        {
            if(handle_dps_address_resolve(p_context,
                                          &(((dps_client_data_t*)rsp)->address_resolve),
                                          domain_id) != DGWY_SUCCESS)
            {
                /* Log error */
                retVal = 1;
            }
            break;
        }
        default:
        {
            /**
             * Log error
             */
            retVal = 1;
        }
    }

dgwy_response_exit:
    return (retVal ? DGWY_ERROR : DGWY_SUCCESS);
}



/*
 ******************************************************************************
 * dps_protocol_send_to_client --                                         *//**
 *
 * \brief This routine called from DPS core api section when 
 *        response recieved from DPS server
 *
 * \param[in] The dps_client_data_t associated with the reply
 *
 * \retval None
 */
void dps_protocol_send_to_client(dps_client_data_t *msg)
{
    //int status = 0;
    //status = dgwy_handle_dps_response((void*)msg);
    dgwy_handle_dps_response((void*)msg);
    return;
}

int dgwy_ctrl_svc_ipv4(dgwy_service_config_t *svcp, int flags)
{
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    log_debug(ServiceUtilLogLevel,
              "cmd %d, %x %x %x\n",
              svcp->ipv4_config.cmd,
              svcp->ipv4_config.ifipv4,
              svcp->ipv4_config.mask,
              svcp->ipv4_config.nexthop);

    if((svcp->ipv4_config.cmd == CMD_ADD_IPV4) &&
       (svcp->ipv4_config.mask) && (svcp->ipv4_config.nexthop) && flags)

    {
        if(update_svc_table_ifipv4(chk_name,
                                   svcp->ipv4_config.ifipv4, 
                                   svcp->ipv4_config.mask,
                                   svcp->ipv4_config.nexthop,
                                   svcp->ipv4_config.vlan_id) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table ifipv4\n");
            return -1;
        }
    }

    dgwy_cb_func = (void *)dgwy_ctrl_svc_ipv4;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_ipv4_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->ipv4_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        svc_dp.ifipv4 = svcp->ipv4_config.ifipv4;
        log_debug(ServiceUtilLogLevel,
                "%s: %s %zu cmd=%u\n",__FUNCTION__, svc_dp.svc_name, 
               sizeof(svc_dp.ifipv4), svc_dp.cmd);
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_ipv4_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure: /* used in NLA_PUT */
    return -1;
}

int dgwy_ctrl_svc_extipv4(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
    dgwy_tunnel_info_t tuninfo;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(svcp->extipv4_config.cmd != CMD_DEL_EXTVIP)
    {
        if(update_svc_table_extipv4(chk_name,
                                    &svcp->extipv4_config.extip) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table extipv4\n");
            return -1;
        }
    }

    if(g_dovenet_ipv4)
    {
        tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
        tuninfo.vnid = svcp->extipv4_config.extip.domain;
        tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
        dove_dps_tunnel_register(&tuninfo);
    }

    dgwy_cb_func = (void*)dgwy_ctrl_svc_extipv4;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_extvip_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->extipv4_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        memcpy(&svc_dp.extip, &svcp->extipv4_config.extip, sizeof(svc_dp.extip));
        log_debug(ServiceUtilLogLevel,
                    "%s: %s %zu cmd=%u\n",__FUNCTION__, svc_dp.svc_name, 
               sizeof(svc_dp.extip), svc_dp.cmd);
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_extvip_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_svc_intipv4(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(update_svc_table_intipv4(chk_name,
                                &svcp->intipv4_config.intip) == -1)
    {
        log_error(ServiceUtilLogLevel,
                    "failed to update svc table intipv4\n");
        return -1;
    }

    dgwy_cb_func = (void*)dgwy_ctrl_svc_intipv4;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_intvip_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->intipv4_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        memcpy(&svc_dp.intip,&svcp->intipv4_config.intip,sizeof(svc_dp.intip));

        log_debug(ServiceUtilLogLevel,
                    "%s: %s %zu cmd=%u\n",__FUNCTION__, svc_dp.svc_name, 
               sizeof(svc_dp.intip), svc_dp.cmd);

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_intvip_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

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
int dgwy_ctrl_svc_macs(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(svcp->ifmac_config.cmd == CMD_ADD_IFMAC)
    {
        if(gMAC[0]==0 && gMAC[1]==0 && gMAC[2]==0 &&
           gMAC[3]==0 && gMAC[4]==0 && gMAC[5]==0)
        {
            gMAC[0]=svcp->ifmac_config.ifmac.mac[0];
            gMAC[1]=svcp->ifmac_config.ifmac.mac[1];
            gMAC[2]=svcp->ifmac_config.ifmac.mac[2];
            gMAC[3]=svcp->ifmac_config.ifmac.mac[3];
            gMAC[4]=svcp->ifmac_config.ifmac.mac[4];
            gMAC[5]=svcp->ifmac_config.ifmac.mac[5];
        }

        if(update_svc_table_macs(chk_name, 
                                 &svcp->ifmac_config.ifmac) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table ifmacs\n");
            return -1;
        }
    }

    dgwy_cb_func = (void*)dgwy_ctrl_svc_macs;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_macs_t svc_dp;

        memset(&svc_dp, 0, sizeof(svc_dp));
        svc_dp.cmd = svcp->ifmac_config.cmd;
        memcpy(svc_dp.svc_name, chk_name, strlen(chk_name));
        memcpy(&svc_dp.mac_cfg, &svcp->ifmac_config.ifmac,
               sizeof(svc_dp.mac_cfg));

        log_info(ServiceUtilLogLevel, "%s cmd=%u\n",
                 svc_dp.svc_name, svc_dp.cmd);

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_svc_macs_t),
                &svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}


int dgwy_ctrl_svc_domains(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(update_svc_table_domain(chk_name, 
                               svcp->domain_config.domainid) == -1)
    {
        log_error(ServiceUtilLogLevel,
                    "failed to update svc table domain\n");
        return -1;
    }


    dgwy_cb_func = (void*)dgwy_ctrl_svc_domains;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_domain_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->domain_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        svc_dp.domains = svcp->domain_config.domainid; 

        log_debug(ServiceUtilLogLevel,
                    "Set svc %s Domains\n",svc_dp.svc_name);
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,
                sizeof(dgwy_ctrl_svc_domain_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_svc_domain_vlans(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
    dgwy_tunnel_info_t tuninfo;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(svcp->dv_config.cmd == CMD_SET_DOMAIN_VLAN)
    {
        if(update_svc_table_dvlist(chk_name,
                                   &svcp->dv_config) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table domain vlan map\n");
            return -1;
        }

        if((svcp->dv_config.vlans[0] > 0) &&
           (svcp->dv_config.vlans[0] < 4096))
        {
            FILE *fp; 
            char command[64];
            memset(command,0,64);
            sprintf(command,"vconfig add %s %d; ifconfig %s.%d up",
                    chk_name,svcp->dv_config.vlans[0],
                    chk_name,svcp->dv_config.vlans[0]);
            log_debug(ServiceUtilLogLevel,
                        "Command %s\n",command);
            fp = popen(command, "r");
            
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,
                            "Error popen:%s\n",command);
            }
            pclose(fp);
        }

        if(g_dovenet_ipv4)
        {
            tuninfo.type = DOVE_VLAN_GATEWAY_AGENT_ID;
            tuninfo.vnid = svcp->dv_config.domain;
            tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
            dove_dps_tunnel_register(&tuninfo);
        }
    }

    if(svcp->dv_config.cmd == CMD_DEL_DOMAIN_VLAN)
    {
        uint32_t vnid = svcp->dv_config.domain;

        check_dregister_tunnel(vnid);

        if(update_svc_table_dvlist(chk_name,
                                   &svcp->dv_config) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table domain vlan map\n");
            return -1;
        }

        if((svcp->dv_config.vlans[0] > 0) &&
           (svcp->dv_config.vlans[0] < 4096))
        {
            FILE *fp; 
            char command[64];
            memset(command,0,64);
            sprintf(command,"ifconfig %s.%d down;vconfig rem %s.%d",
                    chk_name,svcp->dv_config.vlans[0],
                    chk_name,svcp->dv_config.vlans[0]);
            log_debug(ServiceUtilLogLevel,
                        "Command %s\n",command);
            fp = popen(command, "r");
            
            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
            }
            pclose(fp);
        }
    }

    dgwy_cb_func = (void*)dgwy_ctrl_svc_domain_vlans;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_domain_vlans_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->dv_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        svc_dp.domain = svcp->dv_config.domain;
        //svc_dp.vlans[0] = svcp->dv_config.vlans[0];
        memcpy(svc_dp.vlans, svcp->dv_config.vlans, sizeof(svc_dp.vlans));
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_domain_vlans_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}



#if 0
int dgwy_ctrl_svc_domain_vlans(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;

    if(update_svc_table_dvlist(svcp->name,
                               &svcp->dv_config) == -1)
    {
        printf("failed to update svc table domain vlan map\n");
        return -1;
    }


    if(svcp->dv_config.cmd == CMD_SET_DOMAIN_VLAN)
    {
        int i=0;
        for(i=0; i<MAX_VLANS; i++)
        {
            if((svcp->dv_config.vlans[i] > 0) &&
               (svcp->dv_config.vlans[i] < 4096))
            {
                FILE *fp; 
                char command[64];
                memset(command,0,64);
                sprintf(command,"vconfig add %s %d; ifconfig %s.%d up",
                        svcp->name,svcp->dv_config.vlans[i],
                        svcp->name,svcp->dv_config.vlans[i]);
                printf("Command %s\n",command);
                fp = popen(command, "r");
                
                if(fp == NULL)
                {
                    printf("Error popen:%s\n",command);
                }
                pclose(fp);
            }
        }
    }

    if(svcp->dv_config.cmd == CMD_DEL_DOMAIN_VLAN)
    {
        int i=0;
        for(i=0; i<MAX_VLANS; i++)
        {
            if((svcp->dv_config.vlans[i] > 0) &&
               (svcp->dv_config.vlans[i] < 4096))
            {
                FILE *fp; 
                char command[64];
                memset(command,0,64);
                sprintf(command,"ifconfig %s.%d down;vconfig rem %s.%d",
                        svcp->name,svcp->dv_config.vlans[i],
                        svcp->name,svcp->dv_config.vlans[i]);
                printf("Command %s\n",command);
                fp = popen(command, "r");
                
                if(fp == NULL)
                {
                    printf("Error popen:%s\n",command);
                }
                pclose(fp);
            }
        }
    }


    dgwy_cb_func = (void*)dgwy_ctrl_svc_domain_vlans;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_domain_vlans_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->dv_config.cmd;
        memcpy(svc_dp.svc_name,svcp->name,strlen(svcp->name));
        svc_dp.domain = svcp->dv_config.domain;
        memcpy(svc_dp.vlans, svcp->dv_config.vlans, sizeof(svc_dp.vlans));
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_domain_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}
#endif


int dgwy_ctrl_set_global(uint8_t cmd)
{
    dgwy_cb_func = (void*)dgwy_ctrl_set_global;

    struct nl_msg *msg;

    if(cmd == CMD_START)
        GLB_STATE=1;
    else
        GLB_STATE=0;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_global_t svcp;
        svcp.cmd = cmd;
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_global_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}


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
int dgwy_ctrl_set_svc_mtu(uint8_t cmd, char *name, uint16_t mtu)
{
    dgwy_cb_func = (void*)dgwy_ctrl_set_svc_mtu;
    struct dirent **namelist;
    int count = 0;
    char path[SYSFS_PATH_MAX];
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(update_svc_table_mtu(chk_name, mtu) == -1)
    {
        log_error(ServiceUtilLogLevel,
                  "failed to update svc mtu \n");
        return -1;
    }

    g_svc_mtu = mtu;
    snprintf(path, SYSFS_PATH_MAX, SYSFS_CLASS_NET "%s/brif/", chk_name);
    count = scandir(path, &namelist, 0, alphasort);
    log_debug(ServiceUtilLogLevel,
                "Set SVC MTU of SVC : %s count %d\n",path, count);
    while(count--)
    {
        if((1 == strlen(namelist[count]->d_name)) &&
           (memcmp(".", namelist[count]->d_name,1) == 0))
        {
            continue;
        }

        if((2 == strlen(namelist[count]->d_name)) &&
           (memcmp("..", namelist[count]->d_name,1) == 0))
        {
            continue;
        }

        if(isbridge_port(chk_name, namelist[count]->d_name))
        {
            FILE *fp;
            char command[256];
            memset(command,0,256);
            log_debug(ServiceUtilLogLevel,"-> %s \n",namelist[count]->d_name);

            sprintf(command,"/sbin/ifconfig %s mtu %u",
                    namelist[count]->d_name, 9000 /* extra 20 for private HDR 
                                                   * Now set MTU to 9K:
                                                   * But data path use mtu 1520! */);

            fp = popen(command, "r");

            if(fp == NULL)
            {
                log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
                return -1;
            }
            pclose(fp);

        }
    }


    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_mtu_t svcp;

        memset(&svcp,0,sizeof(svcp));
        svcp.cmd = cmd;
        memcpy(svcp.svc_name,chk_name,strlen(chk_name));
        svcp.mtu = mtu;
        log_debug(ServiceUtilLogLevel,
                    "Set svc %s[%s] mtu %d\n",svcp.svc_name,chk_name, svcp.mtu);
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_mtu_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}


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
 * \retval - [ 0] success
 * \retval - [-1] error
 *
 ******************************************************************************
 */
int dgwy_ctrl_set_svc_type(uint8_t cmd, char *name, dgwy_type_t svc_type)
{
    dgwy_cb_func = (void*)dgwy_ctrl_set_svc_type;
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    log_info(ServiceUtilLogLevel,
             "DGWY SVC TYPE SET %u\n",
             svc_type);

    if((cmd==CMD_SET_SVC) || (cmd==CMD_ADD_SVC))
    {
        if(update_svc_table_type(chk_name, svc_type) == -1)
        {
            log_error(ServiceUtilLogLevel,"failed to update svc table \n");
            return -1;
        }
    }

    g_APPBRIDGE_type = svc_type;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_svc_type_t svcp;

        memset(&svcp,0,sizeof(svcp));
        svcp.cmd = cmd;
        memcpy(svcp.svc_name,chk_name,strlen(chk_name));
        svcp.type = svc_type;
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_svc_type_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_reset_stats(void)
{
    dgwy_cb_func = (void*)dgwy_ctrl_reset_stats;
    struct nl_msg *msg;

    log_info(ServiceUtilLogLevel,
             "DGWY RESET Stats\n");

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_req_t req;

        memset(&req,0,sizeof(req));
        req.cmd = CMD_RESET_STATS;
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_req_t),&req);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}



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
int dgwy_add_dps_server(uint32_t domain, uint32_t dpsIp, uint16_t port)
{
    ip_addr_t dps_svr_node;
    dps_return_status status;
    uint32_t curIP=0;

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    if(dpsCfg.dpsIp)
    {
        curIP = dpsCfg.dpsIp;

        if((curIP == dpsIp) &&
           (dpsCfg.port == port))
        {
            /* same entry */
            GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
            return 0;
        }

#if 0
        /* ?? XXX TODO Remove Later ??
         * JUST ignore [WORK AROUND FOR DPS CLIENT BUG]
         * If you add new IP ; causes problem in dps client protocol
         * we can set address only once 
         * */
        GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
        return 0;
#endif
    }

    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

#if 0
    if(curIP)
    {
        dgwy_del_dps_server(curIP);
    }
#endif

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    memset(&dpsCfg,0,sizeof(dpsCfg));
    dpsCfg.domain=domain;
    dpsCfg.dpsIp=dpsIp;
    dpsCfg.port=port;
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

    dps_svr_node.family = AF_INET;
    dps_svr_node.port   = port;
    dps_svr_node.ip4    = ntohl(dpsIp);
    dps_svr_node.xport_type = SOCK_DGRAM;

    log_debug(ServiceUtilLogLevel,"%s: domain %d ip %x port %u\n",
            __FUNCTION__,
            domain, dpsIp, port);

    

    g_got_dcs_info=1;
    status =  dps_server_add(domain, &dps_svr_node);
    if(status != DPS_SUCCESS)
    {
        log_error(ServiceUtilLogLevel, 
                    "Failed to add dps server %d\n",status);
        return -1;
    }

    check_register_tunnel();

    return 0;
}

/*
 ******************************************************************************
 * dgwy_del_dps_server                                                 *//**
 *
 * \brief - Delete DPS server.
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
int dgwy_del_dps_server(uint32_t dpsIp)
{
    int found=0;
    ip_addr_t dps_svr_node;

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    if(dpsCfg.dpsIp == dpsIp)
    {
        found=1;
        dps_svr_node.port   = dpsCfg.port;
        dps_svr_node.ip4    = ntohl(dpsIp);
        memset(&dpsCfg,0,sizeof(dpsCfg));
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
    if(found == 0)
    {
        return -1;
    }

    dps_svr_node.family = AF_INET;
    dps_svr_node.xport_type = SOCK_DGRAM;

    dps_server_del(&dps_svr_node);

    return 0;
}



int dgwy_ctrl_fwd_rule(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;

#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif


    if(svcp->fwdrule_config.cmd == CMD_ADD_FWD_RULE)
    {
        if(update_svc_table_fwdrule(chk_name,
                    &svcp->fwdrule_config.fwdrule) == -1)
        {
            log_error(ServiceUtilLogLevel,"failed to update svc table fwdrule\n");
            return -1;
        }
        else
        {
            int iter=0;
            int vnid  = svcp->fwdrule_config.fwdrule.domain;
            uint32_t minip = ntohl(svcp->fwdrule_config.fwdrule.pip_min);
            uint32_t maxip = ntohl(svcp->fwdrule_config.fwdrule.pip_max);
            int range = maxip-minip;
            log_debug(ServiceUtilLogLevel,
                      "range=%d\n",range);
            for(iter=0;iter<=range;iter++)
            {
                dgwy_register_vip_location(vnid,minip+iter,
                                           ntohl(g_dovenet_ipv4),0,gMAC);
            }
        }
    }

    dgwy_cb_func = (void*)dgwy_ctrl_fwd_rule;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_fwdtbl_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        svc_dp.cmd = svcp->fwdrule_config.cmd;
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        svc_dp.fwdipv4  = svcp->fwdrule_config.fwdrule.fwdipv4; 
        svc_dp.fwdport  = htons(svcp->fwdrule_config.fwdrule.fwdport);
        svc_dp.fwdproto = svcp->fwdrule_config.fwdrule.fwdproto;
        svc_dp.realipv4 = svcp->fwdrule_config.fwdrule.realipv4;
        svc_dp.realport = htons(svcp->fwdrule_config.fwdrule.realport);
        /*svc_dp.domain   = htonl(svcp->fwdrule_config.fwdrule.domain<<8);*/
        svc_dp.domain   = htonl(svcp->fwdrule_config.fwdrule.domain)>>8;
        svc_dp.pip_min  = svcp->fwdrule_config.fwdrule.pip_min;
        svc_dp.pip_max  = svcp->fwdrule_config.fwdrule.pip_max;
#define FWD_PIP_PORT_MIN 16842
#define FWD_PIP_PORT_MAX 56842
        svc_dp.pip_port_min  = FWD_PIP_PORT_MIN;
        svc_dp.pip_port_max  = FWD_PIP_PORT_MAX;

        log_debug (ServiceUtilLogLevel,"%s: %s %zu cmd=%u pipmin=%x pipmax=%x\n",
                   __FUNCTION__, svc_dp.svc_name, 
                   sizeof(svc_dp.fwdipv4), svc_dp.cmd,svc_dp.pip_min,svc_dp.pip_max);

        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_fwdtbl_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_location(dgwy_service_config_t *svcp)
{
    struct nl_msg *msg;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    dgwy_cb_func = (void*)dgwy_ctrl_location;
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_ovl_location_t svc_dp;

        memset(&svc_dp,0,sizeof(svc_dp));
        memcpy(svc_dp.svc_name,chk_name,strlen(chk_name));
        svc_dp.cmd          = svcp->ovl_location.cmd;
        svc_dp.index        = svcp->ovl_location.index;
        svc_dp.is_self      = svcp->ovl_location.location.is_self;
        svc_dp.domain       = svcp->ovl_location.location.domain;
        svc_dp.dst_domain       = svcp->ovl_location.location.dst_domain;
        svc_dp.endpoint_ip  = svcp->ovl_location.location.endpoint_ip;
        svc_dp.endpoint_location = svcp->ovl_location.location.endpoint_location;
        svc_dp.ovl_src_location  = svcp->ovl_location.location.ovl_src_location;
        svc_dp.ovl_header_proto  = svcp->ovl_location.location.ovl_header_proto;
        svc_dp.ovl_src_port      = htons(svcp->ovl_location.location.ovl_src_port);
        svc_dp.ovl_dst_port      = htons(svcp->ovl_location.location.ovl_dst_port);
        memcpy(svc_dp.ovl_dst_mac, svcp->ovl_location.location.ovl_dst_mac,6);

        log_debug(ServiceUtilLogLevel,"%s: %s %zu cmd=%u\n",
                  __FUNCTION__, svc_dp.svc_name, 
               sizeof(svc_dp.endpoint_ip), svc_dp.cmd);
        NLA_PUT(msg,DGWY_CTRL_ATTR_MSG,sizeof(dgwy_ctrl_ovl_location_t),&svc_dp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;

}




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
dgwy_service_list_t *dgwy_svc_table_get(char *svcname, int *result, const char *caller)
{
    int i=0;int found=-1;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcname;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif


    *result = 0;
    if(chk_name==NULL)
    {
        return NULL;
    }
    
    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    for(i=SVC_INDX_START; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(GLB_SVC_TABLE[i].name);
        int inlen = strlen(chk_name); 

        log_debug(ServiceUtilLogLevel,"%s: %d %d\n",
                __FUNCTION__,len, inlen);
       
        if(len && (len == inlen))
        {
            if(!(memcmp(GLB_SVC_TABLE[i].name, chk_name, inlen)))
            {
                if(pthread_mutex_trylock(&GLB_SVC_TABLE[i].lock)==0)
                {
                    /* entry locked */
                    found=i;
                    *result = 2;
                    memset(GLB_SVC_TABLE[i].caller_name,0,
                           sizeof(GLB_SVC_TABLE[i].caller_name));
                    strcpy(GLB_SVC_TABLE[i].caller_name,caller);
                }
                else
                {
                    /* busy in use*/
                    log_info(ServiceUtilLogLevel, "INUSE BY %s\n",
                             (GLB_SVC_TABLE[i].caller_name));
                    *result = 1;
                }
                break;
            }
        }
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

    if(found >= 0)
    {
        /* entry in lock state */
        return &GLB_SVC_TABLE[i];
    }

    return NULL;
}



/*
 ***************************************************************************
 * create_new_svc_entry                                                *//**
 *
 * \brief - create new service table entry: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] svc type
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int create_new_svc_entry(char *svcname, dgwy_type_t svc_type)
{
    int i=0;int found=0;int freeindex=-1;
    int ret = -1;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcname;
    int inlen = strlen(chk_name); 
#else
    char chk_name[BRIDGE_NAME_MAX];
    int inlen = strlen("APBR"); 
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);
    for(i=SVC_INDX_START; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(GLB_SVC_TABLE[i].name);

        if(len && (len == inlen))
        {
            if(!(memcmp(GLB_SVC_TABLE[i].name, chk_name, inlen)))
            {
                found=1;
                break;
            }
        }
        if((freeindex < 0) && 
           (GLB_SVC_TABLE[i].state == 0))
        {
            freeindex = i;
        }
    }

    if(!found)
    {
        /* no entry with given name */
        if(freeindex >=0)
        {
            /* got a free entry */
            GLB_SVC_TABLE[freeindex].state = 1; /* disbled */
            memcpy(GLB_SVC_TABLE[freeindex].name, chk_name, inlen);
            GLB_SVC_TABLE[freeindex].type = svc_type;
            GLB_SVC_TABLE[freeindex].index = freeindex;
            SVC_LOCK_INIT(GLB_SVC_TABLE[freeindex].lock);
            ret = 0;
        }
    }

    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);

    return ret;

}


/*
 ***************************************************************************
 * update_svc_table_type                                               *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] svc type
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_type(char *name, dgwy_type_t svc_type)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
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
        svcp->type = svc_type;
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return create_new_svc_entry(name,svc_type);
    }
}


/*
 ***************************************************************************
 * update_svc_table_domain                                               *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] svc domain
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_domain(char *name, uint32_t domain)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_DOMIANS; i++)
        {
            if(svcp->domainList[i] == domain)
            {
                found=1;
                break;
            }
            if((freeidx<0) &&
               (svcp->domainList[i]== INVALID_DOMAIN_ID))
            {
                freeidx=i;
            }
        }
        if((!found) && (freeidx>=0))
        {
            svcp->domainList[freeidx] = domain;
            ret = 0;
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


int get_all_mac_addess(char *buf)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
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
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp && buf)
    {
        int i=0;
        int ret=0;
        char *ptr=buf+1;
        memset(buf,0,VLAN_HA_MSG_LEN);
        for(i=0; i<MAX_IF_MACS; i++)
        {
            if(svcp->ifmacList[i].mac[0]|svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]|svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]|svcp->ifmacList[i].mac[5])
            {
                memcpy(ptr,svcp->ifmacList[i].mac,6);
                ptr=ptr+6;
                ret++;
            }
        }
        *buf = (uint8_t)ret;

        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_table_macs                                               *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] (ifmac_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_macs(char *name, ifmac_t *ifmac)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_IF_MACS; i++)
        {
            if(memcmp(&svcp->ifmacList[i], ifmac, sizeof(ifmac_t)) == 0)
            {
                found=1;
                break;
            }
            if((freeidx<0) &&
               (!(svcp->ifmacList[i].mac[0]| svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]| svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]| svcp->ifmacList[i].mac[5])))
            {
                freeidx=i;
            }
        }

        if((!found) && (freeidx>=0))
        {
            memcpy(&svcp->ifmacList[freeidx], ifmac, sizeof(ifmac_t));
            ret = 0;
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}



/*
 ***************************************************************************
 * update_svc_table_ifipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] ip
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_ifipv4(char *name, uint32_t ipv4, 
                            uint32_t mask, uint32_t nexthop, int vlan_id)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                    "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            if(svcp->ifipv4List[i].ipv4 == ipv4)
            {
                found=1;
                break;
            }
            if((freeidx<0) && (svcp->ifipv4List[i].ipv4==0))
            {
                freeidx=i;
            }
        }

        if((!found) && (freeidx>=0))
        {
            svcp->ifipv4List[freeidx].ipv4 = ipv4;
            svcp->ifipv4List[freeidx].mask = mask;
            svcp->ifipv4List[freeidx].nexthop = nexthop;
            svcp->ifipv4List[freeidx].vlan_id=(uint16_t)vlan_id;
            ret = 0;
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_table_extipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] (extipv4_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_extipv4(char *name, extipv4_t *extip)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(memcmp(&svcp->extipv4List[i], extip, sizeof(extipv4_t))==0)
            {
                found=1;
                break;
            }
            if((freeidx<0) && (svcp->extipv4List[i].ipv4==0))
            {
                freeidx=i;
            }
        }

        if((!found) && (freeidx>=0))
        {
            memcpy(&svcp->extipv4List[freeidx], extip, sizeof(extipv4_t));
            SVC_UNLOCK(svcp->lock);
            ret = 0;
            if(svcp->type == DGWY_TYPE_VLAN)
            {
                dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                       (char*)"APBR",
                                       DGWY_TYPE_EXT_VLAN);
            }
            else if(svcp->type == DGWY_TYPE_NONE)
            {
                dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                       (char*)"APBR",
                                       DGWY_TYPE_EXTERNAL);
            }
        }
        else
        {
            SVC_UNLOCK(svcp->lock);
        }
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


int update_svc_table_extsharedvnids(char *name, int vnid, 
                                    int domain, int extmcast, int action)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_DOMIANS; i++)
        {
            if(svcp->extsharedVnidList[i].vnid == (uint32_t)vnid)
            {
                found=1;
                break;
            }
            if((freeidx<0) && (svcp->extsharedVnidList[i].vnid==0))
            {
                freeidx=i;
            }
        }

        if((action==0) && found)
        {
            /* delete action */
            uint32_t    tenant_id=svcp->extsharedVnidList[i].tenant_id;
            uint32_t    extmcastvnid=svcp->extsharedVnidList[i].extmcastvnid;
            svcp->extsharedVnidList[i].vnid=0;
            svcp->extsharedVnidList[i].tenant_id=0;
            svcp->extsharedVnidList[i].extmcastvnid=0;
            SVC_UNLOCK(svcp->lock); /* UNLOCK */
            if(is_vnid_empty_intenant(tenant_id))
            {
                if(extmcastvnid)
                {
                    api_dgadmin_unset_ext_mcast_vnid(extmcastvnid,0/* ipv4 0 */);
                }
            }
            ret = 0;
        }
        else if((!found) && (freeidx>=0) && action /*add*/)
        {
            svcp->extsharedVnidList[freeidx].vnid=vnid;
            svcp->extsharedVnidList[freeidx].tenant_id=domain;
            svcp->extsharedVnidList[freeidx].extmcastvnid=extmcast;
            SVC_UNLOCK(svcp->lock); /* UNLOCK */
            ret = 0;
            if(svcp->type == DGWY_TYPE_VLAN)
            {
                dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                       (char*)"APBR",
                                       DGWY_TYPE_EXT_VLAN);
            }
            else if(svcp->type == DGWY_TYPE_NONE)
            {
                dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                       (char*)"APBR",
                                       DGWY_TYPE_EXTERNAL);
            }
        }
        else
        {
            SVC_UNLOCK(svcp->lock); /* UNLOCK */
        }
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                    "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_table_intipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] (intipv4_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_intipv4(char *name, intipv4_t *intip)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_INTIPV4_ADDRESS; i++)
        {
            if(memcmp(&svcp->intipv4List[i], intip, sizeof(intipv4_t))==0)
            {
                found=1;
                break;
            }
            if((freeidx<0) && (svcp->intipv4List[i].ipv4==0))
            {
                freeidx=i;
            }
        }

        if((!found) && (freeidx>=0))
        {
            memcpy(&svcp->intipv4List[freeidx], intip, sizeof(intipv4_t));
            ret = 0;
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_table_fwdrule                                            *//**
 *
 * \brief - Update global service table: 
 *          service with given type
 *
 * \param[in] svc name 
 * \param[in] (fwd_rule_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_fwdrule(char *name, fwd_rule_t *fwdrule)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=0;
        int ret=-1;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if(memcmp(&svcp->fwdruleList[i], fwdrule, 
                      sizeof(fwd_rule_t))==0)
            {
                found=1;
                break;
            }
            if((freeidx<0) && (svcp->fwdruleList[i].fwdipv4==0))
            {
                freeidx=i;
            }
        }

        if((!found) && (freeidx>=0))
        {
            log_debug(ServiceUtilLogLevel,"svc table fwdrule update index %d\n", freeidx);
            memcpy(&svcp->fwdruleList[freeidx], fwdrule, sizeof(fwd_rule_t));
            ret = 0;
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                  "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}

#if 0
/*
 ***************************************************************************
 * update_svc_table_dvlist                                             *//**
 *
 * \brief - Update global service table: 
 *
 * \param[in] svc name 
 * \param[in] (domain_vlan_cfg_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_dvlist(char *name, domain_vlan_cfg_t *dvlcfg)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            printf("%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=-1;
        int ret=-1;
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(svcp->dvList[i].domain == dvlcfg->domain)     
            {
                found=i;
                break;
            }
            if((freeidx<0) && 
               (svcp->dvList[i].domain==INVALID_DOMAIN_ID))
            {
                freeidx=i;
            }
        }

        if((found<0) && (freeidx>=0))
        {
            if(dvlcfg->cmd == CMD_SET_DOMAIN_VLAN)
            {
                svcp->dvList[freeidx].domain = dvlcfg->domain;
                memcpy(svcp->dvList[freeidx].vlans,dvlcfg->vlans,
                       sizeof(dvlcfg->vlans));
                ret = 0;
            }
        }
        if(found >= 0)
        {
            if(dvlcfg->cmd == CMD_DEL_DOMAIN_VLAN)
            {
                memset(&svcp->dvList[found], 0, 
                       sizeof(domain_vlan_map_t));
                svcp->dvList[found].domain=INVALID_DOMAIN_ID;
                ret = 0;
            }
        }
        SVC_UNLOCK(svcp->lock);
        return ret;
    }
    else
    {
        printf("%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}
#endif

/*
 ***************************************************************************
 * update_svc_table_dvlist                                             *//**
 *
 * \brief - Update global service table: 
 *
 * \param[in] svc name 
 * \param[in] (domain_vlan_cfg_t *)
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_dvlist(char *name, domain_vlan_cfg_t *dvlcfg)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                    "%s:%d In use\n",__FUNCTION__, __LINE__);
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int freeidx=-1;int found=-1;
        int ret=-1;
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(svcp->dvList[i].domain == dvlcfg->domain)     
            {
                found=i;
                break;
            }
            if((freeidx<0) && 
               (svcp->dvList[i].domain==INVALID_DOMAIN_ID))
            {
                freeidx=i;
            }
        }

        if((found<0) && (freeidx>=0))
        {
            if(dvlcfg->cmd == CMD_SET_DOMAIN_VLAN)
            {
                /* domain not found first vlan entry */
                svcp->dvList[freeidx].domain = dvlcfg->domain;
                svcp->dvList[freeidx].vlans[0] = dvlcfg->vlans[0];
                SVC_UNLOCK(svcp->lock);
                ret = 0;

                if(svcp->type == DGWY_TYPE_EXTERNAL)
                {
                    dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                           (char*)"APBR",
                                           DGWY_TYPE_EXT_VLAN);
                }
                else if(svcp->type == DGWY_TYPE_NONE)
                {
                    dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                           (char*)"APBR",
                                           DGWY_TYPE_VLAN);
                }
            }
            else
            {
                SVC_UNLOCK(svcp->lock);
            }
        }
        else if(found >= 0)
        {
            if(dvlcfg->cmd == CMD_SET_DOMAIN_VLAN)
            {
                /* domain found check vlan entry */
                int isvlan=0;int vindex=-1;
                for(i=0; i<MAX_VLANS; i++)
                {
                    if(svcp->dvList[found].vlans[i] == dvlcfg->vlans[0])
                    {
                        /* vlan found */
                        isvlan=1;
                    }
                    if((vindex<0) && (svcp->dvList[found].vlans[i]==0))
                    {
                        vindex=i;
                    }
                }
                if(!isvlan)
                {
                    /* vlan not found */
                    if(vindex>=0)
                    {
                        svcp->dvList[found].vlans[vindex] = dvlcfg->vlans[0];
                        ret = 0;
                    }
                }
            }
            else if(dvlcfg->cmd == CMD_DEL_DOMAIN_VLAN)
            {
                int vnid=svcp->dvList[found].domain;
                dvlcfg->vlans[0]=svcp->dvList[found].vlans[0];
                memset(&svcp->dvList[found], 0, 
                       sizeof(domain_vlan_map_t));
                send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_LEAVE);
                svcp->dvList[found].domain=INVALID_DOMAIN_ID;
                ret = 0;
            }
            SVC_UNLOCK(svcp->lock);
        }
        else
        {
            SVC_UNLOCK(svcp->lock);
        }
        return ret;
    }
    else
    {
        log_error(ServiceUtilLogLevel,
                "%s:%d Entry Not found\n",__FUNCTION__, __LINE__);
        return -1;
    }
}

/*
 ***************************************************************************
 * update_svc_table_ext_mcast_vnid                                     *//**
 *
 * \brief - Update global service table: 
 *          service with given ext mcast vnid
 *
 * \param[in] vnid
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_ext_mcast_vnid(uint32_t vnid, uint32_t ipv4, int oper)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
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
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        if(oper)
        {
            int i=0;
            int found=0;
            int freeidx=-1;
            for(i=0;i<MAX_EXT_VNIDS;i++)
            {
                if(svcp->ext_mcast_vnids[i].vnid == vnid)
                {
                    svcp->ext_mcast_vnids[i].ipv4 = ipv4;
                    found=1;
                    break;
                }
                if((freeidx<0) && (svcp->ext_mcast_vnids[i].vnid==0))
                {
                    freeidx=i;
                }
            }

            if((!found) && (freeidx>=0))
            {
                svcp->ext_mcast_vnids[freeidx].vnid = vnid;
                svcp->ext_mcast_vnids[freeidx].ipv4 = ipv4;
                svcp->ext_mcast_vnids[freeidx].master = 0;
            }
        }
        else
        {
            int i=0;
            for(i=0;i<MAX_EXT_VNIDS;i++)
            {
                if(svcp->ext_mcast_vnids[i].vnid == vnid)
                {
                    svcp->ext_mcast_vnids[i].ipv4 = 0;
                    svcp->ext_mcast_vnids[i].vnid = 0;
                    svcp->ext_mcast_vnids[i].master = 0;
                    break;
                }
            }
                    
        }


        SVC_UNLOCK(svcp->lock);

        if(g_dovenet_ipv4)
        {
            if(oper)
            {
                dgwy_tunnel_info_t tuninfo;
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_register(&tuninfo);
                //send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_JOIN);
            }
            else
            {
                dgwy_tunnel_info_t tuninfo;
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_deregister(&tuninfo);
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}







/*
 ***************************************************************************
 * update_svc_table_mtu                                                *//**
 *
 * \brief - Update global service table: 
 *          service with given mtu
 *
 * \param[in] svc name 
 * \param[in] svc mtu
 *
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_table_mtu(char *name, uint16_t mtu)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        svcp->mtu = mtu;
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_dovenet_ipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given mtu
 *
 * \param[in] svc name 
 * \param[in] dove net ipv4
 * 
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_dovenet_ipv4(char *name, uint32_t ipv4)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        svcp->dovenet_ipv4 = ipv4;
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_mgmt_ipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given mtu
 *
 * \param[in] svc name 
 * \param[in] dove net ipv4
 * 
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_mgmt_ipv4(char *name, uint32_t ipv4)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get(name, &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        svcp->mgmt_ipv4 = ipv4;
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return -1;
    }
}


/*
 ***************************************************************************
 * update_svc_dmc_ipv4                                             *//**
 *
 * \brief - Update global service table: 
 *          service with given dmc ip
 *
 * \param[in] dove net ipv4
 * 
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_dmc_ipv4(uint32_t ipv4, uint16_t port)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
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
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        svcp->dmc_ipv4 = ipv4;
        svcp->dmc_port = port;
        log_debug(ServiceUtilLogLevel,"PORT %d [%d]",
                  svcp->dmc_port, g_dmc_port);
        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return -1;
    }
}

/*
 ***************************************************************************
 * update_svc_vnid_info                                                *//**
 *
 * \brief - Update global service table: 
 *          service with given mtu
 *
 * \param[in] svc name 
 * \param[in] dove net ipv4
 * 
 * \retval - [0] success
 * \retval -[-1] error
 *
 ***************************************************************************
 */
int update_svc_vnid_info(uint32_t vnid, 
                         uint32_t IPv4,
                         uint32_t IPv4_mask,
                         uint32_t IPv4_nexthop,
                         uint8_t state,
                         uint8_t oper )
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get((char *)"APBR", &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
            return -1;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
       int i=0; int found=0; int emptyIdx=-1;

        if(oper)
        {
            /* add operation */
            for(i=0; i<MAX_VNID_SUBNETS; i++)
            {
                if((svcp->vnid_subnet[i].IPv4 == IPv4) &&
                   (svcp->vnid_subnet[i].vnid == vnid))
                {
                    svcp->vnid_subnet[i].vnid = vnid;
                    svcp->vnid_subnet[i].IPv4 = IPv4;
                    svcp->vnid_subnet[i].IPv4_mask = IPv4_mask;
                    svcp->vnid_subnet[i].IPv4_nexthop = IPv4_nexthop;
                    svcp->vnid_subnet[i].subnet_mode = state;
                    found=1;
                    break;
                }
                if((emptyIdx==-1) &&
                   (svcp->vnid_subnet[i].IPv4==0))
                {
                    emptyIdx=i;
                }
            }
            if(found==0)
            {
                if(emptyIdx >= 0)
                {
                    svcp->vnid_subnet[emptyIdx].vnid = vnid;
                    svcp->vnid_subnet[emptyIdx].IPv4 = IPv4;
                    svcp->vnid_subnet[emptyIdx].IPv4_mask = IPv4_mask;
                    svcp->vnid_subnet[emptyIdx].IPv4_nexthop = IPv4_nexthop;
                    svcp->vnid_subnet[emptyIdx].subnet_mode = state;
                }
                else
                {
                    log_info(ServiceUtilLogLevel, 
                             " VNID subnet full\n");
                }
            }
        }
        else
        {
            /* del operation */
            for(i=0; i<MAX_VNID_SUBNETS; i++)
            {
                if((svcp->vnid_subnet[i].IPv4 == IPv4) &&
                   (svcp->vnid_subnet[i].vnid == vnid))
                {
                    svcp->vnid_subnet[i].vnid = 0;
                    svcp->vnid_subnet[i].IPv4 = 0;
                    svcp->vnid_subnet[i].IPv4_mask = 0;
                    svcp->vnid_subnet[i].IPv4_nexthop = 0;
                    svcp->vnid_subnet[i].subnet_mode = 0;
                    break;
                }
            }
        }


        SVC_UNLOCK(svcp->lock);
        return 0;
    }
    else
    {
        return -1;
    }
}




void show_dps_cfg(void)
{
    struct sockaddr_in sa;

    sa.sin_addr.s_addr = dpsCfg.dpsIp;
    show_print(" ");
    show_print("DPS Server Info:");
    //show_print("\tDomain: %u", dpsCfg.domain);
    show_print("\tIPV4  : %s", inet_ntoa(sa.sin_addr));
    show_print("\tPort  : %u", dpsCfg.port);
}

void show_svc_addressv4(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;int vlan_id=0;
        for(i=1; i<MAX_IFIPV4_ADDRESS; i++)
        {
            
            if(svcp->ifipv4List[i].ipv4)
            {
                struct sockaddr_in sa,sa1,sa2;
                char *ip=NULL,*mask=NULL,*nexthop=NULL,*sdr=NULL;
                sa.sin_addr.s_addr= svcp->ifipv4List[i].ipv4;
                sa1.sin_addr.s_addr= svcp->ifipv4List[i].mask;
                sa2.sin_addr.s_addr= svcp->ifipv4List[i].nexthop;
                vlan_id = svcp->ifipv4List[i].vlan_id;

                if(pri==0) 
                {
                    pri=1;
                    show_print(" ");
                    show_print("\tIPV4 Adress:");
                }
                sdr = inet_ntoa(sa.sin_addr);
                if(sdr)
                {
                    ip = (char*)malloc(strlen(sdr)+1);
                    if(ip) {
                        memset(ip,0,strlen(sdr)+1);
                        memcpy(ip, sdr, strlen(sdr));
                    }
                }
                sdr = inet_ntoa(sa1.sin_addr);
                {
                    mask = (char*)malloc(strlen(sdr)+1);
                    if(mask){
                        memset(mask,0,strlen(sdr)+1);
                        memcpy(mask, sdr, strlen(sdr));
                    }
                }

                sdr = inet_ntoa(sa2.sin_addr);
                {
                    nexthop = (char*)malloc(strlen(sdr)+1);
                    if(nexthop)
                    {
                        memset(nexthop,0,strlen(sdr)+1);
                        memcpy(nexthop, sdr, strlen(sdr));
                    }
                }

                if(ip && mask && nexthop)
                {
                    show_print("\t%02d : %s netmask %s nexthop %s vlan %d",
                               i,ip,mask,nexthop,vlan_id); 
                }
                if(ip)free(ip);
                if(mask)free(mask);
            }
        }
    }
}



void show_svc_ifmacs(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_IF_MACS; i++)
        {
            if((svcp->ifmacList[i].mac[0]| svcp->ifmacList[i].mac[1]|
                svcp->ifmacList[i].mac[2]| svcp->ifmacList[i].mac[3]|
                svcp->ifmacList[i].mac[4]| svcp->ifmacList[i].mac[5]))
            {
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tMAC Adress:");
                }
                show_print("\t%02d : %02x:%02x:%02x:%02x:%02x:%02x",i,
                     svcp->ifmacList[i].mac[0], svcp->ifmacList[i].mac[1],
                     svcp->ifmacList[i].mac[2], svcp->ifmacList[i].mac[3],
                     svcp->ifmacList[i].mac[4], svcp->ifmacList[i].mac[5]);
            }
        }
    }
}


void show_svc_extipv4(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(svcp->extipv4List[i].ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = svcp->extipv4List[i].ipv4 ;
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tExternal Virtual IPV4 Adress:");
                }
                show_print("\t%02d : VNID %u IP %s ports [%u-%u] domain %d extmcast %d",
                            i,svcp->extipv4List[i].domain,
                            inet_ntoa(sa.sin_addr), 
                            svcp->extipv4List[i].port_start,
                            svcp->extipv4List[i].port_end,
                            svcp->extipv4List[i].tenant_id,
                            svcp->extipv4List[i].extmcastvnid );
            }
        }
    }
}

void show_svc_ext_shared_vnid(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_DOMIANS; i++)
        {
            if(svcp->extsharedVnidList[i].vnid)
            {
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tExternal Shared VNIDs:");
                }
                show_print("\t%02d : VNID %u extmcast %d",
                            i,svcp->extsharedVnidList[i].vnid,
                            svcp->extsharedVnidList[i].extmcastvnid);
            }
        }
    }
}



void show_svc_intipv4(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_INTIPV4_ADDRESS; i++)
        {
            if(svcp->intipv4List[i].ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = svcp->intipv4List[i].ipv4 ;
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tOverlay Virtual IPV4 Adress:");
                }
                show_print("\t%02d : %s ports [%u-%u]",
                            i,inet_ntoa(sa.sin_addr), 
                            svcp->intipv4List[i].port_start,
                            svcp->intipv4List[i].port_end);
            }
        }
    }
}


void show_svc_fwdrule(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_FWDRULE; i++)
        {
            if(svcp->fwdruleList[i].fwdipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr= svcp->fwdruleList[i].fwdipv4;
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tForward Rules:");
                }
                
                show_print("\t%02d : %s protocol %u",
                        i,inet_ntoa(sa.sin_addr), 
                        svcp->fwdruleList[i].fwdproto);
                if(svcp->fwdruleList[i].fwdport)
                {
                    show_print("\t   : Destination Port %u",
                            svcp->fwdruleList[i].fwdport);
                }
                /*
                else
                {
                    show_print("\t   : Destination Port %u (ANY)",
                            svcp->fwdruleList[i].fwdport);
                }
                */
                show_print("\t   : MAP TO");
                sa.sin_addr.s_addr= svcp->fwdruleList[i].realipv4;
                show_print("\t   : Real IP %s",inet_ntoa(sa.sin_addr));
                if(svcp->fwdruleList[i].realport)
                {
                    show_print("\t   : Real Port %u",
                            svcp->fwdruleList[i].realport);
                }
                show_print("\t   : DOMAIN %d",svcp->fwdruleList[i].domain);
                sa.sin_addr.s_addr= svcp->fwdruleList[i].pip_min;
                show_print("\t   : PIP MIN %s",inet_ntoa(sa.sin_addr));
                sa.sin_addr.s_addr= svcp->fwdruleList[i].pip_max;
                show_print("\t   : PIP MAX %s",inet_ntoa(sa.sin_addr));
            }
        }
    }
}

void show_svc_domain_vlans(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;int pri=0;
        for(i=0; i<MAX_DVMAP; i++)
        {
            
            if(svcp->dvList[i].domain!=INVALID_DOMAIN_ID)
            {
                int j=0;
                char bufar[4096];
                char *bufp = bufar;
                if(pri==0) {
                    pri=1;
                    show_print(" ");
                    show_print("\tDomain VLAN Mappings:");
                }
                show_print("\t%04d :Domain %u",i,svcp->dvList[i].domain);
                memset(bufar,0,sizeof(bufar));
                for(j=0; j<MAX_VLANS; j++)
                {
                    if(svcp->dvList[i].vlans[j])
                    {
                        int len=sprintf(bufp,
                                        "%d,",svcp->dvList[i].vlans[j]);
                        bufp += len;
                    }
                }
                show_print("\t     : VLANs [ %s ]",bufar);              
            }
        }
    }
}

void show_svc_ext_mcast_vnid(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;

        show_print("\n\tEXT MCAST VNIDs:");
        for(i=0; i<MAX_EXT_VNIDS; i++)
        {
            if(svcp->ext_mcast_vnids[i].vnid)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = svcp->ext_mcast_vnids[i].ipv4;
                show_print("\t     : VNID %u, EXTIPV4 %s\n",
                           svcp->ext_mcast_vnids[i].vnid,
                           inet_ntoa(sa.sin_addr));
            }
        }
    }
}  

void show_svc_vnid_subnets(dgwy_service_list_t *svcp)
{
    if(svcp)
    {
        int i=0;

        show_print(" ");
        show_print("\tVNID Subnets:");
        for(i=0; i<MAX_VNID_SUBNETS; i++)
        {
            if(svcp->vnid_subnet[i].IPv4)
            {
                struct sockaddr_in sa;
                show_print(" ");
                show_print("\t%04d :VNID %u",i,
                           svcp->vnid_subnet[i].vnid);
                sa.sin_addr.s_addr = svcp->vnid_subnet[i].IPv4;
                show_print("\tIPV4 : %s ",inet_ntoa(sa.sin_addr));              

                sa.sin_addr.s_addr = svcp->vnid_subnet[i].IPv4_mask;
                show_print("\tMask : %s ",inet_ntoa(sa.sin_addr));              

                sa.sin_addr.s_addr = svcp->vnid_subnet[i].IPv4_nexthop;
                show_print("\tNxHop: %s ",inet_ntoa(sa.sin_addr));              

                if(svcp->vnid_subnet[i].subnet_mode)
                {
                    show_print("\t     : Non-Private");              
                }
                else
                {
                    show_print("\t     : Private");              
                }
            }
        }
    }
}

void update_domain_bcast(void)
{
    int j=0;

    for(j=SVC_INDX_START; j<MAX_SERVICE_TABLES; j++)
    {
        int len = strlen(GLB_SVC_TABLE[j].name);
        if(len)
        {
            dgwy_service_list_t *svcp = &GLB_SVC_TABLE[j];
            if(svcp)
            {
                int i=0;
                for(i=0; i<MAX_DVMAP; i++)
                {
                    if(svcp->dvList[i].domain!=INVALID_DOMAIN_ID)
                    {
                        dgwy_request_bcast_lst(svcp->dvList[i].domain,0);
                        dgwy_request_vlangw_lst(svcp->dvList[i].domain);
                    }
                }
                
                for(i=0;i<MAX_EXT_VNIDS;i++)
                {
                    if(svcp->ext_mcast_vnids[i].vnid)
                    {
                        int vnid = svcp->ext_mcast_vnids[i].vnid;
                        dgwy_request_extgw_lst(vnid);
                    }
                }
            }
        }
    }
}

void retry_dmc_register(void)
{
    if(g_dmc_ipv4 && g_dmc_port)
    {
        if(!g_dmc_register_done)
        {
            if(strlen(g_node_uuid) == 0)
            {
                if(dgadm_read_svc_app_uuid() == DOVE_STATUS_OK)
                {
                    dgwy_dmc_registration();
                }
                else
                {
                    log_alert(ServiceUtilLogLevel,
                              "UUID not set : DMC register failed\n");
                }
            }
            else
            {
                dgwy_dmc_registration();
            }
        }
    }
}

void retry_get_dcs_seed_info(void)
{
    if(g_dmc_ipv4 && g_dmc_port)
    {
        if(g_dmc_register_done && 
           (!g_got_dcs_info))
        {
            dgwy_get_dcs_seed_info();
        }
    }
}


void show_dir_files(char *dir, char *filter)
{
    DIR *dp;
    struct dirent *ep;
    int i=0;
   
    dp = opendir(dir);
    if (dp != NULL)
    {   
        while ((ep=readdir(dp)))
        {
            char *ch=strstr(ep->d_name,filter);
            if(ch)
            {
                i++;
                show_print("\t %d: %s ",i,ep->d_name);
            }
        }
        (void) closedir (dp);
    }   
    else
    {   
        perror ("Couldn't open the directory");
    }   
}

int dgwy_cfg_save_tofile(char *cfgname)
{
    FILE *fp = NULL;
    size_t len = 0;
    char fileName[256];
    int ret = 0;

    memset(fileName,0,256);
    sprintf(fileName,"/flash/dgwconfig/%s.gcf",cfgname);

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);

    memset(&fileCfg,0, sizeof(fileCfg));
    
    fileCfg.gState = GLB_STATE;
    memcpy(&fileCfg.dps,&dpsCfg,sizeof(dpsCfg));
    memcpy(fileCfg.cfgList, GLB_SVC_TABLE, sizeof(GLB_SVC_TABLE));
    
    fp = fopen(fileName,"w");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 

        ret=-1;
    }
    else
    {
        len = sizeof(fileCfg);
        fwrite(&fileCfg, len, 1, fp);
        fclose(fp);
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
    return ret;
}


void show_buffer_cfg(char *buffer)
{
    int i=0;
    dgwy_service_cfg_t *bufCfg = (dgwy_service_cfg_t*)buffer;
    struct sockaddr_in sa;

    if(bufCfg->gState)
    {
        show_print("Gateway Service : Enabled");
    }
    else
    {
        show_print("Gateway Service : Disabled");
    }

    show_print(" ");
    show_print("DPS Server Info:");
    show_print("\tDomain: %u", bufCfg->dps.domain);
    sa.sin_addr.s_addr = bufCfg->dps.dpsIp;
    show_print("\tIPV4  : %s", inet_ntoa(sa.sin_addr));
    show_print("\tPort  : %u", bufCfg->dps.port);


    for(i=SVC_INDX_START; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(bufCfg->cfgList[i].name);
        if(len)
        {
            char typStr[16];
            show_print("%03d Service Name: %s ",i,
                       bufCfg->cfgList[i].name);

            memset(typStr,0,sizeof(typStr));

            if(bufCfg->cfgList[i].type==DGWY_TYPE_EXTERNAL)
            {
                strncpy(typStr,"External",strlen("External"));
            }
            else if(bufCfg->cfgList[i].type==DGWY_TYPE_VLAN)
            {
                strncpy(typStr,"VLAN",strlen("VLAN"));
            }
            else if(bufCfg->cfgList[i].type==DGWY_TYPE_NONE)
            {
                strncpy(typStr,"NONE",strlen("NONE"));
            }
            else if(bufCfg->cfgList[i].type==DGWY_TYPE_EXT_VLAN)
            {
                strncpy(typStr,"EXT,VLAN",strlen("EXT,VLAN"));
            }
            else
            {
                strncpy(typStr,"UNKNOWN",strlen("UNKNOWN"));
            }


            show_print("\tType: %s",typStr);
            show_print("\tMTU : %d",bufCfg->cfgList[i].mtu);

            show_svc_addressv4(&bufCfg->cfgList[i]);
            show_svc_ifmacs(&bufCfg->cfgList[i]);
            show_svc_extipv4(&bufCfg->cfgList[i]);
            show_svc_intipv4(&bufCfg->cfgList[i]);
            show_svc_fwdrule(&bufCfg->cfgList[i]);
            show_svc_domain_vlans(&bufCfg->cfgList[i]);

            if(bufCfg->cfgList[i].dovenet_ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = bufCfg->cfgList[i].dovenet_ipv4;
                show_print("\tDOVE NET IPV4: %s", inet_ntoa(sa.sin_addr));
            }
            if(bufCfg->cfgList[i].mgmt_ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = bufCfg->cfgList[i].mgmt_ipv4;
                show_print("\tMgmt IPV4    : %s", inet_ntoa(sa.sin_addr));
            }
            if(bufCfg->cfgList[i].dmc_ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = bufCfg->cfgList[i].dmc_ipv4;
                show_print("\n\tDMC IPV4    : %s", inet_ntoa(sa.sin_addr));
                show_print("\tDMC PORT    : %u", bufCfg->cfgList[i].dmc_port);
            }
            show_svc_vnid_subnets(&bufCfg->cfgList[i]);
            show_svc_ext_mcast_vnid(&bufCfg->cfgList[i]);
        }
    }
}


int dgwy_show_cfg_file(char *cfgname)
{
    FILE *fp = NULL;
    size_t len = 0;
    char fileName[256];
    int ret = 0;
    int redLen=-1;

    memset(fileName,0,256);
    sprintf(fileName,"/flash/dgwconfig/%s",cfgname);

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 

        ret=-1;
    }
    else
    {
        char *cfgBuffer = (char*)malloc(sizeof(fileCfg));

        if(cfgBuffer)
        {
            len = sizeof(fileCfg);
            redLen = fread(cfgBuffer, 1, len, fp);
            log_debug(ServiceUtilLogLevel,"redLen %d\n",redLen);
            show_buffer_cfg(cfgBuffer);
            free(cfgBuffer);
        }
        else
        {
            log_error(ServiceUtilLogLevel,"Error alloc failed"); 
            ret=-1;
        }

        fclose(fp);
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
    return ret;
}


int dgwy_get_cfg_from_file(char *cfgname, dgwy_service_cfg_t *cfg)
{
    FILE *fp = NULL;
    size_t len = 0;
    char fileName[256];
    int ret = 0;
    int redLen=-1;

    memset(fileName,0,256);
    sprintf(fileName,"/flash/dgwconfig/%s",cfgname);

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 

        ret=-1;
    }
    else
    {
        char *cfgBuffer = (char*)malloc(sizeof(fileCfg));

        if(cfgBuffer)
        {
            len = sizeof(fileCfg);
            redLen = fread(cfgBuffer, 1, len, fp);
            if((redLen > 0) && 
               (((size_t)redLen) > sizeof(dgwy_service_cfg_t)))
            {
                log_error(ServiceUtilLogLevel, "%s: Error in config size",
                                                __FUNCTION__);
            }
            memcpy(cfg, cfgBuffer, sizeof(dgwy_service_cfg_t));
            free(cfgBuffer);
        }
        else
        {
            log_error(ServiceUtilLogLevel,"Error alloc failed"); 
            ret=-1;
        }

        fclose(fp);
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
    return ret;
}


int show_ovl_all(void)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)show_ovl_all;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_global_t svcp;
        svcp.cmd = CMD_SHOW_OVL_ENTRY;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_global_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

void show_svc_all(void)
{
    int i=0;
    FILE *fp;
    char command[256];

    memset(command,0,256);
    sprintf(command,"mkdir -p /flash/dgwconfig");

    GLB_SVC_LOCK(GLB_SVC_TABLE_LOCK);

    fp = popen(command, "r");
    if(fp == NULL)
    {
        log_error(ServiceUtilLogLevel,"Error popen:%s\n",command);
    }
    pclose(fp);

    show_print("Saved Configurations");
    show_dir_files((char*)("/flash/dgwconfig"),(char*)(".gcf"));
    show_print(" ");


    if(GLB_STATE)
    {
        show_print("Gateway Service : Enabled");
    }
    else
    {
        show_print("Gateway Service : Disabled");
    }

    show_dps_cfg();

    for(i=SVC_INDX_START; i<MAX_SERVICE_TABLES; i++)
    {
        int len = strlen(GLB_SVC_TABLE[i].name);
        char typStr[16];
        if(len)
        {
            show_print("%03d Service Name: %s",i,GLB_SVC_TABLE[i].name);
            memset(typStr,0,sizeof(typStr));

            if(GLB_SVC_TABLE[i].type==DGWY_TYPE_EXTERNAL)
            {
                strncpy(typStr,"External",strlen("External"));
            }
            else if(GLB_SVC_TABLE[i].type==DGWY_TYPE_VLAN)
            {
                strncpy(typStr,"VLAN",strlen("VLAN"));
            }
            else if(GLB_SVC_TABLE[i].type==DGWY_TYPE_NONE)
            {
                strncpy(typStr,"NONE",strlen("NONE"));
            }
            else if(GLB_SVC_TABLE[i].type==DGWY_TYPE_EXT_VLAN)
            {
                strncpy(typStr,"EXT,VLAN",strlen("EXT,VLAN"));
            }
            else
            {
                strncpy(typStr,"UNKNOWN",strlen("UNKNOWN"));
            }


            show_print("\tType: %s",typStr);
            show_print("\tMTU : %d",GLB_SVC_TABLE[i].mtu);

            show_svc_addressv4(&GLB_SVC_TABLE[i]);
            show_svc_ifmacs(&GLB_SVC_TABLE[i]);
            show_svc_extipv4(&GLB_SVC_TABLE[i]);
            show_svc_intipv4(&GLB_SVC_TABLE[i]);
            show_svc_fwdrule(&GLB_SVC_TABLE[i]);
            show_svc_domain_vlans(&GLB_SVC_TABLE[i]);

            if(GLB_SVC_TABLE[i].dovenet_ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = GLB_SVC_TABLE[i].dovenet_ipv4;
                show_print("\n\tDOVE NET IPV4: %s", inet_ntoa(sa.sin_addr));
            }
            if(GLB_SVC_TABLE[i].mgmt_ipv4)
            {
                struct sockaddr_in sa;
                sa.sin_addr.s_addr = GLB_SVC_TABLE[i].mgmt_ipv4;
                show_print("\n\tMgmt IPV4    : %s", inet_ntoa(sa.sin_addr));
            }
            
            show_dmc_info();
            show_svc_vnid_subnets(&GLB_SVC_TABLE[i]);
            show_svc_ext_mcast_vnid(&GLB_SVC_TABLE[i]);
            show_svc_ext_shared_vnid(&GLB_SVC_TABLE[i]);

        }
    }
    GLB_SVC_UNLOCK(GLB_SVC_TABLE_LOCK);
}


int show_all_ext_sessions(ext_sesion_dump_t *sessions)
{
    FILE *fp = NULL;
    char fileName[256];
    int redLen=-1;
    size_t want_to_read = 
        ((sizeof(ext_sesion_dump_t)/getpagesize())+1)*getpagesize();

    memset(fileName,0,256);
    sprintf(fileName,"/proc/%s",PROC_DGWY_SESSION_EXT);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 
    }
    else
    {
        redLen = fread(sessions, want_to_read, 1, fp);
        log_debug(ServiceUtilLogLevel,"want to read %zu redLen %d\n",
                 want_to_read, redLen);
        fclose(fp);
    }
    return redLen;
}

int show_all_int_sessions(int_session_dump_t *sessions)
{
    FILE *fp = NULL;
    char fileName[256];
    int redLen=-1;
    size_t want_to_read = 
        ((sizeof(int_session_dump_t)/getpagesize())+1)*getpagesize();

    memset(fileName,0,256);
    sprintf(fileName,"/proc/%s",PROC_DGWY_SESSION_INT);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 
    }
    else
    {
        redLen = fread(sessions, want_to_read, 1, fp);
        log_debug(ServiceUtilLogLevel,"want to read %zu redLen %d\n",
                 want_to_read, redLen);
        fclose(fp);
    }
    return redLen;
}


int show_all_fwddyn_sessions(fwddyn_session_dump_t *sessions)
{
    FILE *fp = NULL;
    char fileName[256];
    int redLen=-1;
    size_t want_to_read = 
        ((sizeof(fwddyn_session_dump_t)/getpagesize())+1)*getpagesize();

    memset(fileName,0,256);
    sprintf(fileName,"/proc/%s",PROC_DGWY_SESSION_FWDDYN);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                  fileName); 
    }
    else
    {
        redLen = fread(sessions, want_to_read, 1, fp);
        log_debug(ServiceUtilLogLevel,"want to read %zu redLen %d\n",
                  want_to_read, redLen);
        fclose(fp);
    }
    return redLen;
}

int show_all_vnid_stats(dgwy_vnid_stats_t *stats)
{
    FILE *fp = NULL;
    char fileName[256];
    int redLen=-1;
    size_t want_to_read = 
        ((sizeof(dgwy_vnid_stats_t)/getpagesize())+1)*getpagesize();

    memset(fileName,0,256);
    sprintf(fileName,"/proc/%s",PROC_DGWY_VNID_STATS);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 
    }
    else
    {
        redLen = fread(stats, want_to_read, 1, fp);
        log_debug(ServiceUtilLogLevel,"want to read %zu redLen %d\n",
                 want_to_read, redLen);
        fclose(fp);
    }
    return redLen;
}

int show_all_stats(dgwy_stats_t *stats)
{
    FILE *fp    = NULL;
    int redLen  = -1;
    char fileName[256];
    size_t want_to_read = 
        ((sizeof(dgwy_stats_t)/getpagesize())+1)*getpagesize();

    memset(fileName,0,256);
    sprintf(fileName,"/proc/%s",PROC_DGWY_ALL_STATS);

    fp = fopen(fileName,"r");
    if (fp==NULL) 
    {
        log_error(ServiceUtilLogLevel,"Error can't open %s",
                                       fileName); 
    }
    else
    {
        redLen = fread(stats, want_to_read, 1, fp);
        log_debug(ServiceUtilLogLevel,"want to read %zu redLen %d\n",
                 want_to_read, redLen);
        fclose(fp);
    }
    return redLen;
}


void reset_global_svctable(void)
{
    int i=0;
    FILE *fp=NULL;
    GLB_SVC_LOCK_INIT(GLB_SVC_TABLE_LOCK);
    SVC_ROUTE_LOCK_INIT(GLB_ROUTE_LOCK);
    GLB_SVC_TABLE[0].state = 1; /* do not use index 0*/
    log_debug(ServiceUtilLogLevel, 
                "%s: size %u\n", __FUNCTION__, sizeof(GLB_SVC_TABLE));
    memset(GLB_SVC_TABLE,0,sizeof(GLB_SVC_TABLE));

  
    for(i=SVC_INDX_START; i<MAX_SERVICE_TABLES; i++)
    {
        int j=0;
        for(j=0; j<MAX_DOMIANS; j++)
        {
            GLB_SVC_TABLE[i].domainList[j] = INVALID_DOMAIN_ID;
        }
        for(j=0; j<MAX_DVMAP; j++)
        {
            GLB_SVC_TABLE[i].dvList[j].domain = INVALID_DOMAIN_ID;
        }

    }

    /* prepare /etc/iproute2/rt_tables */

    if((fp=fopen("/etc/iproute2/rt_tables.orig","r")))
    {
        char command[256];

        fclose(fp);
        memset(command,0,256);
        sprintf(command,"cp /etc/iproute2/rt_tables.orig /etc/iproute2/rt_tables");
        fp = popen(command, "r");
        if(fp == NULL)
        {
            log_error(ServiceUtilLogLevel,
                      "Failed to execite command: %s",
                      command);
        }
        pclose(fp);
    }
    init_done=1;
}

void check_register_tunnel(void)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    int is_external=0;
    int is_vlan=0;
    
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
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return ;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        dgwy_tunnel_info_t tuninfo;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(g_dovenet_ipv4 &&
               (svcp->extipv4List[i].ipv4) &&
               (svcp->extipv4List[i].domain > 0) &&
               (svcp->extipv4List[i].domain <= 0xFFFFFF))
            {
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->extipv4List[i].domain;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_register(&tuninfo);
                is_external=1;
            }
        }
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(g_dovenet_ipv4 &&
               (svcp->dvList[i].domain>0) &&
               (svcp->dvList[i].domain <= 0xFFFFFF))
            {
                tuninfo.type = DOVE_VLAN_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->dvList[i].domain;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_register(&tuninfo);
                is_vlan=1;
            }
        }

        for(i=0;i<MAX_EXT_VNIDS;i++)
        {
            if(g_dovenet_ipv4 &&
               svcp->ext_mcast_vnids[i].vnid)
            {
                int vnid=svcp->ext_mcast_vnids[i].vnid;
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_register(&tuninfo);
                is_external=1;
                if(svcp->ext_mcast_vnids[i].master)
                {
                    send_ext_mcast_recv_join(vnid, DPS_MCAST_RECV_JOIN);
                }
            }
        }

        for(i=0;i<MAX_DOMIANS;i++)
        {
            if(g_dovenet_ipv4 && 
               svcp->extsharedVnidList[i].vnid)
            {
                int vnid=svcp->extsharedVnidList[i].vnid;
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_register(&tuninfo);
                is_external=1;
            }
        }

        for(i=0; i<MAX_FWDRULE; i++)
        {
            if(svcp->fwdruleList[i].fwdipv4 && 
               svcp->fwdruleList[i].pip_min)
            {
                int iter=0;
                int vnid  = svcp->fwdruleList[i].domain;
                uint32_t minip = ntohl(svcp->fwdruleList[i].pip_min);
                uint32_t maxip = ntohl(svcp->fwdruleList[i].pip_max);
                int range = maxip-minip;
                log_debug(ServiceUtilLogLevel,
                          "range=%d\n",range);
                for(iter=0;iter<=range;iter++)
                {
                    dgwy_register_vip_location(vnid,minip+iter,
                                               ntohl(g_dovenet_ipv4),0,gMAC);
                }
            }
        }


        SVC_UNLOCK(svcp->lock);

        if(is_external && is_vlan)
        {
            dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                   (char*)"APBR",
                                   DGWY_TYPE_EXT_VLAN);
        }
        else if(is_external)
        {
            dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                   (char*)"APBR",
                                   DGWY_TYPE_EXTERNAL);
        }
        else if(is_vlan)
        {
            dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                   (char*)"APBR",
                                   DGWY_TYPE_VLAN);
        }
        else
        {
            dgwy_ctrl_set_svc_type(CMD_SET_SVC, 
                                   (char*)"APBR",
                                   DGWY_TYPE_NONE);       
        }

        return;
    }
}

void check_dregister_tunnel(uint32_t vnid)
{
    int result = 0;
    dgwy_service_list_t *svcp = NULL;
    int retry = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get((char*)"APBR", &result, __FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,
                        "%s:%d In use\n",__FUNCTION__, __LINE__);
            return;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;int found=0;
        dgwy_tunnel_info_t tuninfo;
        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(svcp->extipv4List[i].domain == vnid)
            {
                found=1;
                break;
            }
        }

        for(i=0; i<MAX_DOMIANS; i++)
        {
            if(svcp->extsharedVnidList[i].vnid == vnid)
            {
                found=1;
                break;
            }
        }

        if(found==0)
        {
            /* no more extips with vnid
             * de register external gateway tunnel 
             * */
            tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
            tuninfo.vnid = vnid;
            tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
            dove_dps_tunnel_deregister(&tuninfo);
        }
        found=0;
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(svcp->dvList[i].domain==vnid)
            {
                found=1;
                break;
            }
        }
        if(found==0)
        {
            /* no more vnid vlan maps 
             * de register external gateway tunnel 
             * */
            tuninfo.type = DOVE_VLAN_GATEWAY_AGENT_ID;
            tuninfo.vnid = svcp->dvList[i].domain;
            tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
            dove_dps_tunnel_deregister(&tuninfo);
        }

        SVC_UNLOCK(svcp->lock);
        return;
    }
}


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
int dgwy_ctrl_set_dovenet_ipv4(char *name, uint32_t IPV4, uint32_t nexthop)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)dgwy_ctrl_set_dovenet_ipv4;

    update_svc_dovenet_ipv4(name,IPV4);
    g_dovenet_ipv4 = IPV4;
    g_dovenet_ipv4_nexthop = nexthop;

    log_debug(ServiceUtilLogLevel,
              "g_dovenet_ipv4=%x\n",g_dovenet_ipv4);

    check_register_tunnel();

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_dovenet_ipv4_t svcp;
        svcp.cmd = CMD_DOVNET_IPV4;
        svcp.dovenet_ipv4 = IPV4;
        svcp.dovenet_ipv4_nexthop = nexthop;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_dovenet_ipv4_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}


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
int dgwy_ctrl_set_external_ipv4(char *name, uint32_t IPV4, uint32_t nexthop)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)dgwy_ctrl_set_external_ipv4;

    g_extif_ipv4 = IPV4;
    g_extif_ipv4_nexthop = nexthop;

    log_debug(ServiceUtilLogLevel,
              "g_extif_ipv4=%x\n",g_extif_ipv4);

    check_register_tunnel();

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_extif_ipv4_t svcp;
        svcp.cmd = CMD_EXTIF_IPV4;
        svcp.extif_ipv4 = IPV4;
        svcp.extif_ipv4_nexthop = nexthop;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_extif_ipv4_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}



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
int dgwy_ctrl_set_mgmt_ipv4(uint32_t IPV4, uint32_t mask, uint32_t nexthop)
{
    if (IPV4) {
        update_svc_mgmt_ipv4((char*)"APBR",IPV4);
        g_mgmt_ipv4 = IPV4;
        log_debug(ServiceUtilLogLevel,
                  "g_mgmt_ipv4=%x\n",g_mgmt_ipv4);
        update_mgmt_ipv4(0, IPV4, mask, nexthop);
    } else {
        /* set dhcp */
        update_mgmt_ipv4(1, 0, 0, 0);
    }

    return 0;
}

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
int dgwy_ctrl_set_dmc_ipv4(uint32_t IPV4, uint16_t port)
{
    update_svc_dmc_ipv4(IPV4,port);

    if(g_dmc_ipv4 != IPV4)
    {
        /* reset config current version */
        g_dgwconfig_curversion=1;
    }

    g_dmc_ipv4 = IPV4;
    g_dmc_port = port;
    log_debug(ServiceUtilLogLevel,
              "g_dmc_ipv4=%x:%d\n",g_dmc_ipv4, port);
    g_dmc_register_done=0;
            
    if(strlen(g_node_uuid) == 0)
    {
        if(dgadm_read_svc_app_uuid() == DOVE_STATUS_OK)
        {
            dgwy_dmc_registration();
        }
        else
        {
            log_alert(ServiceUtilLogLevel,
                      "UUID not set : DMC register failed\n");
        }
    }
    else
    {
        dgwy_dmc_registration();
    }

    return 0;
}



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
int dgwy_ctrl_set_peer_ipv4(char *name, uint32_t IPV4)
{
    dgwy_ha_task_msg_peer_t *msg 
        = (dgwy_ha_task_msg_peer_t*) malloc (sizeof(dgwy_ha_task_msg_peer_t));

    log_debug(ServiceUtilLogLevel,
              "g_peer_ipv4=%x\n",IPV4);

    if(msg)
    {
        msg->msg_info.type = DGWY_HA_CTRL_PEER_IPV4;
        msg->peer_ipv4 = IPV4;
        msg->local_ipv4 = g_dovenet_ipv4;

        if (queue_send(gDgwyHAMsgId, (char *)&msg, sizeof(msg)) != OSW_OK) 
        {
            log_error(ServiceUtilLogLevel, "Send Message error");
            free(msg);
        }
        else if(send_event(gDgwyHATaskId, DGWY_HA_TASK_MSG_EVENT) != OSW_OK)
        {
            log_error(ServiceUtilLogLevel, "Send EVENT error");
        }
    }
    else
    {
        log_error(ServiceUtilLogLevel,"Alloc failed");
    }
    return 0;
}

int dgwy_ctrl_set_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                 uint32_t ipv4)
{
    struct nl_msg *msg;
    uint32_t extmcast_ipv4=0;

    extmcast_ipv4 = get_extipv4_for_extmcast_vnid(ipv4);
    if(extmcast_ipv4==0)
    {
        return -1;
    }

    dgwy_cb_func = (void*)dgwy_ctrl_set_ext_mcast_vnid;

    update_svc_table_ext_mcast_vnid(ext_mcast_vnid, extmcast_ipv4,1/*add*/);
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_vnid_info_t svcp;
        svcp.cmd  = CMD_VNID_INFO;
        /*svcp.vnid = htonl(ext_mcast_vnid<<8);*/ /* vnid netbyte order */
        svcp.vnid = htonl(ext_mcast_vnid)>>8; /* vnid netbyte order */
        svcp.state = VNID_INFO_STATE_EXTMCAST_SLAVE;
        svcp.ipv4 = extmcast_ipv4;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_vnid_info_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_unset_ext_mcast_vnid(uint32_t ext_mcast_vnid,
                                   uint32_t ipv4)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_set_ext_mcast_vnid;

    update_svc_table_ext_mcast_vnid(ext_mcast_vnid, ipv4,0/*del*/);
    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_vnid_info_t svcp;
        svcp.cmd  = CMD_VNID_INFO;
        /*svcp.vnid = htonl(ext_mcast_vnid<<8);*/ /* vnid netbyte order */
        svcp.vnid = htonl(ext_mcast_vnid)>>8; /* vnid netbyte order */
        svcp.state = VNID_INFO_STATE_NONE; /* extmcast delete */
        svcp.ipv4 = ipv4;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_vnid_info_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;

}



int dgwy_ctrl_ext_mcast_master_state(uint32_t ext_mcast_vnid,
                                     uint32_t ipv4, uint32_t master)
{
    struct nl_msg *msg;

    dgwy_cb_func = (void*)dgwy_ctrl_ext_mcast_master_state;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_vnid_info_t svcp;
        svcp.cmd  = CMD_VNID_INFO;
        svcp.vnid = htonl(ext_mcast_vnid)>>8; /* vnid netbyte order */
        if(master)
        {
            svcp.state = VNID_INFO_STATE_EXTMCAST_MASTER;
        }
        else
        {
            svcp.state = VNID_INFO_STATE_EXTMCAST_SLAVE;
        }
        svcp.ipv4 = ipv4;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_vnid_info_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}


int dgwy_ctrl_set_peer_state(uint32_t peer_ipv4, uint8_t state)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)dgwy_ctrl_set_peer_state;

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_peer_t svcp;
        svcp.cmd = CMD_PEER_IPV4_STATE;
        svcp.peer_ipv4 = peer_ipv4;
        svcp.state = state;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_peer_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_add_vnid_info(uint32_t vnid, 
                            uint32_t IPv4,
                            uint32_t IPv4_mask,
                            uint32_t IPv4_nexthop,
                            uint8_t state)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)dgwy_ctrl_add_vnid_info;

    update_svc_vnid_info(vnid, IPv4, IPv4_mask,
                         IPv4_nexthop, state,
                         1 /* add */);

    msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
    if (msg)
    {
        dgwy_ctrl_vnid_info_t svcp;
        svcp.cmd  = CMD_VNID_INFO;
        svcp.vnid = htonl(vnid)>>8; /* vnid netbyte order */
        svcp.state = state;
        svcp.ipv4 = 0;

        NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_vnid_info_t),&svcp);
        return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
    }
nla_put_failure:
    return -1;
}

int dgwy_ctrl_extshared_vnids(int vnid,int domain,int extmcast, cmds_t cmd)
{
    dgwy_tunnel_info_t tuninfo;
#if defined(SEPRATE_SVC_MODE)
    char *chk_name = svcp->name;
#else
    char chk_name[BRIDGE_NAME_MAX];
    memset(chk_name,0,BRIDGE_NAME_MAX);
    strncpy(chk_name,"APBR",strlen("APBR"));
#endif

    if(cmd == CMD_ADD_EXT_SHARED)
    {
        if(update_svc_table_extsharedvnids(chk_name,
                                           vnid,domain,extmcast, 1/* add */) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table ext shared\n");
            return -1;
        }

        if(g_dovenet_ipv4)
        {
            tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
            tuninfo.vnid = vnid;
            tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
            dove_dps_tunnel_register(&tuninfo);
        }
    }
    else if(cmd == CMD_DEL_EXT_SHARED)
    {
        if(update_svc_table_extsharedvnids(chk_name,
                                           vnid,domain,extmcast, 0/* del */) == -1)
        {
            log_error(ServiceUtilLogLevel,
                        "failed to update svc table ext shared\n");
            return -1;
        }

        check_dregister_tunnel(vnid);
        /* to reset svc type */
        check_register_tunnel();
    
    }

    return 0;
}



/* Return 
 * 1 -  Empty subnets
 * 0 -  Not empty
 * */
int is_vnid_subnet_empty(uint32_t vnid)
{
    dgwy_service_list_t *svcp = NULL;
    int retry   = 0;
    int empty   = 1;
    int result  = 0;
    
    while(1)
    {
        svcp = dgwy_svc_table_get((char *)"APBR", &result,__FUNCTION__);
        if(result == 1)
        {
            /* entry exist but inuse try again */
            if(retry < 256) {
                retry++; 
                continue;
            }
            log_error(ServiceUtilLogLevel,"In use : failed\n");
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
        for(i=0; i<MAX_VNID_SUBNETS; i++)
        {
            if(svcp->vnid_subnet[i].vnid == vnid)
            {
                empty=0;
                break;
            }
        }
        SVC_UNLOCK(svcp->lock);
    }

    return empty;
}

int dgwy_ctrl_del_vnid_info(uint32_t vnid, 
                            uint32_t IPv4,
                            uint32_t IPv4_mask,
                            uint32_t IPv4_nexthop,
                            uint8_t state)
{
    struct nl_msg *msg;
    dgwy_cb_func = (void*)dgwy_ctrl_del_vnid_info;

    update_svc_vnid_info(vnid, IPv4, IPv4_mask,
                         IPv4_nexthop, state,
                         0 /* del */);
    if(is_vnid_subnet_empty(vnid))
    {
        /* all subnet gone from this vnid.
         * set vnid state to default (dedicated)
         * */
        msg = dgwy_nl_message(DGWY_CTRL_CMD_SERVICE, 0);
        if (msg)
        {
            dgwy_ctrl_vnid_info_t svcp;
            svcp.cmd  = CMD_VNID_INFO;
            svcp.vnid = htonl(vnid)>>8; /* vnid netbyte order */
            svcp.state = VNID_INFO_STATE_DEDICATED; /* default state */
            svcp.ipv4 = 0;
            
            NLA_PUT(msg, DGWY_CTRL_ATTR_MSG, sizeof(dgwy_ctrl_vnid_info_t),&svcp);
            return dgwy_nl_send_message(msg, dgwy_noop_cb, NULL);
        }
nla_put_failure:
        return -1;
    }
    return 0;
}


void check_del_tep(void)
{
    dgwy_service_list_t *svcp = NULL;
    int result=0, retry=0;

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
            return;
        }
        else
        {
            break;
        }
    }

    if(svcp)
    {
        int i=0;
        dgwy_tunnel_info_t tuninfo;


        for(i=0; i<MAX_EXT_VNIDS; i++)
        {
            if(svcp->ext_mcast_vnids[i].vnid &&
               (INVALID_DOMAIN_ID!=svcp->ext_mcast_vnids[i].vnid))
            {
                memset(&tuninfo,0,sizeof(tuninfo));
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->ext_mcast_vnids[i].vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_deregister(&tuninfo);
            }
        }

        for(i=0; i<MAX_DOMIANS; i++)
        {
            if(svcp->extsharedVnidList[i].vnid && 
               (INVALID_DOMAIN_ID!=svcp->extsharedVnidList[i].vnid))
            {
                memset(&tuninfo,0,sizeof(tuninfo));
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->extsharedVnidList[i].vnid;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_deregister(&tuninfo);
            }
        }


        for(i=0; i<MAX_EXTIPV4_ADDRESS; i++)
        {
            if(svcp->extipv4List[i].ipv4 &&
               (INVALID_DOMAIN_ID!=svcp->extipv4List[i].domain))
            {
                memset(&tuninfo,0,sizeof(tuninfo));
                tuninfo.type = DOVE_EXTERNAL_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->extipv4List[i].domain;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_deregister(&tuninfo);
            }
        }
    
        for(i=0; i<MAX_DVMAP; i++)
        {
            if(svcp->dvList[i].domain &&
               (INVALID_DOMAIN_ID!=svcp->dvList[i].domain))
            {
                memset(&tuninfo,0,sizeof(tuninfo));
                tuninfo.type = DOVE_VLAN_GATEWAY_AGENT_ID;
                tuninfo.vnid = svcp->dvList[i].domain;
                tuninfo.dovenet_ipv4 = g_dovenet_ipv4; 
                dove_dps_tunnel_deregister(&tuninfo);
            }
        }
        SVC_UNLOCK(svcp->lock);
    }
}


void fwdrule_pip_deregister(uint32_t ipmin, uint32_t ipmax, uint32_t domain)
{
    int iter=0;
    uint32_t minip = ntohl(ipmin);
    uint32_t maxip = ntohl(ipmax);
    int range = maxip-minip;
    for(iter=0;iter<=range;iter++)
    { 
        dgwy_deregister_vip_location(domain, minip+iter,
                                     ntohl(g_dovenet_ipv4), 0, gMAC);
    }
}


