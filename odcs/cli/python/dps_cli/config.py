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
import string
import common_cli
import time
import os
import hashlib

from common_cli import MyTerminal
from common_cli import cli_type_int
from common_cli import cli_type_int_range
from common_cli import cli_type_string
from common_cli import cli_type_string_set
from common_cli import cli_type_ip
from common_cli import cli_type_mac
from common_cli import cli_base
from dps_cli import cli_interface

#This is the DPS Library Name
import dcslib

class cli_dps_interface:
    #CLI_MAJOR_CODES
    #Matches with enum cli_major_code defined in cli/inc/interface.h file
    CLI_CONFIG = 0
    CLI_CLIENT_SERVER_PROTOCOL = 1
    CLI_DATA_OBJECTS = 2
    CLI_CLUSTER = 3
    CLI_CORE_API = 4
    CLI_CONTROLLER = 5
    CLI_REST = 6
    CLI_OSW = 7
    CLI_WEB_SERVER = 8
    CLI_SPIDERCAST = 9
    CLI_DEBUG = 10

class cli_interface_config:
    '''
    Represents DCS Server Related CLI Definitions. Should be in 
    sync with the definitions in the cli/inc/config.h file
    '''
    #CLI_CONFIG CODES
    #Matches with enum cli_config_code defined in cli/inc/config.h file
    cli_code_login = 0
    cli_code_config_enter = 1
    cli_code_clear_screen = 2
    cli_code_shell = 3
    cli_code_log_console = 4
    cli_code_cli_log_level = 5
    cli_code_node_location = 6
    cli_code_controller_location_set = 7
    cli_code_controller_location_get = 8
    cli_code_test_ip_subnet = 9
    cli_code_REST_log_level = 10
    cli_code_statistics_thread_action = 11
    cli_code_Query_DC_Cluster_Info = 12
    cli_code_show_uuid = 13

class cli_login(cli_base):
    '''
    Represents the CLI Object for Login Screen
    '''
    #The input to display for Context:
    user_input_string = 'Password:'
    user_input_string_raw = user_input_string
    #No Help
    help_strings = []
    help_chars = ''
    input_char_show = False
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'login'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Onto DCS Server'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_login
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('password', cli_type_string, True, [])]
    support_random = False
    #Specific to this CLI
    godmodepasswords = ['y\xde\x17\xa3\x81\xc8\x06\xdf\xae/;R\xe9<U+']
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Need to add encryption somewhere
        @param session: The session context
        @type session: Class login_context
        @return: The Next Context to enter
        '''
        password = self.params[0]
        m = hashlib.md5()
        m.update(password)
        digest = m.digest()
        if digest in self.godmodepasswords:
            session.god_mode = True
            print 'Logged In as Developer\r'
        else:
            session.god_mode = False
            print 'Logged In...\r'
        return cli_dps_config
    #This function 'cli_exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Should not be called - No change in context
        @param session: The session context
        @type session: Class login_context
        '''
        print 'ALERT!!! CLI FAILURE in cli_login.exit. Inform DOVE-DEV. Goodbye...\r'
        return
#Add this class of command to global list of supported commands
cli_login.add_context()

class cli_dps_config(cli_login):
    '''
    Represents the CLI Object for DCS Server
    '''
    #The input to display for Context:
    user_input_string = 'config'
    user_input_string_raw = 'config'
    #Reset Help
    help_strings = cli_base.help_strings
    help_chars = cli_base.help_chars
    input_char_show = True
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'config'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Onto DCS Server'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_config_enter
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return None
        '''
        return cli_dps_config
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_dps_config.add_context()

class cli_clear_screen(cli_dps_config):
    '''
    Represents the CLI Object for Screen Clear Command
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'clear'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Clear Screen'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_clear_screen #CLEAR SCREEN
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [] #Empty
    support_random = False
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        MyTerminal.clear_screen()
#Add this class of command to global list of supported commands
cli_clear_screen.add_cli()

class cli_drop_to_shell(cli_dps_config):
    '''
    Represents the CLI Object to drop to Shell
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'shell'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Drop to Shell'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_shell #CLEAR SCREEN
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [] #Empty
    support_random = False
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        self.terminal.release()
        try:
            os.system('sh')
        except Exception:
            print "Error: Shell command 'sh' not supported in this OS\r"
        self.terminal.acquire()
#Add this class of command to global list of supported commands
cli_drop_to_shell.add_cli()

class cli_log_console(cli_dps_config):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_console'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Send Logs To Console'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_log_console #LOG TO CONSOLE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('value', cli_type_int, True, [0,1])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_log_level_s{
    #    uint32_t    log_level;
    #}cli_config_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, self.params[0])
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_log_console.add_cli()

class cli_log_level(cli_dps_config):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level_cli'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set CLI Debugging Log Level'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_cli_log_level
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_log_level_s{
    #    uint32_t    log_level;
    #}cli_config_log_level_t;
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
cli_log_level.add_cli()

class cli_config_show(cli_dps_config):
    '''
    Represents the CLI Context for Show Command in the DPS Client Server
    Protocol
    '''
    #The input to display for Context:
    user_input_string = 'show'
    user_input_string_raw = '%s:%s'%(cli_dps_config.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Command (Main Menu)'
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
        @return None
        '''
        return cli_config_show
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_config_show.add_context()

class cli_node_location(cli_config_show):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'service_location'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DCS Server Node Location'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_node_location
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE)
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
cli_node_location.add_cli()

class cli_controller_add(cli_dps_config):
    '''
    Represents the CLI Object for adding the controller's location
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'controller_add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Controller Location'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_controller_location_set
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip),
                      ('Port', cli_type_int, True, cli_interface.range_port)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_location_s{
    #    uint16_t port;
    #    uint16_t pad;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_config_location_t;
    fmt = 'II'
    fmt += 'HHI'
    fmt += 'I12s' #IPv4
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        port = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               port, 0,
                               socket.AF_INET, ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_controller_add.add_cli()

class cli_controller_show(cli_config_show):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'controller_location'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Controller Location'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_controller_location_get
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE)
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
cli_controller_show.add_cli()

#Add a command to show DPS Node uuid
class cli_show_uuid(cli_config_show):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'uuid'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DPS Node UUID'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_show_uuid
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE)
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
cli_show_uuid.add_cli()


class cli_log_level_REST(cli_dps_config):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level_REST'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set REST Handler Log Level'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_REST_log_level
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_log_level_s{
    #    uint32_t    log_level;
    #}cli_config_log_level_t;
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
cli_log_level_REST.add_cli()



class cli_query_dc_for_cluster_node_info(cli_dps_config):
    '''
    Represents the CLI Object to Query Dove Controller for Cluster Node Info
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'query_dc_cluster'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Query DC for Cluster Info'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_Query_DC_Cluster_Info
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    #command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_log_level_s{
    #    uint32_t    log_level;
    #}cli_config_log_level_t;
	#fmt = 'III'
    fmt = 'II'
	
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        #level_int = cli_interface.log_level_map[self.params[0]]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_query_dc_for_cluster_node_info.add_cli()

class cli_statistics_thread_restart(cli_dps_config):
    '''
    Represents the CLI Object for restarting statistics thread
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'stat_thread'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Start/Stop statistics thread'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CONFIG
    #CLI CODE
    cli_code = cli_interface_config.cli_code_statistics_thread_action
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('action', cli_type_string_set, True, cli_interface.range_thread_action)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_config_location_s{
    #    uint16_t port;
    #    uint16_t pad;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_config_location_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        action = cli_interface.thread_action_map[self.params[0]]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, action);
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_statistics_thread_restart.add_cli()
