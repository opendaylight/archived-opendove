/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Forward
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
*  $Log: rest_forward.h $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#ifndef _DPS_REST_FORWARD_H_

/*
 ******************************************************************************
 * REST forward flag.
 * When cluster leader forwards a REST request to appropriate nodes.
 * Forwarding rule is: *        
 *     1 - Forward the POST/DELETE request to all nodes that handle the domain
 *     2 - Forward the GET request to any node that handle the domain
 * Here there are some special cases needed to be addressed
 *     DMC=>Leader:
 *          Domain Add               - forward to available nodes
 *          Domain Delete            - forward to all nodes
 *          DVG Add                  - forward to all nodes
 *          DVG Delete               - forward to all nodes
 *          Service Role             - MUST not forward out
 *          Query Cluster Node
 *          Query Domain Node Mapping
 *     Member Node=>Leader:
 *          Node Hearbeat            - MUST not forward out
 *          Node Statistics
 *
 ****************************************************************************** 
 */
#define DPS_REST_FWD_FLAG_GENERIC               0x00
#define DPS_REST_FWD_FLAG_POST_TO_AVAIL         0x01
#define DPS_REST_FWD_FLAG_POST_TO_ALL           0x02
#define DPS_REST_FWD_FLAG_PUT_TO_AVAIL          0x04
#define DPS_REST_FWD_FLAG_PUT_TO_ALL            0x08
#define DPS_REST_FWD_FLAG_DELETE_TO_ALL         0x10
#define DPS_REST_FWD_FLAG_DENY                  0x20

#define DPS_REST_FWD_TIMEOUT			3 /* seconds */

/* when response cabllback is invoked, pass the struct to it */
typedef struct dps_rest_forward_response_args_s
{
	struct evhttp_request *original_req;
	bool last_one;
	bool local_process;
	bool relay_happened;
	int res_code;
}dps_rest_forward_response_args_t;

/*
 ******************************************************************************
 * \brief This data type defines a callback function which handles special
 *        REST requests. For example:
 *        DMC=>Leader:
 *          Domain Add               - forward to available nodes
 *          Domain Delete            - forward to all nodes
 *          DVG Add                  - forward to all nodes
 *          DVG Delete               - forward to all nodes
 *          Service Role             - MUST not forward out
 *          Query Cluster Node
 *          Query Domain Node Mapping
 *        Member Node=>Leader:
 *          Node Hearbeat            - MUST not forward out
 *          Node Statistics
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  domain_id	A domain ID which the REST request is applied to.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
typedef int (*sp_cb_t)(struct evhttp_request *req, int domain_id, bool *local_process);

/*
 ******************************************************************************
 * dps_rest_forward_handler --                                                    *//**
 *
 * \brief This routine lets cluster leader to forward a REST request to  
 *        appropriate nodes accordingto flag. Forwarding rule is:
 *        1 - Forward the POST/DELETE request to all nodes that handle the domain
 *        2 - Forward the GET request to any node that handle the domain
 *        Here there are some special cases needed to be addressed
 *        DMC=>Leader:
 *          Domain Add               - forward to available nodes
 *          Domain Delete            - forward to all nodes
 *          DVG Add                  - forward to all nodes
 *          DVG Delete               - forward to all nodes
 *          Service Role             - MUST not forward out
 *          Query Cluster Node
 *          Query Domain Node Mapping
 *        Member Node=>Leader:
 *          Node Hearbeat            - MUST not forward out
 *          Node Statistics
 *
 * \param [in]  req 		A pointer to a evhttp_request data structure.
 * \param [in]  flag		The forwarding flag, for example, 
 *                              DPS_REST_FWD_FLAG_POST_TO_ALL.
 * \param [out] local_process	A flag to indicate whether leader needs to further
 *                              process the request.
 *
 * \retval 0 	Success
 * \retval >0 	Failure
 *
 *****************************************************************************/
int dps_rest_forward_handler(struct evhttp_request *req, int flag, bool *local_process);

#endif
