/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Handler
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
*  $Log: rest_req_handler.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _DPS_REST_REQ_HANDLER_H_
#define _DPS_REST_REQ_HANDLER_H_
void dps_req_handler_domains(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_domain(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_dvgs(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_dvg(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_policies(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_policy(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_gateways(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_gateway(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_statistics_load_balancing(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_statistics_general_statistics(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_ipsubnet(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_local_domain_mapping(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_service_role(struct evhttp_request *req, void *arg, int argc, char **argv);

void dps_req_handler_node_statistics(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_node_heartbeat(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_node_heartbeat_request(struct evhttp_request *req, void *arg, int argc, char **argv);
void dps_req_handler_query_cluster_nodes(struct evhttp_request *req,void *arg, int argc, char **argv);
void dps_req_handler_dcslist(struct evhttp_request *req, void *arg, int argc,
                             char **argv);
void dps_req_handler_set_dmc_location(struct evhttp_request *req, void *arg, int argc,
                             char **argv);

void dps_req_handler_vnid_endpoints(struct evhttp_request *req,
                                    void *arg, int argc, char **argv);
void dps_req_handler_vnid_tunnel_endpoints(struct evhttp_request *req,
                                           void *arg, int argc, char **argv);
void dps_req_handler_vnid_get_domain_mapping(struct evhttp_request *req,
                                             void *arg, int argc, char **argv);
void dps_req_handler_vnid_get_allow_policies(struct evhttp_request *req,
                                             void *arg, int argc, char **argv);
void dps_req_handler_vnid_get_subnets(struct evhttp_request *req,
                                      void *arg, int argc, char **argv);
void dps_req_handler_cluster_display(struct evhttp_request *req,
                                     void *arg, int argc, char **argv);

/*
 ******************************************************************************
 * dps_req_handler_node_status --                                         *//**
 *
 * \brief This routine handles when a node status list is received from the
 *        another node typically the leader
 *
 * \param req The evhttp_request
 * \param arg
 * \param argc The number of arguments
 * \param argv Array of arguments
 *
 * \return void
 *
 *****************************************************************************/
void dps_req_handler_node_status(struct evhttp_request *req,
                                 void *arg, int argc, char **argv);

#endif /* _DPS_REST_REQ_HANDLER_H_ */
