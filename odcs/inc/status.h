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
 *      status.h
 *
 *  Abstract:
 *      The DPS Server Status Codes
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

#ifndef _DOVE_STATUS_
#define _DOVE_STATUS_

/*
 *****************************************************************************
 * StatusCodes                                                          *//**
 *
 * \addtogroup DPSServer
 * @{
 * \defgroup StatusCodes Status Code
 * @{
 * DPS Server Specific Status. Should be kept in sync with the following files:
 * 1. cli/python/dps_cli/config.py
 * 2. data_handler/python/request_recv.py
 *
 * ALERT!!! DO NOT ADD NEW VALUES IN THE MIDDLE. ADD NEW VALUES TO THE END
 */

//                              Error-Code                         String                                     Value
//                              ----------                        ---------                                   ----
#define DOVE_ERROR_CODES \
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_OK,                         "Success",                          0)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_FD,                 "Invalid File Descriptor",          1)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_PATH,               "Invalid Path",                     2)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_NO_MEMORY,                  "No Memory",                        3)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_NO_RESOURCES,               "No Resources",                     4)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_EMPTY,                      "Nothing To Report",                5)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_RETRY,                      "Retry",                            6)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_PARAMETER,          "Invalid Parameter",                7)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_BAD_ADDRESS,                "Bad Address",                      8)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_EXCEEDS_CAP,                "Exceeds Current Capability",       9)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_EXISTS,                     "Already Exists",                  10)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_NOT_FOUND,                  "Does not exist!",                 11)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_NOT_SUPPORTED,              "Not Supported",                   12)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INTERRUPT,                  "Interrupt",                       13)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_BIND_FAILED,                "Socket Bind Failed",              14)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_DOMAIN,             "Invalid Domain",                  15)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_DVG,                "Invalid DVG",                     16)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_POLICY,             "Invalid Policy",                  17)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_THREAD_FAILED,              "Thread Creation Failed",          18)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_OSIX_INIT_FAILED,           "OSIX Initialization Failed",      19)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INVALID_SERVICE,            "Invalid Service",                 20)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_RESTC_INIT_FAILED,          "REST Client Init Failed",         21)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_LOCAL_DOMAIN,               "Domain Handled by Local Node",    22)\
    DEFINE_DOVE_ERROR_AT(DOVE_SERVICE_ADD_STATUS_FAIL,           "Failed to Add Service",           23)\
    DEFINE_DOVE_ERROR_AT(DOVE_SERVICE_TYPE_STATUS_INVALID,       "Service Type Invalid",            24)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_ERROR,                      "Error",                           25)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_BUSY,                       "Busy",                            26)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_INACTIVE,                   "Inactive",                        27)\
    DEFINE_DOVE_ERROR_AT(DOVE_STATUS_UNKNOWN,                    "Unknown",                         28) // Always the Final, Change Values

#define DEFINE_DOVE_ERROR_AT(_err, _str, _val) _err = _val,
typedef enum {
    DOVE_ERROR_CODES
}dove_status;
#undef DEFINE_DOVE_ERROR_AT

const char *DOVEStatusToString(dove_status status);

/** @} */
/** @} */

#endif // _DOVE_STATUS_
