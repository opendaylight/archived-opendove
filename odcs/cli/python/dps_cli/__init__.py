'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

@author: abiswas
'''

import logging
from logging import getLogger
log = getLogger(__name__)

import time
import os

class login_context(object):
    '''
    Represents a Login Context to the CLI. Yes we can should be able to support
    multiple sessions, need to figure out how we can support it.
    '''
    idle_time_max = 60 #1 minute idle time

    def __init__(self):
        '''
        Initializes a session object
        '''
        self.god_mode = False
        self.username = ''
        self.password = ''
        #Maintain idle time
        self.idle_time = 0

class cli_interface:
    '''
    Represents DCS Server Related CLI Definitions. Should be in
    sync with the definitions in the following files:
    1. cli/inc/interface.h - CLI definitions
    2. include/status.h - STATUS definitions
    3. include/log.h - Log Level definitions
    '''
    #ERROR CODES returned by the Python C Interfaces
    DOVE_STATUS_OK = 0
    DOVE_STATUS_INVALID_FD = 1
    DOVE_STATUS_INVALID_PATH = 2
    DOVE_STATUS_NO_MEMORY = 3
    DOVE_STATUS_NO_RESOURCES = 4
    DOVE_STATUS_EMPTY = 5
    DOVE_STATUS_RETRY = 6
    DOVE_STATUS_INVALID_PARAMETER = 7
    DOVE_STATUS_BAD_ADDRESS = 8
    DOVE_STATUS_EXCEEDS_CAP = 9
    DOVE_STATUS_EXISTS = 10
    DOVE_STATUS_NOT_FOUND = 11
    DOVE_STATUS_NOT_SUPPORTED = 12
    DOVE_STATUS_INTERRUPT = 13
    DOVE_STATUS_BIND_FAILED = 14
    DOVE_STATUS_INVALID_DOMAIN = 15
    DOVE_STATUS_INVALID_DVG = 16
    DOVE_STATUS_INVALID_POLICY = 17
    DOVE_STATUS_THREAD_FAILED = 18
    DOVE_STATUS_OSW_INIT_FAILED = 19
    DOVE_STATUS_INVALID_SERVICE = 20
    DOVE_STATUS_RESTC_INIT_FAILED = 21
    DOVE_STATUS_LOCAL_DOMAIN = 22
    DOVE_SERVICE_ADD_STATUS_FAIL = 23
    DOVE_SERVICE_TYPE_STATUS_INVALID = 24
    DOVE_STATUS_ERROR = 25
    DOVE_STATUS_BUSY = 26
    DOVE_STATUS_INACTIVE = 27
    DOVE_STATUS_UNKNOWN = 28

    ErrorToString = {DOVE_STATUS_OK: 'Success',
                     DOVE_STATUS_INVALID_FD: 'Invalid File Descriptor',
                     DOVE_STATUS_INVALID_PATH: 'Invalid Path',
                     DOVE_STATUS_NO_MEMORY: 'No Memory',
                     DOVE_STATUS_NO_RESOURCES: 'No Resources',
                     DOVE_STATUS_EMPTY: 'Nothing To Report',
                     DOVE_STATUS_RETRY: 'Retry',
                     DOVE_STATUS_INVALID_PARAMETER: 'Invalid Parameter',
                     DOVE_STATUS_BAD_ADDRESS: 'Bad Address',
                     DOVE_STATUS_EXCEEDS_CAP: 'Exceeds Current Capability',
                     DOVE_STATUS_EXISTS: 'Already Exists',
                     DOVE_STATUS_NOT_FOUND: 'Does not exist!',
                     DOVE_STATUS_NOT_SUPPORTED: 'Not Supported',
                     DOVE_STATUS_INTERRUPT: 'Interrupt',
                     DOVE_STATUS_BIND_FAILED: 'Socket Bind Failed',
                     DOVE_STATUS_INVALID_DOMAIN: 'Invalid Domain',
                     DOVE_STATUS_INVALID_DVG: 'Invalid DVG',
                     DOVE_STATUS_INVALID_POLICY: 'Invalid Policy',
                     DOVE_STATUS_THREAD_FAILED: 'Thread Creation Failed',
                     DOVE_STATUS_OSW_INIT_FAILED: 'OSW initialization Failed',
                     DOVE_STATUS_INVALID_SERVICE: 'Invalid Service',
                     DOVE_STATUS_RESTC_INIT_FAILED: 'REST Client Initialization Failed',
                     DOVE_STATUS_LOCAL_DOMAIN: 'Domain Handled by Local Node',
                     DOVE_SERVICE_ADD_STATUS_FAIL: 'Service Add Failed',
                     DOVE_SERVICE_TYPE_STATUS_INVALID: 'Service Type Status Invalid',
                     DOVE_STATUS_ERROR: 'Status Error',
                     DOVE_STATUS_BUSY: 'Busy',
                     DOVE_STATUS_INACTIVE: 'Inactive',
                     DOVE_STATUS_UNKNOWN: 'Status Unknown'
                     }

    #List of all Log Levels
    #Matches with log.h, for DCS Server
    log_level_map = {'debug': 7, 'info': 6, 'notice': 5, 'warn': 4, 
                     'error': 3, 'critical': 2, 'alert': 1, 'emergency': 0}

    #List of Policy Types
    policy_type_map = {'connectivity': 1}
    #List of Policy Action Connection
    policy_connectivity_map = {'drop': 0, 'allow': 1}

    #List of acceptable Ranges for various values
    range_domain = [1,16777215]
    range_replication = [1,4]
    range_dvg = range_domain
    range_domain_show = [0,16777215]
    range_dvg_show = range_domain_show
    range_policy_type = policy_type_map.keys()
    range_policy_ttl = []
    range_policy_action_connectivity = policy_connectivity_map.keys()
    range_log_level = log_level_map.keys()
    range_pip = []
    range_ip = []
    range_ip_multicast = ['224.0.0.0','240.0.0.0']
    ip_multicast_unknown = '240.0.0.0'
    range_mac = []
    range_port = [1,65534]
    range_report_interval = [0, 600]

    #List of Associated Type
    associated_type_map = {'domain': 1, 'vnid': 2}
    range_associated_type = associated_type_map.keys()

    #List of IPSubnet Associated Type
    ipsubnet_associated_type_map = associated_type_map
    range_ipsubnet_associated_type = ipsubnet_associated_type_map.keys()

    #List of IPSubnet Mode
    ipsubnet_mode_map = {'shared': 1, 'dedicated': 0}
    range_ipsubnet_mode = ipsubnet_mode_map.keys()

    #List of Thread Action
    thread_action_map = {'start': 1, 'stop': 0}
    range_thread_action = thread_action_map.keys()

    @classmethod
    def process_error(cls, error_code):
        '''
        This routine processes error codes
        @param error_code: Error Code of the DCS Server
        @type error_code: Integer
        '''
        try:
            if error_code != cls.DOVE_STATUS_OK:
                print 'Error - %s\r'%(cls.ErrorToString[error_code])
        except Exception:
            print 'Error - Unknown!\r'
