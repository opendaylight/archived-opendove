/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *
 *  Header File:
 *      dps_pkt.h
 *      This file defines structures and msg types used for interacting with dove switches
 *      gateways and dove policy server (DPS).
 *
 *  Author:
 *      Sushma Anantharam
 *
 */

#ifndef _DPS_PKT_SVR_
#define _DPS_PKT_SVR_

#include "include.h"

// General Function prototypes

#if defined(DPS_SERVER)

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
dove_status dps_svr_proto_init(uint32_t portnum);

/*
 ******************************************************************************
 * dps_packet_stats_show                                                  *//**
 *
 * \brief This routine is called to show the packet statistics of DPS Client
 *        Server packets
 *
 * \param[in] pkt_type - DPS Client Server Protocol Message Type i.e.
 *                       dps_client_req_type
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */
dove_status dps_packet_stats_show(uint32_t pkt_type);

/*
 ******************************************************************************
 * dps_packet_stats_clear                                                 *//**
 *
 * \brief This routine is called to clear the packet statistics of DPS Client
 *        Server packets
 *
 * \param[in] pkt_type - DPS Client Server Protocol Message Type i.e.
 *                       dps_client_req_type
 *
 * \retval DOVE_STATUS_OK
 *
 ******************************************************************************
 */
dove_status dps_packet_stats_clear(uint32_t pkt_type);

#endif

#endif // _DPS_PKT_SVR_

/** @} */

