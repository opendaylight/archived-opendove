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
 *  Source File:
 *      log.c
 *
 *  Author:
 *      Amitabha Biswas
 *
 *  Abstract:
 *      This module deals with the logging mechanism in the DOVE DCS Server
 *
 */

#include "include.h"

/**
 * \ingroup DPSServerLog
 * @{
 */

uint32_t log_console = 1;

#define DPS_LOG_PATH "/flash/dcs.log"

#define DEFINE_DOVE_ERROR_AT(_err, _str, _val) _str,
const char *returnDoveStatusStrs[] = {
	DOVE_ERROR_CODES
};
#undef DEFINE_DOVE_ERROR_AT
const char *DOVEStatusToString(dove_status status)
{
	if (status > DOVE_STATUS_UNKNOWN)
	{
		return "Unknown Status";
	}
	else
	{
		return returnDoveStatusStrs[status];
	}
}

/** @} */

/**
 * \brief No need to Doxygen the following routines
 */

void info(int level, const char *fmt, va_list ap)
{
	char buf[MAX_ERRINFO_LEN + 1];
	FILE *logFP = NULL;
	struct stat fileSt;

	memset(buf, 0, MAX_ERRINFO_LEN);
	vsnprintf(buf, MAX_ERRINFO_LEN, fmt, ap);
	syslog(level, "%s", buf);
	if (log_console)
	{
		print_console(buf);
	}

	logFP = fopen(DPS_LOG_PATH,"a");
	if(logFP)
	{
		fstat(fileno(logFP), &fileSt);
		if(fileSt.st_size > 1*1024*1024)
		{
			fclose(logFP);
			logFP = fopen(DPS_LOG_PATH,"w+");
		}
	}
	if(logFP)
	{
		fprintf(logFP,"%s\n",buf);
		fclose(logFP);
	}

	return;
}

void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(-1);
}

void _log_emergency(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_EMERG, fmt, ap);
	va_end(ap);
}

void _log_alert(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_ALERT, fmt, ap);
	va_end(ap);
}

void _log_critical(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_CRIT, fmt, ap);
	va_end(ap);
}


void _log_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_ERR, fmt, ap);
	va_end(ap);
}

void _log_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_WARNING, fmt, ap);
	va_end(ap);
}

void _log_notice(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_NOTICE, fmt, ap);
	va_end(ap);
}

void _log_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_INFO, fmt, ap);
	va_end(ap);
}

void _log_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	info(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

void shprint(const char *fmt, va_list ap)
{
	char buf[510 + 2];

	memset(buf, 0, 510);
	vsnprintf(buf, 510, fmt, ap);
	print_console(buf);
	return;
}
void _show_print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	shprint(fmt, ap);
	va_end(ap);
}
