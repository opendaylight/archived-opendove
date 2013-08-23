/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client for Clustering functionality
**/
/*
{
* Copyright (c) 2010-2013 IBM Corporation
* All rights reserved.
*
* This program and the accompanying materials are made available under the
* terms of the Eclipse Public License v1.0 which accompanies this
* distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
*
*  HISTORY
*
*  $Log: cluster_rest_client.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _DPS_REST_CLIENT_H
#define _DPS_REST_CLIENT_H

#define DPS_REST_HTTPD_PORT    1888
#define DPS_CLUSTER_LOCAL_DOMAINS_URI "/api/dove/dps/local-domains"
#define DOVE_CLUSTER_POLICY_URI     "/api/dove/dps/domains/%d/policies"
#define DPS_NODE_GET_READY          "/api/dove/dps/get-ready"
#define DPS_NODE_DOMAIN_ACTIVATE    "/api/dove/dps/domain-activate"
#define DPS_NODE_DOMAIN_DEACTIVATE  "/api/dove/dps/domain-deactivate"
#define DPS_NODE_DOMAIN_RECOVER     "/api/dove/dps/domain-recover"
#define DPS_NODE_DOMAIN_VNID_LIST   "/api/dove/dps/domains/*/vnid-listing"
#define DOVE_CLUSTER_BULK_POLICY_URI "/api/dove/dps/domains/*/bulk_policy"
#define DOVE_CLUSTER_BULK_SUBNET4_URI "/api/dove/dps/domains/*/bulk_ipv4-subnets"

#define DOVE_CLUSTER_POLICY_INFO_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%d/policies", (unsigned int)(_id))

#define DOVE_CLUSTER_BULK_POLICY_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%d/bulk_policy", (unsigned int)(_id))

#define DOVE_CLUSTER_SUBNET4_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/vns/%d/ipv4-subnets", (unsigned int)(_id))

#define DOVE_CLUSTER_BULK_SUBNET4_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%d/bulk_ipv4-subnets", (unsigned int)(_id))

#define DOVE_CLUSTER_VNID_LISTING_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "api/dove/dps/domains/%d/vnid-listing", (unsigned int)(_id))

#define DOVE_CLUSTER_BULK_POLICY_URI_GEN(_buf, _id)	  \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%d/bulk_policy", (unsigned int)(_id))

#define DPS_SHARED_ADDR_SPACE 1
void dps_cluster_send_local_domains(ip_addr_t *nodes, uint32_t nodes_count,
                                    char *domains);

int dps_cluster_vnid_replication(char *dps_node, uint32_t crud,
                                 uint32_t domain_id, PyObject *pyList_vnid);

int dps_cluster_policy_replication(char *dps_node, uint32_t crud, uint32_t domain_id, uint32_t traffic_type,
                                   uint32_t type, uint32_t sdvg, uint32_t ddvg, uint32_t ttl, uint32_t action);

int dps_cluster_ipsubnet4_replication(char *dps_node, uint32_t crud, uint32_t vnid, char *ip, char *mask, 
                                      char *gw, uint32_t mode);

int dps_cluster_bulk_policy_replication(char *dps_node, uint32_t crud,
                                        uint32_t domain_id, PyObject *pyList_policy);

int dps_cluster_bulk_ip4subnet_replication(char *dps_node, uint32_t crud,
                                           uint32_t domain, PyObject *pyList_subnet);

/*
 ******************************************************************************
 * dps_leader_create_domain                                               *//**
 *
 * \brief - This routine allows the DCS Leader to create a domain organically
 *          and use it for internal purposes. This domain is not available to
 *          DOVE consumers
 *
 * \param domain_id The Domain ID
 * \param name The Domain Name
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status dps_leader_create_domain(int domain_id, char *name);

dove_status dps_leader_create_vn(int domain_id,int vnid,char *name);

dove_status dps_node_get_ready(ip_addr_t *node,uint32_t domain);

dove_status dps_node_domain_activate(ip_addr_t *node,
                                     uint32_t domain,
                                     uint32_t replication_factor);

dove_status dps_cluster_leader_initiate_domain_move(uint32_t domain,
                                                    ip_addr_t *src_node,
                                                    ip_addr_t *dst_node);

dove_status dps_node_domain_deactivate(ip_addr_t *node,uint32_t domain);

/*
 ******************************************************************************
 * dps_node_domain_recover --                                             *//**
 *
 * \brief This routine sends the Domain Recover Message to a specific DCS node
 *
 * \param node IP Address of the DCS Node
 * \param domain The Domain ID
 * \param replication_factor The Replication Factor of the Domain
 *
 * \retval DOVE_STATUS_OK Message was forwarded to a remote Node
 * \retval DOVE_STATUS_NO_MEMORY No Memory
 *
 *****************************************************************************/

dove_status dps_node_domain_recover(ip_addr_t *node,
                                    uint32_t domain,
                                    uint32_t replication_factor);

/*
 ******************************************************************************
 * dps_rest_domain_delete_send_to_dps_node --                             *//**
 *
 * \brief This routine sends a REST Domain Delete message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_domain_delete_send_to_dps_node(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_rest_vnid_add_send_to_dps_node --                                 *//**
 *
 * \brief This routine sends a REST VNID Add message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_vnid_add_send_to_dps_node(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * dps_rest_vnid_delete_send_to_dps_node --                               *//**
 *
 * \brief This routine sends a REST VNID Delete message to the a DPS Node
 *
 * \return void
 *
 *****************************************************************************/
PyObject *dps_rest_vnid_delete_send_to_dps_node(PyObject *self, PyObject *args);

#endif // _DPS_REST_CLIENT_H
