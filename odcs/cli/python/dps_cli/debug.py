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

import struct
import socket
import time
import os
from common_cli import MyTerminal
from common_cli import cli_type_int
from common_cli import cli_type_int_range
from common_cli import cli_type_string
from common_cli import cli_type_string_set
from common_cli import cli_type_ip
from common_cli import cli_type_mac
from common_cli import cli_base
import dps_cli
from dps_cli import cli_interface
from dps_cli.config import cli_dps_interface
from dps_cli.config import cli_dps_config

#This is the DPS Library Name
import dcslib

class cli_interface_debug:
    '''
    Represents DCS Server Related CLI Definitions for the DPS Client Server 
    Protocol. Should be in sync with the following files:
    1. cli/inc/debug.h file
    '''
    #CLI_DEBUG CODES
    #Matches with enum cli_config_code defined in cli/inc/debug.h file
    LOG_LEVEL = 0
    GET_VNID_ENDPOINTS = 1
    GET_VNID_DOVE_SWITCHES = 2
    GET_VNID_GATEWAYS = 3

class cli_debug(cli_dps_config):
    '''
    Represents the CLI Object for Debug
    '''
    #The input to display for Context:
    user_input_string = 'debug'
    user_input_string_raw = 'debug'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'debug'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Debugging for Data Handler (Context)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_dps_interface.CLI_DEBUG
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [] #Empty
    support_random = False
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return cli_debug
        '''
        return cli_debug
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_debug.add_context()

class cli_debug_log_level(cli_debug):
    '''
    Represents the CLI Object for Changing Debug Log Level
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Level for Debugging'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DEBUG
    #CLI CODE
    cli_code = cli_interface_debug.LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_debug_log_level_s{
    #    uint32_t    level;
    #}cli_debug_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        level_int = cli_interface.log_level_map[self.params[0]]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, level_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_debug_log_level.add_cli()

class cli_debug_vnid(cli_debug):
    '''
    Represents the CLI Context for Testing retrieval of VNID Objects
    '''
    #The input to display for Context:
    user_input_string = 'debug-vnid'
    user_input_string_raw = 'debug-vnid'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vnid'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'VNID related Debugging'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_dps_interface.CLI_DEBUG
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [] #Empty
    support_random = False
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return cli_debug_vnid
        '''
        return cli_debug_vnid
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_debug_vnid.add_context()

class cli_debug_vnid_get_endpoints(cli_debug_vnid):
    '''
    Represents the CLI Object for fetching the Endpoints in a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'endpoints_get'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get Endpoints in the VNID'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DEBUG
    #CLI CODE
    cli_code = cli_interface_debug.GET_VNID_ENDPOINTS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_debug_log_level_s{
    #    uint32_t    vnid;
    #}cli_debug_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_debug_vnid_get_endpoints.add_cli()

class cli_debug_vnid_get_switches(cli_debug_vnid):
    '''
    Represents the CLI Object for fetching the Endpoints in a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'switches_get'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get DOVE Switches in the VNID'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DEBUG
    #CLI CODE
    cli_code = cli_interface_debug.GET_VNID_DOVE_SWITCHES
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_debug_log_level_s{
    #    uint32_t    vnid;
    #}cli_debug_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_debug_vnid_get_switches.add_cli()

