/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


#ifndef _DGADMIN_REST_SYNC_H_

#define DGADMIN_REST_SYNC_POST_METHOD_SUPPORTED

#define DGADMIN_REST_SYNC_CONNECT_TIMEOUT		3 /* seconds */

#define DGADMIN_REST_SYNC_VERSION_URI		    "/controller/sb/v2/opendove/odmc/odgw/changeversion" //"/api/dove/dgw/changeversion"

#define DGADMIN_REST_SYNC_OPERATION_GET		    0
#define DGADMIN_REST_SYNC_OPERATION_DELETE		1

typedef struct dgadmin_rest_sync_response_args_s
{
	char *req_body_buf;
	int res_code;
}dgadmin_rest_sync_response_args_t;

dove_status dgadmin_rest_sync_init(void);
int dgadmin_rest_sync_cluster_version_get(void);
int dgadmin_rest_sync_version_get_from_req(struct evhttp_request *req, int *version_create, int *version_update);
int dgadmin_rest_sync_version_get_from_buf(char *buf, int *version_create, int *version_update);

#endif
