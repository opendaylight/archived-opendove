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

class cli_interface_client_server_protocol:
    '''
    Represents DCS Server Related CLI Definitions for the DPS Client Server 
    Protocol. Should be in sync with the following files:
    1. cli/inc/client_server_protocol.h file
    2. 
    '''
    #DPS Protocol Message Type Codes
    DPS_MSG_ALL = 0
    #Matches with enum dps_client_req_type in dps_protocol/inc/dps_client_common.h file
    DPS_ENDPOINT_LOC_REQ = 1
    DPS_ENDPOINT_LOC_REPLY = 2
    DPS_POLICY_REQ = 3
    DPS_POLICY_REPLY = 4
    DPS_ENDPOINT_UPDATE = 5
    DPS_ENDPOINT_UPDATE_REPLY = 6
    DPS_POLICY_INVALIDATE = 7
    DPS_ADDR_RESOLVE = 8
    DPS_ADDR_REPLY = 9

    #No blanks in string
    MsgTypeToString = {DPS_ENDPOINT_LOC_REQ: "Endpoint-Location-Request",
                       DPS_ENDPOINT_LOC_REPLY: "Endpoint-Location-Reply",
                       DPS_POLICY_REQ: "Policy-Request",
                       DPS_POLICY_REPLY: "Policy-Reply",
                       DPS_ENDPOINT_UPDATE: "Endpoint-Update",
                       DPS_ENDPOINT_UPDATE_REPLY: "Endpoint-Update-Reply",
                       DPS_POLICY_INVALIDATE: "Policy-Invalidate",
                       DPS_ADDR_RESOLVE: "Address-Resolution",
                       DPS_ADDR_REPLY: "Address-Resolution-Reply",
                       }

    #No blanks in string
    MsgStringToType = {"All": DPS_MSG_ALL,
                       "Endpoint-Location-Request": DPS_ENDPOINT_LOC_REQ,
                       "Endpoint-Location-Reply": DPS_ENDPOINT_LOC_REPLY,
                       "Policy-Request": DPS_POLICY_REQ,
                       "Policy-Reply": DPS_POLICY_REPLY,
                       "Endpoint-Update": DPS_ENDPOINT_UPDATE,
                       "Endpoint-Update-Reply": DPS_ENDPOINT_UPDATE_REPLY,
                       "Policy-Invalidate": DPS_POLICY_INVALIDATE,
                       "Address-Resolution" :DPS_ADDR_RESOLVE,
                       "Address-Resolution-Reply": DPS_ADDR_REPLY,
                       }

    MsgCodesStringRange = MsgStringToType.keys()

    #CLI_CLIENT_SERVER_PROTOCOL CODES
    #Matches with enum cli_config_code defined in cli/inc/client_server_protocol.h file
    CLI_CLIENT_SERVER_DEV_LOG_LEVEL = 0
    CLI_CLIENT_SERVER_CUST_LOG_LEVEL = 1
    CLI_CLIENT_SERVER_STATISTICS_SHOW = 2
    CLI_CLIENT_SERVER_STATISTICS_CLEAR = 3
    CLI_CLIENT_RETRANSMIT_SHOW = 4
    CLI_CLIENT_RETRANSMIT_LOG_LEVEL = 5

class cli_client_server(cli_dps_config):
    '''
    Represents the CLI Object for Client Server Protocol
    '''
    #The input to display for Context:
    user_input_string = 'dps_protocol'
    user_input_string_raw = 'dps_protocol'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dps_protocol'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DPS Client Server Protocol (Context)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
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
        @return cli_client_server
        '''
        return cli_client_server
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_client_server.add_context()

class cli_cs_dev_log_level(cli_client_server):
    '''
    Represents the CLI Object for Changing Developer Log Level
    in the DPS Client Server Protocol
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level_dev'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Level for Debugging'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_SERVER_DEV_LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cs_protocol_log_level_s{
    #    /**
    #     * \brief log_level representing log level defined in log.h file
    #     */
    #    uint32_t    level;
    #}cli_cs_protocol_log_level_t;
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
cli_cs_dev_log_level.add_cli()

class cli_cs_customer_log_level(cli_client_server):
    '''
    Represents the CLI Object for Changing Customer Log Level
    in the DPS Client Server Protocol
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DPS Protocol Log Level'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_SERVER_CUST_LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cs_protocol_log_level_s{
    #    /**
    #     * \brief log_level representing log level defined in log.h file
    #     */
    #    uint32_t    level;
    #}cli_cs_protocol_log_level_t;
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
cli_cs_customer_log_level.add_cli()

class cli_cs_show(cli_client_server):
    '''
    Represents the CLI Context for Show Command in the DPS Client Server
    Protocol
    '''
    #The input to display for Context:
    user_input_string = 'show'
    user_input_string_raw = '%s:%s]'%(cli_client_server.user_input_string,
                                      user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Command'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
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
        @return cli_cs_show
        '''
        return cli_cs_show
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_cs_show.add_context()

class cli_cs_stats_show(cli_cs_show):
    '''
    Represents the CLI Object for showing statistics in the DPS
    Client Server Protocol
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'statistics'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show DPS Protocol Statistics'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_SERVER_STATISTICS_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('msg-type', cli_type_string_set, False, cli_interface_client_server_protocol.MsgCodesStringRange)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cs_protocol_stats_s{
    #    /**
    #     * \brief Represents DPS Request/Reply Message defined in
    #     *        the dps_client_req_type file
    #     * \remarks A pkt_type of 0 represents Wildcard i.e. All
    #     *          request/reply types
    #     */
    #    uint32_t pkt_type;
    #}cli_cs_protocol_stats_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        try:
            msg_type = cli_interface_client_server_protocol.MsgStringToType[self.params[0]]
        except Exception:
            msg_type = cli_interface_client_server_protocol.DPS_MSG_ALL
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, msg_type)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cs_stats_show.add_cli()

class cli_cs_clear(cli_client_server):
    '''
    Represents the CLI Context for Clear Commands in the DPS Client
    Server Protocol
    '''
    #The input to display for Context:
    user_input_string = 'clear'
    user_input_string_raw = '%s:%s'%(cli_client_server.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'clear'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Clear Commands'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_CLIENT_SERVER_PROTOCOL
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
        @return cli_cs_clear
        '''
        return cli_cs_clear
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_cs_clear.add_context()

class cli_cs_stats_clear(cli_cs_clear):
    '''
    Represents the CLI Object for clearing statistics in the DPS
    Client Server Protocol
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'statistics'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Clear DPS Protocol Statistics'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_SERVER_STATISTICS_CLEAR
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('msg-type', cli_type_string_set, True, cli_interface_client_server_protocol.MsgCodesStringRange)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cs_protocol_stats_s{
    #    /**
    #     * \brief Represents DPS Request/Reply Message defined in
    #     *        the dps_client_req_type file
    #     * \remarks A pkt_type of 0 represents Wildcard i.e. All
    #     *          request/reply types
    #     */
    #    uint32_t pkt_type;
    #}cli_cs_protocol_stats_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        msg_type = cli_interface_client_server_protocol.MsgStringToType[self.params[0]]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, msg_type)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cs_stats_clear.add_cli()

class cli_cs_retransmit_show(cli_cs_show):
    '''
    Represents the CLI Object for showing retransmit statistics
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'retransmit'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Retransmit Timer Details'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_RETRANSMIT_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cs_retransmit_show.add_cli()

class cli_cs_retransmit_log_level(cli_client_server):
    '''
    Represents the CLI Object for Changing Developer Log Level
    in the DPS Client Server Protocol
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level_retransmit'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Level for Retransmit Timer'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLIENT_SERVER_PROTOCOL
    #CLI CODE
    cli_code = cli_interface_client_server_protocol.CLI_CLIENT_RETRANSMIT_LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cs_protocol_log_level_s{
    #}cli_cs_protocol_log_level_t;
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
cli_cs_retransmit_log_level.add_cli()

