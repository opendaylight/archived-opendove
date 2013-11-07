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

class cli_interface_cluster:
    '''
    Represents Data Objects CLI Definitions for the DPS Client Server 
    Protocol. Should be in sync with the following files:
    1. cli/inc/data_objects.h file
    '''
    #CLI_DATA_OBJECTS_CODES
    LOG_LEVEL = 0
    DPS_NODE_ADD = 1
    DPS_NODE_DEL = 2
    DPS_NODE_ADD_DOMAIN = 3
    DPS_NODE_DEL_DOMAIN = 4
    DPS_NODE_DEL_ALL_DOMAINS = 5
    SHOW = 6
    DOMAIN_GET_NODES = 7
    HEAVY_LOAD_THRESHOLD = 8
    LOCAL_ACTIVATE = 9

    node_activate_map = {'deactivate': 0, 'activate': 1}
    node_activate_map_range = node_activate_map.keys()

class cli_cluster(cli_dps_config):
    '''
    Represents the CLI Object for Data Objects
    '''
    #The input to display for Context:
    user_input_string = 'cluster'
    user_input_string_raw = 'cluster'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'cluster'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Cluster Configuration (Remote Nodes)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_DATA_OBJECTS
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_DATA_OBJECTS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [] #Empty
    support_random = False
    #This function 'execute' must exist for all objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return cli_cluster
        '''
        return cli_cluster
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_cluster.add_context()

class cli_cluster_log_level(cli_cluster):
    '''
    Represents the CLI Object for Changing Log Level in the DPS Cluster 
    Configuration
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'log_level'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Log Level'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_log_level_s{
    #    uint32_t    level;
    #}cli_cluster_log_level_t;
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
cli_cluster_log_level.add_cli()

class cli_cluster_local_activate(cli_cluster):
    '''
    Represents the CLI Object for Changing Log Level in the DPS Cluster 
    Configuration
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'set-role'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Activate/Deactivate Local Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.LOCAL_ACTIVATE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('action', cli_type_string_set, True, cli_interface_cluster.node_activate_map_range)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_log_level_s{
    #    uint32_t    activate;
    #}cli_cluster_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        level_int = cli_interface_cluster.node_activate_map[self.params[0]]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, level_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_local_activate.add_cli()

class cli_cluster_heavy_load_value(cli_cluster):
    '''
    Represents the CLI Object for Changing the Heavy Load Threshold Level
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'load_threshold'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'The Load Threshold for Load Balancing'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.HEAVY_LOAD_THRESHOLD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('load', cli_type_int, True, [1,100])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_log_level_s{
    #    uint32_t    level;
    #}cli_cluster_log_level_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        level_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, level_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_heavy_load_value.add_cli()

class cli_cluster_node(cli_cluster):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'node'
    user_input_string_raw = '%s:%s'%(cli_cluster.user_input_string, user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'node'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Remote Node Related Configuration'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_DATA_OBJECTS
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_DATA_OBJECTS
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
        @return cli_cluster_node
        '''
        return cli_cluster_node
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_cluster_node.add_context()

class cli_cluster_node_add(cli_cluster_node):
    '''
    Represents the CLI Object for adding a remote DPS Node to the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add a DPS (remote) Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DPS_NODE_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip),
                      ('Port', cli_type_int, True, cli_interface.range_port)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_node_add_s{
    #    uint16_t port;
    #    uint16_t pad;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_cluster_node_add_t;
    fmt = 'II'
    fmt += 'HH'
    fmt += 'II12s' #IPv4
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
cli_cluster_node_add.add_cli()

class cli_cluster_node_delete(cli_cluster_node):
    '''
    Represents the CLI Object for deleting a remote DPS Node from the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete a (remote) DPS Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DPS_NODE_DEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_node_add_s{
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_cluster_node_add_t;
    fmt = 'III'
    fmt += 'I12s' #IPv4
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               socket.AF_INET, ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_node_delete.add_cli()

class cli_cluster_node_add_domain(cli_cluster_node):
    '''
    Represents the CLI Object for adding a domain to a remote DPS Node
    in the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Domain ID to a (remote)Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DPS_NODE_ADD_DOMAIN
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip),
                      ('Port', cli_type_int, True, cli_interface.range_port),
                      ('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_node_add_domain_s{
    #    uint32_t domain;
    #    uint16_t port;
    #    uint16_t pad;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_cluster_node_add_domain_t;
    fmt = 'III'
    fmt += 'HH'
    fmt += 'II12s' #IPv4
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        port = self.params[1]
        domain_id = self.params[2]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               domain_id,
                               port, 0,
                               socket.AF_INET, ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_node_add_domain.add_cli()

class cli_cluster_node_delete_domain(cli_cluster_node):
    '''
    Represents the CLI Object for deleting a domain from a remote DPS Node
    in the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete Domain ID from a (remote)Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DPS_NODE_DEL_DOMAIN
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip),
                      ('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_node_delete_domain_s{
    #    uint32_t domain;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_cluster_node_delete_domain_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        domain_id = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               domain_id,
                               socket.AF_INET, ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_node_delete_domain.add_cli()

class cli_cluster_node_delete_all_domains(cli_cluster_node):
    '''
    Represents the CLI Object for deleting all domains from a remote DPS Node
    in the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_remove_all'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Remove all domains from a(remote) DPS Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DPS_NODE_DEL_ALL_DOMAINS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('IP_Address', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_node_add_s{
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_cluster_node_add_t;
    fmt = 'III'
    fmt += 'I12s' #IPv4
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               socket.AF_INET, ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_node_delete_all_domains.add_cli()

class cli_cluster_show(cli_cluster):
    '''
    Represents the CLI Context for Show Command in the DPS Client Server
    Protocol
    '''
    #The input to display for Context:
    user_input_string = 'show'
    user_input_string_raw = '%s:%s'%(cli_cluster.user_input_string, user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Command (Cluster Nodes)'
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
        return cli_cluster_show
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_cluster_show.add_context()

class cli_cluster_show_all(cli_cluster_show):
    '''
    Represents the CLI Object for deleting all domains from a remote DPS Node
    in the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'details'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Entire Cluster Details'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.SHOW
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
cli_cluster_show_all.add_cli()

class cli_cluster_domain_show_nodes(cli_cluster_show):
    '''
    Represents the CLI Object for deleting a domain from a remote DPS Node
    in the Cluster
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_nodes'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Nodes in a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_CLUSTER
    #CLI CODE
    cli_code = cli_interface_cluster.DOMAIN_GET_NODES
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('max_nodes', cli_type_int, True, [1,32])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_cluster_domain_nodes_s{
    #    uint32_t domain_id;
    #    uint32_t max_nodes;
    #}cli_cluster_domain_nodes_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain_id = self.params[0]
        max_nodes = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               domain_id, max_nodes)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_cluster_domain_show_nodes.add_cli()
