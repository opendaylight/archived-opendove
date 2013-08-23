/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client
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
*  $Log: rest_client_cluster_leader.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#ifndef _DPS_REST_CLIENT_CLUSTER_LEADER_H
#define _DPS_REST_CLIENT_CLUSTER_LEADER_H

#include "include.h"

extern int statistics_need_send_to_leader;

#define DPS_CLUSTER_URI_LEN 256

#define DPS_CLUSTER_STATISTICS_URI "/api/dove/dps/stats"
#define DPS_CLUSTER_HEARTBEAT_URI "/api/dove/dps/heartbeat"
#define DPS_CLUSTER_HEARTBEAT_REQUEST_URI "/api/dove/dps/heartbeatreq"
#define DPS_CLUSTER_NODE_STATUS_URI "/api/dove/dps/nodestatus"

/*2012-11-01 Now the domain id is not used in dps stats URI generation */
#define CLUSTER_LEADER_DPS_STATISTICS_URI_GEN(_buf) \
	snprintf((_buf), DPS_CLUSTER_URI_LEN, DPS_CLUSTER_STATISTICS_URI)

/*
 ******************************************************************************
 * dps_rest_client_json_send_to_dps_node --                               *//**
 *
 * \brief This routine sends heartbeat message to the remote DPS Node
 *
 * \param js_res: The JSON Body
 * \param uri: The URI of the request
 * \param cmd_type: GET/POST/PUT/DELETE etc
 * \param dps_node_ip: The IP Address of the DPS Node
 *
 * \return PyObject
 *
 *****************************************************************************/
void dps_rest_client_json_send_to_dps_node(json_t *js_res,
                                           char *uri,
                                           enum evhttp_cmd_type cmd_type,
                                           ip_addr_t *dps_node_ip);
#endif // _DPS_REST_CLIENT_CLUSTER_LEADER_H

