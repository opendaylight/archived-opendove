/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   heartbeat.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */

#ifndef _DOVE_DPS_HEARTBEAT_
#define _DOVE_DPS_HEARTBEAT_

/* define a variable to set to NOT send the heartbeat */
extern unsigned char dps_inter_node_heartbeat_send;
extern uint32_t dps_registration_fail_count;
extern int last_heartbeat_to_dmc_is_success;



dove_status dcs_heartbeat_init(char *pythonpath);

/*
 ******************************************************************************
 * dps_rest_heartbeat_send_to_dps_node --                                 *//**
 *
 * \brief This routine sends a REST heartbeat message to a remote DPS Node.
 *
 * \param dps_node: The location of the remote node
 * \param factive: Whether the Node is active
 * \param config_version: The configuration version as seen by this node
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_heartbeat_send_to_dps_node(ip_addr_t *dps_node,
                                         int factive,
                                         long long config_version);

/*
 ******************************************************************************
 * dps_rest_heartbeat_request_send_to_dps_node --                         *//**
 *
 * \brief This routine sends a REST heartbeat request message to a remote
 *        DPS Node. Typically initiated by the Leader when its unable to
 *        contact a node for a while.
 *
 * \param dps_node: The location of the remote node
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_heartbeat_request_send_to_dps_node(ip_addr_t *dps_node);

/*
 ******************************************************************************
 * dps_rest_nodes_status_send_to --                                       *//**
 *
 * \brief This routine sends a REST message containing this node's view of all
 *        other node statuses list to another dps node.
 *        Typically it's used by the leader to send it's view of the cluster
 *        to another node.
 *
 * \param dps_node: The location of the remote node
 * \param config_version: The configuration version as seen by this node
 * \param nodes_status: An array of node location and status
 * \param num_nodes: The number of nodes in the status
 *
 * \return void
 *
 *****************************************************************************/
void dps_rest_nodes_status_send_to(ip_addr_t *dps_node,
                                   long long config_version,
                                   ip_addr_t *nodes_status,
                                   int num_nodes);

int set_heartbeat_interval(int interval);
int get_heartbeat_interval(void);
int set_dps_appliance_registration_needed(unsigned char value);
unsigned char get_dps_appliance_registration_needed(void);

extern char uuid[];

extern long long local_config_version;
extern long long cluster_config_version;

#endif
