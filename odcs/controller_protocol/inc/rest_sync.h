/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Sync
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
*  $Log: rest_sync.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _DPS_REST_SYNC_H_
#define _DPS_REST_SYNC_H_

//#define DPS_REST_SYNC_POST_METHOD_SUPPORTED

#define DPS_REST_SYNC_CONNECT_TIMEOUT		10 /* seconds */
#define DPS_REST_SYNC_VERSION_URI		"/controller/sb/v2/opendove/odmc/odcs/changeversion"
#define DPS_REST_SYNC_VERSION_CREATE_STRING	"create_version"
#define DPS_REST_SYNC_VERSION_UPDATE_STRING	"update_version"
#define DPS_REST_SYNC_OBJECT_DELETED		"is_tombstone"

typedef struct dps_rest_sync_response_args_s
{
	char *req_body_buf;
	int res_code;
}dps_rest_sync_response_args_t;

dove_status dcs_rest_sync_init(void);
int dps_rest_sync_version_get_from_req(struct evhttp_request *req,
                                       int *version_create,
                                       int *version_update);

#endif //_DPS_REST_SYNC_H_
