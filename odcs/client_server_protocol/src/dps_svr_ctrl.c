/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      dps_svr_ctrl.c
 *      This file has functions that deals with network communication between the dps server
 *      and the dps client
 *
 *  Author:
 *      Sushma Anantharam
 *
 */

#include "include.h"
#include "raw_proto_timer.h"
#include "dps_client_common.h"
#include "dps_pkt.h"
#include "dps_log.h"

int32_t DpsProtocolLogLevel = DPS_LOGLEVEL_NOTICE;
static int8_t buffer[DPS_MAX_BUFF_SZ];

/**
 * \brief The Server UDP Socket
 */
static int server_sock;

void dps_retransmit_callback(raw_proto_retransmit_status_t status, char *pRawPkt, 
                             void* context, rpt_owner_t owner);
/*
 *****************************************************************************
 * DPS Client Server Protocol                                            *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup DPSClientServerProtocol DPS Client Server Protocol
 * @{
 *
 * This module handles the DPS Client Server Protocol. Both the Client and
 * Server Processing is described and implemented in this module. Based on
 * how this module is exported, it can be used as a DPS Client or a DPS Server.
 * The DOVE Switches and Gateways MUST import this module as a DPS Client.
 * The DPS Server MUST import this module as a DPS Server.
 *
 * To enable DPS Server functionality the "DPS_SERVER" variable MUST be set
 * in the Makefile otherwise it will be assumed that the module is imported
 * as a DPS Client
 *
 */

/**
 * \brief The Log Level for Customer related logs
 */
int32_t DpsProtocolCustomerLogLevel = DPS_SERVER_LOGLEVEL_WARNING;

/*
 *****************************************************************************
 * DPS Server Protocol Handling                                          *//**
 *
 * \defgroup DPSServerProtocol DPS Client Server Protocol - Server Handling
 * @{
 *
 * This module handles the Server Part of the DPS Client Server Protocol
 *
 */

/*
 ******************************************************************************
 * dps_process_data_rcvd                                                  *//**
 *
 * \brief - Called by the polling thread that gets an indication that the data
 *          has arrived. The function calls the appropriate dps protocol
 *          function to parse the packet, by looking at the type of the pkt
 *          in the header
 *
 * \param[in] socket - The socket on which the information arrived
 * \param[in] context - An index value used to access the dps server information
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */

static int dps_process_data_rcvd(int socket, void *context)
{
	struct sockaddr_in client_addr;
	ip_addr_t sender_addr = {AF_INET, 0};
	int32_t bytes_read, addr_len = sizeof(struct sockaddr_in);
	int32_t loops = 0;

	dps_log_debug(DpsProtocolLogLevel,"Enter");

	// Loop forever till all UDP data is consumed
	do
	{

		memset(&client_addr, 0, sizeof(client_addr));

		bytes_read = recvfrom(socket, buffer, DPS_MAX_BUFF_SZ,
		                      0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);

		if(bytes_read <= 0)
		{
			if (loops == 0)
			{
				// Error in 1st recvfrom itself - Why did we get woken up???
				dps_log_error(DpsProtocolLogLevel, "[loop %d] recvfrom() error %s",
				              loops, strerror(errno));
			}
			else
			{
				// Normal
				dps_log_debug(DpsProtocolLogLevel, "[loop %d] recvfrom() error %s",
				              loops, strerror(errno));
			}
			break;
		}

		dps_log_debug(DpsProtocolLogLevel,"Msg from [%s:%d] bytes read %d",
		          inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), bytes_read);

		if (sender_addr.family == AF_INET)
		{
			sender_addr.ip4 = ntohl(client_addr.sin_addr.s_addr);
		}
		sender_addr.port = ntohs(client_addr.sin_port);
		dps_process_rcvd_pkt((void *)buffer,(void *)&sender_addr);

		loops++;
	}while(loops < 50000); // MAX 50,000 loops before releasing CPU???

	dps_log_debug(DpsProtocolLogLevel, "Exit");

	return DOVE_STATUS_OK;
}

/*
 ******************************************************************************
 * dps_protocol_xmit                                                      *//**
 *
 * \brief - This routine should to be called to send a Buffer/Packet to a
 *          Remote destination
 *
 * \param[in] buff - Pointer to a char buffer
 * \param[in] buff_len - Size of the Buffer
 * \param[in] addr - The Remote End to send the Buffer To
 *
 * \retval DPS_SUCCESS
 *
 ******************************************************************************
 */

uint32_t dps_protocol_xmit(uint8_t *buff, uint32_t buff_len, ip_addr_t *addr, void *context)
{


	int32_t ret_val = 0;
	struct sockaddr_in dst_addr;
	dps_client_hdr_t hdr;
	uint32_t status = DPS_SUCCESS;

	dst_addr.sin_family = AF_INET;
	dst_addr.sin_port = htons(addr->port);
	dst_addr.sin_addr.s_addr = htonl(addr->ip4);

	dps_log_debug(DpsProtocolLogLevel,"Send to: [%s:%d], context %p",
	              inet_ntoa(dst_addr.sin_addr), ntohs(dst_addr.sin_port), context);

	do
	{

		if (context)
		{
			dps_get_pkt_hdr((dps_pkt_hdr_t *)buff, &hdr);
			dps_log_debug(DpsProtocolLogLevel,"Starting Rexmit: Context %p, Pkt Type %d, QID %d",
			              context, hdr.type, hdr.query_id);
		}

		ret_val = sendto(server_sock, (void *)buff, buff_len, 0,
		                 (struct sockaddr *)&(dst_addr), sizeof(struct sockaddr));

		if (ret_val <= 0)
		{
			status = DPS_ERROR;
			dps_log_notice(DpsProtocolLogLevel, "sendto() error %s", strerror(errno));
		}
		else
		{
			status = DPS_SUCCESS;
			dps_log_debug(DpsProtocolLogLevel, "sendto() bytes_sent %d", ret_val);
		}
	}while(0);

	dps_log_debug(DpsProtocolLogLevel, "Exit: Status %d", status);

	return status;
}

/*
 ******************************************************************************
 * dps_svr_proto_init                                                     *//**
 *
 * \brief - This routine initializes the DPS Server part of the Protocol. This
 *          routine MUST be called before the DPS Client Server Protocol can
 *          become alive.
 *
 * \param[in] portnum - The UDP Port on which the DPS Server will be "listening"
 *
 * \retval DOVE_STATUS_OK Success
 * \retval DOVE_STATUS_NO_RESOURCES No Resources
 * \retval DOVE_STATUS_INVALID_FD Couldn't create UDP Socket
 * \retval DOVE_STATUS_BIND_FAILED Bind failed to the Provided Port
 * \retval DOVE_STATUS_NOT_SUPPORTED Cannot function in Non Blocking Mode
 *
 ******************************************************************************
 */

dove_status dps_svr_proto_init(uint32_t portnum)
{
	int32_t rc;
	struct sockaddr_in srvAddr;
	int core_status;
	size_t rcv_size;
	socklen_t rcv_size_len;
	dove_status status = DOVE_STATUS_NO_RESOURCES;

	do
	{
		dps_log_debug(DpsProtocolLogLevel,"dps_svr_proto_init");

		// Register timer callback
		rc = raw_proto_timer_init(&dps_retransmit_callback, RPT_OWNER_DPS);
		if (rc != RAW_PROTO_TIMER_RETURN_OK)
		{
			dps_log_error(DpsProtocolLogLevel, "raw_proto_timer_init %d\n",
			              rc);
			status = DOVE_STATUS_NO_RESOURCES;
			break;
		}

		if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			dps_log_error(DpsProtocolLogLevel, "socket() failed Error %s\n",
			              strerror(errno));
			status = DOVE_STATUS_INVALID_FD;
			break;
		}

		memset(&srvAddr, 0, sizeof(srvAddr));

		srvAddr.sin_family = AF_INET;
		srvAddr.sin_addr.s_addr = INADDR_ANY;
		srvAddr.sin_port = htons(portnum);

		if (bind(server_sock,(struct sockaddr *) &srvAddr,
		         sizeof(struct sockaddr)) == -1)
		{
			dps_log_error(DpsProtocolLogLevel, "bind() failed Error %s\n",
			              strerror(errno));
			status = DOVE_STATUS_BIND_FAILED;
			break;
		}

		if (getsockopt(server_sock, SOL_SOCKET, SO_RCVBUF, (void *)&rcv_size, &rcv_size_len) == 0)
		{
			dps_log_notice(DpsProtocolLogLevel,
			               "getsockopt SO_RCVBUF returns %d, rcv_size_len %d",
			               rcv_size, rcv_size_len);
		}
		else
		{
			dps_log_error(DpsProtocolLogLevel, "getsockopt SO_RCVBUF error %d", errno);
		}

		rcv_size = 1 << 26; // 64 MB
		if (setsockopt(server_sock, SOL_SOCKET, SO_RCVBUF, (void *)&rcv_size, sizeof(rcv_size)) == 0)
		{
			dps_log_notice(DpsProtocolLogLevel, "setsockopt SO_RCVBUF set to %d", rcv_size);
		}
		else
		{
			dps_log_error(DpsProtocolLogLevel, "setsockopt SO_RCVBUF error %d", errno);
		}

		if(fcntl(server_sock, F_SETFL, O_NONBLOCK) == -1)
		{
			dps_log_error(DpsProtocolLogLevel, "fcntl O_NONBLOCK failed Error %d\n",
			        errno);
			status = DOVE_STATUS_NOT_SUPPORTED;
			break;
		}

		memset(buffer, 0, DPS_MAX_BUFF_SZ);
		dps_log_notice(DpsProtocolLogLevel, "Adding Socket %d to CORE API\n", server_sock);
		core_status = fd_process_add_fd(server_sock, dps_process_data_rcvd, NULL);
		if (core_status == 0)
		{
			status = DOVE_STATUS_OK;
		}

	} while(0);

	return status;

}

/** @} */
/** @} */
/** @} */

