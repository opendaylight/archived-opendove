/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      data_object.c
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Abstract:
 *      This module deals with the CLI DCS Server
 *
 */


#include "include.h"
#include "interface.h"
#include <time.h>

/*
 ******************************************************************************
 * DPS CLI Functionality                                                  *//**
 *
 * \ingroup DPSServerCLIDataObjects
 * @{
 */

#define CLI_PERF_TEST_MAX_DOMAINS 4096
#define CLI_PERF_TEST_DVG_PER_DOMAIN 64
#define CLI_PERF_TEST_MAX_DVGS 262144
#define CLI_PERF_TEST_DVG_MASK 262144
#define CLI_PERF_TEST_DOMAIN_START 1000
#define MAX_UPDATE_RETRIES 16

static uint32_t gquery_id = 1;

/**
 * \brief The callback function for CLI_DATA_OBJECTS code
 */

typedef dove_status (*cli_data_object_callback_func)(cli_data_object_t *);

/**
 * \brief An Array of Callbacks for every CLI_DATA_OBJECTS Code
 */
static cli_data_object_callback_func cli_callback_array[CLI_DATA_OBJECTS_MAX];

typedef struct cli_hash_perf_object_s{
	time_t		op_start;
	time_t		op_end_prev;
	time_t		op_end;
	time_t		reply_start;
	time_t		reply_end_prev;
	time_t		reply_end;
	uint32_t	type;
	uint32_t	ops;
	uint32_t	replies;
	uint32_t	ops_total;
	uint32_t	replies_total;
	uint32_t	success;
	uint32_t	success_total;
}cli_hash_perf_object_t;

//                              Error-Code                       String                     Value
//                              ----------                      ---------                   -----
#define CLI_HASH_PERF_OBJECT_CODES \
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_empty,              "Empty",               0)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_endpoint,           "Endpoint",            1)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_domain,             "Domain",              2)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_policy,             "Policy",              3)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_tunnel,             "Dove Tunnel",         4)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_ip_address,         "IP Address",          5)\
	DEFINE_CLI_HASH_PERF_OBJECT_AT(cli_perf_none_max,           "Unknown",             6)

#define DEFINE_CLI_HASH_PERF_OBJECT_AT(_obj, _str, _val) _obj = _val,
typedef enum {
	CLI_HASH_PERF_OBJECT_CODES
}cli_hash_perf_object_type;
#undef DEFINE_CLI_HASH_PERF_OBJECT_AT

#define DEFINE_CLI_HASH_PERF_OBJECT_AT(_obj, _str, _val) _str,
const char *returnCliHashObjectStrs[] = {
	CLI_HASH_PERF_OBJECT_CODES
};
#undef DEFINE_CLI_HASH_PERF_OBJECT_AT

const char *ObjectToString(cli_hash_perf_object_type obj)
{
	if (obj > cli_perf_none_max)
	{
		return "Unknown";
	}
	else
	{
		return returnCliHashObjectStrs[obj];
	}
}

/**
 * \brief An array of Hash Objects
 */
static cli_hash_perf_object_t obj_perf_array[cli_perf_none_max];

/**
 * \brief A mapping of message types to the corresponding Hash Objects
 */
static int dps_request_type[DPS_MAX_MSG_TYPE];

/**
 * \brief Total Repetitions
 */
static int Repetition;

/**
 * \brief How often to print
 */
static int PrintFrequency;

/*
 ******************************************************************************
 * cli_hash_perf_reply                                                    *//**
 *
 * \brief This routine is called by the data handler when the CLI is doing
 *        Performance Testing.
 *
 * \param[in] msg - A pointer to a message that the client wants to send.
 *
 ******************************************************************************
 */
void cli_hash_perf_reply(dps_client_data_t *msg)
{

	cli_hash_perf_object_t *hash_obj = &obj_perf_array[dps_request_type[msg->hdr.type]];

	do
	{
		if (hash_obj->type == cli_perf_empty)
		{
			break;
		}

		hash_obj->replies++;
		hash_obj->replies_total++;
		if (msg->hdr.resp_status == DPS_NO_ERR)
		{
			hash_obj->success++;
			hash_obj->success_total++;
		}
		if (!(hash_obj->replies%PrintFrequency))
		{
			hash_obj->reply_end = time(&hash_obj->reply_end);
			show_print("Hash Performance Test [Message Type %d, Iteration %d]\n\r"
			           "     Current Time %s\r"
			           "     Object %s, Time Taken %d seconds\n\r"
			           "     Total Replies %d, Replies %d, Success %d",
			        msg->hdr.type,
			        Repetition,
			        ctime(&hash_obj->reply_end),
			        ObjectToString((cli_hash_perf_object_type)hash_obj->type),
			        (int)(hash_obj->reply_end - hash_obj->reply_end_prev),
			        hash_obj->replies_total, hash_obj->replies, hash_obj->success);
			show_print("------------------------------------------------------------------");
			hash_obj->reply_end_prev = hash_obj->reply_end;
			hash_obj->success = hash_obj->replies = 0;
		}
	} while(0);
	return;
}

/*
 ******************************************************************************
 * dev_log_level                                                          *//**
 *
 * \brief - Changes the Log Level for Development Code of the DPS Data Handler
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status log_level(cli_data_object_t *cli_data)
{
	PythonDataHandlerLogLevel = (int32_t)cli_data->log_level.level;
	PythonMulticastDataHandlerLogLevel = PythonDataHandlerLogLevel;
	log_info(CliLogLevel, "PythonDataHandlerLogLevel set to %d",
	         PythonDataHandlerLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * domain_add                                                             *//**
 *
 * \brief - Adds a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_add(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_ADD;
	data_op.domain_add.domain_id = cli_data->domain_add.domain_id;
	data_op.domain_add.replication_factor = 1;
	log_info(CliLogLevel, "Adding domain: %d", data_op.domain_add.domain_id);

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * domain_add                                                             *//**
 *
 * \brief - Adds a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_update(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_UPDATE;
	data_op.domain_add.domain_id = cli_data->domain_update.domain_id;
	data_op.domain_add.replication_factor = cli_data->domain_update.replication_factor;
	log_info(CliLogLevel, "Adding domain: %d", data_op.domain_add.domain_id);

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * domain_delete                                                          *//**
 *
 * \brief - Deletes a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_delete(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	data_op.type = DPS_CONTROLLER_DOMAIN_DELETE;
	data_op.domain_delete.domain_id = cli_data->domain_delete.domain_id;
	status = dps_controller_data_msg(&data_op);
	dps_cluster_send_domain_delete_to_all_nodes(data_op.domain_delete.domain_id);
	return status;
}

/*
 ******************************************************************************
 * domains_clear                                                          *//**
 *
 * \brief - Deletes all domains handled by local node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domains_clear(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	data_op.type = DPS_CONTROLLER_DOMAIN_DELETE_ALL_LOCAL;
	status = dps_controller_data_msg(&data_op);
	return status;
}

/*
 ******************************************************************************
 * domain_deactivate                                                      *//**
 *
 * \brief - Deactivate a domain from local node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_deactivate(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_DOMAIN_DEACTIVATE;
	data_op.domain_delete.domain_id = cli_data->domain_delete.domain_id;
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * domain_show                                                             *//**
 *
 * \brief - Shows a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_SHOW;
	data_op.domain_show.domain_id = cli_data->domain_show.domain_id;
	data_op.domain_show.fDetails = cli_data->domain_show.fDetails;
	log_info(CliLogLevel, "Showing domain: %d", data_op.domain_add.domain_id);

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * domain_global_show                                                     *//**
 *
 * \brief - Shows all domain mappings
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status domain_global_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_GLOBAL_SHOW;
	status = dps_controller_data_msg(&data_op);
	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * dvg_add                                                                *//**
 *
 * \brief - Adds a dvg to a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dvg_add(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	data_op.type = DPS_CONTROLLER_DVG_ADD;
	data_op.dvg_add.domain_id = cli_data->dvg_add.domain_id;
	data_op.dvg_add.dvg_id = cli_data->dvg_add.dvg_id;
	status = dps_controller_data_msg(&data_op);
	if (status == DOVE_STATUS_OK)
	{
		dps_cluster_send_vnid_existence_to_all_nodes(cli_data->dvg_add.dvg_id, 1);
	}
	return status;
}

/*
 ******************************************************************************
 * dvg_delete                                                             *//**
 *
 * \brief - Deletes a dvg from a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status dvg_delete(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;
	dove_status status;

	data_op.type = DPS_CONTROLLER_DVG_DELETE;
	data_op.dvg_delete.dvg_id = cli_data->dvg_delete.dvg_id;
	// First send the delete to all nodes otherwise
	// dps_cluster_send_vnid_existence_to_all_nodes cannot figure
	// out domain id
	dps_cluster_send_vnid_existence_to_all_nodes(cli_data->dvg_delete.dvg_id, 0);
	status = dps_controller_data_msg(&data_op);
	return status;
}

/*
 ******************************************************************************
 * vnid_show                                                             *//**
 *
 * \brief - Shows a VNID
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status vnid_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_VNID_SHOW;
	data_op.vnid_show.vnid = cli_data->vnid_show.vnid;
	data_op.vnid_show.fDetails = cli_data->vnid_show.fDetails;
	log_info(CliLogLevel, "Showing VNID: %d", cli_data->vnid_show.vnid);

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * query_vnid                                                             *//**
 *
 * \brief - Query Dove Controller for a VNID by Restful http request
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status query_vnid(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_QUERY_VNID;
	data_op.query_vnid.vnid = cli_data->query_vnid.vnid;
	log_info(CliLogLevel, "Query VNID: %d", cli_data->query_vnid.vnid);
	status = dps_controller_data_msg(&data_op);
	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}




/*
 ******************************************************************************
 * policy_add                                                                *//**
 *
 * \brief - Adds a policy to a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status policy_add(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_POLICY_ADD; 
	data_op.policy_add.traffic_type = cli_data->policy_add.traffic_type;
	data_op.policy_add.domain_id = cli_data->policy_add.domain_id;
	data_op.policy_add.type = cli_data->policy_add.type;
	data_op.policy_add.src_dvg_id = cli_data->policy_add.src_dvg_id;
	data_op.policy_add.dst_dvg_id = cli_data->policy_add.dst_dvg_id;
	data_op.policy_add.ttl = cli_data->policy_add.ttl;
	memcpy((char *)&data_op.policy_add.action, (char *)&cli_data->policy_add.action, sizeof(data_op.policy_add.action));
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * policy_delete                                                             *//**
 *
 * \brief - Deletes a policy from a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status policy_delete(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_POLICY_DELETE; 
	data_op.policy_delete.traffic_type = cli_data->policy_delete.traffic_type;
	data_op.policy_delete.domain_id = cli_data->policy_delete.domain_id;
	data_op.policy_delete.src_dvg_id = cli_data->policy_delete.src_dvg_id;
	data_op.policy_delete.dst_dvg_id = cli_data->policy_delete.dst_dvg_id;
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * endpoint_update                                                             *//**
 *
 * \brief - Performs an endpoint update
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status endpoint_update(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status = DPS_SUCCESS;
	dps_client_data_t dps_msg;
	int retries = 0;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->endpoint_update.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_ENDPOINT_UPDATE;
	dps_msg.hdr.client_id = cli_data->endpoint_update.client_type;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	dps_msg.hdr.sub_type = cli_data->endpoint_update.update_op;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	dps_msg.endpoint_update.vnid = cli_data->endpoint_update.vnid;
	memcpy(dps_msg.endpoint_update.mac, cli_data->endpoint_update.vMac, 6);
	dps_msg.endpoint_update.tunnel_info.num_of_tunnels = 1;
	dps_msg.endpoint_update.tunnel_info.tunnel_list[0].family = cli_data->endpoint_update.pIP_type;
	memcpy(dps_msg.endpoint_update.tunnel_info.tunnel_list[0].ip6,
	       cli_data->endpoint_update.pIPv6,
	       16);
	if(dps_msg.endpoint_update.tunnel_info.tunnel_list[0].family == AF_INET)
	{
		dps_msg.endpoint_update.tunnel_info.tunnel_list[0].ip4 =
			ntohl(dps_msg.endpoint_update.tunnel_info.tunnel_list[0].ip4);
	}
	dps_msg.endpoint_update.vm_ip_addr.family = cli_data->endpoint_update.vIP_type;
	memcpy(dps_msg.endpoint_update.vm_ip_addr.ip6,
	       cli_data->endpoint_update.vIPv6,
	       16);
	if(dps_msg.endpoint_update.tunnel_info.tunnel_list[0].family == AF_INET)
	{
		dps_msg.endpoint_update.vm_ip_addr.ip4 =
			ntohl(dps_msg.endpoint_update.vm_ip_addr.ip4);
	}
	memcpy(&dps_msg.endpoint_update.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));
	dps_msg.endpoint_update.version = 0;

	// Retry till the right version is reached (max 8)
	do
	{
		dps_status = dps_protocol_send_to_server(&dps_msg);
		retries++;
		dps_msg.endpoint_update.version++;
	}while((dps_status != DPS_SUCCESS) &&
	       (retries < MAX_UPDATE_RETRIES) &&
	       (cli_data->endpoint_update.update_op != DPS_ENDPOINT_UPDATE_DELETE));

	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}
	log_info(CliLogLevel, "Exit: status:%s", DOVEStatusToString(ret_status));

	return ret_status;
}

/*
 ******************************************************************************
 * endpoint_lookup_mac                                                    *//**
 *
 * \brief - Performs an endpoint lookup by mac
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status endpoint_lookup_mac(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status;
	dps_client_data_t dps_msg;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->endpoint_lookup_vMac.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_ENDPOINT_LOC_REQ;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	dps_msg.endpoint_loc_req.vnid = cli_data->endpoint_lookup_vMac.vnid;
	memset(&dps_msg.endpoint_loc_req.dps_client_addr, 0, sizeof(ip_addr_t));
	memcpy(dps_msg.endpoint_loc_req.mac, cli_data->endpoint_lookup_vMac.vMac, 6);
	// Zero out IP in the request
	memset(&dps_msg.endpoint_loc_req.vm_ip_addr, 0, sizeof(ip_addr_t));
	memcpy(&dps_msg.endpoint_loc_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(ret_status));

	return ret_status;
}

/*
 ******************************************************************************
 * endpoint_lookup_ip                                                    *//**
 *
 * \brief - Performs an endpoint lookup by IP
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status endpoint_lookup_ip(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status;
	dps_client_data_t dps_msg;

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->endpoint_lookup_vIP.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_ENDPOINT_LOC_REQ;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	memcpy(&dps_msg.endpoint_loc_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));
	dps_msg.endpoint_loc_req.vnid = cli_data->endpoint_lookup_vIP.vnid;
	memset(&dps_msg.endpoint_loc_req.vm_ip_addr, 0, sizeof(ip_addr_t));
	// Zero out MAC in the request
	memset(dps_msg.endpoint_loc_req.mac, 0, 6);
	// IP
	dps_msg.endpoint_loc_req.vm_ip_addr.family = cli_data->endpoint_lookup_vIP.vIP_type;
	memcpy(dps_msg.endpoint_loc_req.vm_ip_addr.ip6,
	       cli_data->endpoint_lookup_vIP.vIPv6,
	       16);
	if (dps_msg.endpoint_loc_req.vm_ip_addr.family == AF_INET)
	{
		dps_msg.endpoint_loc_req.vm_ip_addr.ip4 = ntohl(dps_msg.endpoint_loc_req.vm_ip_addr.ip4);
	}
	memcpy(&dps_msg.endpoint_loc_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	return ret_status;
}

/*
 ******************************************************************************
 * policy_lookup_mac                                                    *//**
 *
 * \brief - Performs an policy lookup by mac
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status policy_lookup_mac(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status;
	dps_client_data_t dps_msg;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->policy_lookup_vMac.src_vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_POLICY_REQ;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	dps_msg.policy_req.src_endpoint.vnid = cli_data->policy_lookup_vMac.src_vnid;
	memcpy(&dps_msg.policy_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));
	memcpy(dps_msg.policy_req.dst_endpoint.mac,
	       cli_data->policy_lookup_vMac.vMac, 6);
	// Zero out IP in the request
	memset(&dps_msg.policy_req.dst_endpoint.vm_ip_addr, 0,
	       sizeof(ip_addr_t));
	memcpy(&dps_msg.policy_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(ret_status));

	return ret_status;
}

/*
 ******************************************************************************
 * policy_lookup_ip                                                    *//**
 *
 * \brief - Performs an policy lookup by IP
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status policy_lookup_ip(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status;
	dps_client_data_t dps_msg;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->policy_lookup_vIP.src_vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_POLICY_REQ;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	memcpy(&dps_msg.policy_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	// Set the Source Values
	dps_msg.policy_req.src_endpoint.vnid = cli_data->policy_lookup_vIP.src_vnid;

	// Set the Destination Values
	// Zero out MAC in the request
	memset(dps_msg.policy_req.dst_endpoint.mac, 0, 6);
	// IP
	dps_msg.policy_req.dst_endpoint.vm_ip_addr.family = cli_data->policy_lookup_vIP.vIP_type;
	memcpy(dps_msg.policy_req.dst_endpoint.vm_ip_addr.ip6,
	       cli_data->policy_lookup_vIP.vIPv6,
	       16);
	if(dps_msg.policy_req.dst_endpoint.vm_ip_addr.family == AF_INET)
	{
		dps_msg.policy_req.dst_endpoint.vm_ip_addr.ip4 =
			ntohl(dps_msg.policy_req.dst_endpoint.vm_ip_addr.ip4);
	}
	memcpy(&dps_msg.policy_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(ret_status));

	return ret_status;
}

static dove_status tunnel_register(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status = DPS_SUCCESS;
	dps_client_data_t *dps_msg;
	uint16_t i;

	log_info(CliLogLevel, "Enter");
	do
	{
		dps_msg = (dps_client_data_t *)malloc(dps_offsetof(dps_client_data_t,
		                                                   tunnel_reg_dereg.tunnel_info.tunnel_list[cli_data->tunnel_reg.num_tunnels]));
		if(dps_msg == NULL)
		{
			dps_status = DPS_ERROR_NO_RESOURCES;
			break;
		}
		// Fill the header
		dps_msg->context = NULL;
		dps_msg->hdr.resp_status = DPS_NO_ERR;
		dps_msg->hdr.vnid = cli_data->tunnel_reg.vnid;
		dps_msg->hdr.query_id = gquery_id++;
		if (cli_data->cli_data_object_code == CLI_DATA_OBJECTS_TUNNEL_REGISTER)
		{
			dps_msg->hdr.type = DPS_TUNNEL_REGISTER;
		}
		else
		{
			dps_msg->hdr.type = DPS_TUNNEL_DEREGISTER;
		}
		dps_msg->hdr.client_id = (uint8_t)cli_data->tunnel_reg.client_type;
		dps_msg->hdr.transaction_type = DPS_TRANSACTION_NORMAL;
		// Set the DPS Client as local
		memcpy(&dps_msg->hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
		if(dps_msg->hdr.reply_addr.family == AF_INET)
		{
			dps_msg->hdr.reply_addr.ip4 = ntohl(dps_msg->hdr.reply_addr.ip4);
		}
		//Fill the body
		memcpy(&dps_msg->tunnel_reg_dereg.dps_client_addr,
		       &dps_msg->hdr.reply_addr,
		       sizeof(ip_addr_t));
		dps_msg->tunnel_reg_dereg.tunnel_info.num_of_tunnels = (uint16_t)cli_data->tunnel_reg.num_tunnels;
		if (dps_msg->tunnel_reg_dereg.tunnel_info.num_of_tunnels > 4)
		{
			dps_msg->tunnel_reg_dereg.tunnel_info.num_of_tunnels = 4;
		}
		for (i = 0; i < dps_msg->tunnel_reg_dereg.tunnel_info.num_of_tunnels; i++)
		{
			dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].family =
				cli_data->tunnel_reg.ip_list[i].IP_type;
			memcpy(dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].ip6,
			       cli_data->tunnel_reg.ip_list[i].pIPv6,
			       16);
			if (dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].family == AF_INET)
			{
				dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].ip4 =
					ntohl(dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].ip4);
			}
			dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].port = 0;
			dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].tunnel_type = TUNNEL_TYPE_VXLAN;
			dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].vnid = cli_data->tunnel_reg.vnid;
			dps_msg->tunnel_reg_dereg.tunnel_info.tunnel_list[i].flags = 0;
		}
		dps_status = dps_protocol_send_to_server(dps_msg);
		free(dps_msg);
	}while(0);

	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_NO_MEMORY;
	}
	log_info(CliLogLevel, "Exit: status:%s", DOVEStatusToString(ret_status));

	return ret_status;
}


/*****************************************************************************/
/*
 ******************************************************************************
 * endpoint_conflict_test                                                    *//**
 *
 * \brief - Performs a simulated endpoint update and conflict test
 *
 * \param[in] perf_test Endpoint Update and Conflict Test Parameters
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status endpoint_conflict_test(cli_data_object_t *perf_test)
{
	return DOVE_STATUS_NOT_SUPPORTED;
}


/*****************************************************************************/

/*
 ******************************************************************************
 * gateway_update                                                         *//**
 *
 * \brief - Performs an Gateway Update
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status gateway_update(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.gateway_update.vnid = cli_data->gateway_update.vnid;
	data_op.gateway_update.IP_type = cli_data->gateway_update.IP_type;
	memcpy(data_op.gateway_update.IPv6,
	       cli_data->gateway_update.IPv6,
	       16);

	switch(cli_data->cli_data_object_code)
	{
		case CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_ADD:
			log_info(CliLogLevel, "DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD");
			data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD;
			break;
		case CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_DEL:
			log_info(CliLogLevel, "CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_DEL");
			data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_DEL;
			break;
		default:
			log_warn(CliLogLevel, "BAD OPCODE!!!");
			data_op.type = DPS_CONTROLLER_OP_MAX;
			break;
	}

	if (data_op.type < DPS_CONTROLLER_OP_MAX)
	{
		status = dps_controller_data_msg(&data_op);
	}

	log_info(CliLogLevel, "Exit status: %s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * gateway_clear                                                         *//**
 *
 * \brief - Performs an Gateway Clear
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status gateway_clear(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.gateway_clear.dvg_id = cli_data->gateway_clear.dvg_id;

	switch(cli_data->cli_data_object_code)
	{
		case CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_CLR:
			data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_CLR;
			break;
		default:
			log_warn(CliLogLevel, "BAD OPCODE!!!");
			data_op.type = DPS_CONTROLLER_OP_MAX;
			break;
	}

	if (data_op.type < DPS_CONTROLLER_OP_MAX)
	{
		status = dps_controller_data_msg(&data_op);
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * gateway_get                                                            *//**
 *
 * \brief - Performs an Gateway Clear
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status gateway_get(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status;
	dps_client_data_t dps_msg;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.vnid = cli_data->gateway_get.dvg_id;
	dps_msg.hdr.query_id = gquery_id++;
	switch(cli_data->cli_data_object_code)
	{
		case CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_GET:
			dps_msg.hdr.type = DPS_EXTERNAL_GW_LIST_REQ;
			break;
		case CLI_DATA_OBJECTS_VLAN_GATEWAY_GET:
			dps_msg.hdr.type = DPS_VLAN_GW_LIST_REQ;
			break;
		default:
			dps_msg.hdr.type = DPS_EXTERNAL_GW_LIST_REQ;
			break;
	}
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	memcpy(&dps_msg.gen_msg_req.dps_client_addr,
	       &dps_msg.hdr.reply_addr,
	       sizeof(ip_addr_t));

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(ret_status));
	return ret_status;
}

/* cli_data_objects_ip_subnet routines */

/*
 ******************************************************************************
 * ip_subnet_add                                                          *//**
 *
 * \brief - Adds an ip subnet to the list
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status ip_subnet_add(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_ADD; 
	data_op.ip_subnet_add.associated_type = cli_data->ip_subnet_add.associated_type;
	data_op.ip_subnet_add.associated_id = cli_data->ip_subnet_add.associated_id;
	data_op.ip_subnet_add.IP_type = cli_data->ip_subnet_add.IP_type;
	data_op.ip_subnet_add.mask = cli_data->ip_subnet_add.mask;
	memcpy(data_op.ip_subnet_add.IPv6, cli_data->ip_subnet_add.IPv6, 16);
	data_op.ip_subnet_add.mode = cli_data->ip_subnet_add.mode;
	memcpy(data_op.ip_subnet_add.gateway_v6, cli_data->ip_subnet_add.gateway_v6, 16);
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * ip_subnet_delete                                                        *//*
 *
 * \brief - Delete an ip subnet from the list
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status ip_subnet_delete(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_DELETE; 
	data_op.ip_subnet_delete.associated_type = cli_data->ip_subnet_delete.associated_type;
	data_op.ip_subnet_delete.associated_id = cli_data->ip_subnet_delete.associated_id;
	data_op.ip_subnet_delete.IP_type = cli_data->ip_subnet_delete.IP_type;
	data_op.ip_subnet_delete.mask = cli_data->ip_subnet_delete.mask;
	memcpy(data_op.ip_subnet_delete.IPv6, cli_data->ip_subnet_delete.IPv6, 16);
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * ip_subnet_lookup                                                       *//**
 *
 * \brief - Lookups an ip subnet from the domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status ip_subnet_lookup(cli_data_object_t *cli_data)
{
#if 0
	dps_controller_data_op_t data_op;

	return dps_controller_data_msg(&data_op);
#else
	return DOVE_STATUS_OK;
#endif
}

/*
 ******************************************************************************
 * ip_subnet_list                                                         *//**
 *
 * \brief - list an IP Subnet List
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status ip_subnet_list(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_LIST; 
	data_op.ip_subnet_list.associated_type = cli_data->ip_subnet_list.associated_type;
	data_op.ip_subnet_list.associated_id = cli_data->ip_subnet_list.associated_id;
	data_op.ip_subnet_list.IP_type = cli_data->ip_subnet_list.IP_type;
	return dps_controller_data_msg(&data_op);
}

/*
 ******************************************************************************
 * ip_subnet_flush                                                        *//**
 *
 * \brief - flush an IP Subnet List
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status ip_subnet_flush(cli_data_object_t *cli_data)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_FLUSH; 
	data_op.ip_subnet_flush.associated_type = cli_data->ip_subnet_flush.associated_type;
	data_op.ip_subnet_flush.associated_id = cli_data->ip_subnet_flush.associated_id;
	data_op.ip_subnet_flush.IP_type = cli_data->ip_subnet_flush.IP_type;
	return dps_controller_data_msg(&data_op);
}

static dove_status outstanding_context_show(cli_data_object_t *cli_data)
{
	show_print("Outstanding Context Count %d\n",
	           outstanding_unsolicited_msg_count);
	return DOVE_STATUS_OK;
}

static dove_status multicast_receiver_register(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status = DPS_SUCCESS;
	dps_client_data_t dps_msg;
	dps_mcast_receiver_t *mcast_receiver;

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.client_id = cli_data->multicast_register.client_type;
	dps_msg.hdr.vnid = cli_data->multicast_register.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	switch(cli_data->cli_data_object_code)
	{
		case CLI_DATA_OBJECTS_MULTICAST_RECEIVER_ADD:
		case CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD:
			dps_msg.hdr.type = DPS_MCAST_RECEIVER_JOIN;
			break;
		case CLI_DATA_OBJECTS_MULTICAST_RECEIVER_DEL:
		case CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL:
			dps_msg.hdr.type = DPS_MCAST_RECEIVER_LEAVE;
			break;
		default:
			ret_status = DOVE_STATUS_INVALID_PARAMETER;
			goto Exit;
	}
	dps_msg.hdr.client_id = (uint8_t)cli_data->multicast_register.client_type;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}

	//DPS Client Address
	mcast_receiver = (dps_mcast_receiver_t *)&dps_msg.mcast_receiver;
	memcpy(&mcast_receiver->dps_client_addr, &dps_msg.hdr.reply_addr, sizeof(ip_addr_t));

	//Tunnel Address
	mcast_receiver->tunnel_endpoint.family = cli_data->multicast_register.tunnel_IP_type;
	memcpy(mcast_receiver->tunnel_endpoint.ip6,
	       cli_data->multicast_register.tunnel_IPv6,
	       16);
	if(mcast_receiver->tunnel_endpoint.family == AF_INET)
	{
		mcast_receiver->tunnel_endpoint.ip4 =
			ntohl(mcast_receiver->tunnel_endpoint.ip4);
	}

	//Multicast MAC
	memcpy(mcast_receiver->mcast_group_rec.mcast_addr.mcast_mac,
	       cli_data->multicast_register.mac,
	       6);
	//Multicast IP
	if ((cli_data->cli_data_object_code == CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD) ||
	    (cli_data->cli_data_object_code == CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL))
	{
		mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V4_ICB_RANGE;
	}
	else if (cli_data->multicast_register.multicast_IP_type == AF_INET)
	{
		mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V4;
	}
	else if (cli_data->multicast_register.multicast_IP_type == AF_INET6)
	{
		mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_V6;
	}
	else
	{
		mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type = MCAST_ADDR_MAC;
	}
	memcpy(mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip6,
	       cli_data->multicast_register.multicast_IPv6,
	       16);
	if (mcast_receiver->mcast_group_rec.mcast_addr.mcast_addr_type == MCAST_ADDR_V4)
	{
		mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip4 =
			ntohl(mcast_receiver->mcast_group_rec.mcast_addr.u.mcast_ip4);
	}

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

Exit:
	log_info(CliLogLevel, "Exit: status:%s", DOVEStatusToString(ret_status));
	return ret_status;
}

static dove_status multicast_sender_register(cli_data_object_t *cli_data)
{
	dove_status ret_status = DOVE_STATUS_OK;
	dps_return_status dps_status = DPS_SUCCESS;
	dps_client_data_t dps_msg;
	dps_mcast_sender_t *mcast_sender;

	log_info(CliLogLevel, "Enter");

	memset(&dps_msg, 0, sizeof(dps_client_data_t));

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.client_id = cli_data->multicast_register.client_type;
	dps_msg.hdr.vnid = cli_data->multicast_register.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	switch(cli_data->cli_data_object_code)
	{
		case CLI_DATA_OBJECTS_MULTICAST_SENDER_ADD:
			dps_msg.hdr.type = DPS_MCAST_SENDER_REGISTRATION;
			break;
		case CLI_DATA_OBJECTS_MULTICAST_SENDER_DEL:
			dps_msg.hdr.type = DPS_MCAST_SENDER_DEREGISTRATION;
			break;
		default:
			break;
	}
	dps_msg.hdr.client_id = (uint8_t)cli_data->multicast_register.client_type;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}

	//DPS Client Address
	mcast_sender = (dps_mcast_sender_t *)&dps_msg.mcast_sender;
	memcpy(&mcast_sender->dps_client_addr, &dps_msg.hdr.reply_addr, sizeof(ip_addr_t));

	//Tunnel Address
	mcast_sender->tunnel_endpoint.family = cli_data->multicast_register.tunnel_IP_type;
	memcpy(mcast_sender->tunnel_endpoint.ip6,
	       cli_data->multicast_register.tunnel_IPv6,
	       16);
	if(mcast_sender->tunnel_endpoint.family == AF_INET)
	{
		mcast_sender->tunnel_endpoint.ip4 =
			ntohl(mcast_sender->tunnel_endpoint.ip4);
	}

	//Multicast MAC
	memcpy(mcast_sender->mcast_addr.mcast_mac,
	       cli_data->multicast_register.mac,
	       6);
	//Multicast IP
	if (cli_data->multicast_register.multicast_IP_type == AF_INET)
	{
		mcast_sender->mcast_addr.mcast_addr_type = MCAST_ADDR_V4;
	}
	else if (cli_data->multicast_register.multicast_IP_type == AF_INET6)
	{
		mcast_sender->mcast_addr.mcast_addr_type = MCAST_ADDR_V6;
	}
	else
	{
		mcast_sender->mcast_addr.mcast_addr_type = MCAST_ADDR_MAC;
	}

	memcpy(mcast_sender->mcast_addr.u.mcast_ip6,
	       cli_data->multicast_register.multicast_IPv6,
	       16);
	if (mcast_sender->mcast_addr.mcast_addr_type == MCAST_ADDR_V4)
	{
		mcast_sender->mcast_addr.u.mcast_ip4 =
			ntohl(mcast_sender->mcast_addr.u.mcast_ip4);
	}

	dps_status = dps_protocol_send_to_server(&dps_msg);
	if (dps_status != DPS_SUCCESS)
	{
		ret_status = DOVE_STATUS_INVALID_PARAMETER;
	}

	log_info(CliLogLevel, "Exit: status:%s", DOVEStatusToString(ret_status));
	return ret_status;
}

/*
 ******************************************************************************
 * multicast_show                                                         *//**
 *
 * \brief - Shows details of Multicast Addresses in a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status multicast_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_MULTICAST_SHOW;
	data_op.multicast_show.associated_type = cli_data->multicast_show.associated_type;
	data_op.multicast_show.associated_id = cli_data->multicast_show.associated_id;

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * multicast_control_get                                                  *//**
 *
 * \brief - Get the Multicast Control Tunnel for a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status multicast_control_get(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_client_data_t dps_msg;
	dps_gen_msg_req_t *gen_msg;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.vnid = cli_data->multicast_control_get.dvg_id;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_MCAST_CTRL_GW_REQ;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}

	//DPS Client Address
	gen_msg = (dps_gen_msg_req_t *)&dps_msg.gen_msg_req;
	memcpy(&gen_msg->dps_client_addr, &dps_msg.hdr.reply_addr, sizeof(ip_addr_t));

	dps_protocol_send_to_server(&dps_msg);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}


/*
 ******************************************************************************
 * multicast_log_level                                                    *//**
 *
 * \brief - Changes the Log Level for Multicast Code of the DPS Data Handler
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status multicast_log_level(cli_data_object_t *cli_data)
{
	int log_level = (int)cli_data->log_level.level;
	PythonMulticastDataHandlerLogLevel = log_level > PythonDataHandlerLogLevel? log_level:PythonDataHandlerLogLevel;
	log_info(CliLogLevel, "PythonMulticastDataHandlerLogLevel set to %d",
	         PythonMulticastDataHandlerLogLevel);
	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * address_resolution_show                                                *//**
 *
 * \brief - Shows details of Multicast Addresses in a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status address_resolution_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_ADDRESS_RESOLUTION_SHOW;
	data_op.domain_add.domain_id = cli_data->domain_add.domain_id;

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * mass_transfer_get_ready                                                *//**
 *
 * \brief - Informs a Node to get ready for a domain
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status mass_transfer_get_ready(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_GET_READY;
	data_op.domain_mass_transfer_get_ready.domain_id = cli_data->domain_add.domain_id;

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;

}

/*
 ******************************************************************************
 * mass_transfer_start                                                    *//**
 *
 * \brief - Informs a Node to start transferring domain data to another node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status mass_transfer_start(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_MASS_TRANSFER_START;
	data_op.domain_mass_transfer_start.domain_id = cli_data->domain_transfer.domain_id;
	data_op.domain_mass_transfer_start.dps_server.family = cli_data->domain_transfer.IP_type;
	data_op.domain_mass_transfer_start.dps_server.port = 0;
	memcpy(data_op.domain_mass_transfer_start.dps_server.ip6,
	       cli_data->domain_transfer.IPv6, 16);

	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;

}

/*
 ******************************************************************************
 * mass_transfer_domain_activate                                          *//**
 *
 * \brief - Informs a Node to activate the Dove in Node
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status mass_transfer_domain_activate(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");

	data_op.type = DPS_CONTROLLER_DOMAIN_ACTIVATE;
	data_op.domain_add.domain_id = cli_data->domain_update.domain_id;
	data_op.domain_add.replication_factor = cli_data->domain_update.replication_factor;

	status = dps_controller_data_msg(&data_op);
	if (status == DOVE_STATUS_OK)
	{
		dps_cluster_exchange_domain_mapping_activate(&dcs_local_ip);
	}

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;

}

static dove_status vm_migrate_hint(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_client_data_t dps_msg;
	dps_return_status dps_status;

	log_info(CliLogLevel, "Enter");

	dps_msg.context = NULL;
	dps_msg.hdr.resp_status = DPS_NO_ERR;
	dps_msg.hdr.vnid = cli_data->endpoint_migrate.vnid;
	dps_msg.hdr.query_id = gquery_id++;
	dps_msg.hdr.type = DPS_VM_MIGRATION_EVENT;
	dps_msg.hdr.client_id = DPS_SWITCH_AGENT_ID;
	dps_msg.hdr.transaction_type = DPS_TRANSACTION_NORMAL;
	dps_msg.hdr.sub_type = 0;
	// Set the DPS Client as local
	memcpy(&dps_msg.hdr.reply_addr, &dcs_local_ip, sizeof(ip_addr_t));
	if(dps_msg.hdr.reply_addr.family == AF_INET)
	{
		dps_msg.hdr.reply_addr.ip4 = ntohl(dps_msg.hdr.reply_addr.ip4);
	}
	//Copy the migrated VM information
	memset(&dps_msg.vm_migration_event.migrated_vm_info, 0, sizeof(dps_endpoint_info_t));
	memcpy(dps_msg.vm_migration_event.migrated_vm_info.vm_ip_addr.ip6,
	       cli_data->endpoint_migrate.dst_vIPv6, 16);
	dps_msg.vm_migration_event.migrated_vm_info.vm_ip_addr.family =
		cli_data->endpoint_migrate.dst_vIP_type;
	if (dps_msg.vm_migration_event.migrated_vm_info.vm_ip_addr.family == AF_INET)
	{
		dps_msg.vm_migration_event.migrated_vm_info.vm_ip_addr.ip4 =
			ntohl(dps_msg.vm_migration_event.migrated_vm_info.vm_ip_addr.ip4);
	}
	//Copy the source VM information
	memset(&dps_msg.vm_migration_event.src_vm_loc, 0 , sizeof(dps_endpoint_loc_reply_t));
	memcpy(dps_msg.vm_migration_event.src_vm_loc.vm_ip_addr.ip6,
	       cli_data->endpoint_migrate.src_vIPv6, 16);
	dps_msg.vm_migration_event.src_vm_loc.vm_ip_addr.family =
		cli_data->endpoint_migrate.src_vIP_type;
	if (dps_msg.vm_migration_event.src_vm_loc.vm_ip_addr.family == AF_INET)
	{
		dps_msg.vm_migration_event.src_vm_loc.vm_ip_addr.ip4 =
			ntohl(dps_msg.vm_migration_event.src_vm_loc.vm_ip_addr.ip4);
	}
	//Copy the source tunnel information
	dps_msg.vm_migration_event.src_vm_loc.tunnel_info.num_of_tunnels = 1;
	memcpy(dps_msg.vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].ip6,
	       cli_data->endpoint_migrate.src_pIPv6, 16);
	dps_msg.vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].family =
		cli_data->endpoint_migrate.src_pIP_type;
	if (dps_msg.vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].family == AF_INET)
	{
		dps_msg.vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].ip4 =
			ntohl(dps_msg.vm_migration_event.src_vm_loc.tunnel_info.tunnel_list[0].ip4);
	}

	// Retry till the right version is reached (max 8)
	dps_status = dps_protocol_send_to_server(&dps_msg);

	if (dps_status != DPS_SUCCESS)
	{
		status = DOVE_STATUS_INVALID_PARAMETER;
	}
	log_info(CliLogLevel, "Exit: status:%s", DOVEStatusToString(status));
	return status;

}

static dove_status dps_clients_show(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	dps_controller_data_op_t data_op;

	log_info(CliLogLevel, "Enter");
	data_op.type = DPS_CONTROLLER_DCS_CLIENTS_SHOW;
	status = dps_controller_data_msg(&data_op);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}

/*
 ******************************************************************************
 * cli_data_object_callback                                               *//**
 *
 * \brief - The callback for CLI_DATA_OBJECTS
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_data_object_callback(void *cli)
{
	cli_data_object_t *cli_data = (cli_data_object_t *)cli;
	cli_data_object_callback_func func;
	dove_status status = DOVE_STATUS_NOT_SUPPORTED;

	log_debug(CliLogLevel, "Enter");

	if (cli_data->cli_data_object_code < CLI_DATA_OBJECTS_MAX)
	{
		func = cli_callback_array[cli_data->cli_data_object_code];
		if (func)
		{
			status = func(cli_data);
		}
	}

	log_debug(CliLogLevel, "Exit: %s", DOVEStatusToString(status));

	return status;

}


/*
 ******************************************************************************
 * heartbeat_interval_set                                                             *//**
 *
 * \brief - Set heartbeat interval, 0 to STOP
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status heartbeat_interval_set(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	uint32_t interval;

	log_info(CliLogLevel, "Enter");

	interval = cli_data->heartbeat_interval.interval;
	set_heartbeat_interval(interval);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}


/*
 ******************************************************************************
 * statistics_interval_set                                                             *//**
 *
 * \brief - Set statistics interval, 0 to STOP
 *
 * \return dove_status
 *
 ******************************************************************************
 */
static dove_status statistics_interval_set(cli_data_object_t *cli_data)
{
	dove_status status = DOVE_STATUS_OK;
	uint32_t interval;

	log_info(CliLogLevel, "Enter");

	interval = cli_data->statistics_interval.interval;
	set_statistics_interval(interval);

	log_info(CliLogLevel, "Exit status:%s", DOVEStatusToString(status));
	return status;
}


/*
 ******************************************************************************
 * cli_data_object_init                                        *//**
 *
 * \brief - Initializes the CLI_DATA_OBJECTS related callbacks
 *
 * \return dove_status
 *
 ******************************************************************************
 */
dove_status cli_data_object_init(void)
{

	dove_status status = DOVE_STATUS_OK;

	log_debug(CliLogLevel, "Enter");

	// Initialize the CLI_DATA_OBJECTS callbacks here

	cli_callback_array[CLI_DATA_OBJECTS_DEV_LOG_LEVEL] = log_level;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_ADD] = domain_add;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_DELETE] = domain_delete;
	cli_callback_array[CLI_DATA_OBJECTS_DVG_ADD] = dvg_add;
	cli_callback_array[CLI_DATA_OBJECTS_DVG_DELETE] = dvg_delete;
	cli_callback_array[CLI_DATA_OBJECTS_ENDPOINT_UPDATE] = endpoint_update;
	cli_callback_array[CLI_DATA_OBJECTS_ENDPOINT_LOOKUP_MAC] = endpoint_lookup_mac;
	cli_callback_array[CLI_DATA_OBJECTS_ENDPOINT_LOOKUP_IP] = endpoint_lookup_ip;
	cli_callback_array[CLI_DATA_OBJECTS_POLICY_LOOKUP_VMAC] = policy_lookup_mac;
	cli_callback_array[CLI_DATA_OBJECTS_POLICY_LOOKUP_IP] = policy_lookup_ip;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_SHOW] = domain_show;
	cli_callback_array[CLI_DATA_OBJECTS_VNID_SHOW] = vnid_show;
	cli_callback_array[CLI_DATA_OBJECTS_POLICY_ADD] = policy_add;
	cli_callback_array[CLI_DATA_OBJECTS_POLICY_DELETE] = policy_delete;
	cli_callback_array[CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_ADD] = gateway_update;
	cli_callback_array[CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_DEL] = gateway_update;
	cli_callback_array[CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_CLR] = gateway_clear;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_CREATE] = NULL;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_ADD_SUBNET] = ip_subnet_add;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_DELETE_SUBNET] = ip_subnet_delete;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_LOOKUP] = ip_subnet_lookup;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_DESTROY] = NULL;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_LIST] = ip_subnet_list;
	cli_callback_array[CLI_DATA_OBJECTS_IP_SUBNET_FLUSH] = ip_subnet_flush;
	cli_callback_array[CLI_DATA_OBJECTS_ENDPOINT_UPDATE_CONFLICT_TEST] = endpoint_conflict_test;
	cli_callback_array[CLI_DATA_OBJECTS_OUTSTANDING_CONTEXT_COUNT] = outstanding_context_show;
	cli_callback_array[CLI_DATA_OBJECTS_QUERY_VNID] = query_vnid;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_GLOBAL_SHOW] = domain_global_show;
	cli_callback_array[CLI_DATA_OBJECTS_TUNNEL_REGISTER] = tunnel_register;
	cli_callback_array[CLI_DATA_OBJECTS_TUNNEL_UNREGISTER] = tunnel_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_RECEIVER_ADD] = multicast_receiver_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_RECEIVER_DEL] = multicast_receiver_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_SENDER_ADD] = multicast_sender_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_SENDER_DEL] = multicast_sender_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_SHOW] = multicast_show;
	cli_callback_array[CLI_DATA_OBJECTS_ADDRESS_RESOLUTION_SHOW] = address_resolution_show;
	cli_callback_array[CLI_DATA_OBJECTS_EXTERNAL_GATEWAY_GET] = gateway_get;
	cli_callback_array[CLI_DATA_OBJECTS_VLAN_GATEWAY_GET] = gateway_get;
	cli_callback_array[CLI_DATA_OBJECTS_HEARTBEAT_REPORT_INTERVAL_SET] = heartbeat_interval_set;
	cli_callback_array[CLI_DATA_OBJECTS_STATISTICS_REPORT_INTERVAL_SET] = statistics_interval_set;	
	cli_callback_array[CLI_DATA_OBJECTS_MASS_TRANSFER_GET_READY] = mass_transfer_get_ready;
	cli_callback_array[CLI_DATA_OBJECTS_MASS_TRANSFER_START] = mass_transfer_start;
	cli_callback_array[CLI_DATA_OBJECTS_MASS_TRANSFER_DOMAIN_ACTIVATE] = mass_transfer_domain_activate;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_DEACTIVATE] = domain_deactivate;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_LOG_LEVEL] = multicast_log_level;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_UPDATE] = domain_update;
	cli_callback_array[CLI_DATA_OBJECTS_LOAD_BALANCE_TEST] = NULL;
	cli_callback_array[CLI_DATA_OBJECTS_DOMAIN_DELETE_ALL_LOCAL] = domains_clear;
	cli_callback_array[CLI_DATA_OBJECTS_ENDPOINT_MIGRATE_HINT] = vm_migrate_hint;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD] = multicast_receiver_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL] = multicast_receiver_register;
	cli_callback_array[CLI_DATA_OBJECTS_MULTICAST_RECEIVER_GLOBAL_SCOPE_GET] = multicast_control_get;
	cli_callback_array[CLI_DATA_OBJECTS_DPS_CLIENTS_SHOW] = dps_clients_show;
	log_debug(CliLogLevel, "Exit");

	return status;
}
/** @} */

