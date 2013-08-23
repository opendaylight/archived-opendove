/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Synch Message
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
*  $Log: rest_sync.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include <stdbool.h>
#include "include.h"
#include "../inc/evhttp_helper.h"
#include "cluster_rest_req_handler.h"

/**
 * \brief The mutex and condition variable used by thread. 
 */
pthread_cond_t dps_rest_sync_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t dps_rest_sync_mp = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief The Interval in which DCS start new sync circle. Unit(second)
 */
int dps_rest_sync_interval = 4;

/**
 * \brief The number of iterations done by this node
 */
static int dps_rest_sync_iterations = 0;


/*
 ******************************************************************************
 * dps_rest_sync_cluster_version_get --                                   *//**
 *
 * \brief This routine returns the last cluster version.
 *
 * \retval cluster_version 
 *
 *****************************************************************************/
static int dps_rest_sync_cluster_version_get(void)
{
	return (int)cluster_config_version;
}

/*
 ******************************************************************************
 * dps_rest_sync_version_get_from_req --                                  *//**
 *
 * \brief This routine extracts create version and update version from 
 *        request body of a specific URI
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 *
 * \param [out]	version_create	The creation version.
 * \param [out]	version_update	The last update version.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dps_rest_sync_version_get_from_req(struct evhttp_request *req, int *version_create, int *version_update)
{
	struct evbuffer *req_body = NULL;
	json_t *js_root = NULL;
	json_t *js_version_create = NULL, *js_version_update = NULL;
	json_error_t jerror;
	int buf_len, n;
	int ret = DOVE_STATUS_ERROR;

	do
	{
		/* extract request input body */
		req_body = evhttp_request_get_input_buffer(req);
		if (req_body == NULL)
		{
			log_debug(RESTHandlerLogLevel,"REQ body NULL");
			break;
		}

		buf_len = evbuffer_get_length(req_body)+1;
		char req_body_buf[buf_len];
		n = evbuffer_copyout(req_body, req_body_buf, buf_len);
		req_body_buf[n]='\0';

		/* extract both create_version and update_version */
		js_root = json_loads(req_body_buf, 0, &jerror);
		if (!js_root)
		{
			log_debug(RESTHandlerLogLevel,"JSON body NULL");
			break;
		}

		/* Borrowed reference, no need to decref */
		js_version_create = json_object_get(js_root, DPS_REST_SYNC_VERSION_CREATE_STRING);
		if (!js_version_create || !json_is_integer(js_version_create))
		{
			break;
		}
		*version_create = json_integer_value(js_version_create);

		js_version_update = json_object_get(js_root, DPS_REST_SYNC_VERSION_UPDATE_STRING);
		if (!js_version_update || !json_is_integer(js_version_update))
		{
			break;
		}
		*version_update = json_integer_value(js_version_update);

		ret = DOVE_STATUS_OK;
	} while(0);


	if (js_root)
	{
		json_decref(js_root);
	}

	return ret;
}

#ifdef DPS_REST_SYNC_POST_METHOD_SUPPORTED
/*
 ******************************************************************************
 * dps_rest_sync_version_get_from_buf --                                  *//**
 *
 * \brief This routine extracts create version and update version from a buffer
 *
 * \param [in]  buf 		A pointer to a buffer.
 *
 * \param [out]	version_create	The creation version.
 * \param [out]	version_update	The last update version.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_sync_version_get_from_buf(char *buf, int *version_create, int *version_update)
{
	json_t *js_root = NULL;
	json_t *js_version_create = NULL, *js_version_update = NULL;
	json_error_t jerror;
	int ret = DOVE_STATUS_ERROR;

	do
	{
		/* extract both create_version and update_version */
		js_root = json_loads(buf, 0, &jerror);
		if (!js_root)
		{
			log_debug(RESTHandlerLogLevel,"JSON body NULL");
			break;
		}

		/* Borrowed reference, no need to decref */
		js_version_create = json_object_get(js_root,
		                                    DPS_REST_SYNC_VERSION_CREATE_STRING);
		if (!js_version_create || !json_is_integer(js_version_create))
		{
			break;
		}
		*version_create = json_integer_value(js_version_create);

		js_version_update = json_object_get( js_root, DPS_REST_SYNC_VERSION_UPDATE_STRING);
		if (!js_version_update || !json_is_integer(js_version_update))
		{
			break;
		}
		*version_update = json_integer_value(js_version_update);

		ret = DOVE_STATUS_OK;
	} while (0);

	if (js_root)
	{
		json_decref(js_root);
	}

	return ret;
}
#endif

/*
 ******************************************************************************
 * dps_rest_sync_version_resp_parser --                                   *//**
 *
 * \brief This routine parses response of version GET request
 *
 * \param [in]	body_buf	A pointer to a version response buffer. 
 *
 * \param [out]	version		The version.
 * \param [out]	target_uri	The URI of the HTTP request.
 * \param [out]	target_method	The method which acts on the URI. Now
 * 				support GET and DELETE.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_sync_version_resp_parser(char *body_buf, int *version, char *target_uri, char *target_method)
{
	json_t *js_root = NULL;
	json_t *js_version = NULL, *js_uri = NULL, *js_method = NULL;
	json_error_t jerror;
	const char *uri, *method;
	int ret = DOVE_STATUS_ERROR;

	do
	{
		js_root = json_loads(body_buf, 0, &jerror);
		if (!js_root)
		{
			log_info(RESTHandlerLogLevel,"JSON body NULL");
			break;
		}

		/* Borrowed reference, no need to decref */
		js_version = json_object_get(js_root, "next_change");
		if (!json_is_integer(js_version))
		{
			log_info(RESTHandlerLogLevel,"No version");
			break;
		}
		*version = json_integer_value(js_version);

		js_uri = json_object_get(js_root, "uri");
		if (!json_is_string(js_uri))
		{
			break;
		}
		if ((uri = json_string_value(js_uri)) == NULL)
		{
			break;
		}
		strcpy(target_uri, uri);

		js_method = json_object_get(js_root, "method");
		if (!json_is_string(js_method))
		{
			break;
		}
		if ((method = json_string_value(js_method)) == NULL)
		{
			break;
		}
		strcpy(target_method, method);

		ret = DOVE_STATUS_OK;
	} while(0);


	if (js_root)
	{
		json_decref(js_root);
	}

	return ret;
}

/*
 ******************************************************************************
 * dps_rest_sync_response_handler --                                      *//**
 *
 * \brief This routine handles response: 
 *        . copy out request input buffer 
 *        . return response code 
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 *
 * \param [out] arg		A pointer to a dps_rest_sync_response_args_t
 *                              data structure including a pointer to input buffer
 *                              and response code.
 *
 * \retval None
 *
 *****************************************************************************/
static void dps_rest_sync_response_handler(struct evhttp_request *req, void *arg)
{
	dps_rest_sync_response_args_t *args = (dps_rest_sync_response_args_t *)arg;
	struct evbuffer *req_body = NULL;
	unsigned int buf_len;
	int n;

	do
	{
		args->res_code = HTTP_NOCONTENT;
		/* sanity check */
		if (req == NULL)
		{
			log_info(RESTHandlerLogLevel,"No evhttp_request");
			break;
		}
		/* extract request body */
		if ((req_body = evhttp_request_get_input_buffer(req)) == NULL)
		{
			log_info(RESTHandlerLogLevel,"No evhttp_request_get_input_buffer");
			break;
		}
		buf_len = evbuffer_get_length(req_body)+1;
		args->req_body_buf = (char *)malloc(buf_len);
		if (args->req_body_buf == NULL)
		{
			log_info(RESTHandlerLogLevel,"dps_rest_sync_response_handler: malloc fails");
			break;
		}
		n = evbuffer_copyout(req_body, args->req_body_buf, buf_len);
		args->req_body_buf[n]='\0';
		args->res_code = evhttp_request_get_response_code(req);
	}while(0);

	return;
}

/*
 ******************************************************************************
 * dps_rest_sync_delete_indicated_in_req --                               *//**
 *
 * \brief This routine checks if delete is required
 *
 * \param [in]  req		A pointer to request input body buffer.
 *
 * \retval 1 deleted
 * \retval 0 exists
 *
 *****************************************************************************/
static int dps_rest_sync_delete_indicated_in_req(char *req)
{
	int fdelete = 0;
	json_t *js_root = NULL;
	json_t *js_deleted = NULL;
	json_error_t jerror;

	do
	{
		//Get the js_root
		js_root = json_loads(req, 0, &jerror);
		if (!js_root)
		{
			log_debug(RESTHandlerLogLevel,"js_root is NULL");
			break;
		}
		js_deleted = json_object_get(js_root, DPS_REST_SYNC_OBJECT_DELETED);
		if (js_deleted == NULL)
		{
			log_debug(RESTHandlerLogLevel,"No deleted flag");
			break;
		}
		if (!json_is_integer(js_deleted))
		{
			break;
		}
		fdelete = (int)json_integer_value(js_deleted);
	}while(0);
	if (js_root)
	{
		json_decref(js_root);
	}
	return fdelete;
}

/*
 ******************************************************************************
 * dps_rest_sync_update_change_version_to_current --                      *//**
 *
 * \brief This routine changes the change_version in the req if it exists
 *        and replaces with the version provided in the parameter
 *
 * \param [in]  req		A pointer to request input body buffer.
 * \param [in]  version		Version value to replace with
 *
 * \return The (potentially) new buffer to use
 *
 *****************************************************************************/
static char *dps_rest_sync_update_change_version_to_current(char *req, int version)
{
	char *new_req = req;
	json_error_t jerror;
	json_t *js_root = NULL;
	json_t *js_version = NULL;
	json_t *js_version_value = NULL;

	do
	{
		//Get the js_root
		js_root = json_loads(req, 0, &jerror);
		if (!js_root)
		{
			log_debug(RESTHandlerLogLevel,"js_root is NULL");
			break;
		}
		js_version = json_object_get(js_root, DPS_REST_SYNC_VERSION_UPDATE_STRING);
		if (js_version == NULL)
		{
			log_debug(RESTHandlerLogLevel,"No update version");
			break;
		}
		if (!json_is_integer(js_version))
		{
			break;
		}
		js_version_value = json_integer((json_int_t)version);
		if (js_version_value == NULL)
		{
			break;
		}
		json_object_update(js_version, js_version_value);
		// Create new request string
		new_req = json_dumps(js_root, JSON_PRESERVE_ORDER);
		if (new_req == NULL)
		{
			new_req = req;
			break;
		}
		log_info(RESTHandlerLogLevel,"Change Version: Old Body %s", req);
		log_info(RESTHandlerLogLevel,"Change Version: New Body %s", new_req);
	}while(0);

	if (js_root)
	{
		json_decref(js_root);
	}
	if (new_req != req)
	{
		free(req);
	}
	return new_req;
}

/*
 ******************************************************************************
 * dps_rest_sync_dmc_agent --                                             *//**
 *
 * \brief This routine constructs new request and sends to REST server.
 *
 * \param [in]	req_body_buf	A pointer to request input body buffer.
 * \param [in]  cmd_type	The method of the HTTP request.
 * \param [in]  uri		The URI of the HTTP request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dps_rest_sync_dmc_agent(char *req_body_buf, enum evhttp_cmd_type cmd_type, const char *uri)
{
	struct evhttp_request *new_request = NULL;
	struct evbuffer *new_req_body = NULL;
	char ip_addr_str[INET6_ADDRSTRLEN];
	dps_rest_sync_response_args_t args;
	int ret = DOVE_STATUS_OK;

	do
	{
		/* step 1 - construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dps_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
			ret = DOVE_STATUS_NO_MEMORY;
			break;
		}
		if (req_body_buf != NULL)
		{
			new_req_body = evbuffer_new();
			if (new_req_body == NULL)
			{
				evhttp_request_free(new_request);
				ret = DOVE_STATUS_NO_MEMORY;
				break;
			}
			evbuffer_add(new_req_body, req_body_buf, strlen(req_body_buf));
			evbuffer_add_buffer(evhttp_request_get_output_buffer(new_request), new_req_body);
		}

		/* step 2 - forward the new request to local REST handler */
		inet_ntop(dps_local_ip.family, dps_local_ip.ip6, ip_addr_str, INET6_ADDRSTRLEN);
		log_debug(RESTHandlerLogLevel, "Routing %s REQ to local REST handler[%s]", uri, ip_addr_str);
		ret = dove_rest_request_and_syncprocess(ip_addr_str, dps_rest_port,
		                                        cmd_type, uri,
		                                        new_request, NULL,
		                                        DPS_REST_SYNC_CONNECT_TIMEOUT);
		if (new_req_body)
		{
			evbuffer_free(new_req_body);
		}

		if (ret)
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
	} while (0);

	if (args.req_body_buf)
	{
		free(args.req_body_buf);
	}
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_sync_target_uri_get --                                        *//**
 *
 * \brief This routine sends a GET request for target URI to DMC.
 *
 * \param [in]  uri		The URI of the HTTP request.
 * \param [out]	req_body_buf	A pointer to request input body buffer.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_sync_target_uri_get(const char *uri, char **req_body_buf)
{
	struct evhttp_request *new_request = NULL;
	dps_rest_sync_response_args_t args;
	char host_header_str[128];
	char ip_addr_str[INET6_ADDRSTRLEN];
	int ret = DOVE_STATUS_OK;

	do
	{
		/* construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dps_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
			ret = DOVE_STATUS_NO_MEMORY;
			break;
		}

		inet_ntop(controller_location.family, controller_location.ip6, ip_addr_str, INET6_ADDRSTRLEN);
		sprintf(host_header_str,"%s:%d", ip_addr_str, controller_location.port_http);
		evhttp_add_header(evhttp_request_get_output_headers(new_request), "Host", host_header_str);
		/* send the GET request for target URI to DMC */
		log_info(RESTHandlerLogLevel, "Sending %s GET REQ to DMC[%s:%d]",
		         uri, ip_addr_str, controller_location.port_http);
		ret = dove_rest_request_and_syncprocess(ip_addr_str,
		                                        controller_location.port_http,
		                                        EVHTTP_REQ_GET, uri,
		                                        new_request, NULL,
		                                        DPS_REST_SYNC_CONNECT_TIMEOUT);

		*req_body_buf = args.req_body_buf;

		if (ret)
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
		if (args.res_code == HTTP_NOTFOUND)
		{
			ret = DOVE_STATUS_NOT_FOUND;
			break;
		}
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
	} while (0);

	return ret;
}

/*
 ******************************************************************************
 * dps_rest_sync_version_query --                                         *//**
 *
 * \brief This routine sends version GET request to DMC.
 *
 * \param [out]	req_body_buf	A pointer to request input body buffer.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_sync_version_query(char **req_body_buf, int version)
{
	struct evhttp_request *new_request = NULL;
	dps_rest_sync_response_args_t args;
	char uri[DPS_URI_LEN];
	char host_header_str[128];
	char ip_addr_str[INET6_ADDRSTRLEN];
	int ret = DOVE_STATUS_OK;

	log_debug(RESTHandlerLogLevel, "Enter");
	do
	{
		/* construct a new request */
		memset(&args, 0, sizeof(args));
		new_request = evhttp_request_new(dps_rest_sync_response_handler, &args);
		if (new_request == NULL)
		{
			ret = DOVE_STATUS_NO_MEMORY;
			break;
		}
		inet_ntop(controller_location.family, controller_location.ip6, ip_addr_str, INET6_ADDRSTRLEN);
		sprintf(host_header_str,"%s:%d", ip_addr_str, controller_location.port_http);
		evhttp_add_header(evhttp_request_get_output_headers(new_request), "Host", host_header_str);

		/* send the GET request to DMC */
		memset(uri, 0, sizeof(uri));
		sprintf(uri, "%s/%d", DPS_REST_SYNC_VERSION_URI, version);
		log_debug(RESTHandlerLogLevel, "Sending version %d GET[%s] REQ to DMC[%s:%d]",
		          version, uri, ip_addr_str, controller_location.port_http);
		ret = dove_rest_request_and_syncprocess(ip_addr_str,
		                                        controller_location.port_http,
		                                        EVHTTP_REQ_GET, uri,
		                                        new_request, NULL,
		                                        DPS_REST_SYNC_CONNECT_TIMEOUT);

		*req_body_buf = args.req_body_buf;

		if (ret)
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
		/* HTTP_NOTFOUND 404 means change has reached the end */
		if ((args.res_code != HTTP_OK) && (args.res_code != 201))
		{
			ret = DOVE_STATUS_ERROR;
			break;
		}
	} while (0);

	log_debug(RESTHandlerLogLevel, "Exit %d", ret);
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_sync_process --                                               *//**
 *
 * \brief This routine starts a new sync circle.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
static int dps_rest_sync_process(void)
{
	char *version_query_body_buf = NULL, *target_uri_body_buf = NULL;
	char target_uri[DPS_URI_LEN];
	char target_method[16];
	enum evhttp_cmd_type cmd_type;
	int version = 0;
	int next_version = 0;
#ifdef DPS_REST_SYNC_POST_METHOD_SUPPORTED
	char **ptr = NULL;
	int version_create, version_update;
#endif
	int buf_len;
	bool more = true;
	int ret = DOVE_STATUS_OK;

	log_debug(RESTHandlerLogLevel, "Enter");
	dps_rest_sync_iterations++;
	//Try to get the next one in the queue
	version = dps_rest_sync_cluster_version_get() + 1;
	log_info(RESTHandlerLogLevel, "[%d] Starting with version %d",
	         dps_rest_sync_iterations, version);
	// Version to start with is the Cluster version at this time
	do
	{
		if (!dcs_role_assigned)
		{
			break;
		}
		if (!dps_cluster_is_local_node_leader())
		{
			break;
		}
		if (!memcmp(controller_location.ip6, dps_local_ip.ip6, 16))
		{
			break;
		}
		do
		{
			/* step 1 - send version query request to DMC */
			log_info(RESTHandlerLogLevel,
			         "[%d] Getting version %d",
			         dps_rest_sync_iterations, version);
			ret = dps_rest_sync_version_query(&version_query_body_buf, version);
			if (ret != DOVE_STATUS_OK)
			{
				more = false;
				log_info(RESTHandlerLogLevel,
				         "[%d] Query for version %d returns [%s], exiting loop.",
				         dps_rest_sync_iterations, version, DOVEStatusToString((dove_status)ret));
				break;
			}

			/* step 2.a - parse and extract version/uri/method from response */
			memset(target_uri, 0, sizeof(target_uri));
			memset(target_method, 0, sizeof(target_method));
			ret = dps_rest_sync_version_resp_parser(version_query_body_buf, 
			                                        &next_version, target_uri, target_method);
			if (ret != DOVE_STATUS_OK)
			{
				log_info(RESTHandlerLogLevel,
				         "dps_rest_sync_version_resp_parser: returns %s",
				         DOVEStatusToString((dove_status)ret));
				break;
			}
			log_info(RESTHandlerLogLevel, "[%d] Next version %d",
			         dps_rest_sync_iterations, next_version);

			buf_len = strlen(target_uri) + 1;
			char new_uri[buf_len];
			strcpy(new_uri, target_uri);

			do
			{
				log_info(RESTHandlerLogLevel,
				         "[%d] Target URI %s: Method %s",
				         dps_rest_sync_iterations, target_uri, target_method);
				if (strlen(target_uri) == 0)
				{
					/* blank uri means goto next */
					break;
				}
				else if (!strcmp(target_method, "DELETE"))
				{
					cmd_type = EVHTTP_REQ_DELETE;
				}
				else if (!strcmp(target_method, "GET"))
				{
					cmd_type = EVHTTP_REQ_PUT;
				}
				else
				{
					ret = DOVE_STATUS_NOT_SUPPORTED;
					break;
				}
				/* step 2.b - send the GET request for target URI to DMC */
				ret = dps_rest_sync_target_uri_get(target_uri, &target_uri_body_buf);
				if (ret != DOVE_STATUS_OK)
				{
					log_info(RESTHandlerLogLevel,
					         "[%d] dps_rest_sync_target_uri_get returns %s",
					         dps_rest_sync_iterations, DOVEStatusToString((dove_status)ret));
					cmd_type = EVHTTP_REQ_DELETE;
					//Temporary FIX:
					break;
					//if (ret == DOVE_STATUS_NOT_FOUND)
					//{
					//	cmd_type = EVHTTP_REQ_DELETE;
					//}
					//else
					//{
					//	more = false;
					//	break;
					//}
				}
				if ((cmd_type == EVHTTP_REQ_PUT) &&
				     dps_rest_sync_delete_indicated_in_req(target_uri_body_buf))
				{
					cmd_type = EVHTTP_REQ_DELETE;
				}
				// Update the buffer version to current
				target_uri_body_buf = dps_rest_sync_update_change_version_to_current(target_uri_body_buf,
				                                                                     version);
				if (cmd_type == EVHTTP_REQ_DELETE)
				{
					log_notice(RESTHandlerLogLevel,
					           "[%d] DELETE: %s",
					           dps_rest_sync_iterations, target_uri);
				}
				else
				{
					log_notice(RESTHandlerLogLevel,
					           "[%d] PUT: %s",
					           dps_rest_sync_iterations,
					           target_uri);
				}
				/* step 3 - route the URI to dmc agent */
				ret = dps_rest_sync_dmc_agent(target_uri_body_buf,
				                              cmd_type, (const char*)target_uri);
				if (ret)
				{
					more = false;
				}
				else if (cmd_type == EVHTTP_REQ_DELETE)
				{
					// ABiswas: Delete not incrementing verison
					//          since delete doesn't have a Body
					//          so version is lost
					dps_cluster_node_heartbeat(&dps_local_ip,
					                           dps_cluster_is_local_node_active(),
					                           version);
				}
			}while(0);
		} while(0);

		if (version_query_body_buf)
		{
			free(version_query_body_buf);
			version_query_body_buf = NULL;
		}
		if (target_uri_body_buf)
		{
			free(target_uri_body_buf);
			target_uri_body_buf = NULL;
		}
		version = next_version;
	} while (more);

	log_debug(RESTHandlerLogLevel, "Exit: ret %d", ret);
	return ret;
}

/*
 ******************************************************************************
 * dps_rest_sync_main --                                                  *//**
 *
 * \brief This routine periodically starts sync circle.
 *
 * \param [in] pDummy	Not used.
 *
 * \retval None
 *
 *****************************************************************************/
static void dps_rest_sync_main(char *pDummy)
{
	struct timespec   ts;
	struct timeval    tp;
	int               it;
	int               rc;

	Py_Initialize();
	while (TRUE) {
		pthread_mutex_lock(&dps_rest_sync_mp);

		it = dps_rest_sync_interval;
		gettimeofday(&tp, NULL);
		/* Convert from timeval to timespec */
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += it;
		rc = pthread_cond_timedwait(&dps_rest_sync_cv, &dps_rest_sync_mp, &ts);
		if (rc == ETIMEDOUT) {
			pthread_mutex_unlock(&dps_rest_sync_mp);
			/* Start new sync circle. */
			log_info(RESTHandlerLogLevel, "New sync circle start");
			dps_rest_sync_process();
			continue;
		}
		if (rc == 0) {
			log_notice(RESTHandlerLogLevel, "Sync thread exiting");
			pthread_mutex_unlock(&dps_rest_sync_mp);
			pthread_mutex_destroy(&dps_rest_sync_mp);
			pthread_cond_destroy(&dps_rest_sync_cv);
#ifdef PTHREAD_REPLACEMENT
			/* pthread delete */
#endif
			return;
		}
		pthread_mutex_unlock(&dps_rest_sync_mp);
	}
	Py_Finalize();

	return;
}

/*
 ******************************************************************************
 * dps_rest_sync_init --                                                  *//**
 *
 * \brief This routine creates sync task and resource.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
dove_status dps_rest_sync_init(void)
{
	dove_status status = DOVE_STATUS_OK;
	pthread_t rest_sync_thread;
	do
	{
		/* Initialize mutex and condition variable objects */
		if (pthread_mutex_init(&dps_rest_sync_mp, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		if (pthread_cond_init (&dps_rest_sync_cv, NULL) != 0) {
			status = DOVE_STATUS_INVALID_PARAMETER;
			break;
		}
		// Add pthread_replacement wrapper. Need to change this func call once the pthread wrapper is implemented 
		pthread_create(&rest_sync_thread, NULL, (void *)dps_rest_sync_main, NULL);

	} while (0);

	return status;
}

