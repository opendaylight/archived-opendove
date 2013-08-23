/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 * File:   retransmit_interface.h
 * Author: Amitabha Biswas
 *
 * Created on Feb 25, 2012, 12:32 PM
 */

#include "include.h"
#include "raw_proto_timer.h"

/**
 * \ingroup DPSRetransmitInterface
 * @{
 */

#ifndef _PYTHON_DPS_RESTRANSMIT_INTERFACE_H_
#define _PYTHON_DPS_RESTRANSMIT_INTERFACE_H_

extern int PythonRetransmitLogLevel;

/*
 ******************************************************************************
 * retransmit_data --                                                       *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to retransmit
 *        a message.
 *
 * \param[in] self  PyObject
 * \param[in] args  dps_retransmit_context_t as Python Object
 *
 * \retval 0 Success
 * \retval -1 Failure
 *
 ******************************************************************************/
PyObject *retransmit_data(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * retransmit_timeout --                                                  *//**
 *
 * \brief This is the routine that the PYTHON Scripts must call to indicate
 *        timeout to the DPS Protocol.
 *
 * \param[in] self  PyObject
 * \param[in] args  dps_retransmit_context_t as Python Object
 *
 * \retval 0 Success
 *
 ******************************************************************************/
PyObject *retransmit_timeout(PyObject *self, PyObject *args);

/*
 ******************************************************************************
 * retransmit_timer_start --                                              *//**
 *
 * \brief This is the routine that the DPS Protocol Handler must call to start
 *        a retransmit timer on a packet
 *
 * \param[in] data  The Data to send
 * \param[in] data_len  Data Length
 * \param[in] query_id  The Query ID to associated with the packet
 * \param[in] sockFd  The socket file descriptor
 * \param[in] addr  The Socket Address
 * \param[in] context  The Context associated with the packet
 * \param[in] callback  The callback to invoke when retransmission times out
 * \param[in] owner  The owner type
 *
 * \retval 0 Success
 * \retval non-zero Failure
 *
 ******************************************************************************/

int retransmit_timer_start(char *data, int data_len, uint32_t query_id,
                           int sockFd, struct sockaddr *addr,
                           void *context, rpt_callback_ptr callback, rpt_owner_t owner);

/*
 ******************************************************************************
 * retransmit_timer_show --                                               *//**
 *
 * \brief This is the routine that the DPS Protocol Handler must call to stop
 *        the retransmit timer on a packet. It should be called by the DPS
 *        Protocol Handler when it receives a reply packet.
 *        Once this routine is called a valid context returned, the timer
 *        routine will not longer own any resources associated with the packet.
 *
 * \param[in] query_id  The query
 * \param[in] pcontext  Pointer to a location that will return the context
 *
 * \retval 0 Success
 * \retval non-zero Failure
 *
 ******************************************************************************/
int retransmit_timer_stop(uint32_t query_id, void **pcontext);

/*
 ******************************************************************************
 * retransmit_timer_show --                                                *//**
 *
 * \brief This is the routine that shows the retransmit timer details
 *
 * \retval None
 *
 ******************************************************************************/
void retransmit_timer_show();

/*
 ******************************************************************************
 * python_init_retransmit_interface --                                  *//**
 *
 * \brief This routine initializes the functions needed to handle the
 *        Retransmit Timer module
 *
 * \param pythonpath - Pointer to the PYTHON Path. If this value is NULL, it
 *                     assumes the PYTHON path to be in the same directory.
 *
 * \return dove_status
 *
 *****************************************************************************/
dove_status python_init_retransmit_interface(char *pythonpath);

/** @} */

#endif // _PYTHON_DPS_RESTRANSMIT_INTERFACE_H_
