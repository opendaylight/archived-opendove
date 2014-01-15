/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Api 
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
*  $Log: rest_api.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _CONTROLLER_REST_API_H_
#define _CONTROLLER_REST_API_H_
#define DPS_DOMAINS_URI "/api/dove/dps/domains"
#define DPS_DOMAIN_URI "/controller/sb/v2/opendove/odmc/domains/bynumber/*"
#define DPS_DVGS_URI "/api/dove/dps/domains/bynumber/*/dvgs"
//#define DPS_DVG_URI "/api/dove/dps/domains/*/dvgs/*"
#define DPS_DVG_URI "/controller/sb/v2/opendove/odmc/domains/bynumber/*/networks/*"
#define DPS_POLICIES_URI "/api/dove/dps/domains/bynumber/*/policies"
#define DPS_POLICY_URI "/api/dove/dps/domains/bynumber/*/policies/*"
#define DPS_EXTERNAL_GATEWAYS_URI "/api/dove/dps/vns/*/ipv4-external-gateways"
#define DPS_EXTERNAL_GATEWAY_URI "/api/dove/dps/vns/*/ipv4-external-gateways/*"
#define DPS_STATISTICS_LOAD_BALANCING_URI "/api/dove/dps/domains/bynumber/*/load-balancing"
#define DPS_STATISTICS_GENERAL_STATISTICS_URI "/api/dove/dps/domains/bynumber/*/general-statistics"

#define DPS_DOMAIN_IPV4SUBNETS_URI "/api/dove/dps/domains/bynumber/*/ipv4-subnets"
#define DPS_DOMAIN_IPV4SUBNET_URI "/api/dove/dps/domains/bynumber/*/ipv4-subnets/*"
#define DPS_DVG_IPV4SUBNETS_URI "/api/dove/dps/vns/*/ipv4-subnets"
#define DPS_DVG_IPV4SUBNET_URI "/api/dove/dps/vns/*/ipv4-subnets/*"

#define DPS_SERVICE_ROLE_URI "/api/dove/dps/service/role"

#define DPS_CLUSTER_LEADER_URI 	"/api/dove/dps/cluster/leader"
#define DPS_APPLIANCE_REGISTRATION_URI 	"/controller/sb/v2/opendove/odmc/odcs"
#define DPS_DOMAIN_TO_NODE_MAPPING_URI "/controller/sb/v2/opendove/odmc/odcs/domains/bynumber/*/dcslist"
#define DPS_CONTROLLER_LOCATION_UPDATE_URI "/api/dove/dps/dmc"
#define DOVE_CONTROLLER_DSA_SYSLOG_URI "/api/dove/sys/event-logs"

/* ODCS URI definitions */
#define ODCS_SERVICE_ROLE_ASSIGNMENT_URI "/controller/sb/v2/opendove/odcs/role"
#define ODCS_DVG_IPV4SUBNET_URI "/controller/sb/v2/opendove/odmc/networks/*/subnets/*"
#define ODCS_POLICY_URI "/controller/sb/v2/opendove/odmc/domains/bynumber/*/policy/*"

/*Trevor: 2013-01-15 DPS Debug URI for DMC  */

#define DPS_DEBUG_VNID_ENDPOINTS_URI    "/api/dove/dps/vns/*/endpoints"
#define DPS_DEBUG_VNID_TUNNEL_ENDPOINTS_URI     "/api/dove/dps/vns/*/tunnel-endpoints"
#define DPS_DEBUG_VNID_DOMAIN_MAPPING    "/api/dove/dps/vnid_domain_mapping"
#define DPS_DEBUG_VNID_ALLOW_POLICIES    "/api/dove/dps/vns/*/policy_listing"
#define DPS_DEBUG_VNID_SUBNETS           "/api/dove/dps/vns/*/subnet_listing"
#define DPS_DEBUG_VNID_DPS_CLIENTS_URI   "/api/dove/dps/vns/*/dps-clients"
#define DPS_DEBUG_VNID_MULTICAST_URI   "/api/dove/dps/vns/*/multicasts"
#define DPS_DEBUG_CLUSTER_DISPLAY      "/api/dove/dps/cluster_display"

#define DPS_URI_LEN 128
#define DPS_DOMAIN_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u", (unsigned int)(_id))
#define DPS_DVGS_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/dvgs", (unsigned int)(_id))
#define DPS_DVG_URI_GEN(_buf, _id, _dvg_id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/dvgs/%u", (unsigned int)(_id), (unsigned int)(_dvg_id))
#define DPS_POLICIES_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/policies", (unsigned int)(_id))
#define DPS_POLICY_URI_GEN(_buf, _id, _src_dvg_id, _dst_dvg_id, _traffic_type) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/policies/%u-%u-%u", (unsigned int)(_id), (unsigned int)(_src_dvg_id), (unsigned int)(_dst_dvg_id), (unsigned int)(_traffic_type))
#define DPS_EXTERNAL_GATEWAY_KEY "ipv4-external-gateways"
#define DPS_EXTERNAL_GATEWAYS_URI_GEN(_buf, _id) snprintf((_buf),DPS_URI_LEN, "/api/dove/dps/vns/%u/ipv4-external-gateways", (unsigned int)(_id))
#define DPS_EXTERNAL_GATEWAY_URI_GEN(_buf,_id,_external_gateway_id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/vns/%u/ipv4-external-gateways/%d", (unsigned int)(_id), (_external_gateway_id))
#define DPS_DOMAIN_IPV4SUBNETS_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/ipv4-subnets", (unsigned int)(_id))
#define DPS_DOMAIN_IPV4SUBNET_URI_GEN(_buf, _id, _ip_str, _mask_str) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/domains/bynumber/%u/ipv4-subnets/%s-%s", (unsigned int)(_id), (_ip_str), (_mask_str))
#define DPS_DVG_IPV4SUBNETS_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/vns/%u/ipv4-subnets", (unsigned int)(_id))
#define DPS_DVG_IPV4SUBNET_URI_GEN(_buf, _id, _ip_str, _mask_str) snprintf((_buf), DPS_URI_LEN, "/api/dove/dps/vns/%u/ipv4-subnets/%s-%s", (unsigned int)(_id), (_ip_str), (_mask_str))
#define DPS_REST_LISTEN_DEFAULT_IP "0.0.0.0"
dove_status dps_rest_api_create_domain(unsigned int domain_id, unsigned int replication_factor);
dove_status dps_rest_api_del_domain(unsigned int domain_id);
//dove_status dps_rest_api_update_domain(unsigned int domain_id, char *domain_name);
json_t *dps_rest_api_get_domain(unsigned int domain_id);
json_t *dps_rest_api_get_domain_all(void);
json_t *dps_rest_api_get_dvg_all(unsigned int domain_id);
json_t *dps_rest_api_get_dvg(unsigned int domain_id, unsigned int dvg_id);
dove_status dps_rest_api_create_dvg(unsigned int domain_id, unsigned int dvg_id,
                                    dps_controller_data_op_enum_t method);
dove_status dps_rest_api_del_dvg(unsigned int domain_id, unsigned int dvg_id);
dove_status dps_rest_api_create_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type, unsigned int type, unsigned int ttl, dps_object_policy_action_t *action);
json_t *dps_rest_api_get_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type);
dove_status dps_rest_api_del_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type);
dove_status dps_rest_api_update_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type, json_t *data);
json_t *dps_rest_api_get_policies_all(unsigned int domain_id);
dove_status dps_rest_api_create_policy_from_jdata(unsigned int domain_id, json_t *data, unsigned int *src_dvg, unsigned int *dst_dvg, unsigned int *traffic_type);
dove_status dps_rest_api_domain_validate(unsigned int domain_id);
json_t *dps_rest_api_get_gateway_all(unsigned int vn_id);
dove_status dps_rest_api_create_gateway(unsigned int vn_id, unsigned int gateway_id);
json_t *dps_rest_api_get_gateway(unsigned int vn_id, unsigned int gateway_id);
dove_status dps_rest_api_del_gateway_all(unsigned int vn_id);
dove_status dps_rest_api_del_gateway(unsigned int vn_id, unsigned int gateway_id);
json_t *dps_rest_api_get_statistics_load_balancing(unsigned int domain_id);
json_t *dps_rest_api_get_statistics_general_statistics(unsigned int domain_id);
dove_status dps_rest_api_create_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask, unsigned int mode, unsigned char *gateway);
dove_status dps_rest_api_del_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask);
json_t *dps_rest_api_get_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask);

extern unsigned int ip_id_to_v4_addr (char *str, unsigned int *addr);
extern unsigned int v4_addr_to_ip_id (char *str, unsigned int addr);

/*
 ******************************************************************************
 * dps_rest_remote_node_get --                                            *//**
 *
 * \brief This routine derives the location of node which sent
 *
 * \param req - The REST request struct evhttp_request
 * \param remote_node - Pointer to a location which will be filled up by
 *                      this routine.
 *
 * \retval 1 Success
 * \retval 0 or -1 Failure
 *
 *****************************************************************************/

int dps_rest_remote_node_get(struct evhttp_request *req,
                             ip_addr_t *remote_node);

/*
 ******************************************************************************
 * dps_rest_remote_node_touch --                                          *//**
 *
 * \brief This routine touches (in the cluster handler) the dps node which sent
 *        a REST request.
 *
 * \return void
 *
 *****************************************************************************/

void dps_rest_node_touch(struct evhttp_request *req);

#endif
