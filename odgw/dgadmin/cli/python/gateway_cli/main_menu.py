'''
Created on Apr 2, 2012

@summary: The DPS Server related CLI definitions

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
from common_cli import MyTerminal
from common_cli import cli_type_int
from common_cli import cli_type_int_range
from common_cli import cli_type_string
from common_cli import cli_type_string_set
from common_cli import cli_type_ip
from common_cli import cli_type_mac
from common_cli import cli_base
import gateway_cli
from gateway_cli import cli_interface

#This is the DOVE GATEWAY LIBRARY NAME
import dgadmin

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

class cli_gateway_interface:
    #CLI_MAJOR_CODES
    #Matches with enum cli_major_code defined in cli/inc/interface.h file
    CLI_MAIN_MENU = 0
    CLI_SERVICE = 1
    CLI_DPS_PROTOCOL = 2
    CLI_WEB_SERVICES = 3

class cli_main_menu_code:
    '''
    Represents DPS Server Related CLI Definitions. Should be in 
    sync with the definitions in the cli/inc/config.h file
    '''
    #CLI_MAIN_MENU CODES
    #Matches with enum cli_main_menu_code defined in cli/inc/main_menu.h file
    cli_code_login = 0
    cli_code_config_enter = 1
    cli_code_clear_screen = 2
    cli_code_shell = 3
    cli_code_log_console = 4
    cli_code_cli_log_level = 5
    cli_code_dps_log_level = 6

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
    command_def = 'Log Onto DOVE Gateway'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_login
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('password', cli_type_string, True, [])]
    support_random = False
    #Specific to this CLI
    #godmodepasswords = ['god', 'miGyi2!', 
    #                    'benny', 'uday', 'gail',
    #                    'lab4man1', 'dove', 'admin']

    godmodepasswords = ['admin']

    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Need to add encryption somewhere
        @param session: The session context
        @type session: Class login_context
        @return: The Next Context to enter
        '''
        password = self.params[0]
        if password.lower() in self.godmodepasswords:
            session.god_mode = True
            print 'Logged In as Admin'
        else:
            session.god_mode = False
            print 'Logged In...'
        return cli_main_menu
    #This function 'cli_exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Should not be called - No change in context
        @param session: The session context
        @type session: Class login_context
        '''
        print 'ALERT!!! CLI FAILURE in cli_login.exit. Inform DOVE-DEV. Goodbye...'
        return
#Add this class of command to global list of supported commands
cli_login.add_context()

class cli_main_menu(cli_login):
    '''
    Represents the CLI Object for DPS Server
    '''
    #The input to display for Context:
    user_input_string = 'main_menu'
    user_input_string_raw = '[main_menu]>>'
    #Reset Help
    help_strings = cli_base.help_strings
    help_chars = cli_base.help_chars
    input_char_show = True
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'main_menu'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Onto DOVE Gateway'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_config_enter
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
        return None
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_main_menu.add_context()

class cli_clear_screen(cli_main_menu):
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
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_clear_screen #CLEAR SCREEN
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

class cli_drop_to_shell(cli_main_menu):
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
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_shell #CLEAR SCREEN
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
            print "Error: Shell command 'sh' not supported in this OS"
        self.terminal.acquire()
#Add this class of command to global list of supported commands
cli_drop_to_shell.add_cli()

class cli_log_console(cli_main_menu):
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
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_log_console #LOG TO CONSOLE
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
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_log_console.add_cli()

class cli_log_level(cli_main_menu):
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
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_cli_log_level
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
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_log_level.add_cli()

class dps_log_level(cli_main_menu):
    '''
    Represents the CLI Object to Log to Console
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level_dps'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set DPS Debugging Log Level'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_MAIN_MENU
    #CLI CODE
    cli_code = cli_main_menu_code.cli_code_dps_log_level
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
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
dps_log_level.add_cli()
