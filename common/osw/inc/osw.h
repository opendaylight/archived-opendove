/*
 * Copyright (c) 2010-2013 IBM Corporation
 * All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this
 * distribution, and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 *  Header File:
 *      osw.h
 *
 *  Abstract:
 *      Describes the Misc Definitiones of OS Wrapper
 *
 *  Author:
 *      DOVE Development Team
 *
 *  Environment:
 *      Userspace Mode
 *
 *  Revision History:
 *
 */

#ifndef _OSW_H_
#define _OSW_H_

#include "osw_list.h"
#include "osw_task.h"
#include "osw_semaphore.h"
#include "osw_queue.h"
#include "osw_timer.h"

typedef char                        BOOLEAN;
typedef char                        BOOL1;
typedef char                        CHR1;
typedef signed char                 INT1;
typedef unsigned char               UINT1;
typedef UINT1                       BYTE;
typedef void                        VOID;
typedef signed short                INT2;
typedef unsigned short              UINT2;
typedef signed   int                INT4;
typedef unsigned int                UINT4;
typedef size_t                      UINT_PTR;
typedef signed long long            INT8;
typedef unsigned long long          UINT8;
typedef float                       FLT4;
typedef double                      DBL8;

#define OSW_OK                      0
#define OSW_ERROR                   -1

#define OSW_WAIT                    (~0UL)
#define OSW_NO_WAIT                 0

#define OSW_EV_ANY                  0x0

#define OSW_DEF_MSG_LEN             8
#define OSW_MAX_Q_MSG_LEN           1024

#define OSW_DEFAULT_STACK_SIZE      10000

#define STRCPY(d,s)                 strcpy ((char *)(d),(const char *)(s))
#define STRNCPY(d,s,n)              strncpy ((char *)(d), (const char *)(s), n)
#define STRCAT(d,s)                 strcat((char *)(d),(const char *)(s))
#define STRCMP(s1,s2)               strcmp  ((const char *)(s1), (const char *)(s2))
#define STRNCMP(s1,s2,n)            strncmp ((const char *)(s1), (const char *)(s2), n)
#define STRCHR(s,c)                 strchr((const char *)(s), (int)c)
#define STRRCHR(s,c)                strrchr((const char *)(s), (int)c)
#define STRLEN(s)                   strlen((const char *)(s))

typedef void (*task_entry)(void *);

int osw_init(void);

#endif // _OSW_H_
