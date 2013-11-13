/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/

#ifndef _DOVE_REST_CLIENT_H_
typedef struct dove_rest_request_info {
	char *address;
	char *uri;
	unsigned short port;
	enum evhttp_cmd_type type;
	struct evhttp_request *request;
} dove_rest_request_info_t;
#define DOVE_REST_REQ_INFO_INIT(_rinfo, _addr, _uri, _port, _type, _request) {\
    (_rinfo)->address = strdup(_addr);\
    (_rinfo)->uri = strdup(_uri);\
    (_rinfo)->port = _port;\
    (_rinfo)->type = _type;\
    (_rinfo)->request = _request;\
}
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


/*
 ******************************************************************************
 * dove_rest_client_init --                *//**
 *
 * \brief This routine is used to initialize DOVE REST Client infrastructure
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
int dove_rest_client_init(void);
#endif

