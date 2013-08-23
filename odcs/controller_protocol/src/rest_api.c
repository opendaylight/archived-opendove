/******************************************************************************
** File Main Owner:   DOVE DPS Development Team
** File Description:  Rest Client Message Creation
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
*  $Log: rest_api.c $
*  $EndLog$
*
*  PORTING HISTORY
*
}
*/
#include "include.h"
#include <jansson.h>
#include "../inc/rest_api.h"


#define IS_DVG_ID_VALID(_dvg) (((_dvg)>=0)&&((_dvg)<=16777215))
#define IS_DOMAIN_ID_VALID(_id) (((_id)>=0)&&((_id)<=16777215))

dove_status dps_rest_api_domain_validate(unsigned int domain_id)
{
	dps_controller_data_op_t data_op;

	if(IS_DOMAIN_ID_VALID(domain_id))
	{
		data_op.type = DPS_CONTROLLER_DOMAIN_VALIDATE;
		data_op.domain_validate.domain_id = domain_id;
		return dps_controller_data_msg(&data_op);
	}
	return DOVE_STATUS_INVALID_DOMAIN;
}

dove_status dps_rest_api_create_domain(unsigned int domain_id, unsigned int replication_factor)
{
	dps_controller_data_op_t data_op;

	if(!IS_DOMAIN_ID_VALID(domain_id))
	{
		return DOVE_STATUS_INVALID_DOMAIN;
	}

	data_op.type = DPS_CONTROLLER_DOMAIN_ADD;
	data_op.domain_add.domain_id = domain_id;
	data_op.domain_add.replication_factor = replication_factor;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_del_domain(unsigned int domain_id)
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_DOMAIN_DELETE;
	data_op.domain_delete.domain_id = domain_id;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_create_dvg(unsigned int domain_id,
                                    unsigned int dvg_id,
                                    dps_controller_data_op_enum_t method)
{
	if(!IS_DVG_ID_VALID(dvg_id))
	{
		return DOVE_STATUS_INVALID_DVG;
	}
	dps_controller_data_op_t data_op;
	data_op.type = method;
	data_op.dvg_add.domain_id = domain_id;
	data_op.dvg_add.dvg_id = dvg_id;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_del_dvg(unsigned int domain_id, unsigned int dvg_id)
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_DVG_DELETE;
	data_op.dvg_delete.dvg_id = dvg_id;
	return dps_controller_data_msg(&data_op);
}

json_t *dps_rest_api_get_domain(unsigned int domain_id)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;
	char domain_uri[DPS_URI_LEN];
	char dvgs_uri[DPS_URI_LEN];
	char policies_uri[DPS_URI_LEN];

	data_op.type = DPS_CONTROLLER_DOMAIN_VALIDATE;
	data_op.domain_validate.domain_id = domain_id;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		/* {
		"id":12345,
		"uri": "/api/dps/domains/12345",
		"DVGsUri": "/api/dps/domains/12345/dvgs"
		"PoliciesUri": "/api/dps/domains/12345/policies"
		} */
		DPS_DOMAIN_URI_GEN(domain_uri, domain_id);
		DPS_DVGS_URI_GEN(dvgs_uri, domain_id);
		DPS_POLICIES_URI_GEN(policies_uri, domain_id);
		js_root = json_pack("{s:i, s:s, s:s, s:s}", 
		                    "id", (int)domain_id,
		                    "uri", domain_uri,
		                    "DVGsUri", dvgs_uri,
		                    "PoliciesUri", policies_uri);

	}
	return js_root;
}

json_t *dps_rest_api_get_domain_all()
{
	json_t *js_domains = NULL;
	json_t *js_domain = NULL;
	json_t *js_root = NULL;
	char *saveptr = NULL;
	dps_controller_data_op_t data_op;
	char *tok = NULL;
	int domain_id;

	js_domains = json_array();

	data_op.type = DPS_CONTROLLER_DOMAIN_GETALLIDS;
	data_op.return_val = NULL;
	dps_controller_data_msg(&data_op);

	if (NULL != data_op.return_val)
	{
		tok = strtok_r((char *)data_op.return_val, ",", &saveptr);
		while (NULL != tok)
		{
			domain_id = atoi(tok);
			js_domain = dps_rest_api_get_domain((unsigned int)domain_id);
			if (NULL != js_domain)
			{
				json_array_append_new(js_domains, js_domain);
			}
			tok = strtok_r(NULL, ",", &saveptr);
		}
		free(data_op.return_val);
	}
	js_root = json_pack("{s:o}", "domains", js_domains);
	return js_root;
}

json_t *dps_rest_api_get_dvg(unsigned int domain_id, unsigned int dvg_id)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;
	char dvg_uri[DPS_URI_LEN];
	char ipv4subnets_uri[DPS_URI_LEN];
	char ipv4extgateway_uri[DPS_URI_LEN];

	data_op.type = DPS_CONTROLLER_DVG_VALIDATE;
	data_op.dvg_validate.domain_id = domain_id;
	data_op.dvg_validate.dvg_id = dvg_id;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		/* {
			"id":0,
		        "uri":"/api/dps/domains/12345/dvgs/0" 
		        "IPv4SubnetsUri":"/api/dove/dps/vns/789/ipv4-subnets" 
			}*/
		DPS_DVG_URI_GEN(dvg_uri, domain_id, dvg_id);
		DPS_DVG_IPV4SUBNETS_URI_GEN(ipv4subnets_uri, dvg_id);
		DPS_EXTERNAL_GATEWAYS_URI_GEN(ipv4extgateway_uri, dvg_id);
		js_root = json_pack("{s:i, s:s, s:s, s:s}", 
		                    "id", (int)dvg_id,
		                    "uri", dvg_uri, 
		                    "IPv4SubnetsUri", ipv4subnets_uri,
		                    "IPv4ExternalGatewaysUri", ipv4extgateway_uri);
	}
	return js_root;
}

json_t *dps_rest_api_get_dvg_all(unsigned int domain_id)
{
	json_t *js_dvgs = NULL;
	json_t *js_dvg = NULL;
	json_t *js_root = NULL;
	char *saveptr = NULL;
	dps_controller_data_op_t data_op;
	char *tok = NULL;
	int dvg_id;

	js_dvgs = json_array();

	data_op.type = DPS_CONTROLLER_DVG_GETALLIDS;
	data_op.dvg_getallids.domain_id = domain_id;
	data_op.return_val = NULL;
	dps_controller_data_msg(&data_op);

	if (NULL != data_op.return_val)
	{
		tok = strtok_r((char *)data_op.return_val, ",", &saveptr);
		while (NULL != tok)
		{
			dvg_id = atoi(tok);
			js_dvg = dps_rest_api_get_dvg(domain_id, (unsigned int)dvg_id);
			if (NULL != js_dvg)
			{
				json_array_append_new(js_dvgs, js_dvg);
			}
			tok = strtok_r(NULL, ",", &saveptr);
		}
		free(data_op.return_val);
	}
	js_root = json_pack("{s:o}", "dvgs", js_dvgs);
	return js_root;
}

json_t *dps_rest_api_get_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;
	char policy_uri[DPS_URI_LEN];

	data_op.type = DPS_CONTROLLER_POLICY_GET;
	/* Use the same structure as DPS_CONTROLLER_POLICY_ADD */
	data_op.policy_add.traffic_type = traffic_type;
	data_op.policy_add.domain_id = domain_id;
	data_op.policy_add.dst_dvg_id = dst_dvg;
	data_op.policy_add.src_dvg_id = src_dvg;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		/* {
		 "traffic_type":0,
		 "type":1,
		 "src_dvg":0,
		 "dst_dvg":1,
		 "ttl":120,
		 "action":1, 
		 "version":1 
		 "uri":" /api/dps/domains/12345/policies/0000-0001-0"
		} */
		DPS_POLICY_URI_GEN(policy_uri, domain_id, src_dvg, dst_dvg, traffic_type);
		if(DPS_POLICY_TYPE_CONN == data_op.policy_add.type)
		{
			js_root = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:s}", 
					    "traffic_type", data_op.policy_add.traffic_type,
			                    "type", data_op.policy_add.type,
			                    "src_dvg", data_op.policy_add.src_dvg_id,
			                    "dst_dvg", data_op.policy_add.dst_dvg_id,
			                    "ttl", data_op.policy_add.ttl,
			                    "action", (unsigned int)data_op.policy_add.action.connectivity,
			                    "version", data_op.policy_add.version,
			                    "uri", policy_uri);
		}
		else
		{
			/* TODO: support for other policy type */
		}

	}
	return js_root;
}

dove_status dps_rest_api_del_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_POLICY_DELETE; 
	data_op.policy_delete.traffic_type = traffic_type;
	data_op.policy_delete.domain_id = domain_id;
	data_op.policy_delete.src_dvg_id = src_dvg;
	data_op.policy_delete.dst_dvg_id = dst_dvg;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_create_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type, unsigned int type, unsigned int ttl, dps_object_policy_action_t *action)
{
	dps_controller_data_op_t data_op;

	if(DPS_POLICY_TYPE_CONN == type)
	{
		data_op.type = DPS_CONTROLLER_POLICY_ADD;
		data_op.policy_add.traffic_type = traffic_type;
		data_op.policy_add.domain_id = domain_id;
		data_op.policy_add.type = type;
		data_op.policy_add.src_dvg_id = src_dvg;
		data_op.policy_add.dst_dvg_id = dst_dvg;
		data_op.policy_add.ttl = ttl;
		memcpy(&(data_op.policy_add.action), action, sizeof(dps_object_policy_action_t));
	}
	else
	{
		/* TODO: support for other policy type */
		return DOVE_STATUS_NOT_SUPPORTED;
	}
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_create_policy_from_jdata(unsigned int domain_id, json_t *data, unsigned int *src_dvg, unsigned int *dst_dvg, unsigned int *traffic_type)
{
	json_t *js_tok;
	unsigned int type;
	unsigned int ttl;
	dove_status status = DOVE_STATUS_INVALID_PARAMETER;
	dps_object_policy_action_t action;

	memset(&action, 0, sizeof(dps_object_policy_action_t));
	do
	{
		/* Borrowed reference, no need to decref */
		js_tok = json_object_get(data, "type");
		if (NULL == js_tok || !json_is_integer(js_tok))
		{
			break;
		}
		type = (unsigned int)json_integer_value(js_tok);
		if(DPS_POLICY_TYPE_CONN == type)
		{
			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "src_dvg");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				break;
			}
			*src_dvg = (unsigned int)json_integer_value(js_tok);

			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "dst_dvg");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				break;
			}
			*dst_dvg = (unsigned int)json_integer_value(js_tok);

			if(!IS_DVG_ID_VALID(*src_dvg)||!IS_DVG_ID_VALID(*dst_dvg))
			{
				break;
			}

			js_tok = json_object_get(data, "traffic_type");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				break;
			}
			*traffic_type = (unsigned int)json_integer_value(js_tok);
			if ((*traffic_type != DPS_POLICY_TRAFFIC_UNICAST) && 
			    (*traffic_type != DPS_POLICY_TRAFFIC_MULTICAST))
			{
				break;
			}

			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "ttl");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				break;
			}
			ttl = (unsigned int)json_integer_value(js_tok);

			/* Borrowed reference, no need to decref */
			js_tok = json_object_get(data, "action");
			if (NULL == js_tok || !json_is_integer(js_tok))
			{
				break;
			}
			action.connectivity = (unsigned short)json_integer_value(js_tok);
			action.connectivity = (action.connectivity == DPS_CONNECTIVITY_DROP)?DPS_CONNECTIVITY_DROP:DPS_CONNECTIVITY_ALLOW;
			action.ver = 1;
			status = dps_rest_api_create_policy(domain_id, *src_dvg, *dst_dvg, *traffic_type, type, ttl, &action);
		}
		else
		{
			/* TODO: support for other policy type */
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}
	} while (0);
	return status;
}

dove_status dps_rest_api_update_policy(unsigned int domain_id, unsigned int src_dvg, unsigned int dst_dvg, unsigned int traffic_type, json_t *data)
{
	dps_controller_data_op_t data_op;
	json_t *js_type;
	json_t *js_ttl;
	json_t *js_action;
	unsigned int type;
	unsigned int ttl;
	dps_object_policy_action_t action;

	memset(&action, 0, sizeof(action));

	data_op.type = DPS_CONTROLLER_POLICY_GET;
	/* Use the same structure as DPS_CONTROLLER_POLICY_ADD */
	data_op.policy_add.traffic_type = traffic_type;
	data_op.policy_add.domain_id = domain_id;
	data_op.policy_add.dst_dvg_id = dst_dvg;
	data_op.policy_add.src_dvg_id = src_dvg;
	if (DOVE_STATUS_OK != dps_controller_data_msg(&data_op))
	{
		return DOVE_STATUS_INVALID_POLICY;
	}

	type = data_op.policy_add.type;
	ttl = data_op.policy_add.ttl;
	memcpy(&action, &(data_op.policy_add.action), sizeof(action));

	/* Borrowed reference, no need to decref */
	js_type = json_object_get(data, "type");
	if (NULL != js_type)
	{
		if(json_is_integer(js_type))
		{
			type = (unsigned int)json_integer_value(js_type);
		}
		else
		{
			return DOVE_STATUS_INVALID_PARAMETER;
		}
	}

	/* Borrowed reference, no need to decref */
	js_ttl = json_object_get(data, "ttl");
	if (NULL != js_ttl)
	{
		if(json_is_integer(js_ttl))
		{
			ttl = (unsigned int)json_integer_value(js_ttl);
		}
		else
		{
			return DOVE_STATUS_INVALID_PARAMETER;
		}
	}

	if (DPS_POLICY_TYPE_CONN == type &&  DPS_POLICY_TYPE_CONN == data_op.policy_add.type)
	{
		/* Borrowed reference, no need to decref */
		js_action = json_object_get(data, "action");
		if (NULL != js_action)
		{
			if(json_is_integer(js_action))
			{
				action.connectivity = (unsigned int)json_integer_value(js_action);
				action.connectivity = (action.connectivity == DPS_CONNECTIVITY_DROP)?DPS_CONNECTIVITY_DROP:DPS_CONNECTIVITY_ALLOW;
			}
			else
			{
				return DOVE_STATUS_INVALID_PARAMETER;
			}
		}
		return dps_rest_api_create_policy(domain_id, src_dvg, dst_dvg, traffic_type, type, ttl, &action);
	}
	else
	{
		/* TODO: support for other policy type */
	}
	return DOVE_STATUS_NOT_SUPPORTED;
}


json_t *dps_rest_api_get_policies_all(unsigned int domain_id)
{
	json_t *js_policies[2] = {NULL, NULL};
	json_t *js_policy = NULL;
	json_t *js_root = NULL;
	char *saveptr = NULL;
	char *saveptr1 = NULL;
	char *p = NULL;
	char *endptr = NULL;
	dps_controller_data_op_t data_op;
	char *tok = NULL;
	unsigned int src_dvg;
	unsigned int dst_dvg;
	unsigned int traffic_type;
	unsigned int i;

	for (traffic_type = DPS_POLICY_TRAFFIC_UNICAST, i = 0; 
	     traffic_type <= DPS_POLICY_TRAFFIC_MULTICAST; 
	     traffic_type++, i++)
	{
		js_policies[i] = json_array();

		data_op.type = DPS_CONTROLLER_POLICY_GETALLIDS;
		/* Use the same structure as DPS_CONTROLLER_POLICY_ADD, only use domain_id field */
		data_op.policy_add.traffic_type = traffic_type;
		data_op.policy_add.domain_id = domain_id;
		data_op.return_val = NULL;
		dps_controller_data_msg(&data_op);

		if (NULL != data_op.return_val)
		{
			tok = strtok_r((char *)data_op.return_val, ",", &saveptr);
			saveptr1 = NULL;
			while (NULL != tok)
			{
				/* Decode SRC DVG */
				p = strtok_r(tok, ":", &saveptr1);
				if(NULL == p)
				{
					continue;
				}
				src_dvg = strtoul(p, &endptr, 10);
				if (*endptr != '\0')
				{
					continue;
				}
				/* Decode DST DVG */
				p = strtok_r(NULL, ":", &saveptr1);
				if(NULL == p)
				{
					continue;
				}
				dst_dvg = strtoul(p, &endptr, 10);
				if (*endptr != '\0')
				{
					continue;
				}
				js_policy = dps_rest_api_get_policy((unsigned int)domain_id, src_dvg, dst_dvg, traffic_type);
				if (NULL != js_policy)
				{
					json_array_append_new(js_policies[i], js_policy);
				}
				tok = strtok_r(NULL, ",", &saveptr);
			}
			free(data_op.return_val);
		}
	}

	js_root = json_pack("{s:o, s:o}", 
			    "unicast policies", js_policies[0],
			    "multicast policies", js_policies[1]);
	return js_root;
}

unsigned int v4_addr_to_ip_id (char *str, unsigned int addr)
{
	int ret = -1;

	if (NULL != str)
	{
		sprintf(str, "%02x%02x%02x%02x", ((addr >> 24) & 0x00FF),
		        ((addr >> 16) & 0x00FF), ((addr >> 8) & 0x00FF),
		        (addr & 0x00FF));
		ret = 0;
	}

	return ret;
}

json_t *dps_rest_api_get_gateway_all(unsigned int vn_id)
{
	json_t *js_egs = NULL;
	json_t *js_eg = NULL;
	json_t *js_root = NULL;
	char *saveptr = NULL;
	dps_controller_data_op_t data_op;
	char *tok = NULL;
	
	struct in_addr v4_addr;

	js_egs = json_array();
	data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_GETALLIDS;
	data_op.gateway_update.vnid = vn_id;
	data_op.gateway_update.IP_type = AF_INET;
	data_op.return_val = NULL;
	dps_controller_data_msg(&data_op);

	if (NULL != data_op.return_val)
	{
		tok = strtok_r((char *)data_op.return_val, ",", &saveptr);
		while (NULL != tok)
		{
			while (' ' == *tok)
			{
				tok++;
				continue;
			}
		
			if (0 != inet_pton(AF_INET, tok, (void *)&v4_addr))
			{
				js_eg = dps_rest_api_get_gateway (vn_id, v4_addr.s_addr);
				if (NULL != js_eg)
				{
					json_array_append_new(js_egs, js_eg);
				}
			}
			tok = strtok_r(NULL, ",", &saveptr);
		}
		free(data_op.return_val);
	}
	js_root = json_pack("{s:o}", "ipv4-external-gateways", js_egs);
	return js_root;
}

dove_status dps_rest_api_create_gateway(unsigned int vn_id, unsigned int gateway_id)
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_ADD;
	data_op.gateway_update.vnid = vn_id;
	data_op.gateway_update.IP_type = AF_INET;
	data_op.gateway_update.IPv4 = gateway_id;
	return dps_controller_data_msg(&data_op);
}

json_t *dps_rest_api_get_gateway(unsigned int vn_id, unsigned int gateway_id)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;
	char eg_uri[DPS_URI_LEN];
	char ip_id_str[16];
	char ip_str[16];
	struct in_addr ip;

	data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_VALIDATE;
	data_op.gateway_update.vnid = vn_id;
	data_op.gateway_update.IP_type = AF_INET;
	data_op.gateway_update.IPv4 = gateway_id;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{	
		if (0 == v4_addr_to_ip_id(ip_id_str, gateway_id))
		{
			ip.s_addr = gateway_id;
			inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
			/*
			 {
			 "id":"c0a80101",
			 "gateway_ip":192.168.1.1,
			 "uri":"/api/dove/dps/vns/12345/ipv4-external-gateways/c0a80101"
			 },
			 */
			DPS_EXTERNAL_GATEWAY_URI_GEN(eg_uri, vn_id, gateway_id);
			js_root = json_pack("{s:i, s:s, s:s}", 
					    "id", gateway_id,
			                    "gateway_ip", ip_str, 
					    "uri", eg_uri);
		}

	}
	return js_root;
}

dove_status dps_rest_api_del_gateway_all(unsigned int vn_id)
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_CLR;
	data_op.gateway_update.vnid = vn_id;
	data_op.gateway_update.IP_type = AF_INET;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_del_gateway(unsigned int vn_id, unsigned int gateway_id)
{
	dps_controller_data_op_t data_op;
	data_op.type = DPS_CONTROLLER_EXTERNAL_GATEWAY_DEL;
	data_op.gateway_update.vnid = vn_id;
	data_op.gateway_update.IP_type = AF_INET;
	data_op.gateway_update.IPv4 = gateway_id;
	return dps_controller_data_msg(&data_op);
}

json_t *dps_rest_api_get_statistics_load_balancing(unsigned int domain_id)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_LOAD_BALANCING_GET;
	data_op.load_balancing.domain_id = domain_id;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		/* {
		        "sample_interval":600, 
		        "endpoint_update_count":1001,
		        "endpoint_lookup_count":2012,
		        "policy_lookup_count":1612,
		        "endpoints_count":21023 
		}*/
		js_root = json_pack("{s:i, s:i, s:i, s:i, s:i}", 
		                    "sample_interval", data_op.load_balancing.sample_interval,
		                    "endpoint_update_count", data_op.load_balancing.endpoint_update_count,
		                    "endpoint_lookup_count", data_op.load_balancing.endpoint_lookup_count,
		                    "policy_lookup_count", data_op.load_balancing.policy_lookup_count,
		                    "endpoints_count", data_op.load_balancing.endpoints_count);
	}
	return js_root;
}

json_t *dps_rest_api_get_statistics_general_statistics(unsigned int domain_id)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_GENERAL_STATISTICS_GET;
	data_op.general_statistics.domain_id = domain_id;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		/* {
			"endpoint_update_count":1001,
		        "endpoint_lookup_count":2012,
		        "policy_lookup_count":1612,
		        "multicast_lookup_count":21023 
		        "internal_gw_lookup_count":3489 
		        "endpoint_update_rate":13, 
		        "endpoint_lookup_rate":23, 
		        "policy_lookup_rate":32 
		}*/
		js_root = json_pack("{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i}", 
		                    "endpoint_update_count", data_op.general_statistics.counts.endpoint_update_count,
		                    "endpoint_lookup_count", data_op.general_statistics.counts.endpoint_lookup_count,
		                    "policy_lookup_count", data_op.general_statistics.counts.policy_lookup_count,
		                    "multicast_lookup_count", data_op.general_statistics.counts.multicast_lookup_count,
				    "internal_gw_lookup_count", data_op.general_statistics.counts.internal_gw_lookup_count,
				    "endpoint_update_rate", data_op.general_statistics.rates.endpoint_update_rate, 
				    "endpoint_lookup_rate", data_op.general_statistics.rates.endpoint_lookup_rate,
				    "policy_lookup_rate", data_op.general_statistics.rates.policy_lookup_rate);
	}
	return js_root;
}

dove_status dps_rest_api_create_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask, unsigned int mode, unsigned char *gateway)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_ADD;
	data_op.ip_subnet_add.associated_type = associated_type;
	data_op.ip_subnet_add.associated_id = associated_id;
	data_op.ip_subnet_add.IP_type = type;
	if (type == AF_INET)
	{
		memcpy(data_op.ip_subnet_add.IPv6, ip, 4);
		memcpy(data_op.ip_subnet_add.gateway_v6, gateway, 4);
	}
	else
	{
		memcpy(data_op.ip_subnet_add.IPv6, ip, 16);
		memcpy(data_op.ip_subnet_add.gateway_v6, gateway, 16);
		log_warn(RESTHandlerLogLevel, "IPv6 Mask not supported!");
	}
	data_op.ip_subnet_add.mask = mask;
	data_op.ip_subnet_add.mode = mode;
	return dps_controller_data_msg(&data_op);
}

dove_status dps_rest_api_del_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask)
{
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_DELETE;
	data_op.ip_subnet_delete.associated_type = associated_type;
	data_op.ip_subnet_delete.associated_id = associated_id;
	data_op.ip_subnet_delete.IP_type = type;
	if (type == AF_INET)
	{
		memcpy(data_op.ip_subnet_delete.IPv6, ip, 4);
	}
	else
	{
		memcpy(data_op.ip_subnet_delete.IPv6, ip, 16);
	}
	data_op.ip_subnet_delete.mask = mask;
	return dps_controller_data_msg(&data_op);
}

json_t *dps_rest_api_get_ipsubnet(unsigned int associated_type, unsigned int associated_id, unsigned int type, unsigned char *ip, unsigned int mask)
{
	json_t *js_root = NULL;
	dps_controller_data_op_t data_op;

	data_op.type = DPS_CONTROLLER_IP_SUBNET_GET;
	data_op.ip_subnet_get.associated_type = associated_type;
	data_op.ip_subnet_get.associated_id = associated_id;
	data_op.ip_subnet_get.IP_type = type;
	if (type == AF_INET)
	{
		memcpy(data_op.ip_subnet_get.IPv6, ip, 4);
	}
	else
	{
		memcpy(data_op.ip_subnet_get.IPv6, ip, 16);
	}
	data_op.ip_subnet_get.mask = mask;
	if (DOVE_STATUS_OK == dps_controller_data_msg(&data_op))
	{
		char ip_str[INET6_ADDRSTRLEN], mask_str[INET6_ADDRSTRLEN], gateway_str[INET6_ADDRSTRLEN];
		if (type == AF_INET6)
		{
			log_warn(RESTHandlerLogLevel, "IPv6 MASK not supported!!!");
		}
		inet_ntop(type, data_op.ip_subnet_get.IPv6, ip_str, INET6_ADDRSTRLEN);
		inet_ntop(type, &data_op.ip_subnet_get.mask, mask_str, INET6_ADDRSTRLEN);
		inet_ntop(type, data_op.ip_subnet_get.gateway_v6, gateway_str, INET6_ADDRSTRLEN);
		/* {
			"ip":"192.168.1.1",
			"mask":"255.255.255.0",
			"mode":"dedicated",
			"gateway":"192.168.1.254"
		}*/
		js_root = json_pack("{s:s, s:s, s:s, s:s}",
		                    "ip", ip_str,
		                    "mask", mask_str,
		                    "mode", data_op.ip_subnet_get.mode?"shared":"dedicated",
		                    "gateway", gateway_str);
	}
	return js_root;
}

json_t *dps_rest_api_get_ipsubnets_all(unsigned int associated_type, unsigned int associated_id, unsigned int IP_type)
{
	json_t *js_subnets = NULL;
	json_t *js_subnet = NULL;
	json_t *js_root = NULL;
	char *saveptr = NULL;
	char res_uri[DPS_URI_LEN];
	dps_controller_data_op_t data_op;
	char *tok = NULL;
	int len = 0;

	js_subnets = json_array();
	data_op.type = DPS_CONTROLLER_IP_SUBNET_GETALLIDS;
	data_op.ip_subnet_getallids.associated_type = associated_type;
	data_op.ip_subnet_getallids.associated_id = associated_id;
	data_op.ip_subnet_getallids.IP_type = IP_type;
	data_op.return_val = NULL;
	dps_controller_data_msg(&data_op);

	if (NULL != data_op.return_val)
	{
		tok = strtok_r((char *)data_op.return_val, " ", &saveptr);
		while (NULL != tok)
		{
			if (associated_type == IP_SUBNET_ASSOCIATED_TYPE_DOMAIN)
			{
				len = DPS_DOMAIN_IPV4SUBNETS_URI_GEN(res_uri, associated_id);
			}
			else
			{
				len = DPS_DVG_IPV4SUBNETS_URI_GEN(res_uri, associated_id);
			}
			sprintf(res_uri+len, "/%s", tok);
			js_subnet = json_pack("{s:s, s:s}", "id", tok,  "uri", res_uri);
			if (NULL != js_subnet)
			{
				json_array_append_new(js_subnets, js_subnet);
			}
			tok = strtok_r(NULL, " ", &saveptr);
		}
		free(data_op.return_val);
	}
	js_root = json_pack("{s:o}", "ipv4-subnets", js_subnets);
	return js_root;
}
