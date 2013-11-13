/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef _DPS_REST_REQ_HANDLER_H_
#define _DPS_REST_REQ_HANDLER_H_
void dgw_req_handler_dpses(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_dps(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_services(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_nics(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_nic(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_ipv4s(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_ipv4(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_svc_ext_vips(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_svc_ext_vip(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_internal_vips(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_internal_vip(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_rules(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_rule(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_vlanmaps(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_vlanmap(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_subnets(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_subnet(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_service_dmc(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_sessions_ext(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_sessions_int(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_fwddyn_sessions(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_ovlport(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_mcasts_external(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_serivce_mcast_external(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_vnid_stats(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_stats(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_networks(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_network(struct evhttp_request *req, void *arg, int argc, char **argv);
void dgw_req_handler_resetstats(struct evhttp_request *req, void *arg, int argc, char **argv);
#endif /* _DPS_REST_REQ_HANDLER_H_ */
