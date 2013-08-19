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
 *      dps_log.h
 *
 *  Abstract:
 *      The file contains the defines and the structures for the DPS Server
 *      Logs.
 *
 *  Author:
 *      Sushma Anantharam
 *
 *  Environment:
 *      User World
 *
 *  Revision History:
 *
 */

#ifndef _DPS_LOG_H_
#define _DPS_LOG_H_

#include "dps_client_common.h"

#if defined(DPS_SERVER)
#define DPS_LOG_TEXT "DOVE-DPS-SERVER: "
extern uint32_t log_console;
#else
#define DPS_LOG_TEXT "DOVE-DPS-CLIENT: "
#endif

#define MAX_ERRINFO_LEN                 512

void dps_die(const char *fmt, ...);
void _dps_log_emergency(const char *fmt, ...);
void _dps_log_alert(const char *fmt, ...);
void _dps_log_critical(const char *fmt, ...);
void _dps_log_error(const char *fmt, ...);
void _dps_log_warn(const char *fmt, ...);
void _dps_log_notice(const char *fmt, ...);
void _dps_log_info(const char *fmt, ...);
void _dps_log_debug(const char *fmt, ...);

/*
 ******************************************************************************
 * DPSLog                                                           *//**
 *
 * \addtogroup DPS
 * @{
 * \defgroup DPSLog Logging Mechanism
 * @{
 *
 * This module describes the DPS Protocol logging mechanism. Logging has two
 */


#define DPS_LOGLEVEL_EMERGENCY        0x00000000
#define DPS_LOGLEVEL_ALERT            0x00000001
#define DPS_LOGLEVEL_CRITICAL         0x00000002
#define DPS_LOGLEVEL_ERROR            0x00000003
#define DPS_LOGLEVEL_WARNING          0x00000004
#define DPS_LOGLEVEL_NOTICE           0x00000005
#define DPS_LOGLEVEL_INFO             0x00000006
#define DPS_LOGLEVEL_VERBOSE          0x00000007
#define DPS_LOGLEVEL_MASK             0x0000000f


/**
 * \brief Developer Log Only: Log Emergency Message
 */
#define dps_log_emergency(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_EMERGENCY) \
		_dps_log_emergency(fmt, ##args)
/**
 * \brief Developer Log Only: Log Alert Message
 */
#define dps_log_alert(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_ALERT) \
		_dps_log_alert(fmt, ##args)
/**
 * \brief Developer Log Only: Log Critical Message
 */
#define dps_log_critical(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_CRITICAL) \
		_dps_log_critical(fmt, ##args)
/**
 * \brief Developer Log Only: Log Error Message
 */
#define dps_log_error(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_ERROR)\
		_dps_log_error(fmt, ##args)
/**
 * \brief Developer Log Only: Log Warning Message
 */
#define dps_log_warn(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_WARNING)\
		_dps_log_warn(fmt, ##args)
/**
 * \brief Developer Log Only: Log Notice Message
 */
#define dps_log_notice(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_NOTICE)\
		_dps_log_notice(fmt, ##args)

/**
 * \brief Developer Log Only: Log Info Message
 */

#define dps_log_info(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_INFO)\
		_dps_log_info(DPS_LOG_TEXT"%s:%s:%d: "fmt, __FILE__, __FUNCTION__,__LINE__,##args)
/**
 * \brief Developer Log Only: Log Verbose Message
 */
#if defined (NDEBUG) || defined (VMX86_DEBUG)
#define dps_log_debug(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_LOGLEVEL_VERBOSE)\
		_dps_log_debug(DPS_LOG_TEXT"%s:%s:%d: "fmt, __FILE__, __FUNCTION__,__LINE__,##args)
#else
/**
 * \brief Developer Log Only: Log Verbose Message
 */
#define dps_log_debug(module_log_level, fmt, args...)
#endif

/** @} */
/** @} */

#endif // _DPS_LOG_H_
