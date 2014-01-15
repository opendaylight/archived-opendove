/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */





#ifndef __DGWY_CTL_H__
#define __DGWY_CTL_H__


#if defined(__KERNEL__)
#define EXT_FT_LOCKING
#define INT_FT_LOCKING
#define OVL_LOC_LOCKING
#define LEGACY_IP_TBL_LOCKING
#define VLAN_PEER_RUNTIME_LOCK
#define VLAN_PEER_TBL_LOCKING
#define VNID_INFO_LOCKING
#define MCAST_LOCKING
#define FWD_DYNAMIC_TABLE_LOCKING
#endif /* __KERNEL__ */

#define SVC_NAME 128

#define PORT_POOL_SZ (1<<14) /* 2^14 */

typedef enum {
    DGWY_SUCCESS  = 0,
    DGWY_ERROR    = 1,
    DGWY_ERROR_EXCEEDS_CAP = 2,
    DGWY_ERROR_NO_RESOURCES = 3 
} dgwy_return_status;

typedef enum dgwy_type {
    DGWY_TYPE_NONE=0,
    DGWY_TYPE_EXTERNAL=1,
    DGWY_TYPE_VLAN,
    DGWY_TYPE_EXT_VLAN,
}dgwy_type_t;


#define DGWY_CTRL_IOCTL 'G'
#define DGWY_READ_IOCTL _IOR(DGWY_CTRL_IOCTL, 0, int)
#define DGWY_WRITE_IOCTL _IOW(DGWY_CTRL_IOCTL, 1, int)
#define DGWY_VERSION_NR 1

#define INVALID_DOMAIN_ID 0x1FFFFFF /* more than 24 bit */

enum {
    DGWY_CTRL_ATTR_UNSPEC,
    DGWY_CTRL_ATTR_MSG,
    __DGWY_CTRL_ATTR_MAX,
};
#define DGWY_CTRL_ATTR_MAX (__DGWY_CTRL_ATTR_MAX - 1)
#define MAX_MSG_LEN 4096

enum {
    DGWY_CTRL_CMD_UNSPEC,
    DGWY_CTRL_CMD_SERVICE,
    DGWY_CTRL_CMD_GETINFO,
    DGWY_CTRL_CMD_INFO_RESP,
    DGWY_CTRL_CMD_LISTENER,
    DGWY_CTRL_CMD_LISTENER_EVENTS,
    DGWY_CTRL_CMD_LISTENER_TEST,
    __DGWY_CTRL_CMD_MAX,
};
#define DGWY_CTRL_CMD_MAX (__DGWY_CTRL_CMD_MAX - 1)
#define MAX_DOMIANS          1024 /* MAX domains per service can serve */
#define MAX_IF_MACS          8    /* MAX MACS possible in one service */
#define MAX_VLANS            16   /* MAX VLANS mapped to a domain */
#define MAX_IFIPV4_ADDRESS   1024 /* MAX IPV4 address assigned to service */
#define MAX_EXTIPV4_ADDRESS  1024 /* MAX ext-virt-ips (domain public-ips */
#define MAX_INTIPV4_ADDRESS  1024 /* MAX internal virtual ips */
#define MAX_FWDRULE          1024 /* MAX port fwd rules (service maps) */
#define MAX_DVMAP            MAX_DOMIANS /* MAX DOMAIN VLAN Mappings */
#define MAX_VNID_SUBNETS     32
#define MAX_EXT_VNIDS        1024


/* @@@ !!! WARNING !!! @@@ 
 * svc index used as route table index.
 * linux can have max of 255 route tables in which 
 * 0,255,254,253 are assigned for system.
 * Do not increase MAX_SERVICE_TABLES beyond 253 
 * and do not use 0 for INDX START!
 * */
#define SVC_INDX_START       1
#define MAX_SERVICE_TABLES   2

#if defined(__KERNEL__)
#include <net/genetlink.h>
#else
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#endif /* __KERNEL__ */

#define DGWY_IOCTL_MAX_SZ   8*1024    /* MAX IOCTL MSG LEN */
#define DGWY_GENL_NAME      "DGWY_CONTROL"

#define OVL_PROTO           17      /* UDP */
#define OVL_PROTO_SRC_PORT  0x00FF  /* OVL SRC PORT [Gateway use DVG 0]*/

extern uint16_t OVL_PROTO_DST_PORT; //60000   /* OVL DST PORT */ 

typedef enum cmds 
{
    CMD_ADD_SVC         =   0,
    CMD_SET_SVC         =   1,
    CMD_DEL_SVC         =   2,
    CMD_TYPE            =   3,
    CMD_MTU             =   4,
    CMD_SET_DOMAIN      =   5,
    CMD_DEL_DOMAIN      =   6,
    CMD_ADD_IFMAC       =   7,
    CMD_DEL_IFMAC       =   8,
    CMD_ADD_IPV4        =   9,
    CMD_DEL_IPV4        =  10,
    CMD_ADD_EXTVIP      =  11,
    CMD_DEL_EXTVIP      =  12,
    CMD_ADD_FWD_RULE    =  13,
    CMD_SET_FWD_RULE    =  14,
    CMD_DEL_FWD_RULE    =  15,
    CMD_ADD_INFO_RESP   =  16,
    CMD_ADD_INTVIP      =  17,
    CMD_DEL_INTVIP      =  18,
    CMD_SET_OVL_IP      =  19,
    CMD_DEL_OVL_IP      =  20,
    CMD_START           =  21,
    CMD_STOP            =  22,
    CMD_SET_DOMAIN_VLAN =  23,
    CMD_DEL_DOMAIN_VLAN =  24,
	CMD_UPDATE_OVL_IP   =  25,
	CMD_BCAST_LIST      =  26,
	CMD_MCAST_LIST      =  27,
    CMD_DOVNET_IPV4     =  28,
    CMD_PEER_IPV4_STATE =  29,
    CMD_VNID_INFO       =  30,
    CMD_SHOW_OVL_ENTRY  =  31,
    CMD_ADDR_RESOLVE    =  32,
    CMD_SET_OVL_PORT    =  33,
    CMD_LEGACY_NXTHOP   =  34,
    CMD_REFRESH_OVL_IP  =  35,
    CMD_UPDATE_OVL_IP_SHARED = 36,
    CMD_ADD_EXT_SHARED  =  37,
    CMD_DEL_EXT_SHARED  =  38,
    CMD_RESET_STATS     =  39,
    CMD_EXTIF_IPV4      =  40,
    CMD_NONE,
} cmds_t;

typedef struct dgwy_ctrl_global
{
    uint8_t cmd;
    /* in future there may be some global propertes */
}dgwy_ctrl_global_t;

typedef struct dgwy_ctrl_dovenet_ipv4
{
    uint8_t cmd;
    uint32_t dovenet_ipv4;
    uint32_t dovenet_ipv4_nexthop;
}dgwy_ctrl_dovenet_ipv4_t;

typedef struct dgwy_ctrl_extif_ipv4
{
    uint8_t cmd;
    uint32_t extif_ipv4;
    uint32_t extif_ipv4_nexthop;
}dgwy_ctrl_extif_ipv4_t;

typedef struct dgwy_ctrl_peer
{
    uint8_t     cmd;
    uint32_t    peer_ipv4;
    uint8_t     state;
}dgwy_ctrl_peer_t;

typedef struct dgwy_ctrl_vnid_shared
{
    uint8_t     cmd;
    uint32_t    vnid;
    uint8_t     state;
    uint32_t    ipv4;
}dgwy_ctrl_vnid_info_t;

typedef struct dgwy_ctrl_req
{
    uint8_t cmd;
    char data[0];
} dgwy_ctrl_req_t;

typedef struct dgwy_ctrl_resp
{
    uint8_t cmd;
    char data[0];
} dgwy_ctrl_resp_t;

typedef struct dgwy_ctrl_info_resp
{
    uint8_t cmd;
    uint8_t version;
	int 	chardev_major;
} dgwy_ctrl_info_resp_t;

typedef struct dgwy_ctrl_event_resp
{
    uint8_t cmd;
    uint8_t version;
} dgwy_ctrl_event_resp_t;

typedef struct dgwy_ctrl_svc_mtu
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    uint16_t    mtu;
} dgwy_ctrl_svc_mtu_t;

typedef struct dgwy_ctrl_ovl_port
{
    uint8_t     cmd;
    uint16_t    port;
} dgwy_ctrl_ovl_port_t;

typedef struct dgwy_ctrl_svc_type
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    dgwy_type_t type;
} dgwy_ctrl_svc_type_t;

typedef struct dgwy_ctrl_svc_domain
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    uint32_t    domains;
} dgwy_ctrl_svc_domain_t;

typedef struct dgwy_ctrl_svc_domain_vlans
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    uint32_t    domain;
    uint16_t    vlans[MAX_VLANS];
} dgwy_ctrl_svc_domain_vlans_t;


typedef struct mac_cfg {
    uint8_t mac[6];
    uint16_t vlan;
} mac_cfg_t;

typedef struct dgwy_ctrl_svc_macs
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    mac_cfg_t   mac_cfg;
}dgwy_ctrl_svc_macs_t;

typedef struct dgwy_ctrl_svc_ipv4
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    uint32_t    ifipv4;
} dgwy_ctrl_svc_ipv4_t;

struct extipv4 {
    uint16_t    port_start;
    uint16_t    port_end;
    uint32_t    ipv4;
    uint32_t    domain;
    uint32_t    tenant_id;
    uint32_t    extmcastvnid;
}__attribute__((packed));
typedef struct extipv4 extipv4_t;

struct ifipv4 {
    uint32_t    ipv4;
    uint32_t    mask;
    uint32_t    nexthop;
    uint16_t    vlan_id;
}__attribute__((packed));
typedef struct ifipv4 ifipv4_t;

struct extshared_vnids {
    uint32_t    vnid;
    uint32_t    tenant_id;
    uint32_t    extmcastvnid;
}__attribute__((packed));
typedef struct extshared_vnids extshared_vnids_t;


typedef struct dgwy_ctrl_svc_extvip
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    extipv4_t   extip;
} dgwy_ctrl_svc_extvip_t;


struct intipv4 {
    uint16_t    port_start;
    uint16_t    port_end;
    uint32_t    ipv4;
}__attribute__((packed));
typedef struct intipv4 intipv4_t;

typedef struct dgwy_ctrl_svc_intvip
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    intipv4_t   intip;
} dgwy_ctrl_svc_intvip_t;

typedef struct dgwy_ctrl_fwdtbl
{
    uint8_t     cmd;
    uint8_t     svc_name[SVC_NAME];
    uint32_t    fwdipv4;
    uint16_t    fwdport;
    uint8_t     fwdproto;
    uint32_t    realipv4;
    uint16_t    realport;
    uint32_t    domain;
    uint32_t    pip_min;
    uint32_t    pip_max;
    uint32_t    pip_port_min;
    uint32_t    pip_port_max;
} dgwy_ctrl_fwdtbl_t;

typedef struct dgwy_ctrl_ovl_location
{
    uint8_t     cmd;
    uint8_t     index;
    uint8_t     svc_name[SVC_NAME];
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
} dgwy_ctrl_ovl_location_t;

typedef struct dgwy_ctrl_timer
{
	uint16_t	interval;
	void 		*data;
} dgwy_ctrl_timer_t;

typedef struct dgwy_legacy_nexthop_info
{
    uint8_t     cmd;
    uint32_t    vnid;
    __be32      legacy_ip4;
    __be32      legacy_nexthop;
} dgwy_legacy_nexthop_info_t;


#define DPS_LOCATION_QUERY                  1   
#define DPS_LOCATION_MAC_UPDATE             2
#define DPS_LOCATION_MAC_DELETE             3
#define DPS_LOCATION_VIP_UPDATE             4
#define DPS_LOCATION_VIP_DELETE             5
#define DPS_LOCATION_UNAVAILABLE            6
#define DPS_MCAST_RECV_JOIN                 7
#define DPS_MCAST_RECV_LEAVE                8
#define DPS_MCAST_SENDER_REGISTER           9
#define DPS_MCAST_SENDER_LEAVE              10
#define DPS_LOCATION_QUERY_TEST             11
#define DPS_MCAST_VGW_ALL_RECV_JOIN         12
#define DPS_MCAST_VGW_ALL_RECV_LEAVE        13


#define MAX_TUNNEL_INFO 16

struct dgwy_dps_query_list_info
{
    uint8_t                 type;
    uint32_t                domain;
    __be32                  ip4;
    __be32                  phyip4;
    uint8_t                 endpoint_mac[6];        /* VMS MAC */ 
    __be32                  src_ipv4       ;        /* SRC VMS IPV4*/ 
    uint8_t                 src_endpoint_mac[6];    /* SRC VMS MAC */ 
    uint8_t                 pad;
} __attribute__( ( packed ) );

typedef struct dgwy_dps_query_list_info dgwy_dps_query_list_info_t;

struct dgwy_tunnel_info
{
    uint8_t                 type;
    uint32_t                vnid;
    __be32                  dovenet_ipv4;
} __attribute__( ( packed ) );

typedef struct dgwy_tunnel_info dgwy_tunnel_info_t;


#define MAX_BROADCAST_MEMBERS_PER_DOMAIN 64 //TODO: Max 4096 should be supported
#define MAX_INTGW_MEMBERS_PER_DOMAIN 8      //TODO: Max 4096 should be supported
struct dgwy_ctrl_broadcast_lst {
    uint8_t     cmd;               /* CMD_SET_DOMAIN_VLAN (or) CMD_DEL_DOMAIN_VLAN */ 
    uint32_t    domain_id;
    uint16_t    num_v4_switch;
    uint16_t    num_v6_switch;
    uint32_t    switch_list[MAX_BROADCAST_MEMBERS_PER_DOMAIN];
    uint32_t    source_ip_list[MAX_BROADCAST_MEMBERS_PER_DOMAIN];
    uint16_t    sport;
    uint16_t    dport;
}__attribute__((packed));
typedef struct dgwy_ctrl_broadcast_lst dgwy_ctrl_broadcast_lst_t;


#define SHARED_VNID_MARK 0x2FFFFFF
#define SHARED_VNID_MARK_NETBYTE (htonl(0x2FFFFFF))

#define GLB_SCOPE_IPMC_START_ADDR   0xe0000100 //0x100e0
#define GLB_SCOPE_IPMC_STOP_ADDR    0xeeffffff //0xffffffee
#define LCL_SCOPE_IPMC_START_ADDR   0xef
#define LCL_SCOPE_IPMC_STOP_ADDR    0xffffffef

typedef struct mcast_entry_s
{
    uint32_t                vnid;                  /* Receiver vnid */
    uint32_t                tep_ip;                /* Receiver TEP IPV4*/ 
}mcast_entry_t;

#define MAX_MCAST_RCVER 256         /* TODO INCREASE TO 4K */
struct dgwy_ctrl_mcast_lst {
    uint8_t     cmd;               /* CMD_SET_DOMAIN_VLAN (or) CMD_DEL_DOMAIN_VLAN */ 
    uint32_t    vnid;
    uint32_t    mcast_ip;
    uint8_t     mcast_mac[6];
    uint16_t    num_v4_switch;
    mcast_entry_t recv_list[MAX_MCAST_RCVER];
    uint16_t    sport;
    uint16_t    dport;
}__attribute__((packed));
typedef struct dgwy_ctrl_mcast_lst dgwy_ctrl_mcast_lst_t;


struct dgwy_extern_flow_info
{
    uint32_t                orig_overlay_sip;   /* Original outer sip */
    uint32_t                orig_overlay_dip;   /* Original outer dip */
    uint32_t                orig_overlay_proto; /* Original outer proto */
    uint16_t                orig_overlay_sport;
    uint16_t                orig_overlay_dport;

    uint32_t                orig_sip;           /* ORIGINAL SIP */
    uint32_t                orig_dip;           /* ORIGINAL DIP */
    __be16                  orig_sport;         /* ORIGINAL SPORT */
    __be16                  orig_dport;         /* ORIGINAL DPORT */
    int64_t                 timesince;
    
    uint32_t                sip;                /* inner sip */
    uint32_t                dip;                /* inner dip */
    uint8_t                 proto;              /* inner ip proto number*/
    __be16                  sport;              /* inner tcp/udp sport number*/
    __be16                  dport;              /* inner tcp/udp dport number*/
    uint8_t                 action;             /* action SNAT/DNAT/FULLNAT/NOOP */
    uint32_t                snat_ip;            /* SNAT IP */
    __be16                  snat_port;          /* SNAT PORT */
    uint32_t                vnid;
};

#define MAX_SESSION_DUMP_COUNT (20*1000) /*(2K)*/
typedef struct ext_sesion_dump_s
{
    uint32_t    count;
    struct dgwy_extern_flow_info sess_info[MAX_SESSION_DUMP_COUNT];
}ext_sesion_dump_t;

struct dgwy_fwddyn_flow_info
{
    uint32_t                svc_vip_ip;         /* EXTERNAL IP   */
    uint16_t                svc_vport;          /* EXTERNAL PORT */
    uint8_t                 svc_proto;          /* PROTOCOL      */
    uint32_t                real_ip;            /* OVERLAY IP    */
    uint16_t                real_port;          /* OVERLAY PORT  */
    uint8_t                 type;               /* TYPE */
    int64_t                 timesince;
    uint32_t                vnid;
};

typedef struct fwddyn_session_dump_s
{
    uint32_t    count;
    struct dgwy_fwddyn_flow_info sess_info[MAX_SESSION_DUMP_COUNT];
}fwddyn_session_dump_t;


struct dgwy_intern_flow_info 
{
    uint32_t                vnid;
#if 0
    union nf_inet_addr      overlay_sip;        /* overlay header sip  */
    union nf_inet_addr      overlay_dip;        /* overlay header dip  */
    u8                      overlay_proto;      /* overlay ip proto number 
                                                 * GRE, DOVE, VXLAN etc */
#endif
    uint32_t                sip;                /* inner sip */
    uint32_t                dip;                /* inner dip */
    uint8_t                 proto;              /* inner ip proto number*/
    __be16                  sport;              /* inner tcp/udp sport number*/
    __be16                  dport;              /* inner tcp/udp dport number*/
    uint8_t                 action;             /* action SNAT/DNAT/FULLNAT/NOOP */

    uint32_t                dnat_ip;            /* DNAT IP */
    __be16                  dnat_port;          /* DNAT PORT */
    uint32_t                snat_ip;            /* SNAT IP */
    __be16                  snat_port;          /* SNAT PORT */

    uint32_t                orig_sip;           /* ORIGINAL SIP */
    uint32_t                orig_dip;           /* ORIGINAL DIP */
    __be16                  orig_sport;         /* ORIGINAL SPORT */
    __be16                  orig_dport;         /* ORIGINAL DPORT */

    unsigned long           lastjiffies;        /* last access time in jiffies */
    int64_t                 timesince;
};

typedef struct int_session_dump_s
{
    uint32_t    count;
    struct dgwy_intern_flow_info sess_info[MAX_SESSION_DUMP_COUNT];
}int_session_dump_t;
    

struct dgwy_vnid_stats
{
    uint32_t    vnid;
    /* PER VNID RX COUNTERS */
    uint64_t    ext_gw_overlay_rxbytes;
    uint64_t    ext_gw_overlay_rxpkts;
    uint64_t    ext_gw_overlay_rxbps;
    uint64_t    ext_gw_overlay_rxpps;
    uint64_t    vlan_gw_overlay_rxbytes;
    uint64_t    vlan_gw_overlay_rxpkts;
    uint64_t    vlan_gw_overlay_rxbps;
    uint64_t    vlan_gw_overlay_rxpps;

    /* PER VNID TX COUNTERS */
    uint64_t    ext_gw_overlay_txbytes;
    uint64_t    ext_gw_overlay_txpkts;
    uint64_t    vlan_gw_overlay_txbytes;
    uint64_t    vlan_gw_overlay_txpkts;
    uint64_t    ext_gw_overlay_txbps;
    uint64_t    ext_gw_overlay_txpps;
    uint64_t    vlan_gw_overlay_txbps;
    uint64_t    vlan_gw_overlay_txpps;
};

#define MAX_VNID_STATS_COUNT (128) /*(2K)*/
typedef struct dgwy_vnid_stats_s
{
    uint32_t    count;
    struct dgwy_vnid_stats vnid_stats[MAX_VNID_STATS_COUNT];
}dgwy_vnid_stats_t;

typedef struct dgwy_stats
{
    /* OVERALL RX COUNTERS */
    uint64_t    ext_gw_overlay_rxbytes;
    uint64_t    ext_gw_overlay_rxpkts;
    uint64_t    ext_gw_overlay_rxbps;
    uint64_t    ext_gw_overlay_rxpps;
    uint64_t    ext_gw_overlay_mcast_rxbytes;
    uint64_t    ext_gw_overlay_mcast_rxpkts;

    uint64_t    ext_gw_rxbytes;
    uint64_t    ext_gw_rxpkts;
    uint64_t    ext_gw_rxbps;
    uint64_t    ext_gw_rxpps;
    uint64_t    ext_gw_mcast_rxbytes;
    uint64_t    ext_gw_mcast_rxpkts;

    uint64_t    vlan_gw_overlay_rxbytes;
    uint64_t    vlan_gw_overlay_rxpkts;
    uint64_t    vlan_gw_overlay_rxbps;
    uint64_t    vlan_gw_overlay_rxpps;
    uint64_t    vlan_gw_overlay_mcast_rxbytes;
    uint64_t    vlan_gw_overlay_mcast_rxpkts;

    uint64_t    vlan_gw_rxbytes;
    uint64_t    vlan_gw_rxpkts;
    uint64_t    vlan_gw_rxbps;
    uint64_t    vlan_gw_rxpps;
    uint64_t    vlan_gw_mcast_rxbytes;
    uint64_t    vlan_gw_mcast_rxpkts;

    /* OVERALL TX COUNTERS */
    uint64_t    ext_gw_overlay_txbytes;
    uint64_t    ext_gw_overlay_txpkts;
    uint64_t    ext_gw_overlay_txbps;
    uint64_t    ext_gw_overlay_txpps;
    uint64_t    ext_gw_overlay_mcast_txbytes;
    uint64_t    ext_gw_overlay_mcast_txpkts;

    uint64_t    ext_gw_txbytes;
    uint64_t    ext_gw_txpkts;
    uint64_t    ext_gw_txbps;
    uint64_t    ext_gw_txpps;
    uint64_t    ext_gw_mcast_txbytes;
    uint64_t    ext_gw_mcast_txpkts;

    uint64_t    vlan_gw_overlay_txbytes;
    uint64_t    vlan_gw_overlay_txpkts;
    uint64_t    vlan_gw_overlay_txbps;
    uint64_t    vlan_gw_overlay_txpps;
    uint64_t    vlan_gw_overlay_mcast_txbytes;
    uint64_t    vlan_gw_overlay_mcast_txpkts;

    uint64_t    vlan_gw_txbytes;
    uint64_t    vlan_gw_txpkts;
    uint64_t    vlan_gw_txbps;
    uint64_t    vlan_gw_txpps;
    uint64_t    vlan_gw_mcast_txbytes;
    uint64_t    vlan_gw_mcast_txpkts;
}dgwy_stats_t;

#define PROC_DGWY_SESSION_EXT     "dgwy_session_ext"
#define PROC_DGWY_SESSION_FWDDYN  "dgwy_session_fwddyn"
#define PROC_DGWY_SESSION_INT     "dgwy_session_int"

#define PROC_DGWY_EXT_OVL_RX      "dgwy_ext_ovl_rxcounters"
#define PROC_DGWY_EXT_OVL_TX      "dgwy_ext_ovl_txcounters"
#define PROC_DGWY_EXT_RX          "dgwy_ext_rxcounters"
#define PROC_DGWY_EXT_TX          "dgwy_ext_txcounters"

#define PROC_DGWY_VLAN_OVL_RX      "dgwy_vlan_ovl_rxcounters"
#define PROC_DGWY_VLAN_OVL_TX      "dgwy_vlan_ovl_txcounters"
#define PROC_DGWY_VLAN_RX          "dgwy_vlan_rxcounters"
#define PROC_DGWY_VLAN_TX          "dgwy_vlan_txcounters"

#define PROC_DGWY_VNID_STATS       "dgwy_vnid_stats"
#define PROC_DGWY_ALL_STATS        "dgwy_all_stats"

#define ACTION_EXT_SNAT             1
#define ACTION_EXT_SNAT_REV         2
#define ACTION_INT_SNAT             3
#define ACTION_INT_SNAT_REV         4
#define ACTION_VLAN_ENCAP           5
#define ACTION_VLAN_DECAP           6
#define ACTION_EXT_SHARED           7
#define ACTION_EXT_SHARED_REV       8
#define ACTION_EXT_IGMP             9
#define ACTION_EXT_IPMC             10
#define ACTION_VLAN_ENCAP_LOOPBACK  11
#define ACTION_EXT_SNAT_REV_LOOP    12
#define ACTION_INT_SNAT_LOOP        13
#define ACTION_EXT_SHARED_REV_LOOP  14

#define DGWY_FWDDYN_TYPE_EXT        1  
#define DGWY_FWDDYN_TYPE_INT_PORT   2
#define DGWY_FWDDYN_TYPE_INT_IPONLY 3 /* Created as pasrt of FWD IP ONLY rule ; DON'T AGE */

#if defined(__KERNEL__)
#if __x86_64__
typedef atomic64_t dgw_stats_t;
#define DGW_STAT_ADD atomic64_add 
#define DGW_STAT_SET atomic64_set 
#define DGW_STAT_INC atomic64_inc 
#define DGW_STAT_READ atomic64_read 
#else
typedef atomic_t dgw_stats_t;
#define DGW_STAT_ADD atomic_add 
#define DGW_STAT_SET atomic_set 
#define DGW_STAT_INC atomic_inc 
#define DGW_STAT_READ atomic_read 
#endif /*__x86_64__*/
#endif /*__KERNEL__*/


#define VNID_INFO_STATE_DEDICATED       0
#define VNID_INFO_STATE_SHARED          1
#define VNID_INFO_STATE_EXTMCAST_SLAVE  2
#define VNID_INFO_STATE_EXTMCAST_MASTER 3
#define VNID_INFO_STATE_NONE            4


#endif /* __DGWY_CTL_H__ */
