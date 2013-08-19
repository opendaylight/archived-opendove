/*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Header File:
 *      include.h
 *
 *  Abstract:
 *      The main header file for the DPS Server. All DPS Server sources and header
 *      files should include only include this file. This file will resolve all
 *      dependencies.
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

#ifndef _DPS_INCLUDE_H_
#define _DPS_INCLUDE_H_

#define LOG_TEXT "DOVE-DPS-SERVER: "


// Including local features.h since /usr/include/features.h on 9.43.95.197 
// is incompatible with python2.6 compilation flags.

// Python Headers
#include <Python.h>
// #include "features.h"

// Standard C Headers
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <assert.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>
#include "status.h"
#include "log.h"
#include "fd_process_public.h"
#include "dps_client_common.h"
#include "dps_pkt.h"
#include "dps_pkt_svr.h"
#include "python_interface.h"
#include "client_protocol_interface.h"
#include "controller_interface.h"

#define dps_offsetof(_type, _member) ((size_t) &((_type *)0)->_member)

/**
 * \brief Contains the Local IP address of the DPS Node
 */
extern ip_addr_t dps_local_ip;

/**
 * \brief The DPS REST Services Port
 */
extern short dps_rest_port;

/**
 * \brief Contains the IP address of the DPS Cluster Leader
 */
extern ip_addr_t controller_location;

/**
 * \brief Whether the controller location was configured by user
 */
extern int controller_location_set;

/**
 * \brief Contains the IP address of DPS Cluster Leader in Readable
 *        string format
 */
extern char *controller_location_ip_string;

/**
 * \brief Contains the DSA version string
 */
extern char *dsa_version_string;

/**
 * \brief Contains the Local IP Address
 */
extern char *dps_local_ip_string;

/**
 * \brief Contains the IP address of the DPS Cluster Leader
 */
extern ip_addr_t dps_cluster_leader;

/**
 * \brief Contains the IP address of DPS Cluster Leader in Readable
 *        string format
 */
extern char *dps_cluster_leader_ip_string;

/*
 ******************************************************************************
 * dps_set_service_role --                                                *//**
 *
 * \brief This routine starts or stops the DCS service role
 *
 * \param action 1:Activate/Set Local Node, 0:Reset Local Node
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status dps_set_service_role(uint32_t action);

/*
 ******************************************************************************
 * dps_initialize                                                         *//**
 *
 * \brief - Initializes the DPS Server
 *
 * \param[in] udp_port - The UDP Port to run the DPS Server on
 * \param[in] rest_port - The port the REST Services should run on
 * \param[in] python_path - The Location of the Python Scripts. This should be
 *                          NULL in most cases since the code will assume the
 *                          scripts are in the "." directory (i.e. the same
 *                          directory) as the dcslib module.
 *
 * \retval -1: Should never happen - This is an infinite loop.
 *
 ******************************************************************************
 */
int dps_initialize(int udp_port, int rest_port, char *python_path);

/*
 ******************************************************************************
 * print_console --                                                       *//**
 *
 * \brief This routine prints the message to the console. This routine should
 *        be called to print messages to the console.
 *
 * \param output_string - The String To Print
 *
 * \remarks DO NOT log in this routine. Will cause infinite loop :)
 *
 *****************************************************************************/

void print_console(const char *output_string);


#endif /* _DPS_INCLUDE_H_ */
