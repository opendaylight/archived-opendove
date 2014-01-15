/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 * */


#ifndef _CLI_SERVICE_
#define _CLI_SERVICE_

/*
 *****************************************************************************
 * CLI (Service) Configuration Handling                                  *//**
 *
 * \addtogroup DOVEGatewayCLI
 * @{
 * \defgroup DOVEGatewayCLIService Service for CLI configuration
 * @{
 * ALERT!!!
 * 1. No pointers allowed in these structures.
 * 2. Only expand CLI codes in increasing order. DO NOT insert a new code in
 *    between.
 * 3. This file MUST be kept in sync with the following PYTHON scripts.
 *    cli/python/gateway_cli/service.py
 *
 */

/**
 * \brief The MINOR Command Code for SERVICE CLI Requests sent to DOVE
 *        GATEWAY
 *
 * \remarks: Only expand CLI codes in increasing order. DO NOT insert a
 *           new code in between.
 *           CLI_SERVICE_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define CLI_SERVICE_CODES \
	CLI_SERVICE_CODE_AT(CLI_SERVICE_LOG_LEVEL,                       0)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_TYPE,                        1)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_IFMAC,                       2)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_IPV4,                        3)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_EXTVIP,                      4)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_OVERLAYVIP,                  5)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_FWRULE,                      6)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_LOCATION,                    7)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_REGISTER_LOCATION,               8)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_ADD_DPS,                         9)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_DPS_LOOKUP,                     10)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_SET_MTU,                        11)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_SET_START,                      12)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_SET_STOP,                       13)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_DVLAN,                          14)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_SHOW_ALL,                       15)\
    CLI_SERVICE_CODE_AT(CLI_SERVICE_SAVE_TOFILE,                    16)\
    CLI_SERVICE_CODE_AT(CLI_SERVICE_SHOW_CONFIG,                    17)\
    CLI_SERVICE_CODE_AT(CLI_SERVICE_SAVE_STARTUP,                   18)\
    CLI_SERVICE_CODE_AT(CLI_SERVICE_LOAD_CONFIG,                    19)\
    CLI_SERVICE_CODE_AT(CLI_SERVICE_SET_TYPE,                       20)\
    CLI_SERVICE_CODE_AT(CLI_DOVE_NET_IPV4,                          21)\
    CLI_SERVICE_CODE_AT(CLI_MGMT_IPV4,                              22)\
    CLI_SERVICE_CODE_AT(CLI_PEER_IPV4,                              23)\
    CLI_SERVICE_CODE_AT(CLI_DMC_IPV4,                               24)\
    CLI_SERVICE_CODE_AT(CLI_ADD_VNID_SUBNET,                        25)\
    CLI_SERVICE_CODE_AT(CLI_DEL_VNID_SUBNET,                        26)\
    CLI_SERVICE_CODE_AT(CLI_SHOW_OVL_SYS,                           27)\
    CLI_SERVICE_CODE_AT(CLI_EXT_MCAST_VNID,                         28)\
    CLI_SERVICE_CODE_AT(CLI_SHOW_EXT_SESSIONS,                      29)\
    CLI_SERVICE_CODE_AT(CLI_MGMT_DHCP,                              30)\
	CLI_SERVICE_CODE_AT(CLI_SERVICE_MAX,                            31)

#define CLI_SERVICE_CODE_AT(_cli_code, _val) _cli_code = _val,
typedef enum {
	CLI_SERVICE_CODES
}cli_service_code;
#undef CLI_SERVICE_CODE_AT

#define BRIDGE_NAME_MAX 16
#define SERVICE_TYPE_MAX 8
/**
 * \brief The Structure for Changing Log Level of Service
 */
typedef struct cli_service_log_level_s{
	uint32_t	log_level;
}cli_service_log_level_t;


/**
 * \brief The Structure for Adding MAC Interface CLI_SERVICE_ADD_IFMAC
 */
typedef struct cli_service_type_s{
	/**
	 * \brief The Name of the bridge for e.g. "br0"
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The MAC Address
	 */
	char type[SERVICE_TYPE_MAX];
} cli_service_type_t;



/**
 * \brief The Structure for Adding MAC Interface CLI_SERVICE_ADD_IFMAC
 */
typedef struct cli_service_ifmac_s{
	/**
	 * \brief The Name of the bridge for e.g. "br0"
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The MAC Address
	 */
	char mac[6];
} cli_service_ifmac_t;

/**
 * \brief The Structure for Adding IP Address CLI_SERVICE_ADD_IPV4
 */
typedef struct cli_service_addressv4_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The IP Address being added
	 */
	uint32_t IPv4;
	/**
	 * \brief The Network Mask of IP Address being added
	 */
	uint32_t IPv4_netmask;
	uint32_t IPv4_nexthop; 
    int vlan_id;
}cli_service_addressv4_t;

/**
 * \brief The Structure for  DOVE NET IP Address CLI_DOVE_NET_IPV4
 */
typedef struct cli_dove_net_ipv4_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The IP Address being added
	 */
	uint32_t IPv4;
    uint32_t IPv4_nexthop;
}cli_dove_net_ipv4_t;

/**
 * \brief The Structure for mgmt IP Address CLI_MGMT_IPV4
 */
typedef struct cli_mgmt_ipv4_s{
	/**
	 * \brief The Name of the bridge
	 */
	//char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The IP Address being added
	 */
	uint32_t IPv4;
	uint32_t IPv4_mask;
	uint32_t IPv4_nexthop;
}cli_mgmt_ipv4_t;

/**
 * \brief The Structure for mgmt IP Address CLI_PEER_IPV4
 */
typedef struct cli_peer_ipv4_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The IP Address being added
	 */
	uint32_t IPv4;
}cli_peer_ipv4_t;


/**
 * \brief The Structure for mgmt IP Address CLI_DMC_IPV4
 */
typedef struct cli_dmc_ipv4_s{
	/**
	 * \brief The IP Address being added
	 */
	uint32_t IPv4;
	uint16_t port;
} cli_dmc_ipv4_t;

typedef struct cli_ext_mcast_vnid_s{
	/**
	 * \brief ext mcast vnid
	 */
	uint32_t vnid;
	uint32_t ipv4;
}cli_ext_mcast_vnid_t;

/**
 * \brief The Structure for vnid subnet CLI_ADD_VNID_SUBNET
 */
typedef struct cli_vnid_subnet_s{
	/**
	 * \brief The IP Address being added
	 */
    uint32_t vnid;
	uint32_t IPv4;
	uint32_t IPv4_mask;
	uint8_t  subnet_mode; /* 1 - shared */
    uint32_t IPv4_nexthop;
} cli_vnid_subnet_t;

/**
 * \brief The Structure for Adding external VIP CLI_SERVICE_ADD_EXTVIP and
 *        The Structure for Adding Overlay Virtual IP CLI_SERVICE_ADD_OVERLAYVIP
 */

typedef struct cli_service_extvip_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief The Virtual IP being added
	 */
	uint32_t vIPv4;
	/**
	 * \brief The Min of the Range of Ports;
	 */
	uint16_t port_min;
	/**
	 * \brief The Max of the Range of Ports;
	 */
	uint16_t port_max;

	uint32_t tenant_id;
    uint32_t extmcastvnid;
} cli_service_extvip_t, cli_service_overlayip_t;

/**
 * \brief The Structure for adding a forwarding rule CLI_SERVICE_ADD_FWRULE
 */
typedef struct cli_service_fwdrule_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief The IP Address (IPv4)
	 */
	uint32_t IPv4;
	/**
	 * \brief Mapping IP
	 */
	uint32_t IPv4_map;
	/**
	 * \brief Port
	 */
	uint16_t port;
	/**
	 * \brief Mapping Port
	 */
	uint16_t port_map;
	/**
	 * \brief Protocol
	 */
	uint16_t protocol;

	uint32_t pip_min;
	uint32_t pip_max;
}cli_service_fwdrule_t;

/**
 * \brief The Structure for adding Location
 */
typedef struct cli_service_location_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief The IP Address (IPv4)
	 */
	uint32_t location_IP;
	/**
	 * \brief End-Location
	 */
	uint32_t end_location_IP;
	/**
	 * \brief End-Source-Location
	 */
	uint32_t end_src_location_IP;
	/**
	 * \brief Overlay Protocol
	 */
	uint16_t ovl_proto;
	/**
	 * \brief Overlay source port
	 */
	uint16_t ovl_src_port;
	/**
	 * \brief Overlay destination port
	 */
	uint16_t ovl_dst_port;
	/**
	 * \brief Overlay Destination Mac
	 */
	char ovl_dst_mac[6];
}cli_service_location_t;

/**
 * \brief The Structure for register Location
 */
typedef struct cli_register_location_s{
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief The IP Address (IPv4)
	 */
	uint32_t vm_location_IP;
	/**
	 * \brief End-Location
	 */
	uint32_t phy_location_IP;
	/**
	 * \brief Overlay DVG
	 */
	uint16_t dvg;
	/**
	 * \brief Overlay Destination Mac
	 */
	char ovl_dst_mac[6];
}cli_register_location_t;


/**
 * \brief The Structure for adding dps server
 */
typedef struct cli_dps_server_s{
	/**
	 * \brief The IP Address (IPv4)
	 */
	uint32_t dps_IP;
	/**
	 * \brief Server Port
	 */
	uint16_t port;
   /**
	 * \brief domain
	 */
	uint32_t domain;
}cli_dps_server_t;

/**
 * \brief The Structure for dps lookup
 */
typedef struct cli_dps_lookup_s{
	/**
	 * \brief domain
	 */
	uint32_t domain;

	/**
	 * \brief The IP Address (IPv4)
	 */
	uint32_t vm_IP;
}cli_dps_lookup_t;


/**
 * \brief The Structure for Adding MAC Interface CLI_SERVICE_SET_MTU
 */
typedef struct cli_service_mtu_s{
	/**
	 * \brief The Name of the bridge for e.g. "br0"
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief The MTU bytes
	 */
    uint16_t mtu;
} cli_service_mtu_t;

/**
 * \brief The Structure for SAVING CONFIG TO FILE
 */
typedef struct cli_service_cfg_s{
	/**
	 * \brief The Name of the config
	 */
	char file_name[BRIDGE_NAME_MAX];
} cli_service_cfg_t;


typedef struct cli_service_dvlan_s{
	/**
	 * \brief The Name of the bridge
	 */
	char bridge_name[BRIDGE_NAME_MAX];
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief The VLAN
	 */
	uint16_t vlan;
}cli_service_dvlan_t;


/**
 * \brief The CLI Base Structure
 */
typedef struct cli_service_s{
	/**
	 * \brief The service_code cli_service_code
	 */
	uint32_t service_code;
	/**
	 * \brief The union of all possible service requests
	 */
	union{
		cli_service_log_level_t log_level;
		cli_service_type_t      type_add;
		cli_service_ifmac_t     ifmac_add;
		cli_service_addressv4_t addressv4_add;
		cli_service_extvip_t    extvip_add;
		cli_service_overlayip_t overlayvip_add;
		cli_service_fwdrule_t   fwdrule_add;
		cli_service_location_t  location_add;
		cli_register_location_t location_register;
        cli_dps_server_t        dps_server;
        cli_dps_lookup_t        dps_lookup;   
        cli_service_mtu_t       svc_mtu;
        cli_service_dvlan_t     dvlan;
        cli_service_cfg_t       cfg_save;
        cli_dove_net_ipv4_t     dove_net_ipv4;
        cli_mgmt_ipv4_t         mgmt_ipv4;
        cli_peer_ipv4_t         peer_ipv4;
        cli_dmc_ipv4_t          dmc_ipv4;
        cli_vnid_subnet_t       vnid_subnet;
        cli_ext_mcast_vnid_t    ext_mcast_vnid;
	};
}cli_service_t;

/*
 ******************************************************************************
 * cli_service_callback                                                   *//**
 *
 * \brief - The callback for CLI_SERVICE
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_service_callback(void *cli);

/*
 ******************************************************************************
 * cli_service_init                                                       *//**
 *
 * \brief - Initializes the DPS Server CLI_SERVICE related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_service_init(void);

/** @} */
/** @} */

#endif // _CLI_SERVICE_
