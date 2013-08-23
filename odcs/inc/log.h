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
 *      log.h
 *
 *  Abstract:
 *      The file contains the defines and the structures for the DPS Server
 *      Logs.
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

#ifndef _LOG_H_
#define _LOG_H_

#include "include.h"

#define MAX_ERRINFO_LEN                 512

void die(const char *fmt, ...);
void _log_emergency(const char *fmt, ...);
void _log_alert(const char *fmt, ...);
void _log_critical(const char *fmt, ...);
void _log_error(const char *fmt, ...);
void _log_warn(const char *fmt, ...);
void _log_notice(const char *fmt, ...);
void _log_info(const char *fmt, ...);
void _log_debug(const char *fmt, ...);
void _show_print(const char *fmt, ...);

/*
 ******************************************************************************
 * DPSServerLog                                                           *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup DPSServerLog Logging Mechanism
 * @{
 *
 * This module describes the DPS Server logging mechanism. Logging has two
 */


#define DPS_SERVER_LOGLEVEL_EMERGENCY        0x00000000
#define DPS_SERVER_LOGLEVEL_ALERT            0x00000001
#define DPS_SERVER_LOGLEVEL_CRITICAL         0x00000002
#define DPS_SERVER_LOGLEVEL_ERROR            0x00000003
#define DPS_SERVER_LOGLEVEL_WARNING          0x00000004
#define DPS_SERVER_LOGLEVEL_NOTICE           0x00000005
#define DPS_SERVER_LOGLEVEL_INFO             0x00000006
#define DPS_SERVER_LOGLEVEL_VERBOSE          0x00000007
#define DPS_SERVER_LOGLEVEL_MASK             0x0000000f

extern uint32_t log_console;

/**
 * \brief Developer Log Only: Log Emergency Message
 */
#define log_emergency(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_EMERGENCY) \
		_log_emergency(fmt, ##args)
/**
 * \brief Developer Log Only: Log Alert Message
 */
#define log_alert(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_ALERT) \
		_log_alert(fmt, ##args)
/**
 * \brief Developer Log Only: Log Critical Message
 */
#define log_critical(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_CRITICAL) \
		_log_critical(fmt, ##args)
/**
 * \brief Developer Log Only: Log Error Message
 */
#define log_error(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_ERROR)\
		_log_error(fmt, ##args)
/**
 * \brief Developer Log Only: Log Warning Message
 */
#define log_warn(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_WARNING)\
		_log_warn(fmt, ##args)
/**
 * \brief Developer Log Only: Log Notice Message
 */
#define log_notice(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_NOTICE)\
		_log_notice(fmt, ##args)
/**
 * \brief Developer Log Only: Log Info Message
 */
#define log_info(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_INFO)\
		_log_info(LOG_TEXT"%s:%s:%d: "fmt, __FILE__, __FUNCTION__,__LINE__,##args)
#if defined(NDEBUG)
/**
 * \brief Developer Log Only: Log Verbose Message
 */
#define log_debug(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_VERBOSE)\
		_log_debug(LOG_TEXT"%s:%s:%d: "fmt, __FILE__, __FUNCTION__,__LINE__,##args)

#else
/**
 * \brief Developer Log Only: Log Verbose Message
 */
#define log_debug(module_log_level, fmt, args...)
#endif

#define show_print(fmt, args...) \
	_show_print(fmt,##args);

/**
 * The following are logging mechanism for messages that can be read by
 * customer - These logs will not have file, function or line number
 * embedded in them
 */

/**
 * \brief CLI Log Only: Log Emergency Message
 */
#define customer_log_emergency(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_EMERGENCY) \
		_log_emergency(fmt, ##args)
/**
 * \brief CLI Log Only: Log Alert Message
 */
#define customer_log_alert(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_ALERT) \
		_log_alert(fmt, ##args)
/**
 * \brief CLI Log Only: Log Critical Message
 */
#define customer_log_critical(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_CRITICAL) \
		_log_critical(fmt, ##args)
/**
 * \brief CLI Log Only: Log Error Message
 */
#define customer_log_error(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_ERROR)\
		_log_error(fmt, ##args)
/**
 * \brief CLI Log Only: Log Warning Message
 */
#define customer_log_warn(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_WARNING)\
		_log_warn(fmt, ##args)
/**
 * \brief CLI Log Only: Log Notice Message
 */
#define customer_log_notice(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_NOTICE)\
		_log_notice(fmt, ##args)
/**
 * \brief CLI Log Only: Log Info Message
 */
#define customer_log_info(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_INFO)\
		_log_info(fmt, ##args)
/**
 * \brief CLI Log Only: Log Verbose Message
 */
#define customer_log_debug(module_log_level, fmt, args...) \
	if (module_log_level >= DPS_SERVER_LOGLEVEL_VERBOSE)\
		_log_debug(fmt, ##args)

#define MAC_FMT "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
#define MAC_OCTETS(_mac)					\
	(_mac)[0], (_mac)[1], (_mac)[2], (_mac)[3], (_mac)[4], (_mac)[5]

/** @} */
/** @} */

#endif // _LOG_H_
