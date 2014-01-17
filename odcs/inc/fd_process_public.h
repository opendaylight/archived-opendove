/*
 *
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Source File:
 *      fd_process_public.h
 *      This file contains the CORE APIs (Phase 1) Public Interfaces
 *
 *  Author:
 *      Open DOVE development team
 *
 */


#ifndef _FD_PROCESS_PUBLIC_
#define _FD_PROCESS_PUBLIC_

#include "include.h"

/*
 ******************************************************************************
 * init_fd_process_p1 --                                                    *//**
 *
 * \brief This Initializes the CORE API
 *
 * \retval DOVE_STATUS_OK Success
 * \retval Other Failure
 *
 *****************************************************************************/

dove_status fd_process_init(void);

/*
 ******************************************************************************
 * fd_process_start --                                                   *//**
 *
 * \brief This starts the CORE API.
 *
 * \retval None
 *
 *****************************************************************************/

void fd_process_start(void);

#endif /* _FD_PROCESS_PUBLIC_ */
