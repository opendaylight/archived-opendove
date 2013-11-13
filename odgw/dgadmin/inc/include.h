/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
*/


/*
 *  Copyright (c) IBM, Inc.  2011 -
 *  All rights reserved
 *
 *  Header File:
 *      include.h
 *
 *  Abstract:
 *      The main header file for the DOVE Gateway Process. All DOVE Gateway Process
 *      files should include only include this file. This file will resolve all
 *      dependencies.
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

#ifndef _DOVE_GATEWAY_INCLUDE_H_
#define _DOVE_GATEWAY_INCLUDE_H_

#define LOG_TEXT "DOVE-GATEWAY: "

// Including local features.h since /usr/include/features.h on 9.43.95.197 
// is incompatible with python2.6 compilation flags.

#include <Python.h>
// #include "features.h"
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <assert.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/socket.h>
#include "status.h"
#include "log.h"
#include "fd_process_public.h"
#include "dps_client_common.h"
#include "dps_pkt.h"
#include "python_interface.h"
#include "cli_interface.h"
#include "osw.h"
#include "dgwy_ctrl.h"
#include "dgadm.h"
#include "dgadmin_utils.h"

#endif /* _DOVE_GATEWAY_INCLUDE_H_ */
