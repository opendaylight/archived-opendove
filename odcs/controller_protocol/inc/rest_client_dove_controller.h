/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  DPS Rest Client to Dove Controller
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
*  $Log: rest_client_dove_controller.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/

#ifndef _DPS_REST_CLIENT_DOVE_CONTROLLER_H
#define _DPS_REST_CLIENT_DOVE_CONTROLLER_H

#include "include.h"

#define HTTPD_DEFAULT_PORT  80

#define DPS_DOVE_CONTROLLER_ENDPOINT_CONFLICT_URI "/api/dove/dps/domains/*/endpoint-conflict"
#define DPS_DOVE_CONTROLLER_DPS_STATISTICS_URI "/api/dove/dps/stats"
#define DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI "/api/dove/dps/heartbeat"

/* DPS Query Dove Controller for Cluster info URI */
#define DPS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO_URI "/api/dove/dps/cluster"

/* DPS Query Dove Controller for VNID info URI (Generate)*/
#define DOVE_CONTROLLER_VNID_INFO_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/vnid/%u/info", (unsigned int)(_id))

/* DPS Query Dove Controller for POLICY info URI (Generate)*/
#define DOVE_CONTROLLER_POLICY_INFO_URI_GEN(_buf, _domain, _src_dvg, _dst_dvg, _traffic_type) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%u/policies/%u-%u-%u", \
	         (unsigned int)(_domain), (unsigned int)(_src_dvg), (unsigned int)(_dst_dvg), (unsigned int)(_traffic_type))

typedef struct dps_controller_query_policy_args_s
{
	uint32_t res_code;
	uint32_t domain;
	uint32_t src_dvg;
	uint32_t dst_dvg;
	uint32_t traffic_type;
	uint32_t ttl;
	uint32_t type;
	uint32_t action;
}dps_controller_query_policy_args_t;

/* DPS Query Dove Controller for VNID info URI
 * Align with DOVE_CONTROLLER_VNID_INFO_URI_GEN*/
#define DPS_DOVE_CONTROLLER_QUERY_DPS_VNID_INFO_URI "/api/dove/dps/vnid/*/info"

#define URI_IS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO(uri) \
	((strcmp((const char *)(uri),(const char *)DPS_DOVE_CONTROLLER_QUERY_DPS_CLUSTER_INFO_URI)) == 0)

#define URI_IS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI(uri) \
	((strcmp((const char *)(uri), (const char *)DPS_DOVE_CONTROLLER_DPS_HEARTBEAT_URI)) == 0)

#define DOVE_CONTROLLER_URI_LEN 256
#define DOVE_CONTROLLER_ENDPOINT_CONFLICT_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%u/endpoint-conflict", (unsigned int)(_id))

/*2012-11-01 Now the domain id is not used in dps stats URI generation */
#define DOVE_CONTROLLER_DPS_STATISTICS_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/stats")

#define DOVE_CONTROLLER_VNID_SUBNET_URI_GEN(_buf, _id) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/vns/%u/ipv4-subnets", (unsigned int)(_id))
#define DOVE_CONTROLLER_VNID_SUBNET_URI "/api/dove/dps/vns/%u/ipv4-subnets"

#define DOVE_CONTROLLER_DOMAIN_ENDPOINT_REG_URI_GEN(_buf, _dom, _vnid) \
	snprintf((_buf), DOVE_CONTROLLER_URI_LEN, \
	         "/api/dove/dps/domains/%u/vnid/%u/endpoint-register", (unsigned int)(_dom), (unsigned int)(_vnid))

#define DOVE_CONTROLLER_REST_HTTPD_PORT    4325

/*
 ******************************************************************************
 * dove_rest_request_and_asyncprocess --                *//**
 *
 * \brief This routine is used to send HTTP request to a server and process the
 * response asynchronously. The callback for the response is register in the
 * struct evhttp_request data structure pointered by the member request.
 * The callback
 * - void (*cb)(struct evhttp_request *request, void *arg))-
 * will be invoked when corresponding response is received or interanl error
 * occurs.The first parameter passed to callback is the a pointer to a
 * evhttp_request data structure, and is always the same one in the input
 * parameter of this routine. In some internal error cases, NULL might be
 * passed to the callback as the first parameter.
 * The REST client infrastructure ensures first-in-first-serviced for the HTTP
 * request to the same destination. (determined by IP address and TCP port
 * number)
 *
 * \param[in] rinfo A pointer to a dove_rest_request_info_t data structure.
 * The data structure should be allocated on heap. The ownership of the data
 * structure will passed to the REST client infrastructure, which is reponsible
 * to free it when completes. The caller should not dereference it after
 * invoking this routine. Member of data structure dove_rest_request_info_t:
 *	char *address; Pointer to IP address string of the server to which the
 *   request is sent, should be allocated on heap, the infrastructure will call
 *   free() to release it when completes the process.
 *	char *uri; The URI which the HTTP request is accessing, should be allocated
 *   on heap, the infrastructure will call free() to release it when completes
 *   the process.
 *	unsigned short port; The tcp port of the server to which the request is
 *   sent.
 *	enum evhttp_cmd_type type; HTTP method.
 *	struct evhttp_request *request; A pointer to a evhttp_request data
 *   structure. The data structure should be allocated on heap. The
 *   infrastructure will release it when completes the process.
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dps_rest_client_dove_controller_send_asyncprocess (char *address,
	char *uri, unsigned short port, enum evhttp_cmd_type type,
	struct evhttp_request *request);


enum evhttp_send_mode {
	EVHTTP_SEND_SYNCHRONOUS    = 0,
	EVHTTP_SEND_ASYNCHRONOUS    = 1
};


int dps_rest_client_dove_controller_send(char *address,
	char *uri, unsigned short port, enum evhttp_cmd_type type, 
	struct evhttp_request *request, enum evhttp_send_mode mode);


int dps_rest_client_dove_controller_fill_evhttp(struct evhttp_request *req, json_t *js_res);

struct evhttp_request *dps_rest_client_dove_controller_request_new(void);

struct evhttp_request *dps_cluster_request_new(void);

#if 0
json_t *dps_form_json_endpoint_update_and_conflict_json(
	uint32_t domain_id,  uint32_t dvg, uint32_t version,
	char *vMac1, char *vMac2, char *pIP_address1, char *pIP_address2,
	char *vIP_addr, uint32_t family);
#endif

json_t *dps_form_json_endpoint_update_and_conflict_json(
	uint32_t domain_id, uint32_t dvg1, uint32_t dvg2, uint32_t version,
	char *vMac1, char *vMac2, char *pIP_address1, char *pIP_address2,
	char *vIP_addr, uint32_t family);



/* Send the Restful Get Request to Dove Controller for all DPS cluster nodes info */
void dps_rest_client_query_dove_controller_cluster_info();

void dps_rest_client_query_dove_controller_vnid_info(uint32_t vnid);


/**********************************/
/* Use the simulated Dove Controller httpd server */

//extern int SIMULATE_DOVE_CONTROLLER_HTTPD_flag;

//dove_status dps_server_rest_init2(short rest_port);
void dove_controller_rest_httpd_server_log_endpoint_conflict(struct evhttp_request *req, void *arg, int argc, char **argv);
void dove_controller_rest_httpd_server_log_dps_statistics(struct evhttp_request *req, void *arg, int argc, char **argv);
void dove_controller_rest_httpd_server_process_dps_heartbeat(struct evhttp_request *req, void *arg, int argc, char **argv);
void dove_controller_rest_httpd_server_process_dps_query_cluster_info(struct evhttp_request *req, void *arg, int argc, char **argv);
void dove_controller_rest_httpd_server_process_dps_query_vnid_info(struct evhttp_request *req, void *arg, int argc, char **argv);
void dove_controller_rest_httpd_server_process_dps_appliance_registration(
	struct evhttp_request *req, void *arg, int argc, char **argv);


/***************************************/

/*
 ******************************************************************************
 * dps_rest_client_json_send_to_dove_controller --                        *//**
 *
 * \brief A more general sending function for PUSHING something to the DMC.
 *        It encapsulates the json string into a http request and sends it to
 *        the Dove Controller based on the uri and DC's address info
 *
 * \param[in] js_res The JSON Body as a string. NOTE that this routine will
 *                   free the JSON string, so the calling MUST NOT touch
 *                   the JSON Body after this call is made
 * \param[in] uri The URI in String Format
 * \param[in] cmd_type Command Type (POST/PUT/GET etc).
 *
 * \note The calling routine MUST NOT touch the js_res parameter after this
 *       routine is invoked
 *
 * \retval None
 *
 ******************************************************************************/
void dps_rest_client_json_send_to_dove_controller(json_t *js_res,
                                                  char *uri,
                                                  enum evhttp_cmd_type cmd_type);

#endif
