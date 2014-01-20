'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: Amitabha Biswas

 @summary: The DPS Server related CLI definitions for Data Object Handler
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
from gateway_cli.main_menu import cli_gateway_interface
from gateway_cli.main_menu import cli_main_menu

#This is the DOVE GATEWAY LIBRARY NAME
import dgadmin

class cli_service_code:
    '''
    Represents Data Objects CLI Definitions for the DPS Client Server 
    Protocol. Should be in sync with the following files:
    1. cli/inc/service.h file
    '''
    #CLI_SERVICE_CODES
    LOG_LEVEL = 0       #CLI_SERVICE_LOG_LEVEL
    ADD_TYPE = 1        #CLI_SERVICE_ADD_TYPE
    ADD_IFMAC = 2       #CLI_SERVICE_ADD_IFMAC
    ADD_IPV4 = 3        #CLI_SERVICE_ADD_IPV4
    ADD_EXTVIP = 4      #CLI_SERVICE_ADD_EXTVIP
    ADD_OVERLAYVIP = 5  #CLI_SERVICE_ADD_OVERLAYVIP
    ADD_FWRULE = 6      #CLI_SERVICE_ADD_FWRULE
    ADD_LOCATION = 7    #CLI_SERVICE_ADD_LOCATION
    REG_LOCATION = 8    #CLI_SERVICE_REG_LOCATION
    ADD_DPS = 9         #CLI_SERVICE_ADD_DPS
    DPS_LOOKUP = 10     #CLI_SERVICE_DPS_LOOKUP
    SET_MTU = 11        #CLI_SERVICE_SET_MTU
    SVC_START = 12      #CLI_SERVICE_START
    SVC_STOP = 13       #CLI_SERVICE_STOP
    SVC_DOMAIN_VLAN = 14#CLI_SERVICE_DVLAN
    SVC_SHOW_ALL = 15   #CLI_SERVICE_SHOW_ALL
    SVC_SAVE_TOFILE = 16#CLI_SERVICE_SAVE_TOFILE
    SVC_SHOW_CONFIG = 17#CLI_SERVICE_SHOW_CONFIG
    SVC_SAVE_STARTUP = 18 #CLI_SERVICE_SAVE_STARTUP
    SVC_LOAD_CONFIG = 19#CLI_SERVICE_LOAD_CONFIG
    SET_TYPE = 20       #CLI_SERVICE_SET_TYPE
    SET_DOVE_NET_IPV4 = 21 #CLI_DOVE_NET_IPV4
    SET_MGMT_IPV4 = 22  #CLI_MGMT_IPV4
    SET_PEER_IPV4 = 23  #CLI_PEER_IPV4
    SET_DMC_IPV4 = 24   #CLI_DMC_IPV4
    ADD_VNID_SUBNET = 25   #CLI_ADD_VNID_SUBNET
    DEL_VNID_SUBNET = 26   #CLI_DEL_VNID_SUBNET
    SHOW_OVL_SYS    = 27   #CLI_SHOW_OVL_SYS
    SET_EXT_MCAST_VNID = 28 #CLI_EXT_MCAST_VNID
    SHOW_EXT_SESSIONS = 29 #CLI_SHOW_EXT_SESSIONS
    SET_MGMT_DHCP = 30  #CLI_MGMT_DHCP

    #NAME_MAX
    BRIDGE_NAME_MAX = 16
    SERVICE_TYPE_MAX = 8

class cli_service(cli_main_menu):
    '''
    Represents the CLI Object for Gateway Service
    '''
    #The input to display for Context:
    user_input_string = 'service'
    user_input_string_raw = '[service]>>'
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'service'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DOVE Gateway Service (Context)'
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
    #This function 'execute' must exist for all objects
    def execute(self, session):
        '''
        Execute cli_service mode: Nothing To Do here
        @param session: The session context
        @type session: Class login_context
        @return None
        '''
        return cli_service
    #This function 'exit' must be present for all contexts
    def cli_exit(self, session):
        '''
        Exits configuration mode. Go back to Previous Context
        @param session: The session context
        @type session: Class login_context
        '''
        return
#Add this class of command to global list of supported commands
cli_service.add_context()

class cli_service_log_level(cli_service):
    '''
    Represents the CLI Object for Changing Log Level in the Gateway Services
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
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.LOG_LEVEL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('level', cli_type_string_set, True, cli_interface.range_log_level)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_log_level_s{
    #    uint32_t    log_level;
    #}cli_service_log_level_t;
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
cli_service_log_level.add_cli()

class cli_service_add_type(cli_service):
    '''
    Represents the CLI Service Object for Adding a service type
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'addservice'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Service Type (EXT/VLAN)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_TYPE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('type', cli_type_string, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += '%ss'%(cli_service_code.SERVICE_TYPE_MAX)
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        stype = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, stype)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_type.add_cli()


class cli_service_set_type(cli_service):
    '''
    Represents the CLI Service Object for Adding a service type
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'setservice'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set Service Type (EXT/VLAN/EXTVLAN)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_TYPE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('type', cli_type_string, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += '%ss'%(cli_service_code.SERVICE_TYPE_MAX)
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        stype = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, stype)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_set_type.add_cli()


class cli_service_set_mtu(cli_service):
    '''
    Represents the CLI Service Object for setting service MTU
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'mtuset'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set service MTU (bytes)'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_MTU
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('mtu', cli_type_int, True, [512,9000])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'H'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        mtu = self.params[1]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, mtu)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_set_mtu.add_cli()

class cli_service_start(cli_service):
    '''
    Represents the CLI Service Object for enabling all gw service
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'start'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Start'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_START
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_start.add_cli()

class cli_service_stop(cli_service):
    '''
    Represents the CLI Service Object for enabling all gw service
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'stop'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Stop'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_STOP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_stop.add_cli()



class cli_service_add_ifmac(cli_service):
    '''
    Represents the CLI Service Object for Adding a Interface MAC
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'portadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add MAC Interface'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_IFMAC
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('mac', cli_type_mac, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_ifmac_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char mac[6];
    #} cli_service_ifmac_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += '6s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        mac = cli_type_mac.bytes(self.params[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, mac)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_ifmac.add_cli()

class cli_service_add_addressv4(cli_service):
    '''
    Represents the CLI Service Object for Adding a IPV4 interface
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'ipv4add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add IPv4 interface'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_IPV4
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('ip-addr', cli_type_ip, True, []),
                      ('netmask', cli_type_ip, True, []),
                      ('nexthop', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        ip = cli_type_ip.ipv4_atoi(self.params[1])
        try:
            if not cli_type_ip.netmask_valid(self.params[2]):
                raise Exception('Bad Mask')
        except Exception:
            print 'Netmask %s not valid'%self.params[2]
            return
        mask = cli_type_ip.ipv4_atoi(self.params[2])
        nexthop = cli_type_ip.ipv4_atoi(self.params[3])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, ip, mask,nexthop)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_addressv4.add_cli()

class cli_service_add_extvip(cli_service):
    '''
    Represents the CLI Service Object for Adding a External IP Address
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'extvipadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Domain Public IP Address'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_EXTVIP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("svc-name", cli_type_string, True, []),
                      ("domain", cli_type_int, True, cli_interface.range_domain),
                      ("ip address", cli_type_ip, True, []),
                      ("port range 'A-B'", cli_type_int_range, True, [1,65535])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_extvip_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t vIPv4;
    #    uint16_t port_min;
    #    uint16_t port_max;
    #} cli_service_extvip_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'IIHH'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        domain = self.params[1] 
        ip = cli_type_ip.ipv4_atoi(self.params[2])
        port_range = self.params[3]
        ports = port_range.split('-')
        port_min = int(ports[0])
        port_max = int(ports[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, 
                               domain, ip, port_min, port_max)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_extvip.add_cli()

class cli_service_add_overlayip(cli_service):
    '''
    Represents the CLI Service Object for Adding a Overlay IPV4 Address
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'overlayvipadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Overlay IP Address'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_OVERLAYVIP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("svc-name", cli_type_string, True, []),
                      ("ip address", cli_type_ip, True, []),
                      ("port range 'A-B'", cli_type_int_range, True, [1,65535])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_overlayip_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t vIPv4;
    #    uint16_t port_min;
    #    uint16_t port_max;
    #} cli_service_overlayip_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'IHH'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        ip = cli_type_ip.ipv4_atoi(self.params[1])
        port_range = self.params[2]
        ports = port_range.split('-')
        port_min = int(ports[0])
        port_max = int(ports[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, 
                               ip, port_min, port_max)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_overlayip.add_cli()

class cli_service_add_fwdrule(cli_service):
    '''
    Represents the CLI Service Object for Adding a Forwarding Rule
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'fwdruleadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Forwarding Rule'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_FWRULE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("svc-name", cli_type_string, True, []),
                      ("domain", cli_type_int, True, cli_interface.range_domain),
                      ("protocol", cli_type_int, True, [0,255]),
                      ("ip address", cli_type_ip, True, []),
                      ("port", cli_type_int, True, [0,65535]),
                      ("real ip", cli_type_ip, True, []),
                      ("real port", cli_type_int, True, [0,65535])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_fwdrule_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t domain;
    #    uint32_t IPv4;
    #    uint32_t IPv4_map;
    #    uint16_t port;
    #    uint16_t port_map;
    #    uint16_t protocol;
    #} cli_service_fwdrule_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'IIIHHH'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        domain = self.params[1]
        protocol = self.params[2]
        ip = cli_type_ip.ipv4_atoi(self.params[3])
        port = self.params[4]
        ip_map = cli_type_ip.ipv4_atoi(self.params[5])
        port_map = self.params[6]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               bridge_name, domain, ip, ip_map,
                               port, port_map, protocol)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_fwdrule.add_cli()

class cli_service_add_location(cli_service):
    '''
    Represents the CLI Service Object for Adding a Location
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'locationadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add Location'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_LOCATION
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("svc-name", cli_type_string, True, []),
                      ("domain", cli_type_int, True, cli_interface.range_domain),
                      ("location IP", cli_type_ip, True, []),
                      ("end location IP", cli_type_ip, True, []),
                      ("end src location IP", cli_type_ip, True, []),
                      ("overlay protocol", cli_type_int, True, [1,65535]),
                      ("overlay src port", cli_type_int, True, [1,65535]),
                      ("overlay dst port", cli_type_int, True, [1,65535]),
                      ('mac', cli_type_mac, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_location_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t domain;
    #    uint32_t location_IP;
    #    uint32_t end_location_IP;
    #    uint32_t end_src_location_IP;
    #    uint16_t ovl_proto;
    #    uint16_t ovl_src_port;
    #    uint16_t ovl_dst_port;
    #    char ovl_dst_mac[6];
    #} cli_service_location_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'IIII'
    fmt += 'HHH6s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        domain = self.params[1]
        location_ip = cli_type_ip.ipv4_atoi(self.params[2])
        end_location_ip = cli_type_ip.ipv4_atoi(self.params[3])
        end_src_location_ip = cli_type_ip.ipv4_atoi(self.params[4])
        protocol = self.params[5]
        src_port = self.params[6]
        dst_port = self.params[7]
        mac = cli_type_mac.bytes(self.params[8])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               bridge_name, domain, location_ip,
                               end_location_ip, end_src_location_ip,
                               protocol, src_port, dst_port, mac
                               )
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_location.add_cli()


class cli_service_register_location(cli_service):
    '''
    Represents the CLI Service Object for Register a Location
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'register-location'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Register Location'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.REG_LOCATION
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("domain", cli_type_int, True, cli_interface.range_domain),
                      ("location IP", cli_type_ip, True, []),
                      ("physical IP", cli_type_ip, True, []),
                      ("dvg", cli_type_int, True, [1,65535]),
                      ('mac', cli_type_mac, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_location_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t domain;
    #    uint32_t location_IP;
    #    uint32_t end_location_IP;
    #    uint32_t end_src_location_IP;
    #    uint16_t ovl_proto;
    #    uint16_t ovl_src_port;
    #    uint16_t ovl_dst_port;
    #    char ovl_dst_mac[6];
    #} cli_service_location_t;
    fmt = 'II'
    fmt += 'III'
    fmt += 'H6s'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain = self.params[0]
        vm_location_ip = cli_type_ip.ipv4_atoi(self.params[1])
        phy_location_ip = cli_type_ip.ipv4_atoi(self.params[2])
        dvg = self.params[3]
        mac = cli_type_mac.bytes(self.params[4])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               domain, vm_location_ip, phy_location_ip,
                               dvg, mac
                               )
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_register_location.add_cli()

class cli_service_add_dps_server(cli_service):
    '''
    Represents the CLI Service Object for Adding a DPS server
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dpsadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add DPS Server Node'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_DPS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('server-ip', cli_type_ip, True, []),
                      ('port', cli_type_int, True, [1,65535]),
                      ("domain", cli_type_int, True, cli_interface.range_domain)]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'
    fmt += 'I'

    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        dps_server_ip = cli_type_ip.ipv4_atoi(self.params[0])
        dps_port = self.params[1]
        dps_domain = self.params[2]

        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, dps_server_ip, dps_port, dps_domain)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_dps_server.add_cli()

class cli_service_test_dps_lookup(cli_service):
    '''
    Represents the CLI Service Object for manual dps lookup
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dpslookup'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'DPS Lookup'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.DPS_LOOKUP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("domain", cli_type_int, True, cli_interface.range_domain),
                      ('vmip', cli_type_ip, True, [])]

    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        domain = self.params[0]
        vmip = cli_type_ip.ipv4_atoi(self.params[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, domain, vmip)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_test_dps_lookup.add_cli()


class cli_service_domain_vlan(cli_service):
    '''
    Represents the CLI Service Object for domain vlan mapping
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'domainvlan-add'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set domain vlan mapping'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_DOMAIN_VLAN
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("svc-name", cli_type_string, True, []),
                      ("domain", cli_type_int, True, cli_interface.range_domain),
                      ("vlan", cli_type_int, True, [1,4094])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_fwdrule_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t domain;
    #    uint32_t IPv4;
    #    uint32_t IPv4_map;
    #    uint16_t port;
    #    uint16_t port_map;
    #    uint16_t protocol;
    #} cli_service_fwdrule_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        domain = self.params[1]
        vlan = self.params[2]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, 
                               bridge_name, domain, vlan)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_domain_vlan.add_cli()


class cli_service_show(cli_service):
    '''
    Represents the CLI Service Object for enabling all gw service
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_SHOW_ALL
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_show.add_cli()

class cli_service_savetofile(cli_service):
    '''
    Represents the CLI Service Object for service config save
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'configsave'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Save running config with given name'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_SAVE_TOFILE
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]

    command_format = [("config-name", cli_type_string, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        file_name = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               file_name)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_savetofile.add_cli()


class cli_service_showconfig(cli_service):
    '''
    Represents the CLI Service Object for service config show
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'showconfig'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show config with given name'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_SHOW_CONFIG
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]

    command_format = [("config-name", cli_type_string, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        file_name = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               file_name)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_showconfig.add_cli()

class cli_service_savestarup(cli_service):
    '''
    Represents the CLI Service Object for service config save
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'save'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Save config to startup'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_SAVE_STARTUP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]

    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_savestarup.add_cli()

class cli_service_loadconfig(cli_service):
    '''
    Represents the CLI Service Object for service config load
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'loadconfig'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Load config with given name'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SVC_LOAD_CONFIG
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]

    command_format = [("config-name", cli_type_string, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        file_name = self.params[0]
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code,
                               file_name)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_loadconfig.add_cli()

class cli_service_dove_net_ipv4(cli_service):
    '''
    Represents the CLI Service Object for Adding a IPV4 interface
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dovenetipv4'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set IPV4 to communicate with DOVE Switches '
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_DOVE_NET_IPV4
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('ip-addr', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'I'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        ip = cli_type_ip.ipv4_atoi(self.params[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, ip)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_dove_net_ipv4.add_cli()

class cli_service_mgmt_ipv4(cli_service):
    '''
    Represents the CLI Service Object for Adding a IPV4 interface
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'mgmtipv4'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set IPV4 to communicate with managment '
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_MGMT_IPV4
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('ipaddr', cli_type_ip, True, []),
                      ('netmask', cli_type_ip, True, []),
                      ('nexthop', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    fmt += 'III'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        ip = cli_type_ip.ipv4_atoi(self.params[0])
        try:
            if not cli_type_ip.netmask_valid(self.params[1]):
                raise Exception('Bad Mask')
        except Exception:
            print 'Netmask %s not valid'%self.params[1]
            return
        mask = cli_type_ip.ipv4_atoi(self.params[1])
        nexthop = cli_type_ip.ipv4_atoi(self.params[2])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, ip, mask, nexthop)

        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_mgmt_ipv4.add_cli()

class cli_service_mgmt_ipv4_dhcp(cli_service):
    '''
    Represents the CLI Service Object for Adding a IPV4 interface
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'mgmt-set-dhcp'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set DHCP For Managment IP [1/0]'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_MGMT_DHCP
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = []

    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = "APBR"
        ip = 0
        mask = 0
        nexthop = 0
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
# , bridge_name, ip, mask,nexthop)

        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_mgmt_ipv4_dhcp.add_cli()


class cli_service_peer_ipv4(cli_service):
    '''
    Represents the CLI Service Object for Adding a IPV4 interface
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'peeripv4'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set IPV4 to communicate with PEER  '
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_PEER_IPV4
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('svc-name', cli_type_string, True, []),
                      ('ip-addr', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    fmt += '%ss'%(cli_service_code.BRIDGE_NAME_MAX)
    fmt += 'I'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        bridge_name = self.params[0]
        ip = cli_type_ip.ipv4_atoi(self.params[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, bridge_name, ip)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_peer_ipv4.add_cli()


class cli_service_add_dmc(cli_service):
    '''
    Represents the CLI Service Object for Adding DMC
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'dmcadd'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add DMC info'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_DMC_IPV4
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('dmc-ip', cli_type_ip, True, []),
                      ('port', cli_type_int, True, [1,65535])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'

    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        dmc_server_ip = cli_type_ip.ipv4_atoi(self.params[0])
        dmc_port = self.params[1]

        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, dmc_server_ip, dmc_port)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_dmc.add_cli()

class cli_service_add_vnidsunets(cli_service):
    '''
    Represents the CLI Service Object for Adding VNID Subnets
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vnidaddsubnet'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Add VNID Subnet Info'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.ADD_VNID_SUBNET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("vnid", cli_type_int, True, cli_interface.range_domain),
                      ('ip_address', cli_type_ip, True, []),
                      ('ip_mask', cli_type_ip, True, []),
                      ('mode', cli_type_string_set, True, cli_interface.range_ipsubnet_mode),
                      ('gateway', cli_type_ip, True,[])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'
    fmt += 'I'
    fmt += 'I'
    fmt += 'I'
    
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        vnid_id = self.params[0]
        vnid_subnet_ip = cli_type_ip.ipv4_atoi(self.params[1])
        vnid_subnet_mask = cli_type_ip.ipv4_atoi(self.params[2])
        vnid_subnet_mode = cli_interface.ipsubnet_mode_map[self.params[3]]
        vnid_subnet_gateway = cli_type_ip.ipv4_atoi(self.params[4])

        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid_id, vnid_subnet_ip, vnid_subnet_mask, vnid_subnet_mode, vnid_subnet_gateway)

        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_add_vnidsunets.add_cli()

class cli_service_del_vnidsunets(cli_service):
    '''
    Represents the CLI Service Object for delete VNID Subnets
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'vnidremsubnet'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Rem VNID Subnet Info'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.DEL_VNID_SUBNET
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [("vnid", cli_type_int, True, cli_interface.range_domain),
                      ('ip_address', cli_type_ip, True, []),
                      ('ip_mask', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'
    fmt += 'I'
    
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        vnid_id = self.params[0]
        vnid_subnet_ip = cli_type_ip.ipv4_atoi(self.params[1])
        vnid_subnet_mask = cli_type_ip.ipv4_atoi(self.params[2])

        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid_id, vnid_subnet_ip, vnid_subnet_mask )

        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_del_vnidsunets.add_cli()


class cli_service_show_ovl(cli_service):
    '''
    Represents the CLI Service Object for enabling all gw service
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'show-ovl-entries'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SHOW_OVL_SYS
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    # command_format = [('svc-name', cli_type_string, True, []),
    #                  ('mtu', cli_type_int, True, [512,9000])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_type_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    char type[5];
    #} cli_service_type_t;
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_show_ovl.add_cli()

class cli_service_extern_mcast_vnid(cli_service):
    '''
    Represents the CLI Service Object for setting external mcast vnid
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'extmcast'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Set extern mcast VNID '
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SET_EXT_MCAST_VNID
    #The Parameters of the rest of the command
    #Each parameter must have ('Name', 'Type', 'Non-Optional?', 'Range' )
    #All optional parameters MUST come after the required parameters
    command_format = [('vnid', cli_type_int, True, cli_interface.range_domain),
                      ('ip-addr', cli_type_ip, True, [])]
    support_random = False
    #Structure to send
    # 2 Integers (MAJOR CODE, MINOR CODE) followed by
    #typedef struct cli_service_addressv4_s{
    #    char bridge_name[BRIDGE_NAME_MAX];
    #    uint32_t IPv4;
    #    uint32_t IPv4_netmask;
    #} cli_service_addressv4_t;
    fmt = 'II'
    fmt += 'I'
    fmt += 'I'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        vnid_id = self.params[0]
        ip = cli_type_ip.ipv4_atoi(self.params[1])
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code, vnid_id, ip)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_extern_mcast_vnid.add_cli()

class cli_service_sessions_ext(cli_service):
    '''
    Represents the CLI Service Object for enabling all gw service
    '''
    #The Possible Unique Names that identify this command
    #These will be the 1st parameter in the CLI command (20 chars max)
    command_id = 'sessions'
    #Definition of the Command - Keep it short (40 chars)
    command_def = 'Show active sessions'
    #Whether it's a hidden command
    hidden = False
    #Whether the command is only available in god mode
    GodModeOnly = True
    #This is MAJOR TYPE of the Command
    major_type = cli_gateway_interface.CLI_SERVICE
    #CLI CODE
    cli_code = cli_service_code.SHOW_EXT_SESSIONS
    support_random = False
    fmt = 'II'
    #This function 'execute' must exist for all CLI objects
    def execute(self, session):
        '''
        @param session: The session context
        @type session: Class login_context
        '''
        c_struct = struct.pack(self.fmt, self.major_type, self.cli_code)
        ret = dgadmin.process_cli_data(c_struct)
        cli_interface.process_error(ret)
#Add this class of command to global list of supported commands
cli_service_sessions_ext.add_cli()



