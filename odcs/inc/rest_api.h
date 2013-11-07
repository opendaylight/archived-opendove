/*
 *  Copyright (c) IBM, Inc.  2011 -
 *  All rights reserved
 *
 *  Header File:
 *      rest_api.h
 *
 *  Abstract:
 *      This header file defines the APIs that provided by the DPS REST Services
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _DPS_REST_API_
#define _DPS_REST_API_

/**
 * \ingroup DPSRESTServices
 * @{
 */

/**
 * \brief Has the role been assigned to the DCS
 */
extern int dcs_role_assigned;

extern int RESTHandlerLogLevel;

#define LARGE_REST_BUFFER_SIZE 2097152 // 2MB
/**
 * \brief The Buffer to store large incoming REST messages
 */
extern char *large_REST_buffer;

/**
 * \brief username and password to be used in the Authorization header of
 * 		  REST messages.
 */
#define AUTH_HEADER_USERNAME "admin"
#define AUTH_HEADER_PASSWORD "admin"

/**
 * \brief flag which signifies capability of a DSA.
 */
#define CAN_BE_DCS 	1

/*
 ******************************************************************************
 * dcs_server_rest_init                                                    *//**
 *
 * \brief - Initializes the DCS Server REST infrastructure
 *
 * \param[in] rest_port - The Port on which the REST Services should run on
 *
 * \retval DOVE_STATUS_RESTC_INIT_FAILED Cannot initialize REST Client
 * \retval DOVE_STATUS_THREAD_FAILED Cannot start HTTP Server
 * \retval DOVE_STATUS_OK Success
 *
 ******************************************************************************
 */
dove_status dcs_server_rest_init(short rest_port);

typedef struct dove_rest_request_info {
	char *address;
	char *uri;
	unsigned short port;
	enum evhttp_cmd_type type;
	struct evhttp_request *request;
} dove_rest_request_info_t;

/*
 ******************************************************************************
 * dove_rest_request_and_syncprocess --           *//**
 *
 * \brief This routine sends a HTTP request to a server, and waits until
 * corresponding reponse is returned or internal error occurs or timeouts
 * , then invokes the callback
 * - void (*cb)(struct evhttp_request *request, void *arg)) -
 * registered in parameter "request" to handle the response. The first
 * parameter passed to callback is the a pointer to a evhttp_request data
 * structure, and is always the same one in the input parameter of this routine
 * In some internal error cases, NULL might be passed to the callback as the
 * first parameter.
 *
 * \param[in] address The IP address string of the server to which the request
 * is sent.
 * \param[in] port The TCP port of the server to which the request is sent.
 * \param[in] type The HTTP method.
 * \param[in] uri The URI of the HTTP request.
 * \param[in] request A pointer to a evhttp_request data structure.
 * The data structure should be allocated on heap. The ownership of the data
 * structure will passed to this routine, this routine is reponsible to free
 * it before returns. The caller should not dereference it after invoking
 * this routine.
 * \param[in] pre_alloc_base  The event base used for the HTTP operation. If
 * provided, this routine will use it for all related event operations and this
 * routine never free the pre_alloc_base when returns. If NULL, this routine
 * will allocated new event base, and use the new event base for all related
 * event operations and free it when this routine returns.
 * \param[in] timeout_in_secs Sets the timeout (in second) for events related
 * to this HTTP connection.A negative or zero value will set the timeout to
 * default value
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dove_rest_request_and_syncprocess (
	const char *address, unsigned short port,
	enum evhttp_cmd_type type, const char *uri,
	struct evhttp_request *request ,
	struct event_base *pre_alloc_base,
	int timeout_in_secs);

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
int dove_rest_request_and_asyncprocess (dove_rest_request_info_t *rinfo);

int dps_query_dmc_vnid_subnet_config(uint32_t vnid);

dove_status dps_query_domain_config(uint32_t domain_id,
                                    uint32_t *replication_factor);

int dps_query_dmc_policy_info(uint32_t domain, uint32_t src_dvg,
                              uint32_t dst_dvg, uint32_t traffic_type);

void dps_rest_client_send_endpoint_conflict_to_dove_controller(
	uint32_t domain_id, uint32_t dvg1, uint32_t dvg2, uint32_t version,
	unsigned char *vMac1, unsigned char *vMac2, char *pIP_address1, char *pIP_address2,
	char *vIP_addr, uint32_t family);

typedef struct dps_rest_client_to_dove_controller_endpoint_conflict_s {
	/**
	 * \brief Domain
	 */
	uint32_t domain;
	/**
	 * \brief VALID MAC Address: Cannot be 0s
	 */
	uint8_t vMac1[6];

	/**
	 * \brief VALID MAC Address: Cannot be 0s
	 */
	uint8_t vMac2[6];
	/**
	 * \brief The DVG of the Endpoint
	 */
	uint32_t dvg1;
	/**
	 * \brief The DVG of the Endpoint
	 */
	uint32_t dvg2;
	/**
	 * \brief The Version of the Endpoint as seen by the DOVE Switch
	 */
	uint32_t version;
	/**
	 * \brief The Virtual IP Address of the Endpoint
	 * TODO: Make this an ARRAY
	 */
	ip_addr_t virtual_addr;
	/*
	 * \brief The DOVE Switch IP hosting the Endpoint. This MUST be
	 *        filled by the DPS Client
	 */
	ip_addr_t physical_addr1;
	/** \brief The DOVE Switch IP hosting the Endpoint. This MUST be
	 *        filled by the DPS Client
	 */
	ip_addr_t physical_addr2;

	/*
	 * \brief The DCS Client Service Location (IP Address + Port) of the
	 *        Endpoint.
	 *        DCS Clients MUST NOT set this value i.e. family should be 0
	 *        DCS Server (during replication) MUST fill this field in
	 *        based on the address provided by the Protocol Handler.
	 */
	ip_addr_t dps_client_addr;
} dps_rest_client_to_dove_controller_endpoint_conflict_t;

void dps_rest_client_send_endpoint_conflict_to_dove_controller2(
	dps_rest_client_to_dove_controller_endpoint_conflict_t *endpoint_conflict_msg);

/*
 ******************************************************************************
 * dps_cluster_leader_reporting                                        *//**
 *
 * \brief - This function is called by the Cluster Leader to report to the
 *			DOVE Controller (DMC)
 * \retval - None
 ******************************************************************************
 */
void dps_cluster_leader_reporting();

/*
 ******************************************************************************
 * dps_appliance_registration                                        *//**
 *
 * \brief - This function is called to register a DPS (DCS) appliance with
 * 	    the Controller (DMC)
 * \retval - None
 ******************************************************************************
 */

void dps_appliance_registration();

int dps_dmc_register_domain_endpoints(uint32_t domain, uint32_t vnid);

/** @} */

#endif // _DPS_REST_API_
