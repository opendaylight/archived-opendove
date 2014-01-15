/*
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
 *      Sushma Anantharam
 *
 *  Abstract:
 *      This module deals with the logging mechanism in the DOVE DPS Server
 *
 */

#if defined(DOVE_SERVICE_APPLIANCE)
#include "include.h"
#endif
#include "dps_log.h"

/**
 * \brief No need to Doxygen the following routines
 */

void dps_info(int level, const char *fmt, va_list ap)
{
	char buf[MAX_ERRINFO_LEN + 1];

	memset(buf, 0, MAX_ERRINFO_LEN);
	vsnprintf(buf, MAX_ERRINFO_LEN, fmt, ap);
	syslog(level, "%s", buf);
#if defined(DOVE_SERVICE_APPLIANCE)
{
#define DPS_LOG_PATH "/flash/dcs.log"
	FILE *logFP = NULL;
	struct stat fileSt;
	if (log_console)
	{
		strcat(buf, "\r");
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
		fprintf(logFP,"%s\n\r",buf);
		fclose(logFP);
	}
}
#endif
	return;
}

void dps_die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_ERR, fmt, ap);
	va_end(ap);
	exit(-1);
}

void _dps_log_emergency(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_EMERG, fmt, ap);
	va_end(ap);
}

void _dps_log_alert(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_ALERT, fmt, ap);
	va_end(ap);
}

void _dps_log_critical(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_CRIT, fmt, ap);
	va_end(ap);
}


void _dps_log_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_ERR, fmt, ap);
	va_end(ap);
}

void _dps_log_warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_WARNING, fmt, ap);
	va_end(ap);
}

void _dps_log_notice(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_NOTICE, fmt, ap);
	va_end(ap);
}

void _dps_log_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_INFO, fmt, ap);
	va_end(ap);
}

void _dps_log_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	dps_info(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

