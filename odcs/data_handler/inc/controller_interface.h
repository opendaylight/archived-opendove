/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   controller_interface.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */


#include "include.h"

/**
 * \ingroup DPSControllerInterface
 * @{
 */

#ifndef _PYTHON_DPS_CONTROLLER_H_
#define _PYTHON_DPS_CONTROLLER_H_

/**
 * \brief The Command Code for DPS DATA OBJECTS Operations related to the
 *        Controller and DPS Interactions
 *
 * \remarks: Only expand the codes in increasing order. DO NOT insert a new
 *           code in between.
 *           DPS_CONTROLLER_OP_MAX MUST BE THE FINAL ELEMENT
 */

//                              Command-Code                  Value
//                             ------------                   -----
#define DPS_CONTROLLER_DATA_OP_CODES \
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_ADD,             0)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_DELETE,          1)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DVG_ADD,                2)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DVG_DELETE,             3)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_SHOW,            4)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_POLICY_ADD,             5)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_POLICY_DELETE,          6)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_VALIDATE,        7)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_GETALLIDS,       8)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DVG_VALIDATE,           9)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DVG_GETALLIDS,         10)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD,  11)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_EXTERNAL_GATEWAY_DEL,  12)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_EXTERNAL_GATEWAY_CLR,  13)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_EXTERNAL_GATEWAY_VALIDATE,   14)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_EXTERNAL_GATEWAY_GETALLIDS,  15)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_POLICY_GET,            16)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_POLICY_GETALLIDS,      17)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_LOCATION_SET,          18)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_LOCATION_GET,          19)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_LOAD_BALANCING_GET,    20)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_GENERAL_STATISTICS_GET,21)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_ADD,         22)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_DELETE,      23)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_GET,         24)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_LIST,        25)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_FLUSH,       26)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_VNID_SHOW,             27)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_QUERY_VNID,            28)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_IP_SUBNET_GETALLIDS,   29)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_GLOBAL_SHOW,    30)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_SERVICE_ROLE,          31)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_MULTICAST_SHOW,        32)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_ADDRESS_RESOLUTION_SHOW, 33)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_GET_READY, 34)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_START,     35)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_ACTIVATE,       36)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_DEACTIVATE,     37)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_UPDATE,         38)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_LOCATION_DELETE,       39)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_DELETE_ALL_LOCAL,    40)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DOMAIN_RECOVERY_START, 41)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DVG_ADD_QUERY,         42)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_DCS_CLIENTS_SHOW,      43)\
	DPS_CONTROLLER_DATA_OP_CODE_AT(DPS_CONTROLLER_OP_MAX,                44)

#define DPS_CONTROLLER_DATA_OP_CODE_AT(_code, _val) _code = _val,
typedef enum {
	DPS_CONTROLLER_DATA_OP_CODES
}dps_controller_data_op_enum_t;
#undef DPS_CONTROLLER_DATA_OP_CODE

typedef enum {
	IP_SUBNET_ASSOCIATED_TYPE_DOMAIN = 1,
	IP_SUBNET_ASSOCIATED_TYPE_VNID
}dps_object_ip_subnet_associated_type;

/**
 * \brief The structure for adding a domain
 */
typedef struct dps_object_domain_add_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/**
	 * \brief The Replication Factor
	 */
	uint32_t replication_factor;
} dps_object_domain_add_t;

/**
 * \brief The structure for deleting a domain
 */
typedef struct dps_object_domain_del_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
} dps_object_domain_del_t;

/**
 * \brief The structure for adding a DVG
 */
typedef struct dps_object_dvg_add_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The DVG ID
	 */
	uint32_t dvg_id;
} dps_object_dvg_add_t;

/**
 * \brief The structure for deleting a DVG
 */
typedef struct dps_object_dvg_del_s{
	/*
	 * \brief The DVG ID
	 */
	uint32_t dvg_id;
} dps_object_dvg_del_t;

/**
 * \brief The structure for validate a DVG ID
 */
typedef struct dps_object_dvg_validate_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The DVG ID
	 */
	uint32_t dvg_id;
} dps_object_dvg_validate_t;

/**
 * \brief The structure for get all DVG IDs in specified domain
 */
typedef struct dps_object_dvg_getallids_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
} dps_object_dvg_getallids_t;

/**
 * \brief Enum of Policy Connectivity
 */
typedef enum {
	/**
	 * \brief Drop = 0
	 */
	DPS_CONNECTIVITY_DROP = 0,
	/**
	 * \brief Allow = 1
	 */
	DPS_CONNECTIVITY_ALLOW = 1,
} dps_policy_connectivity_enums;

/**
 * \brief Enum of Policy Type
 */
typedef enum {
	/**
	 * \brief Connectivity only = 1
	 */
	DPS_POLICY_TYPE_CONN = 1,
} dps_policy_type_enums;
/**
 *\brief The structure for policy action:
 *\brief DO NOT CHANGE THIS STRUCTURE without taking into account the
 *       following PYTHON files:
 *       1. cli/python/dps_cli/data_objects.py (policy_add)
 *       2. data_handler/python/dps_objects/Policy.py
 */
typedef struct dps_object_policy_action_s{
	/*
	 * \brief The connectivity or source routing
	 */
	uint8_t ver;
	uint8_t pad;
	union
	{
		/**
		 * \brief Connectivity: dps_policy_connectivity_enums
		 *                      Drop = 0
		 *                      Forward = 1
		 */
		uint16_t connectivity;
		struct
		{
			uint16_t address_count;
			uint8_t ip_list[0];
		} source_routing;
	};
} dps_object_policy_action_t;

/**
 * \brief Whether the Policy is for Multicast or Unicast.
 */
typedef enum {
	DPS_POLICY_TRAFFIC_UNICAST  = 0,
	DPS_POLICY_TRAFFIC_MULTICAST = 1,
} dps_object_policy_traffic;

/**
 * \brief The structure for adding a Policy
 */
typedef struct dps_object_policy_add_s{
	/*
	 * \brief The Policy Traffic Type (Unicast or Multicast)
	 *        Should be one of enum dps_object_policy_traffic
	 */
	uint32_t traffic_type;
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The Policy Type. Should be of type dps_policy_type_enums
	 */
	uint32_t type;
	/*
	 * \brief The Source DVG ID
	 */
	uint32_t src_dvg_id;
	/*
	 * \brief The Destination DVG ID
	 */
	uint32_t dst_dvg_id;
	/*
	 * \brief Time-To-Live
	 */
	uint32_t ttl;
	/*
	 * \brief Version
	 */
	uint32_t version;
	/*
	 * \brief action:
	 */
	dps_object_policy_action_t action;
} dps_object_policy_add_t;

/**
 * \brief The structure for deleting a Policy
 */
typedef struct dps_object_policy_del_s{
	/*
	 * \brief The Policy Traffic Type (Unicast or Multicast)
	 *        Should be one of enum dps_object_policy_traffic
	 */
	uint32_t traffic_type;
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/*
	 * \brief The Source DVG ID
	 */
	uint32_t src_dvg_id;
	/*
	 * \brief The Destination DVG ID
	 */
	uint32_t dst_dvg_id;
} dps_object_policy_del_t;

/**
 * \brief The structure for adding a domain
 */
typedef struct dps_object_domain_show_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/**
	 * \brief Whether to show the details
	 *        0 - Don't show details
	 *        1 - Do show details
	 */
	uint32_t fDetails;
} dps_object_domain_show_t;

/**
 * \brief The structure for validate a domain id
 */
typedef struct dps_object_domain_validate_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
} dps_object_domain_validate_t;

/**
 * \brief The Structure for adding a Gateway to a Domain
 */
typedef struct dps_object_gateway_s{
	/**
	 * \brief The VNID
	 */
	uint32_t vnid;
	/*
	 * \brief The IP type AF_INET or AF_INET6 of gateway
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
}dps_object_gateway_t;

/**
 * \brief The structure for adding a IP subnet
 */
typedef struct dps_object_ip_subnet_add_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
	/**
	 * \brief The Mask Value or Prefix length (In network byte order)
	 */
	union {
		uint32_t mask;
		uint32_t prefix_len;
	};
	/**
	 * \brief The Mode Dedicated(0) or Shared(1)
	 */
	uint32_t mode;
	/**
	 * \brief The Gateway for the subnet (In network byte order)
	 */
	union {
		uint32_t gateway_v4;
		char gateway_v6[16];
	};
} dps_object_ip_subnet_add_t;

/**
 * \brief The structure for deleting a IP subnet
 */
typedef struct dps_object_ip_subnet_delete_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
	/**
	 * \brief The IP Address Value (In network byte order)
	 */
	union {
		uint32_t IPv4;
		char IPv6[16];
	};
	/**
	 * \brief The Mask Value or Prefix length (In network byte order)
	 */
	union {
		uint32_t mask;
		uint32_t prefix_len;
	};
} dps_object_ip_subnet_delete_t;

/**
 * \brief The structure for getting a IP subnet
 */
typedef dps_object_ip_subnet_add_t dps_object_ip_subnet_get_t;

/**
 * \brief The structure for listing a IP subnet list
 */
typedef struct dps_object_ip_subnet_list_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
	/*
	 * \brief The IP type AF_INET or AF_INET6
	 */
	uint32_t IP_type;
}dps_object_ip_subnet_list_t;

/**
 * \brief The structure for showing a VNID
 */
typedef struct dps_object_vnid_show_s{
	/*
	 * \brief The VNID
	 */
	uint32_t vnid;
	/**
	 * \brief Whether to show the details
	 *        0 - Don't show details
	 *        1 - Do show details
	 */
	uint32_t fDetails;
} dps_object_vnid_show_t;

/**
 * \brief The structure for Query a VNID
 */
typedef struct dps_object_query_vnid_s{
	/*
	 * \brief The VNID
	 */
	uint32_t vnid;
} dps_object_query_vnid_t;

/**
 * \brief The structure for flushing a IP subnet list
 */
typedef dps_object_ip_subnet_list_t dps_object_ip_subnet_flush_t;

/**
 * \brief The structure for Service Role Start/Stop
 */
typedef struct dps_object_service_role_s{
	/*
	 * \brief The Action
	 */
	uint32_t action;
} dps_object_service_role_t;

/**
 * \brief The structure for Mass Transfer Start
 */
typedef struct dps_object_mass_transfer_start_s{
	/*
	 * \brief The Domain ID
	 */
	uint32_t domain_id;
	/**
	 * \brief Remote DCS Server Address
	 * \note IPv4 and IPv6 Address MUST be in network byte order
	 */
	ip_addr_t dps_server;
} dps_object_mass_transfer_start_t;

/**
 * \brief The structure for showing Multicast
 */
typedef struct dps_object_multicast_show_s{
	/*
	 * \brief The Associated Type (Domain or VNID)
	 */
	uint32_t associated_type;
	/*
	 * \brief The Associated ID
	 */
	uint32_t associated_id;
}dps_object_multicast_show_t;

/**
 * \brief The common API structure
 */

typedef struct dps_controller_data_op_s{
	/**
	 * \brief The operation type
	 */
	dps_controller_data_op_enum_t type;
	union
	{
		/**
		 * \brief Adding a Domain
		 */
		dps_object_domain_add_t domain_add;
		/**
		 * \brief Deleting a Domain
		 */
		dps_object_domain_del_t domain_delete;
		/**
		 * \brief Getting Ready For Mass Transfer
		 */
		dps_object_domain_add_t domain_mass_transfer_get_ready;
		/**
		 * \brief Start the Mass Transfer
		 */
		dps_object_mass_transfer_start_t domain_mass_transfer_start;
		/**
		 * \brief Recover a Domain
		 */
		dps_object_domain_add_t domain_recover;
		/**
		 * \brief Add a DVG to a Domain
		 */
		dps_object_dvg_add_t dvg_add;
		/**
		 * \brief Delete a DVG from a domain.
		 */
		dps_object_dvg_del_t dvg_delete;
		/**
		 * \brief Validate a DVG
		 */
		dps_object_dvg_validate_t dvg_validate;
		/**
		 * \brief Get all DVG ids in a Domain
		 */
		dps_object_dvg_getallids_t dvg_getallids;
		/**
		 * \brief Add a policy
		 */
		dps_object_policy_add_t policy_add;
		/**
		 * \brief Delete a policy
		 */
		dps_object_policy_del_t policy_delete;
		/**
		 * \brief Show the Domain details
		 */
		dps_object_domain_show_t domain_show;
		/**
		 * \brief Validate a Domain
		 */
		dps_object_domain_validate_t domain_validate;
		/**
		 * \brief Update Gateways
		 */
		dps_object_gateway_t gateway_update;
		/**
		 * \brief Clear Gateway
		 */
		dps_object_dvg_del_t gateway_clear;
		/**
		 * \brief Add Controller Location
		 * \note IPv4 and IPv6 Address MUST be in network byte order
		 */
		ip_addr_t controller_location;
		/**
		 * \brief Get load balancing for a specified domain
		 */
		dps_object_load_balancing_t load_balancing;
		/**
		 * \brief Get general statistics for a specified domain
		 */
		dps_object_general_statistics_t general_statistics;
		/**
		 * \brief Adding a IP subnet
		 */
		dps_object_ip_subnet_add_t ip_subnet_add;
		/**
		 * \brief Deleting a IP subnet
		 */
		dps_object_ip_subnet_delete_t ip_subnet_delete;
		/**
		 * \brief Looking for a IP subnet
		 */
		dps_object_ip_subnet_get_t ip_subnet_get;
		/**
		 * \brief Listing a IP subnet list
		 */
		dps_object_ip_subnet_list_t ip_subnet_list;
		/**
		 * \brief Flushing a IP subnet list
		 */
		dps_object_ip_subnet_flush_t ip_subnet_flush;
		/**
		 * \brief Show the Domain details
		 */
		dps_object_vnid_show_t vnid_show;
		/**
		 * \brief Query VNID details
		 */
		dps_object_query_vnid_t query_vnid;
		/**
		 * \brief Start/Stop service role
		 */
		dps_object_service_role_t service_role;
		/**
		 * \brief Show Multicast
		 */
		dps_object_multicast_show_t multicast_show;
	};
	/**
	 * \brief The return object
	 */
	void *return_val;
} dps_controller_data_op_t;

/*
 ******************************************************************************
 * python_init_controller_interface --                                *//**
 *
 * \brief This routine initializes the DPS Controller Interface to PYTHON
 *        OBHECTS
 *
 * \param pythonpath - Pointer to the Python Path
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status python_init_controller_interface(char *pythonpath);

/*
 ******************************************************************************
 * dcs_controller_interface_stop --                                       *//**
 *
 * \brief This routine stops the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dcs_controller_interface_stop();

/*
 ******************************************************************************
 * dcs_controller_interface_start --                                       *//**
 *
 * \brief This routine starts the DPS Controller Interface
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No resources
 *
 *****************************************************************************/
dove_status dcs_controller_interface_start();

/*
 ******************************************************************************
 * dps_controller_data_msg --                                             *//**
 *
 * \brief This routine initializes handles DPS Controller Data Object Messages
 *
 * \param data - Pointer to DPS Client Server Protocol Message
 *
 * \return dove_status
 *
 *****************************************************************************/

dove_status dps_controller_data_msg(dps_controller_data_op_t *data);

/** @} */

#endif // _PYTHON_DPS_CONTROLLER_H_
