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

class cli_interface_data_objects:
    '''
    Represents Data Objects CLI Definitions for the DPS Client Server 
    Protocol. Should be in sync with the following files:
    1. cli/inc/data_objects.h file
    '''
    #CLI_DATA_OBJECTS_CODES
    LOG_LEVEL = 0
    DOMAIN_ADD = 1
    DOMAIN_DELETE = 2
    DVG_ADD = 3
    DVG_DELETE = 4
    ENDPOINT_UPDATE = 5
    ENDPOINT_LOOKUP_MAC = 6
    ENDPOINT_LOOKUP_IP = 7
    POLICY_LOOKUP_MAC = 8
    POLICY_LOOKUP_IP = 9
    DOMAIN_SHOW = 10
    PERFORMANCE_TEST = 11
    POLICY_ADD = 12
    POLICY_DELETE = 13
    EXTERNAL_GATEWAY_ADD = 14
    EXTERNAL_GATEWAY_DEL = 15
    EXTERNAL_GATEWAY_CLR = 16
    IP_SUBNET_CREATE     = 17
    IP_SUBNET_ADD_SUBNET = 18
    IP_SUBNET_DELETE_SUBNET = 19
    IP_SUBNET_LOOKUP     = 20
    IP_SUBNET_DESTROY    = 21
    IP_SUBNET_LIST       = 22
    IP_SUBNET_FLUSH       = 23
    ENDPOINT_CONFLICT_TEST = 24
    OUTSTANDING_CONTEXT_COUNT = 25
    VNID_SHOW = 26
    QUERY_VNID = 27
    DOMAIN_GLOBAL_SHOW = 28
    TUNNEL_REGISTER = 29
    TUNNEL_UNREGISTER = 30
    MULTICAST_RECEIVER_ADD = 31
    MULTICAST_RECEIVER_DEL = 32
    MULTICAST_SENDER_ADD = 33
    MULTICAST_SENDER_DEL = 34
    MULTICAST_SHOW = 35
    ADDRESS_RESOLUTION_SHOW = 36
    EXTERNAL_GATEWAY_GET = 37
    VLAN_GATEWAY_GET = 38
    HEARTBEAT_REPORT_INTERVAL_SET = 39
    STATISTICS_REPORT_INTERVAL_SET = 40
    MASS_TRANSFER_GET_READY = 41
    MASS_TRANSFER_START = 42
    DOMAIN_ACTIVATE = 43
    DOMAIN_DEACTIVATE = 44
    MULTICAST_LOG_LEVEL = 45
    DOMAIN_UPDATE = 46
    LOAD_BALANCE_TEST = 47
    DOMAIN_DELETE_ALL_LOCAL = 48
    ENPOINT_MIGRATE_HINT = 49
    MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD = 50
    MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL = 51
    MULTICAST_RECEIVER_GLOBAL_SCOPE_GET = 52
    DPS_CLIENTS_SHOW = 53

    details_map = {'details': 1, 'overview': 0}
    range_details_map = details_map.keys()
    #Endpoint Update Options
    #typedef enum {
    #    DPS_ENDPOINT_UPDATE_ADD = 1,
    #    DPS_ENDPOINT_UPDATE_DELETE,
    #    DPS_ENDPOINT_UPDATE_VIP_ADD,
    #    DPS_ENDPOINT_UPDATE_VIP_DEL,
    #    DPS_ENDPOINT_UPDATE_MIGRATE_IN,
    #    DPS_ENDPOINT_UPDATE_MIGRATE_OUT,
    #} dps_endpoint_update_option;
    endpoint_update_options = {'add': 1, 'delete': 2, 'vIP_add': 3, 'vIP_del': 4, 'migrate_in': 5, 'migrate_out': 6}
    endpoint_update_options_range = endpoint_update_options.keys()
    #typedef enum {
    #    DPS_POLICY_SERVER_ID = 1,
    #    DPS_SWITCH_AGENT_ID = 2,
    #    DOVE_VLAN_GATEWAY_AGENT_ID = 3,
    #    DOVE_EXTERNAL_GATEWAY_AGENT_ID = 4,
    #} dps_client_id;
    dps_client_types = {'dove_switch': 2, 'vlan_gateway': 3, 'external_gateway': 4}
    dps_client_types_range = dps_client_types.keys()
    multicast_sender_options = ['register','unregister']
    multicast_receiver_options = ['join','leave','global_add','global_delete']
    multicast_receiver_operations = {'join': MULTICAST_RECEIVER_ADD,
                                     'leave': MULTICAST_RECEIVER_DEL,
                                     'global_add': MULTICAST_RECEIVER_GLOBAL_SCOPE_ADD,
                                     'global_delete': MULTICAST_RECEIVER_GLOBAL_SCOPE_DEL}
    traffic_type_unicast = 0
    traffic_type_multicast = 1

class cli_dps_objects(cli_dps_config):
    '''
    Represents the CLI Object for Data Objects
    '''
    #The input to display for Context:
    user_input_string = 'dmc'
    user_input_string_raw = 'odmc'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dmc_config'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DMC Related Configuration (Debug)'
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
        @return None
        '''
        return cli_dps_objects
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_dps_objects.add_context()

class cli_data_objects_log_level(cli_dps_objects):
    '''
    Represents the CLI Object for Changing Log Level in the DPS Data Handler
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
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_log_level_s{
    #    /**
    #     * \brief log_level representing log level defined in log.h file
    #     */
    #    uint32_t    level;
    #}cli_data_object_log_level_t;
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
cli_data_objects_log_level.add_cli()

class cli_data_objects_domain(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'domain'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Domain Related Configuration'
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
    value = None
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return None
        '''
        return cli_data_objects_domain
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        cli_data_objects_domain.value = None
        return
#Add this class of command to global list of supported commands
cli_data_objects_domain.add_context()

class cli_data_objects_domain_value(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    with a value (for testing)
    '''
    #The input to display for Context:
    user_input_string = 'domain_val_context'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_val'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Domain Related Configuration'
    #Whether it's a hidden command
    hidden = True
    #Whether the command is only available in god mode
    GodModeOnly = True
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_DATA_OBJECTS
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_DATA_OBJECTS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain id', cli_type_int, True, cli_interface.range_domain)] #Empty
    support_random = False
    value = None
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return None
        '''
        return_cli_context = None
        try:
            cli_data_objects_domain.value = self.params[0]
            return_cli_context = cli_data_objects_domain
        except Exception:
            pass
        return return_cli_context
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        cli_data_objects_domain.value = None
        return
#Add this class of command to global list of supported commands
#cli_data_objects_domain_value.add_context()

class cli_data_objects_domain_add(cli_data_objects_domain):
    '''
    Represents the CLI Object for Adding a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_add_s{
    #    /**
    #     * \brief The Domain ID
    #     */
    #    uint32_t    domain_id;
    #}cli_data_object_domain_add_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_domain_add.add_cli()

class cli_data_objects_domain_update(cli_data_objects_domain):
    '''
    Represents the CLI Object for Adding a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'update'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Update Replication Factor of a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_UPDATE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('replication_factor', cli_type_int, True, cli_interface.range_replication)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_update_s{
    #    uint32_t    domain_id;
    #    uint32_t    replication_factor;
    #}cli_data_object_domain_update_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain_int = self.params[0]
        replication = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int, replication)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_domain_update.add_cli()

class cli_data_objects_domain_delete(cli_data_objects_domain):
    '''
    Represents the CLI Object for deleting a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete Domain from DOVE Environment'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_DELETE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_delete_s{
    #    /**
    #     * \brief The Domain ID
    #     */
    #    uint32_t    domain_id;
    #}cli_data_object_domain_delete_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
#        domain_range = self.params[0]
#        domains = domain_range.split('-')
#        domain_min = int(domains[0])
#        domain_max = int(domains[1])
#        for domain in range(domain_min, domain_max):
#            c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain)
#            ret = dcslib.process_cli_data(c_struct)
#            cli_interface.process_error(ret)
        domain_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_domain_delete.add_cli()

class cli_data_objects_domain_delete_all(cli_data_objects_domain):
    '''
    Represents the CLI Object for deleting all Domain handled by this node
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'clear'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete All Domain from this Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_DELETE_ALL_LOCAL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_delete_all_s{
    #}cli_data_object_domain_delete_all_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_domain_delete_all.add_cli()

class cli_data_objects_vnid(cli_dps_objects):
    '''
    Represents the CLI Context for VNID Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'vnid'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vnid'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'VNID Related Configuration'
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
        @return cli_data_objects_vnid
        '''
        return cli_data_objects_vnid
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_vnid.add_context()

class cli_data_objects_vnid_add(cli_data_objects_vnid):
    '''
    Represents the CLI Object for adding a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add a VNID to a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DVG_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_dvg_add_s{
    #    /**
    #     * \brief The Domain ID
    #     */
    #    uint32_t    domain_id;
    #    /**
    #     * \brief The DVG ID
    #     */
    #    uint32_t    dvg_id;
    #}cli_data_object_dvg_add_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        vnid = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain, vnid)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_vnid_add.add_cli()

class cli_data_objects_vnid_delete(cli_data_objects_vnid):
    '''
    Represents the CLI Object for deleting a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete a VNID'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DVG_DELETE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_dvg_delete_s{
    #    uint32_t    domain_id;
    #}cli_data_object_dvg_delete_t;
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
cli_data_objects_vnid_delete.add_cli()

#####################################################
#DPS Query Dove Controller VNIDs
#
#####################################################

class cli_data_objects_query_vnid(cli_data_objects_vnid):
    '''
    Represents the CLI Object for Query Dove Controller VNID by Restful http
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'query'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Query VNID from Controller'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.QUERY_VNID
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    #    command_format = [('iterations', cli_type_int, True, [1,10000000]),
    #                      ('print_frequency', cli_type_int, True, [1,1000000])]
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_perf_test_s{
    #    int        iterations;
    #    int        print_frequency;
    #} cli_data_object_perf_test_t;
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
cli_data_objects_query_vnid.add_cli()

class cli_data_objects_policy(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'policy'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'policy'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Policy Related Configuration'
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
        @return None
        '''
        return cli_data_objects_policy
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_policy.add_context()

class cli_data_objects_policy_add(cli_data_objects_policy):
    '''
    Represents the CLI Object for Policy Add
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add a Policy'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain', cli_type_int, True, cli_interface.range_domain),
                      ('policy_type', cli_type_string_set, True, cli_interface.range_policy_type),
                      ('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('ttl (minutes)', cli_type_int, True, cli_interface.range_policy_ttl),
                      ('action', cli_type_string_set, True, cli_interface.range_policy_action_connectivity)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_add_s{
    #        uint32_t traffic_type;
    #        uint32_t domain_id;
    #        uint32_t type;
    #        uint32_t src_dvg_id;
    #        uint32_t dst_dvg_id;
    #        uint32_t ttl;
    #        cli_data_object_policy_action_t action;
    #}cli_data_object_policy_add_t;
    fmt = 'IIIIIIII'
    fmt += 'BBH' #ver, pad, connectivity in action struct
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        policy_type = cli_interface.policy_type_map[self.params[1]]
        src_dvg = self.params[2]
        dst_dvg = self.params[3]
        ttl = self.params[4]
        action = cli_interface.policy_connectivity_map[self.params[5]]
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               cli_interface_data_objects.traffic_type_unicast, 
                               domain, policy_type, src_dvg, 
                               dst_dvg, ttl, 1, 0, action)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_policy_add.add_cli()

class cli_data_objects_policy_delete(cli_data_objects_policy):
    '''
    Represents the CLI Object for deleting a Policy
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete a Policy'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_DELETE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_delete_s{
    #    uint32_t    domain_id;
    #    uint32_t    src_dvg_id;
    #    uint32_t    dst_dvg_id;
    #}cli_data_object_policy_delete_t;
    fmt = 'IIIIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        src_dvg_id = self.params[1]
        dst_dvg_id = self.params[2]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               cli_interface_data_objects.traffic_type_unicast, 
                               domain, src_dvg_id, dst_dvg_id)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_policy_delete.add_cli()

class cli_data_objects_policy_lookup_mac(cli_data_objects_policy):
    '''
    Represents the CLI Object for Policy Resolution using MAC
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'lookup_mac'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Policy Resolution MAC'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_LOOKUP_MAC
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vMAC', cli_type_mac, True, cli_interface.range_mac)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_lookup_vMac_s{
    #    uint32_t    src_vnid;
    #    char        vMac[6];
    #}cli_data_object_policy_lookup_vMac_t;
    fmt = 'III6s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        src_vnid = self.params[0]
        vMAC = cli_type_mac.bytes(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, src_vnid, vMAC)
        ret = dcslib.process_cli_data(c_struct)
        if ret == cli_interface.DOVE_STATUS_OK:
            print 'Policy found in Collection\r'
        else:
            cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_policy_lookup_mac.add_cli()

class cli_data_objects_policy_lookup_ip(cli_data_objects_policy):
    '''
    Represents the CLI Object for Policy Resolution using IP Address
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    #command_id = 'endpoint_lookup_vIP'
    command_id = 'lookup_vIP'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Policy Resolution IPv4'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_LOOKUP_IP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vIPv4', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_lookup_vIP_s{
    #    uint32_t    src_vnid;
    #    uint32_t    vIP_type;
    #    union{
    #        uint32_t    vIPv4;
    #        char        vIPv6[16];
    #    };
    #}cli_data_object_policy_lookup_vIP_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        src_vnid = self.params[0]
        vIP = cli_type_ip.ipv4_atoi(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, src_vnid,
                               socket.AF_INET, vIP, '')
        ret = dcslib.process_cli_data(c_struct)
        if ret == cli_interface.DOVE_STATUS_OK:
            print 'Policy found in Collection\r'
        else:
            cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_policy_lookup_ip.add_cli()

class cli_data_objects_endpoint(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'endpoint'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'endpoint'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Endpoint Related Configuration'
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
        @return cli_data_objects_endpoint
        '''
        return cli_data_objects_endpoint
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_endpoint.add_context()

class cli_data_objects_endpoint_update(cli_data_objects_endpoint):
    '''
    Represents the CLI Object for Endpoint Update
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'update'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Endpoint Update'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.ENDPOINT_UPDATE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('client_type', cli_type_string_set, True, cli_interface_data_objects.dps_client_types_range),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('operation', cli_type_string_set, True, cli_interface_data_objects.endpoint_update_options_range),
                      ('physical IP(IPv4)', cli_type_ip, True, cli_interface.range_pip),
                      ('vIP(IPv4)', cli_type_ip, True, cli_interface.range_ip),
                      ('vMAC', cli_type_mac, True, cli_interface.range_mac)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_endpoint_update_s{
    #    uint32_t    vnid;
    #    uint32_t    update_op;
    #    uint32_t    client_type;
    #    uint32_t    pIP_type;
    #    union{
    #        uint32_t    pIPv4;
    #        char        pIPv6[16];
    #    };
    #    uint32_t    vIP_type;
    #    union{
    #        uint32_t    vIPv4;
    #        char        vIPv6[16];
    #    };
    #    char        vMac[6];
    #}cli_data_object_endpoint_update_t;
    #Only support IPv4 now
    fmt = 'II'
    fmt += 'IIII' #vnid, operation, pIP_type
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    fmt += 'I' #vIP_type
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    fmt += '6s' #vMAC
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        client_type = cli_interface_data_objects.dps_client_types[self.params[0]]
        vnid = self.params[1]
        operation = cli_interface_data_objects.endpoint_update_options[self.params[2]]
        pIP = cli_type_ip.ipv4_atoi(self.params[3])
        vIP = cli_type_ip.ipv4_atoi(self.params[4])
        vMAC = cli_type_mac.bytes(self.params[5])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               vnid, operation, client_type,
                               socket.AF_INET,
                               pIP, '',
                               socket.AF_INET,
                               vIP, '',
                               vMAC)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_endpoint_update.add_cli()

class cli_data_objects_endpoint_lookup_mac(cli_data_objects_endpoint):
    '''
    Represents the CLI Object for Endpoint Lookup using MAC
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'lookup_mac'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Endpoint Lookup MAC'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.ENDPOINT_LOOKUP_MAC
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_domain),
                      ('vMAC', cli_type_mac, True, cli_interface.range_mac)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_endpoint_lookup_vMac_s{
    #    uint32_t    vnid;
    #    char        vMac[6];
    #}cli_data_object_endpoint_lookup_vMac_t;
    fmt = 'III6s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        vMAC = cli_type_mac.bytes(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid, vMAC)
        ret = dcslib.process_cli_data(c_struct)
        if ret == cli_interface.DOVE_STATUS_OK:
            print 'Endpoint found in Collection\r'
        else:
            cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_endpoint_lookup_mac.add_cli()

class cli_data_objects_endpoint_lookup_ip(cli_data_objects_endpoint):
    '''
    Represents the CLI Object for Endpoint Lookup using IP Address
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    #command_id = 'endpoint_lookup_vIP'
    command_id = 'lookup_vIP'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Endpoint Lookup IPv4'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.ENDPOINT_LOOKUP_IP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_domain),
                      ('vIPv4', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_endpoint_lookup_vIP_s{
    #    uint32_t    vnid;
    #    uint32_t    vIP_type;
    #    union{
    #        uint32_t    vIPv4;
    #        char        vIPv6[16];
    #    };
    #}cli_data_object_endpoint_lookup_vIP_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        vIP = cli_type_ip.ipv4_atoi(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid,
                               socket.AF_INET, vIP, '')
        ret = dcslib.process_cli_data(c_struct)
        if ret == cli_interface.DOVE_STATUS_OK:
            print 'Endpoint found in Collection\r'
        else:
            cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_endpoint_lookup_ip.add_cli()

class cli_data_objects_endpoint_migrate_hint(cli_data_objects_endpoint):
    '''
    Represents the CLI Object for Endpoint Lookup using IP Address
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    #command_id = 'endpoint_lookup_vIP'
    command_id = 'migrate_hint'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Endpoint Migrate Hint'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.ENPOINT_MIGRATE_HINT
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_domain),
                      ('src_pIPv4', cli_type_ip, True, cli_interface.range_ip),
                      ('src_vIPv4', cli_type_ip, True, cli_interface.range_ip),
                      ('migrated_vIPv4', cli_type_ip, True, cli_interface.range_ip),]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_endpoint_migrate_s{
    #    uint32_t vnid;
    #    uint32_t src_pIP_type;
    #    union {
    #        uint32_t src_pIPv4;
    #        char src_pIPv6[16];
    #    };
    #    uint32_t src_vIP_type;
    #    union {
    #        uint32_t src_vIPv4;
    #        char src_vIPv6[16];
    #    };
    #    uint32_t dst_vIP_type;
    #    union {
    #        uint32_t dst_vIPv4;
    #        char dst_vIPv6[16];
    #    };
    #}cli_data_object_endpoint_migrate_t;
    fmt = 'III'
    fmt += 'II12s' #Type + IPv4 + 12 extra bytes = 16 bytes for union
    fmt += 'II12s' #Type + IPv4 + 12 extra bytes = 16 bytes for union
    fmt += 'II12s' #Type + IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        pIP_src = cli_type_ip.ipv4_atoi(self.params[1])
        vIP_src = cli_type_ip.ipv4_atoi(self.params[2])
        vIP_dst = cli_type_ip.ipv4_atoi(self.params[3])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid,
                               socket.AF_INET, pIP_src, '',
                               socket.AF_INET, vIP_src, '',
                               socket.AF_INET, vIP_dst, '')
        ret = dcslib.process_cli_data(c_struct)
        if ret != cli_interface.DOVE_STATUS_OK:
            cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_endpoint_migrate_hint.add_cli()

class cli_data_objects_gateway(cli_dps_objects):
    '''
    Represents the CLI Context for Gateway Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'gateway'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'gateway'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Gateway Related Configuration'
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
        @return cli_data_objects_gateway
        '''
        return cli_data_objects_gateway
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_gateway.add_context()

class cli_data_objects_gateway_external_add(cli_data_objects_gateway):
    '''
    Represents the CLI Object for adding External Gateway
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'external_add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add External Gateway'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.EXTERNAL_GATEWAY_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('IPv4', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_gateway_s{
    #    uint32_t domain_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_data_object_gateway_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        IP = cli_type_ip.ipv4_atoi(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain,
                               socket.AF_INET, IP, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_gateway_external_add.add_cli()

class cli_data_objects_gateway_external_del(cli_data_objects_gateway):
    '''
    Represents the CLI Object for deleting External Gateway
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'external_delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete External Gateway'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.EXTERNAL_GATEWAY_DEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('IPv4', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_gateway_s{
    #    uint32_t domain_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_data_object_gateway_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        IP = cli_type_ip.ipv4_atoi(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain,
                               socket.AF_INET, IP, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_gateway_external_del.add_cli()

class cli_data_objects_gateway_external_clr(cli_data_objects_gateway):
    '''
    Represents the CLI Object for clearing External Gateways
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'external_clear'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Clear External Gateways'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.EXTERNAL_GATEWAY_CLR
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_gateway_s{
    #    uint32_t domain_id;
    #}cli_data_object_domain_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_gateway_external_clr.add_cli()

class cli_data_objects_gateway_external_get(cli_data_objects_gateway):
    '''
    Represents the CLI Object for getting list External Gateways
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'external_get'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get External Gateways'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.EXTERNAL_GATEWAY_GET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_gateway_s{
    #    uint32_t domain_id;
    #}cli_data_object_domain_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_gateway_external_get.add_cli()

class cli_data_objects_gateway_vlan_get(cli_data_objects_gateway):
    '''
    Represents the CLI Object for getting list External Gateways
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vlan_get'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get VLAN Gateways'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.VLAN_GATEWAY_GET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_gateway_s{
    #    uint32_t domain_id;
    #}cli_data_object_domain_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_gateway_vlan_get.add_cli()

class cli_data_objects_show(cli_dps_objects):
    '''
    Represents the CLI Context for Show Command in the DPS Client Server
    Protocol
    '''
    #The input to display for Context:
    user_input_string = 'show'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Command (DPS Objects)'
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
        @return cli_data_objects_show
        '''
        return cli_data_objects_show
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_show.add_context()

class cli_data_objects_domain_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Domain Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Domain Overview/Details'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain_show),
                      ('level_of_detail', cli_type_string_set, False, cli_interface_data_objects.range_details_map)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_show_s{
    #    uint32_t    domain_id;
    #    uint32_t    fDetails;
    #}cli_data_object_domain_show_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain_int = self.params[0]
        try:
            details = cli_interface_data_objects.details_map[self.params[1]]
        except Exception:
            details = 0
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int, details)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_domain_show.add_cli()

class cli_data_objects_domain_global_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Global Domain Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'mapping'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Global VNID mapping'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_GLOBAL_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = []
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_show_s{
    #    uint32_t    domain_id;
    #    uint32_t    fDetails;
    #}cli_data_object_domain_show_t;
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
cli_data_objects_domain_global_show.add_cli()

class cli_data_objects_vnid_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing VNID Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vnid'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show VNID Overview/Details'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.VNID_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg_show),
                      ('level_of_detail', cli_type_string_set, False, cli_interface_data_objects.range_details_map)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_vnid_show_s{
    #    uint32_t    vnid;
    #    uint32_t    fDetails;
    #}cli_data_object_vnid_show_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        vnid = self.params[0]
        try:
            details = cli_interface_data_objects.details_map[self.params[1]]
        except Exception:
            details = 0
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid, details)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_vnid_show.add_cli()

class cli_data_objects_context_count_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Domain Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'context_count'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Outstanding Context Count'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.OUTSTANDING_CONTEXT_COUNT
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
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
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_context_count_show.add_cli()

class cli_data_objects_heartbeat_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Heartbeat Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'heartbeat'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show heartbeat content'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.OUTSTANDING_CONTEXT_COUNT
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
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
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_heartbeat_show.add_cli()

class cli_data_objects_dps_clients_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing DPS Clients
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dps_clients'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show List of DPS Clients'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DPS_CLIENTS_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
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
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_dps_clients_show.add_cli()

####################################################################
####################################################################
# IP Subnet CLI
class cli_ipsubnet(cli_dps_objects):
    '''
    Represents the CLI Object for Data Objects
    '''
    #The input to display for Context:
    user_input_string = 'ipsubnet'
    user_input_string_raw = 'ipsubnet'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'ipsubnet'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'IP subnet Configuration'
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

    #In this system, now only 1 ip subnet list can be created, so if it has been created, it can not 
    #be created again
    ip_subnet_list1_been_created = 0
    #This function 'execute' must exist for all objects
    def execute(self, session):
        '''
        Execute Login: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return cli_ipsubnet
        '''
        return cli_ipsubnet
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_ipsubnet.add_context()

class cli_ipsubnet_add(cli_ipsubnet):
    '''
    Represents the CLI Object for adding an IP Subnet to the Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add an IP Subnet, including an IP Address and an IP Mask'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.IP_SUBNET_ADD_SUBNET

    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('associated_type', cli_type_string_set, True, cli_interface.range_ipsubnet_associated_type),
                      ('associated_id', cli_type_int, True, cli_interface.range_domain),
                      ('subnet', cli_type_ip, True, cli_interface.range_ip),
                      ('mask', cli_type_ip, True, cli_interface.range_ip),
                      ('mode', cli_type_string_set, True, cli_interface.range_ipsubnet_mode),
                      ('gateway', cli_type_ip, True, cli_interface.range_ip)]

    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_ip_subnet_add_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #    union {
    #        uint32_t mask;
    #        uint32_t prefix_len;
    #    };
    #    uint32_t mode;
    #    union {
    #	     uint32_t gateway_v4;
    #	     char gateway_v6[16];
    #    };
    #}cli_data_object_ip_subnet_add_t;


    fmt = 'II'
    fmt += 'IIII12sIII12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        associated_type = cli_interface.ipsubnet_associated_type_map[self.params[0]]
        associated_id = self.params[1]
        #uDestNet
        ipaddr = cli_type_ip.ipv4_atoi(self.params[2])
        #Mask
        ipmask = cli_type_ip.ipv4_atoi(self.params[3])
        #Mode
        mode = cli_interface.ipsubnet_mode_map[self.params[4]]
        #Gateway
        gateway = cli_type_ip.ipv4_atoi(self.params[5])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, associated_type, associated_id, socket.AF_INET, ipaddr, '', ipmask, mode, gateway, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)

#Add this class of command to global list of supported commands
cli_ipsubnet_add.add_cli()

class cli_ipsubnet_delete(cli_ipsubnet):
    '''
    Represents the CLI Object for deleting an IP Subnet from the Domain 
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete an IP Subnet, including an IP Prefix and an IP Mask'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.IP_SUBNET_DELETE_SUBNET

    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('associated_type', cli_type_string_set, True, cli_interface.range_ipsubnet_associated_type),
                      ('associated_id', cli_type_int, True, cli_interface.range_domain),
                      ('ip_address', cli_type_ip, True, cli_interface.range_ip),
                      ('ip_mask', cli_type_ip, True, cli_interface.range_ip)]
    #                  ('Port', cli_type_int, True, cli_interface.range_port)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_ip_subnet_delete_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #    union {
    #        uint32_t mask;
    #        uint32_t prefix_len;
    #    };
    #}cli_data_object_ip_subnet_delete_t;
    fmt = 'II'
    fmt += 'IIII12sI' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        associated_type = cli_interface.ipsubnet_associated_type_map[self.params[0]]
        associated_id = self.params[1]
        #uDestNet
        ipaddr = cli_type_ip.ipv4_atoi(self.params[2])
        #Mask
        ipmask = cli_type_ip.ipv4_atoi(self.params[3])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, associated_type, associated_id, socket.AF_INET, ipaddr, '', ipmask)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)

#Add this class of command to global list of supported commands
cli_ipsubnet_delete.add_cli()

class cli_ipsubnet_lookup(cli_ipsubnet):
    '''
    Represents the CLI Object for looking up an IP Subnet from the Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'lookup'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Lookup an IP Address, get the IP Subnet info'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.IP_SUBNET_LOOKUP

    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('ip_address', cli_type_ip, True, cli_interface.range_ip)]
    #                  ('Port', cli_type_int, True, cli_interface.range_port)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_ip_subnet_lookup_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_data_object_ip_subnet_lookup_t;
    fmt = 'II'
    fmt += 'III12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        #uDestNet
        ipaddr = cli_type_ip.ipv4_atoi(self.params[1])  
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain, socket.AF_INET, ipaddr, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
#cli_ipsubnet_lookup.add_cli()

class cli_ipsubnet_list(cli_ipsubnet):
    '''
    Represents the CLI Object for list all IP Subnets in the Domain 
    '''
    #The input to display for Context:
    #user_input_string = 'create'
    #user_input_string_raw = '%s:%s'%(cli_cluster.user_input_string,
    #                                 user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'list'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'List all ip subnets in an IP Subnet List'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True

    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.IP_SUBNET_LIST

    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_DATA_OBJECTS
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_DATA_OBJECTS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('associated_type', cli_type_string_set, True, cli_interface.range_ipsubnet_associated_type),
                      ('associated_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False

    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_ip_subnet_list_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #    uint32_t IP_type;
    #}cli_data_object_ip_subnet_list_t;

    fmt = 'IIIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        associated_type = cli_interface.ipsubnet_associated_type_map[self.params[0]]
        associated_id = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, associated_type, associated_id, socket.AF_INET)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_ipsubnet_list.add_cli()

class cli_ipsubnet_flush(cli_ipsubnet):
    '''
    Represents the CLI Object for flush all IP Subnets in the Domain
    '''
    #The input to display for Context:
    #user_input_string = 'create'
    #user_input_string_raw = '%s:%s'%(cli_cluster.user_input_string,
    #                                 user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'flush'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Flush all ip subnets in an IP Subnet List'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.IP_SUBNET_FLUSH
    #MAJOR CODE not needed for this context
    #major_type = cli_interface.CLI_DATA_OBJECTS
    #CLI CODE not needed for this context
    #cli_code = cli_interface.CLI_DATA_OBJECTS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Required', 'Random Function', '(Input to Random Function)' )
    #All optional parameters MUST come after the required parameters
    command_format = [('associated_type', cli_type_string_set, True, cli_interface.range_ipsubnet_associated_type),
                      ('associated_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_ip_subnet_flush_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #    uint32_t IP_type;
    #}cli_data_object_ip_subnet_flush_t;
    fmt = 'IIIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        associated_type = cli_interface.ipsubnet_associated_type_map[self.params[0]]
        associated_id = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, associated_type, associated_id, socket.AF_INET)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_ipsubnet_flush.add_cli()

class cli_tunnel(cli_dps_objects):
    '''
    Represents the CLI Context for Tunnel Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'tunnel'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'tunnel'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Tunnel Related Configuration'
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
        @return cli_tunnel
        '''
        return cli_tunnel
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_tunnel.add_context()

##############################################################
#Add the CLI for IP Subnet
##############################################################
class cli_tunnel_register(cli_tunnel):
    '''
    Represents the CLI Object for adding an IP Subnet to the Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'register'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Register Tunnel IP Address (4 max)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.TUNNEL_REGISTER
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('client_type', cli_type_string_set, True, cli_interface_data_objects.dps_client_types_range),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('ip_address1', cli_type_ip, True, cli_interface.range_ip),
                      ('ip_address2', cli_type_ip, False, cli_interface.range_ip)
                      ]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_tunnel_register_s{
    #    uint32_t client_type;
    #    uint32_t vnid;
    #    uint32_t num_tunnels;
    #    struct{
    #        uint32_t IP_type;
    #        union {
    #            uint32_t pIPv4;
    #            char pIPv6[16];
    #        };
    #    }ip_list[4];
    #}cli_data_object_tunnel_register_t;
    base_fmt = 'IIIII'
    ip_fmt = 'II12s' #type, IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        ip_list = []
        #self.params hold the variables
        client_type = cli_interface_data_objects.dps_client_types[self.params[0]]
        vnid = self.params[1]
        num_elements = 0
        for i in range(2):
            try:
                #From the 2nd parameter onwards all IP Addresses
                ipaddr = cli_type_ip.ipv4_atoi(self.params[2+i])
                ip_list.append(ipaddr)
                num_elements += 1
            except Exception:
                break
        c_struct = struct.pack(self.base_fmt, self.major_type, self.cli_code, client_type, vnid, num_elements)
        for i in range(num_elements):
            c_struct += struct.pack(self.ip_fmt, socket.AF_INET, ip_list[i], '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
cli_tunnel_register.add_cli()

class cli_tunnel_deregister(cli_tunnel):
    '''
    Represents the CLI Object for adding an IP Subnet to the Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'deregister'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'De-Register Tunnel IP Address (4 max)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.TUNNEL_UNREGISTER
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('client_type', cli_type_string_set, True, cli_interface_data_objects.dps_client_types_range),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('ip_address1', cli_type_ip, True, cli_interface.range_ip),
                      ('ip_address2', cli_type_ip, False, cli_interface.range_ip)
                      ]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_tunnel_register_s{
    #    uint32_t client_type;
    #    uint32_t vnid;
    #    uint32_t num_tunnels;
    #    struct{
    #        uint32_t IP_type;
    #        union {
    #            uint32_t pIPv4;
    #            char pIPv6[16];
    #        };
    #    }ip_list[4];
    #}cli_data_object_tunnel_register_t;
    base_fmt = 'IIIII'
    ip_fmt = 'II12s' #type, IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        ip_list = []
        #self.params hold the variables
        client_type = cli_interface_data_objects.dps_client_types[self.params[0]]
        vnid = self.params[1]
        num_elements = 0
        for i in range(2):
            try:
                #From the 2nd parameter onwards all IP Addresses
                ipaddr = cli_type_ip.ipv4_atoi(self.params[2+i])
                ip_list.append(ipaddr)
                num_elements += 1
            except Exception:
                break
        c_struct = struct.pack(self.base_fmt, self.major_type, self.cli_code, client_type, vnid, num_elements)
        for i in range(num_elements):
            c_struct += struct.pack(self.ip_fmt, socket.AF_INET, ip_list[i], '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
cli_tunnel_deregister.add_cli()

class cli_data_objects_multicast(cli_dps_objects):
    '''
    Represents the CLI Context for VNID Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'multicast'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'multicast'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Multicast Related Configuration'
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
        @return cli_data_objects_multicast
        '''
        return cli_data_objects_multicast
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_multicast.add_context()

class cli_data_objects_multicast_sender(cli_data_objects_multicast):
    '''
    Represents the CLI Object for adding a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'sender'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Sender Register/Unregister'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MULTICAST_SENDER_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('operation', cli_type_string_set, True, cli_interface_data_objects.multicast_sender_options),
                      ('client_type', cli_type_string_set, True, cli_interface_data_objects.dps_client_types_range),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('tunnel_IP', cli_type_ip, True, cli_interface.range_ip),
                      ('multicast_mac', cli_type_mac, True, cli_interface.range_mac),
                      ('multicast_ip', cli_type_ip, True, cli_interface.range_ip_multicast)
                      ]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_multicast_register_s{
    #    uint32_t client_type;
    #    uint32_t vnid;
    #    uint32_t multicast_IP_type;
    #    char mac[6];
    #    union {
    #        uint32_t multicast_IPv4;
    #        char multicast_IPv6[16];
    #    };
    #    uint32_t tunnel_IP_type;
    #    union {
    #        uint32_t tunnel_IPv4;
    #        char tunnel_IPv6[16];
    #    };
    #}cli_data_object_multicast_register_t;
    fmt = 'II'
    fmt += 'III'
    fmt += '6s' #mac
    fmt += 'I12s'
    fmt += 'II12s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        operation = self.params[0]
        if operation == 'register':
            code = cli_interface_data_objects.MULTICAST_SENDER_ADD
        else:
            code = cli_interface_data_objects.MULTICAST_SENDER_DEL
        client_type = cli_interface_data_objects.dps_client_types[self.params[1]]
        vnid = self.params[2]
        tunnel_ip = cli_type_ip.ipv4_atoi(self.params[3])
        mac = cli_type_mac.bytes(self.params[4])
        ip = cli_type_ip.ipv4_atoi(self.params[5])
        ip_packed = struct.pack('I', ip)
        ip_string = socket.inet_ntop(socket.AF_INET, ip_packed)
        if ip_string == cli_interface.ip_multicast_unknown:
            ip_type = 0
            ip = 0
        else:
            ip_type = socket.AF_INET
        c_struct = struct.pack(self.fmt, 
                               self.major_type, code,
                               client_type, vnid,
                               ip_type, mac, ip, '',
                               socket.AF_INET, tunnel_ip, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_sender.add_cli()

class cli_data_objects_multicast_receiver(cli_data_objects_multicast):
    '''
    Represents the CLI Object for adding a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'receiver'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Receiver Join/Leave'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MULTICAST_RECEIVER_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('operation', cli_type_string_set, True, cli_interface_data_objects.multicast_receiver_options),
                      ('client_type', cli_type_string_set, True, cli_interface_data_objects.dps_client_types_range),
                      ('vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('tunnel_IP', cli_type_ip, True, cli_interface.range_ip),
                      ('multicast_mac', cli_type_mac, True, cli_interface.range_mac),
                      ('multicast_ip', cli_type_ip, True, cli_interface.range_ip_multicast)
                      ]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_multicast_register_s{
    #    uint32_t client_type;
    #    uint32_t vnid;
    #    uint32_t multicast_IP_type;
    #    char mac[6];
    #    union {
    #        uint32_t multicast_IPv4;
    #        char multicast_IPv6[16];
    #    };
    #    uint32_t tunnel_IP_type;
    #    union {
    #        uint32_t tunnel_IPv4;
    #        char tunnel_IPv6[16];
    #    };
    #}cli_data_object_multicast_register_t;
    fmt = 'II'
    fmt += 'III'
    fmt += '6s' #mac
    fmt += 'I12s'
    fmt += 'II12s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        while True:
            operation = self.params[0]
            try:
                code = cli_interface_data_objects.multicast_receiver_operations[operation]
            except Exception:
                ret = cli_interface.DOVE_STATUS_INVALID_PARAMETER
                break
            client_type = cli_interface_data_objects.dps_client_types[self.params[1]]
            vnid = self.params[2]
            tunnel_ip = cli_type_ip.ipv4_atoi(self.params[3])
            mac = cli_type_mac.bytes(self.params[4])
            ip = cli_type_ip.ipv4_atoi(self.params[5])
            ip_packed = struct.pack('I', ip)
            ip_string = socket.inet_ntop(socket.AF_INET, ip_packed)
            if ip_string == cli_interface.ip_multicast_unknown:
                ip_type = 0
                ip = 0
            else:
                ip_type = socket.AF_INET
            c_struct = struct.pack(self.fmt, 
                                   self.major_type, code,
                                   client_type, vnid,
                                   ip_type, mac, ip, '',
                                   socket.AF_INET, tunnel_ip, '')
            ret = dcslib.process_cli_data(c_struct)
            break
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_receiver.add_cli()

class cli_data_objects_multicast_global_scope_get(cli_data_objects_multicast):
    '''
    Represents the CLI Object for adding a VNID
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'global_scope_get'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get Global Scope'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MULTICAST_RECEIVER_GLOBAL_SCOPE_GET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_multicast_register_s{
    #    uint32_t client_type;
    #    uint32_t vnid;
    #    uint32_t multicast_IP_type;
    #    char mac[6];
    #    union {
    #        uint32_t multicast_IPv4;
    #        char multicast_IPv6[16];
    #    };
    #    uint32_t tunnel_IP_type;
    #    union {
    #        uint32_t tunnel_IPv4;
    #        char tunnel_IPv6[16];
    #    };
    #}cli_data_object_multicast_register_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        while True:
            vnid = self.params[0]
            c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid)
            ret = dcslib.process_cli_data(c_struct)
            break
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_global_scope_get.add_cli()

class cli_data_objects_multicast_policy(cli_data_objects_multicast):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'policy'
    user_input_string_raw = '%s:%s'%(cli_data_objects_multicast.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'policy'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Policy Related Configuration'
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
        @return cli_data_objects_multicast_policy
        '''
        return cli_data_objects_multicast_policy
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_multicast_policy.add_context()

class cli_data_objects_multicast_policy_add(cli_data_objects_multicast_policy):
    '''
    Represents the CLI Object for Policy Add
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add a Policy'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_ADD
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain', cli_type_int, True, cli_interface.range_domain),
                      ('policy_type', cli_type_string_set, True, cli_interface.range_policy_type),
                      ('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('ttl (minutes)', cli_type_int, True, cli_interface.range_policy_ttl),
                      ('action', cli_type_string_set, True, cli_interface.range_policy_action_connectivity)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_add_s{
    #        uint32_t traffic_type;
    #        uint32_t domain_id;
    #        uint32_t type;
    #        uint32_t src_dvg_id;
    #        uint32_t dst_dvg_id;
    #        uint32_t ttl;
    #        cli_data_object_policy_action_t action;
    #}cli_data_object_policy_add_t;
    fmt = 'IIIIIIII'
    fmt += 'BBH' #ver, pad, connectivity in action struct
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        policy_type = cli_interface.policy_type_map[self.params[1]]
        src_dvg = self.params[2]
        dst_dvg = self.params[3]
        ttl = self.params[4]
        action = cli_interface.policy_connectivity_map[self.params[5]]
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               cli_interface_data_objects.traffic_type_multicast,
                               domain, policy_type, src_dvg, 
                               dst_dvg, ttl, 1, 0, action)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_policy_add.add_cli()

class cli_data_objects_multicast_policy_delete(cli_data_objects_multicast_policy):
    '''
    Represents the CLI Object for deleting a Policy
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'delete'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Delete a Policy'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.POLICY_DELETE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('src_vnid', cli_type_int, True, cli_interface.range_dvg),
                      ('dst_vnid', cli_type_int, True, cli_interface.range_dvg)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_policy_delete_s{
    #    uint32_t    domain_id;
    #    uint32_t    src_dvg_id;
    #    uint32_t    dst_dvg_id;
    #}cli_data_object_policy_delete_t;
    fmt = 'IIIIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        src_dvg_id = self.params[1]
        dst_dvg_id = self.params[2]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               cli_interface_data_objects.traffic_type_multicast, 
                               domain, src_dvg_id, dst_dvg_id)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_policy_delete.add_cli()

class cli_data_objects_multicast_log_level(cli_data_objects_multicast):
    '''
    Represents the CLI Object for Changing Log Level in the DPS Data Handler
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
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MULTICAST_LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_log_level_s{
    #    /**
    #     * \brief log_level representing log level defined in log.h file
    #     */
    #    uint32_t    level;
    #}cli_data_object_log_level_t;
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
cli_data_objects_multicast_log_level.add_cli()

class cli_data_objects_multicast_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Multicast Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'multicast'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Multicast Details'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = False
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MULTICAST_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('type', cli_type_string_set, True, cli_interface.range_associated_type),
                      ('id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_multicast_show_s{
    #    uint32_t associated_type;
    #    uint32_t associated_id;
    #}cli_data_object_multicast_show_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        associated_type = cli_interface.associated_type_map[self.params[0]]
        associated_id = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, associated_type, associated_id)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_multicast_show.add_cli()

class cli_data_objects_address_resolution_show(cli_data_objects_show):
    '''
    Represents the CLI Object for showing Multicast Details
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'address_resolution'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show Address Resolution Waiters'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.ADDRESS_RESOLUTION_SHOW
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_add_s{
    #    uint32_t    domain_id;
    #}cli_data_object_domain_add_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_address_resolution_show.add_cli()



#Add command for DPS Heartbeat, now only the report interval
class cli_data_objects_heartbeat(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'heartbeat'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'heartbeat'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Heartbeat Related Configuration'
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
        @return cli_data_objects_heartbeat
        '''
        return cli_data_objects_heartbeat
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_heartbeat.add_context()

class cli_data_objects_hearbeat_interval(cli_data_objects_heartbeat):
    '''
    Represents the CLI Object for Setting the heartbeat interval
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'interval'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set report interval,0:Not Send'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.HEARTBEAT_REPORT_INTERVAL_SET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('interval', cli_type_int, True, cli_interface.range_report_interval)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_add_s{
    #    /**
    #     * \brief The Domain ID
    #     */
    #    uint32_t    domain_id;
    #}cli_data_object_domain_add_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        interval_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, interval_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_hearbeat_interval.add_cli()


#Add command for DPS Statistics, now only the report interval
class cli_data_objects_statistics(cli_dps_objects):
    '''
    Represents the CLI Context for Domain Command in the DPS Object
    '''
    #The input to display for Context:
    user_input_string = 'statistics'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'statistics'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Statistics Related Configuration'
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
        @return cli_data_objects_statistics
        '''
        return cli_data_objects_statistics
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_statistics.add_context()


class cli_data_objects_statistics_interval(cli_data_objects_statistics):
    '''
    Represents the CLI Object for Setting the heartbeat interval
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'interval'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set report interval,0:Not Send'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.STATISTICS_REPORT_INTERVAL_SET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('interval', cli_type_int, True, cli_interface.range_report_interval)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_add_s{
    #    /**
    #     * \brief The Domain ID
    #     */
    #    uint32_t    domain_id;
    #}cli_data_object_domain_add_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        interval_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, interval_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_statistics_interval.add_cli()

class cli_data_objects_mass_transfer(cli_dps_objects):
    '''
    Represents the CLI Context for Mass Transfer of Domain Data Testing in DPS
    '''
    #The input to display for Context:
    user_input_string = 'mass_transfer'
    user_input_string_raw = '%s:%s'%(cli_dps_objects.user_input_string,
                                     user_input_string)
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'mass_transfer'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Mass Transfer Testing'
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
        @return cli_data_objects_mass_transfer
        '''
        return cli_data_objects_mass_transfer
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Login Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_data_objects_mass_transfer.add_context()

class cli_data_objects_mass_transfer_get_ready(cli_data_objects_mass_transfer):
    '''
    Represents the CLI Object for informing a DPS Node to get ready for a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'get_ready'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Get Ready for a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MASS_TRANSFER_GET_READY
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_add_s{
    #    uint32_t    domain_id;
    #}cli_data_object_domain_add_t;
    fmt = 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain_int = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_mass_transfer_get_ready.add_cli()

class cli_data_objects_mass_transfer_start(cli_data_objects_mass_transfer):
    '''
    Represents the CLI Object for informing a DPS Node to get ready for a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'start'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Start transfer of Domain Data'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.MASS_TRANSFER_START
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('dps_ip_address', cli_type_ip, True, cli_interface.range_ip)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_transfer_s{
    #    uint32_t domain_id;
    #    uint32_t IP_type;
    #    union {
    #        uint32_t IPv4;
    #        char IPv6[16];
    #    };
    #}cli_data_object_domain_transfer_t;
    fmt = 'IIII'
    fmt += 'I12s' #IPv4 + 12 extra bytes = 16 bytes for union
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        #self.params hold the variables
        domain = self.params[0]
        IP = cli_type_ip.ipv4_atoi(self.params[1])
        #Pack into structure
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain,
                               socket.AF_INET, IP, '')
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_mass_transfer_start.add_cli()

class cli_data_objects_mass_transfer_domain_activate(cli_data_objects_mass_transfer):
    '''
    Represents the CLI Object for informing a DPS Node to get ready for a Domain
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domain_activate'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Activate a Domain'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_dps_interface.CLI_DATA_OBJECTS
    #CLI CODE
    cli_code = cli_interface_data_objects.DOMAIN_ACTIVATE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('domain_id', cli_type_int, True, cli_interface.range_domain),
                      ('replication_factor', cli_type_int, True, cli_interface.range_replication)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_data_object_domain_update_s{
    #    uint32_t    domain_id;
    #    uint32_t    replication_factor;
    #}cli_data_object_domain_update_t;
    fmt = 'IIII'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain_int = self.params[0]
        replication = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain_int, replication)
        ret = dcslib.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_data_objects_mass_transfer_domain_activate.add_cli()

