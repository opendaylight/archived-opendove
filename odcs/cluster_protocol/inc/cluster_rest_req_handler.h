/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest handler functions for Clustering functionality
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
*  $Log: cluster_rest_req_handler.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _DPS_CLUSTER_REST_REQ_HANDLER_H
#define _DPS_CLUSTER_REST_REQ_HANDLER_H

#define DPS_CLUSTER_TRANSFER_DOMAIN_URI "/api/dove/dps/domains/*/transfer"
#define DPS_URI_LEN 128

#define DPS_CLUSTER_TRANSFER_DOMAIN_URI_GEN(_buf, _id) snprintf((_buf), DPS_URI_LEN, \
                                                     "/api/dove/dps/domains/%u/transfer", (unsigned int)(_id))

void dps_req_handler_transfer_domain(struct evhttp_request *req, void *arg,
                                 int argc, char **argv);

void dps_req_handler_get_ready(struct evhttp_request *req, void *arg,
                                 int argc, char **argv);

void dps_req_handler_domain_activate(struct evhttp_request *req, void *arg,
                                 int argc, char **argv);

void dps_req_handler_domain_recover(struct evhttp_request *req, void *arg,
                                    int argc, char **argv);

void dps_req_handler_domain_deactivate(struct evhttp_request *req, void *arg,
                                     int argc, char **argv);

void dps_req_handler_domain_vnid_listing(struct evhttp_request *req, void *arg,
                                         int argc, char **argv);

void dps_req_handler_domain_bulk_policy(struct evhttp_request *req, void *arg,
                                        int argc, char **argv);

void dps_req_handler_domain_bulk_ip4subnets(struct evhttp_request *req, void *arg,
                                            int argc, char **argv);

#endif // _DPS_CLUSTER_REST_REQ_HANDLER_H
