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
 *      fd_process_internal.h
 *
 *  Abstract:
 *      The file contains the defines and the internal structures for the
 *      CORE (Communication) APIs Phase 1. This will eventually be replaced
 *      by the General CORE APIs used in the controller.
 *
 *  Author:
 *      Amitabha Biswas
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _FD_PROCESS_INTERNAL_
#define _FD_PROCESS_INTERNAL_

#include "include.h"

/**
 * \ingroup DPSServerCoreAPI
 * @{
 */

 /* \brief The The File Descriptor used to wake up the poll thread immediately when
   * fd_process_pfd changes, and then poll can be called with updated fd_process_pfd
 */
#define POLL_DUMMY_FD_INDEX 0

/**
 * \brief Maximum number of Events to process at a time before giving up
 * the CPU
 */
#define MAX_FD_EVENT_PROCESS 64

/** @} */

#endif // _FD_PROCESS_INTERNAL_
