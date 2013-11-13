/*
 *  Copyright (c) IBM Inc.  2011 -
 *  All rights reserved
 *
 *  Header File:
 *      fd_process_public.h
 *
 *  Abstract:
 *      This file contains the CORE APIs (Phase 1) Public Interfaces
 *
 *  Author:
 *      DOVE DPS Development Team
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
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
