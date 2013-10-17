'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

@author: amitabha.biswas
'''

import logging
from logging import getLogger
log = getLogger(__name__)

import sys
import time
import struct, os
import signal
import socket
import random
import string
import terminal
from terminal import MyTerminal
from terminal import CommandHistory

class cli_type(object):
    '''
    Represent the base CLI type
    '''
    @classmethod
    def value(cls, user_input, val_range):
        raise Exception("Type %s, Function 'value' not defined"%(cls.__name__))

    @classmethod
    def random(cls, val_range):
        raise Exception("Type %s, Function 'random' not defined"%(cls.__name__))

    @classmethod
    def get_min_max(cls, val_range):
        '''
        @param range: List containing [min, max] or [max] or []
        @type range: List
        '''
        try:
            val_min = int(val_range[0])
        except Exception:
            val_min = 0
        try:
            val_max = int(val_range[1])
        except Exception:
            if val_min > 0:
                val_max = min
            else:
                val_max = 2147483647
            val_min = 0
        if val_min > val_max:
            return val_max, val_min
        else:
            return val_min, val_max

    @classmethod
    def get_min_max_string(cls, val_range):
        '''
        @param range: List containing [min, max] or [max] or []
        @type range: List
        '''
        try:
            min = int(val_range[0])
        except Exception:
            min = 1
        try:
            max = int(val_range[1])
        except Exception:
            if min > 1:
                max = min
            else:
                max = 256
            min = 0
        if min > max:
            return max, min
        else:
            return min, max

class cli_type_int(cli_type):
    '''
    Represent an Integer
    '''

    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns an integer
        @param user_input: The Integer as entered by the user
        @type user_input: String
        @param val_range: List containing [min, max]
        @type val_range: List
        @raise exception: If user input cannot be a an integer
        '''
        val = int(user_input)
        min, max = cls.get_min_max(val_range)
        if val < min or val > max:
            raise Exception('%s not in expected range[%s-%s]'%(val, min, max))
        return val

    @classmethod
    def random(cls, val_range):
        '''
        Generates a random integer from 1 to max
        @type val_range: List contain [min, max] or [max]
        @rtype: Integer
        '''
        min, max = cls.get_min_max(val_range)
        return random.randint(min, max)

class cli_type_int_range(cli_type):
    '''
    Represents a Integer Range Min-Max
    '''

    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns a string of the form 'Min-Max'
        @param user_input: String of the form 'min-max'
        @type user_input: String
        @param range: [min, max]
        @type range: List
        @raise exception: If user input is not of the format min-max.
        '''
        vals = user_input.split('-')
        min_val = int(vals[0])
        max_val = int(vals[1])
        min_range, max_range = cls.get_min_max(val_range)
        if min_val < min_range or max_val > max_range:
            raise Exception ('%s not in supported range [%s-%s]'%(user_input, min_range, max_range))
        return '%s-%s'%(min_val,max_val)

    @classmethod
    def random(cls, val_range):
        '''
        Returns a random val_range
        @type val_range: List contain [min, max] or [max] or []
        @rtype: String of the form 'min-max'
        '''
        min, max = cls.get_min_max(val_range)
        val1 = random.randint(min, max)
        val2 = random.randint(min, max)
        if val1 < val2:
            return '%s-%s'%(val1,val2)
        else:
            return '%s-%s'%(val2,val1)

class cli_type_string(cli_type):
    '''
    Represents a generic English String
    '''
    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns the string input by the user.
        @param user_input: User Input
        @type user_input: String
        @param val_range: Contains (min, max) size of the input
        @type val_range: List [min, max] or [max] or []
        @raise exception: If user input cannot be a an string
        '''
        usr_input_str = str(user_input)
        min_range, max_range = cls.get_min_max_string(val_range)
        if len(usr_input_str) < min_range or len(usr_input_str) > max_range:
            raise Exception("Length of %s does not fit into size [%s-%s]"%(usr_input_str, min_range, max_range))
        return usr_input_str

    @classmethod
    def random(cls, val_range):
        '''
        Returns a random string
        @param val_range: Contains (min, max) size of the input
        @type val_range: List [min, max] or [max] or []
        '''
        min_range, max_range = cls.get_min_max_string(val_range)
        size = random.randint(min_range, max_range)
        return ''.join(random.choice(string.ascii_letters + string.digits) for i in range(size))

class cli_type_string_set(cli_type):
    '''
    Represents an user input string which should one of the values in a
    List of String
    '''
    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns the string input by the user if it falls in the set
        @param user_input: User Input
        @type user_input: String
        @param val_range: Contains the set of acceptable strings
        @type val_range: List
        @raise exception: If user input cannot not in set
        '''
#        range_lower = map(str.lower,val_range)
#        if user_input.lower() not in range_lower:
#            raise Exception('%s does not belong in acceptable input list %s'%(user_input, val_range))
        ret_input = None
        for val in val_range:
            if user_input.lower() == val.lower():
                ret_input = val
                break
        if ret_input is None:
            raise Exception('%s does not belong in acceptable input list %s'%(user_input, val_range))
        return ret_input

    @classmethod
    def random(cls, val_range):
        '''
        @param val_range: Contains the set of acceptable strings
        @type val_range: List
        '''
        len_range = len(val_range)
        size = random.randint(0, (len_range-1))
        return val_range[size]

class cli_type_ip(cli_type):
    '''
    Represents a user input of type IP address.
    '''
    @classmethod
    def get_address_type(cls, user_input):
        '''
        Get socket.AF_INET or socket.AF_INET6 based on user_input
        @param user_input: The user input
        @type user_input: String
        @raise - If not in right format A.B.C.D or A::B:C...
        '''
        if '.' in user_input:
            return socket.AF_INET
        elif ':' in user_input:
            return socket.AF_INET6
        else:
            raise Exception('Invalid IP Address')

    @classmethod
    def get_ipv4_range_as_int(cls, val_range):
        '''
        This routine takes a range of IPv4 given as [A.B.C.D - W.X.Y.Z] and returns
        the minIP and maxIP as integers
        '''
        ipmin = 0
        ipmax = 4294967295
        try:
            ipmin = int(struct.unpack('I', socket.inet_pton(socket.AF_INET, val_range[0]))[0])
            try:
                ipmax = int(struct.unpack('I', socket.inet_pton(socket.AF_INET, val_range[1]))[0])
            except Exception:
                ipmax = ipmin
                ipmin = 0
        except Exception:
            #No val_range exists
            ipmin = 0
            ipmax = 4294967295
        if ipmin > ipmax:
            return ipmax, ipmin
        else:
            return ipmin, ipmax

    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns a string representing a valid IPv4 or IPv6 address
        based on user input
        @param user_input: The user input
        @type user_input: String
        @param val_range: A range in the form of [IP1, IP2] - Only supported in IPv4
                      Ignored for IPv6
        @type val_range: List
        @raise exception: If user input cannot be an IP Address
        '''
        ip_type = cls.get_address_type(user_input)
        if ip_type == socket.AF_INET:
            try:
                ipval = int(struct.unpack('I', socket.inet_pton(socket.AF_INET, user_input))[0])
            except Exception:
                raise Exception('Invalid IP Address')
            ipmin, ipmax = cls.get_ipv4_range_as_int(val_range)
            if socket.ntohl(ipval) < socket.ntohl(ipmin) or socket.ntohl(ipval) > socket.ntohl(ipmax):
                raise Exception('IPv4 %s not in range %s'%(user_input, val_range))
        return user_input

    @classmethod
    def random_ipv4(cls, val_range):
        '''
        This returns a IPv4 address in the range
        '''
        ipmin, ipmax = cls.get_ipv4_range_as_int(val_range)
        iprand = socket.htonl(random.randint(socket.ntohl(ipmin), socket.ntohl(ipmax)))
        return socket.inet_ntop(socket.AF_INET, struct.pack('I',iprand))

    @classmethod
    def random(cls, val_range):
        '''
        This returns a random IP address in the range
        @param val_range: A range in the form of [IP1, IP2] - Only supported in IPv4
                          Ignored for IPv6
        @type val_range: List
        '''
        #Determine Type of range
        try:
            ip_type = cls.get_address_type(val_range[0])
        except Exception:
            ip_type = socket.AF_INET
        if ip_type == socket.AF_INET:
            return cls.random_ipv4(val_range)
        else:
            #Not supported
            return 'fe80::221:5eff:fe5d:f4c'

    @classmethod
    def ipv4_atoi(cls, IP_String):
        '''
        Converts a IP address in String Format 'A.B.C.D' to Integer format.
        In case the IP address is not well formed this routine will raise
        and Exception
        @param IP_String: IP Address in String Format: must be a Valid IP Address
        @type IP_String: String
        @return: The Integer value of IP Address
        @rtype: Integer
        '''
        ip_type = cls.get_address_type(IP_String)
        if ip_type == socket.AF_INET:
            IP_int = int(struct.unpack('I', socket.inet_pton(socket.AF_INET, IP_String))[0])
        else:
            raise Exception ('IPv6 not supported')
        return IP_int

    @classmethod
    def netmask_valid(cls, IP_String):
        '''
        Checks if the string represents a valid netmask
        '''
        IP_int = socket.ntohl(cls.ipv4_atoi(IP_String))
        fmask_started = False
        for i in range(31):
            if fmask_started and ((IP_int & 1) == 0):
                return False
            IP_int = IP_int >> 1
            if not fmask_started and ((IP_int & 1) == 1):
                fmask_started = True
        return True

class cli_type_mac(cli_type):
    '''
    Represents a MAC address
    '''
    @classmethod
    def get_valid(cls, mac):
        '''
        Get a valid MAC address and a Long Integer representing value of the MAC
        @raise exception: MAC not valid
        '''
        try:
            macs = mac.split(':')
        except Exception:
            raise Exception('Invalid MAC')
        if len(macs) != 6:
            raise Exception('Invalid MAC')
        for i in range(6):
            macs[i] = int(macs[i], 16)
            if macs[i] < 0 or macs[i] > 255:
                raise Exception('MAC Address Bytes not well formed')
        #Create the mac string
        #mac_string = ''.join(chr(macs[i]) for i in range(6))
        #Create the mac LONG value
        #Upper 2bytes
        mac_upper = (macs[0]<<8) + (macs[1])
        mac_lower = (macs[2]<<24) + (macs[3]<<16) + (macs[4]<<8) + (macs[5])
        mac_long = long((mac_upper<<32)+mac_lower)
        return mac, mac_long

    @classmethod
    def get_min_max_long(cls, val_range):
        '''
        Gets the MAC range as integer
        '''
        try:
            mac_string, mac_min_long = cls.get_valid(val_range[0])
        except Exception:
            mac_min_long = 0
        try:
            mac_string, mac_max_long = cls.get_valid(val_range[1])
        except Exception:
            mac_string, mac_max_long = cls.get_valid('ff:ff:ff:ff:ff:ff')
        if mac_min_long > mac_max_long:
            return mac_max_long, mac_min_long
        else:
            return mac_min_long, mac_max_long

    @classmethod
    def value(cls, user_input, val_range):
        '''
        Returns a string representing a valid MAC address
        @val_range: Ignored
        '''
        mac_string, mac_long = cls.get_valid(user_input)
        return mac_string

    @classmethod
    def random(cls, val_range):
        '''
        Returns a random MAC
        @rtype: String of type 'AA:BB:CC:DD:EE:FF'
        '''
        mac_min_long, mac_max_long = cls.get_min_max_long(val_range)
        mac_rand = random.randint(mac_min_long, mac_max_long)
        #Convert MAC
        mac0 = (mac_rand>>40)&0xff
        mac1 = (mac_rand>>32)&0xff
        mac2 = (mac_rand>>24)&0xff
        mac3 = (mac_rand>>16)&0xff
        mac4 = (mac_rand>>8)&0xff
        mac5 = (mac_rand)&0xff
        return'%x:%x:%x:%x:%x:%x'%(mac0, mac1, mac2, mac3, mac4, mac5)

    @staticmethod
    def bytes(mac_string):
        '''
        Returns the byte array representing a MAC String.
        This routine expects the MAC to be of the format A:B:C:D:E:F
        @raise exception: If MAC address not of right format
        '''
        try:
            macs = mac_string.split(':')
            if len(macs) != 6:
                raise Exception('MAC Address should be of the form aa:bb:cc:dd:ee:ff')
            for i in range(6):
                macs[i] = int(macs[i], 16)
                if macs[i] < 0 or macs[i] > 255:
                    raise Exception('MAC Address Bytes not well formed')
        except Exception:
            raise Exception('MAC Address should be of the form aa:bb:cc:dd:ee:ff')
        return ''.join(chr(macs[i]) for i in range(6))

class cli_base(object):
    '''
    Represent the base CLI class.
    '''
    command_history = CommandHistory()

    command_id = ''
    #Collection of all CLI commands: Key command id
    cli_dict = {}
    #Collection of all Context: Key command id
    cli_context_dict = {}
    #String to exit from a Context.
    #exit_strings = ['exit']
    #Inputs to quit on
    help_strings = ['help', '?']
    help_chars = '\t?'
    user_help_output = "Enter CTRL+C to end, or '%s' for help"%(help_strings[0])
    #User input
    user_input_string = ''
    #Show user chars: Whether the user input should be displayed
    input_char_show = True
    #Maximum length of a command
    command_id_max_len = 20

    class cli_generic_exit(object):
        '''
        Represents the generic method to quit a CLI context
        '''
        exit_strings = ['exit']
        command_id = exit_strings[0]
        command_def = 'Exit Current Context'
        major_type = -1 #No major type
        cli_code = -1 #No CLI Code
        command_format = [] #Empty
        support_random = False
        #Whether to hide the command from user
        hidden = False
        #Whether the command is only available in god mode
        GodModeOnly = False
        command_example = 'exit'
        def execute(self):
            return

    #Store the exit string
    cli_dict[cli_generic_exit.exit_strings[0].lower()] = cli_generic_exit

    @classmethod
    def verify_format(cls):
        '''
        Verifies the format of a CLI subclass
        '''
        if len(cls.command_id) > cli_base.command_id_max_len:
            raise Exception ('Command %s Command ID [%s] length exceeds %s'%(cls.__name__, 
                                                                             cls.command_id, 
                                                                             cli_base.command_id_max_len))
        #Validate Necessary Paramters exist
        cls.command_def = cls.command_def
        cls.hidden = cls.hidden
        cls.GodModeOnly = cls.GodModeOnly
        cls.support_random = cls.support_random

        fOptionalParamStart = False
        cls.params_total = 0 #Initialize
        cls.params_non_opt = 0 #Initialize
        cls.command_example = cls.command_id
        for param in cls.command_format:
            cls.params_total += 1
            param_optional = param[2]
            if fOptionalParamStart and param_optional:
                raise Exception('Invalid CLI format for Command %s, Optional Parameters must be at the end'%(cls.command_id))
            if not param_optional:
                if cls.params_total == 1:
                    cls.command_example = cls.command_example + ' ' + '<%s(optional) %s>'%(param[0].lower(),param[3])
                else:
                    cls.command_example = cls.command_example + ' ' + '<%s(optional)>'%(param[0].lower())
                fOptionalParamStart = True
            else:
                if cls.params_total == 1:
                    cls.command_example = cls.command_example + ' ' + '<%s %s>'%(param[0].lower(), param[3])
                else:
                    cls.command_example = cls.command_example + ' ' + '<%s>'%(param[0].lower())
                cls.params_non_opt = cls.params_non_opt + 1

    @classmethod
    def verify_no_conflict(cls, existing_types):
        '''
        Verifies there are no conflicts with already existing types
        @param existing_types: Types already added
        @type existing_types: Dictionary
        '''
        fConflict = False
        if cls.command_id.lower() in cli_base.cli_generic_exit.exit_strings:
            raise Exception("Class '%s' cannot have reserved command '%s'"%(cls.__name__, cli_base.cli_generic_exit.exit_strings))
        if cls.command_id.lower() in cls.help_strings:
            raise Exception("CLI '%s' cannot have reserved command '%s'"%(cls.__name__, cls.help_strings))
        for key in existing_types.keys():
            existing_type = existing_types[key]
            if cls.command_id.lower() == existing_type.command_id.lower():
                raise Exception("CLI '%s' command id '%s' conflicts with class '%s'"%(cls.__name__, cls.command_id, existing_type.__name__))
        return fConflict

    @classmethod
    def help(cls, fGodMode):
        '''
        Provides all possible user inputs for a particular context.
        @param fGodMode: Whether the caller is in God Mode
        @type fGodMode: Boolean
        '''
        print "\r"
        print "List of %s Commands\r"%cls.command_id
        print "\r"
        keys = cls.cli_dict.keys()
        keys.sort()
        for key in keys:
            try:
                #Find the CLI description
                cli_type = cls.cli_dict[key]
                if not fGodMode and cli_type.GodModeOnly:
                    continue
                if cli_type.hidden:
                    continue
                command_id = cli_type.command_id
                if len(command_id) > 20:
                    raise Exception('Too big to add description')
                spaces = ''.join((' ')for i in range((cli_base.command_id_max_len+5)-len(command_id)))
                print_line = command_id + spaces
                print_line = print_line + cli_type.command_def
                print '%s\r'%print_line
            except Exception:
                print '%s\r'%command_id
        print "\r"

    @classmethod
    def add_cli(cls):
        '''
        Adds a CLI to a CLI context. Each CLI should only be added to a single 
        CLI context.
        '''
        command_id = cls.command_id.lower()
        words = command_id.split()
        if len(words) > 1:
            raise Exception('CLI %s, command id [%s] cannot have more than 1 word'%(cls.__name__, cls.command_id))
        cls.verify_no_conflict(cls.cli_dict)
        cls.verify_format()
        #Insert into CLI dictionary
        cls.cli_dict[command_id] = cls
        #Each CLI gets it's own command history
        cls.command_history = CommandHistory()

    @classmethod
    def add_context(cls):
        '''
        Adds a CLI context. Each CLI context should be only added to a single parent 
        CLI context
        '''
        #It's a CLI for it's Base Class
        cls.add_cli()
        #Insert into Context Dictionary of it's Base Class
        cls.cli_context_dict[cls.command_id.lower()] = cls
        #Each context get's its own Sub CLIs and Sub Contexts
        cls.cli_dict = {}
        #Store the exit string
        cls.cli_dict[cli_base.cli_generic_exit.exit_strings[0].lower()] = cli_base.cli_generic_exit
        cls.cli_context_dict = {}

    @classmethod
    def get_sub_types_support_random(cls, major_types):
        '''
        This routine fetches all the cli sub-types which
        are in the major_types [array] and support random
        method.
        @param major_types: A List of all Major CLI Types
        @type major_types: List
        @return: A List of all Sub-Types
        @rtype: List
        '''
        rlist = []
        for key in cls.cli_dict.keys():
            _type = cls.cli_dict[key]
            if _type.major_type in major_types and _type.support_random:
                    rlist.append(_type)
        return rlist

    @classmethod
    def get_cli_code_from_command_id(cls, command_id):
        '''
        This routine determines the command's CLI code based on the command_id
        @param command_id: A unique command id
        @type command_id: String
        @return: The CLI Found if found
        @raise exception: Cli code not found
        '''
        try:
            _type = cls.cli_dict[command_id.lower()]
            cli_code = _type.cli_code
        except Exception:
            raise Exception('CLI Code not found')
        return cli_code

    @classmethod
    def get_cli_context(cls, fGodMode, user_command, cli_context_string):
        '''
        This routine get the CLI context associated with the user command.
        It also returns the relevant part of the user command. For e.g.
        Let's say that the user command is: 'A B C D' and that 'A B' identify
        the CLI context, then the relevant user command is 'C D'
        All CLI commands get executed in a CLI context.
        @param fGodMode: Whether the call is made in God Mode
        @type fGodMode: Boolean
        @param user_command: The user command
        @type user_command: String
        @param cli_context_string: This represents 'A B' in the above example i.e.
                                   the string that identifies the context.
        @type cli_context_string: String
        '''
        cli_params = user_command.split()
        cli_context = cls
        sub_command = user_command
        try:
            cli_context = cls.cli_context_dict[cli_params[0].lower()]
            if not fGodMode and cli_context.GodModeOnly:
                cli_context = cls#Revert
                raise Exception ('Not in God Mode')
            cli_context_string = cli_context_string + ' %s'%cli_params[0]
            cli_context_string = cli_context_string.lstrip()
            sub_command = ''.join((cli_params[i]+' ') for i in range(1,len(cli_params)))
            sub_command = sub_command.rstrip()
            cli_context, sub_command, cli_context_string = cli_context.get_cli_context(fGodMode, sub_command, cli_context_string)
        finally:
            return cli_context, sub_command, cli_context_string

    @classmethod
    def get_cli_command(cls, fGodMode, user_command):
        '''
        This routine gets the CLI command class associated with the user command.
        It also returns the relevant part of the user command. For e.g.
        Let's say that the user command is: 'A B C D' and that 'A B' identify
        the CLI, then the relevant user command is 'C D'
        @param fGodMode: Whether the call is made in God Mode
        @type fGodMode: Boolean
        @type user_command: String
        @param user_command: String
        '''
        cli_params = user_command.split()
        cli_type = cls
        sub_command = user_command
        try:
            cli_type = cls.cli_dict[cli_params[0].lower()]
            if not fGodMode and cli_type.GodModeOnly:
                cli_type = cls#Revert
                raise Exception ('Not in God Mode')
            sub_command = ''.join((cli_params[i]+' ') for i in range(1,len(cli_params)))
            sub_command = sub_command.rstrip()
        finally:
            return cli_type, sub_command

    @classmethod
    def get_values(cls, params):
        '''
        This routine fetches the value of a cli given the parameters
        @param cls: The CLI Type Class
        @type cls: Class
        @param params: The List of Parameters
        @type params: List []
        @raise exception: If Parameters are not correct
        '''
        values = []
        for i in range(0,len(params)):
            #Convert to right type
            try:
                command_format = cls.command_format[i]
            except Exception:
                break
            try:
                param_name = cls.command_format[i][0]#Name
                param_type = cls.command_format[i][1]#Type
                param_range = cls.command_format[i][3]#Range
                values.append(param_type.value(params[i], param_range))
            except Exception, ex:
                raise Exception('<%s> %s'%(param_name, ex))
        return values

    @classmethod
    def get_command_match(cls, fGodMode, user_input):
        '''
        This routine gets the closest command that matches the user input
        @param fGodMode: Whether the call is made in God Mode
        @type fGodMode: Boolean
        @param user_input: The command that the user has input so far
        @type user_input: String, maybe ''
        @return : cli_context, cli_context_string, sub_command, list
        @return cli_context: The CLI context class
        @return cli_context_string: The CLI context unique string
        @return sub_command: The Sub Command
        @rtype sub_command: String
        @return sub_command_complete: If the sub_command has been completed
        @rtype sub_command_complete: Boolean
        @return list: The list of matching commands
        '''
        #Get the deepest Context
        cli_context, sub_command, cli_context_string = cls.get_cli_context(fGodMode, user_input, '')
        list = []
        list_cli = []
        cli_params = sub_command.split()
        try:
            cli_command_id = cli_params[0]
        except Exception:
            cli_command_id = ''
        if len(cli_params) <= 1:
            #User has doesn't know the command id, provide list of all CLIs that match
            for key in cli_context.cli_dict.keys():
                cli_type = cli_context.cli_dict[key]
                if not fGodMode and cli_type.GodModeOnly:
                    continue
                #Not exact match, other possibilities exists
                if cli_type.command_id.lower().find(cli_command_id.lower()) == 0:
                    list.append(cli_type.command_id)
                    list_cli.append(cli_type)
            if len(list_cli) == 1:
                #There is exact match, provide the exact
                cli_type = list_cli.pop()
                list = []
                list.append(cli_type.command_example)
                if len(cli_params) == 0:
                    cli_params.append(cli_type.command_id)
                else:
                    cli_params[0] = cli_type.command_id
                sub_command = ''.join((cli_params[i]+' ') for i in range(len(cli_params)))
                sub_command = sub_command.rstrip()
            elif len(list_cli) > 1:
                #Multiple matches, get the longest substring match
                max_match = cli_base.longest_match(list)
                sub_command = list[0][:max_match]
        #Check if the CLI Type can be derived based on user input so far
        if len(list_cli) <= 1:
            cli_type, cli_command = cli_context.get_cli_command(fGodMode, sub_command)
        else:
            cli_type = cli_context
            cli_command = sub_command
        if cli_type == cli_context:
            #No clear CLI Type as yet, return with matching List
            list.sort(key=str.lower)
            return cli_context, cli_context_string, sub_command, False, list
#        else:
#            cli_type = cli_context
#            cli_command = sub_command
#            log.warning('cli_type %s, cli_command %s\r', cli_type, cli_command)
        #User has input part of a command, Find the matching CLI
        list = []
        sub_command_completed = False
        cli_params = cli_command.split()
        while True:
            if len(cli_params) == 0:
                list.append(cli_type.command_example)
                sub_command_completed = True
                break
            if len(cli_params) > 0:
                #Verify all parameters but the last
                try:
                    values = cli_type.get_values(cli_params[:len(cli_params)-1])
                except Exception, ex:
                    print 'ERROR: %s\r'%(ex)
                    break
                #Try and see if the final parameter needs to be completed
                final_input = cli_params[len(cli_params)-1]
                try:
                    final_param_format = cli_type.command_format[len(cli_params)-1]
                except Exception:
                    #Ignore input since format doesn't exist
                    sub_command_completed = True
                    #EXIT - Nothing to DO
                    break
                #Get Final Parameter Types
                final_param_name = final_param_format[0]
                final_param_type = final_param_format[1]
                final_param_range = final_param_format[3]
                if final_param_type == cli_type_string_set:
                    #Check for matches
                    final_param_match_list = []
                    for match_string in final_param_range:
                        if match_string.lower().find(final_input.lower()) == 0:
                            final_param_match_list.append(match_string)
                    if len(final_param_match_list) == 0:
                        #No match, Bad user input
                        print 'ERROR: <%s> must be in %s\r'%(final_param_name, final_param_range)
                        break
                    elif len(final_param_match_list) > 1:
                        #Multiple possibilities exist
                        sample_input_start = cli_type.command_id + ' ' + ''.join((cli_params[i]+' ') for i in range(len(cli_params)-1))
                        sample_input_final = ''.join((final_param_match_list[i]+':') for i in range(len(final_param_match_list)))
                        sample_input_final = sample_input_final.rstrip(':')
                        sample_input = sample_input_start + '<' + sample_input_final + '>'
                        list.append(sample_input)
                        #Extend final parameter to the max match
                        max_match = cli_base.longest_match(final_param_match_list)
                        cli_params[len(cli_params)-1] = final_param_match_list[0][:max_match]
                        cli_command = ''.join((cli_params[i]+' ') for i in range(len(cli_params)))
                        cli_command = cli_command.rstrip()
                        break
                    #Extend final parameter to the only match
                    cli_params[len(cli_params)-1] = final_param_match_list[0]
                    cli_command = ''.join((cli_params[i]+' ') for i in range(len(cli_params)))
                    cli_command = cli_command.rstrip()
                else:
                    try:
                        value = final_param_type.value(final_input, final_param_range)
                    except Exception, ex:
                        print 'ERROR: <%s> %s\r'%(final_param_name, ex)
                        break
            #Sub Command Has been completed - Give user a hint about next parameter
            sub_command_completed = True
            try:
                next_param_format = cli_type.command_format[len(cli_params)]
            except Exception:
                next_param_format = None
            if next_param_format is not None:
                try:
                    next_param_name = next_param_format[0]
                    next_param_range = next_param_format[3]
                    sample_input = cli_type.command_id + ' ' + ''.join((cli_params[i]+' ') for i in range(len(cli_params)))
                    if len(next_param_range) == 0:
                        sample_input = sample_input + '<' + next_param_name +'>'
                    else:
                        sample_input = sample_input + '<' + '%s %s'%(next_param_name,next_param_range) + '>'
                    list.append(sample_input)
                except Exception, ex:
                    print 'PARSE Error [%s]>> Cannot get Command Format[%d] for Type %s\r'%(ex, len(cli_params), cli_type.__name__)
            break
        sub_command = cli_type.command_id + ' ' + cli_command
        sub_command = sub_command.rstrip()
        list.sort(key=str.lower)
        return cli_context, cli_context_string, sub_command, sub_command_completed, list

    @staticmethod
    def longest_starting_substring(string1, string2, max):
        '''
        This routine compares string1 and string2 till "max" characters
        and returns the number of characters in a substring that starts
        from the beginning of the strings.
        @param string1: The 1st string to match on
        @type string1: String
        @param string2: The 2nd string to match on
        @type string2: String
        @param max: The number of chars to compare
        @type max: Integer
        @return: The number of matching chars in the substring
        '''
        match = 0
        try:
            for i in range(max):
                if string1[i] != string2[i]:
                    break
                match = match + 1
        finally:
            return match

    @staticmethod
    def longest_match(list_string):
        '''
        Returns the maximum number of common starting characters in a list
        of strings
        '''
        if len(list_string) == 0:
            return 0
        max_match = len(list_string[0])
        for i in range(len(list_string)-1):
            max_match = cli_base.longest_starting_substring(list_string[i],
                                                            list_string[i+1],
                                                            max_match)
        return max_match

    @classmethod
    def process_matching_list(cls, cli_context_string, command, command_complete, matching_list):
        '''
        This routine process the user_command according to the matching list of commands.
        @param cli_context_string: The prefix that lead to the correct CLI context
        @type cli_context_string: String
        @param command: The command for the CLI context
        @type command: String
        @param command_complete: If the command has been completed
        @type command_complete: Boolean
        @param matching_list: The List of matching command for that CLI context
        @type matching_list: List 
        1. If the user hasn't completed the 1st command and there is an
           only 1 match, then complete the command.
        2. If there are matches print the list
        3. If there are no matches then beep
        @raise exception: If there is something wrong with user supplied parameters
        @return: The command to be used for the next time broken into a list of characters
        @rtype: List
        '''
        append_whitespace = command_complete
        user_params = command.split()
        if len(user_params) <= 1:
            if len(matching_list) == 1:
                #Case 1: Complete the command
                matching_params = matching_list[0].split()
                command = matching_params[0]
            elif len(matching_list) > 1:
                max_match = cli_base.longest_match(matching_list)
                matching_params = matching_list[0].split()
                command = matching_params[0][:max_match]
        if len(matching_list) >= 1:
            #Print the matching commands
            if len(matching_list) > 1:
                print '\r'
            for i in range(len(matching_list)):
                if cli_context_string == '':
                    matching_list_params = matching_list[i].split()
                    if len(matching_list_params) == 1:
                        try:
                            #Find the CLI description
                            command_id = matching_list_params[0]
                            cli_type = cls.cli_dict[command_id.lower()]
                            if len(command_id) > 20:
                                raise Exception('Too big to add description')
                            spaces = ''.join((' ')for i in range((cli_base.command_id_max_len+5)-len(command_id)))
                            print_line = command_id + spaces
                            print_line = print_line + cli_type.command_def
                            print '%s\r'%print_line
                        except Exception:
                            print '%s\r'%matching_list[i]
                    else:
                        print '%s\r'%matching_list[i]
                else:
                    print '%s %s\r'%(cli_context_string, matching_list[i])
            if len(matching_list) > 1:
                print '\r'
        else:
            sys.stdout.write(chr(7)) #BEEP
        matching_command = []
        #Create the user input characters
        for i in range(len(cli_context_string)):
            matching_command.append(cli_context_string[i])
        if cli_context_string != '':
            matching_command.append(' ')
        for i in range(len(command)):
            matching_command.append(command[i])
        if append_whitespace:
            matching_command.append(' ')
        return matching_command

    @classmethod
    def complete_user_input(cls, fGodMode, user_command):
        '''
        This routine completes the user based on the closest match and returns
        the new command string in form of characters.
        @param fGodMode: Whether the call is made in God Mode
        @type fGodMode: Boolean
        @param user_command: The command entered by the user so far
        @type user_command: Type
        '''
        cli_context, cli_context_string, sub_command, complete, matching_list = cls.get_command_match(fGodMode, user_command)
        command_chars = cli_context.process_matching_list(cli_context_string, sub_command, complete, matching_list)
        return command_chars

    @classmethod
    def random_cli(cls):
        '''
        This routine creates a cli object of the given class and assigns it
        random parameters based on its command_format
        '''
        cli_object = cls()
        cli_object.params = []
        #The Identifier for the command
        for command_format in cls.command_format:
            param_type = command_format[1]#Type
            param_range = command_format[3]#Range
            param_random = param_type.random(param_range)
            cli_object.params.append(param_random)
        return cli_object

    def get_one_param(self, terminal, cli_context, cli_type, command_history, param_index):
        '''
        This routine gets all the parameters from a cli object.
        The parameters are determined based on the CLI Class/Sub-Class.
        The parameters are appended to self.params list
        @param terminal: The instance of the terminal to use
        @type terminal: MyTerminal
        @param cli_context: The CLI Context
        @type cli_context: Class
        @param cli_type: The CLI Object Class
        @type cli_type: Class
        @param command_history: A history of commands to use for this command
        @type command_history: List of commands
        @param param_index: The Parameter Index of the CLI
        @type param_index: Integer
        @return user_input: The User String as input
        @rtype: String
        '''
        param_input = cli_type.command_format[param_index][0]
        param_type = cli_type.command_format[param_index][1]#Type
        param_range = cli_type.command_format[param_index][3]#Range
        user_input_chars = []
        while True:
            input_string = '[%s:%s:%s]>>'%(cli_context.user_input_string, cli_type.command_id, param_input)
            sys.stdout.write(input_string)
            terminal_chars, finished = terminal.get_user_input(command_history,
                                                               user_input_chars,
                                                               cli_context.help_chars,
                                                               cli_context.input_char_show)
            user_input = ''.join(terminal_chars)
            if finished:
                if user_input == '':
                    break
                try:
                    user_input = param_type.value(user_input, param_range)
                except Exception, ex:
                    #terminal.release()
                    print '\r'
                    print 'INPUT-ERROR>> %s\r'%(ex)
                    #terminal.acquire()
                    continue
                break
            #User hasn't finished inputing, see if can help them finish the command
            user_input_chars = []
            #terminal.release()
            print '\r'
            if param_type == cli_type_string_set:
                match_list = []
                for match_string in param_range:
                    if match_string.lower().find(user_input.lower()) == 0:
                        match_list.append(match_string)
                if len(match_list) == 0:
                    print '%s\r'%param_range
                else:
                    longest_match = cli_base.longest_match(match_list)
                    user_input = match_list[0][:longest_match]
                    print '%s\r'%match_list
                    for i in range(len(user_input)):
                        user_input_chars.append(user_input[i])
            elif len(param_range) == 2:
                if param_type == cli_type_string:
                    print 'String length should be in the between %s-%s bytes\r'%(param_range[0],param_range[1])
                else:
                    print 'Value should be between %s-%s\r'%(param_range[0],param_range[1])
            elif len(param_range) == 1:
                if param_type == cli_type_string:
                    print 'Maximum string length allowed %s\r'%param_range[0]
                else:
                    print 'Max Value allowed %s\r'%param_range[0]
            #terminal.acquire()
        #User Finished Input - Print a line
        #terminal.release()
        print '\r'
        #terminal.acquire()
        return user_input

    def get_all_params(self, terminal, cli_context, cli_type, command_history):
        '''
        This routine gets all the parameters from a cli object.
        The parameters are determined based on the CLI Class/Sub-Class.
        The parameters are appended to self.params list
        @param terminal: The instance of the terminal to use
        @type terminal: MyTerminal
        @param cli_context: The CLI Context
        @type cli_context: Class
        @param cli_type: The CLI Object Class
        @type cli_type: Class
        @param command_history: A history of commands to use for this command
        @type command_history: List of commands
        @rtype: None
        '''
        #Process all existing parameters to conform to correct type
        self.params = cli_type.get_values(self.params)
        #Get all non optional parameters
        while len(self.params) < cli_type.params_non_opt:
            param_index = len(self.params)
            sub_commmand_history = command_history.subcommand_history(param_index)
            user_input = self.get_one_param(terminal, cli_context, cli_type,
                                            sub_commmand_history, param_index)
            if user_input == '':
                break
            self.params.append(user_input)
        if len(self.params) < cli_type.params_non_opt:
            raise Exception("Did not get all required parameters")
        #Get all the optional parameters
        while len(self.params) < cli_type.params_total:
            param_index = len(self.params)
            sub_commmand_history = command_history.subcommand_history(param_index)
            user_input = self.get_one_param(terminal, cli_context, cli_type,
                                            sub_commmand_history, param_index)
            if user_input == '':
                break
            self.params.append(user_input)
        return

    @classmethod
    def parse(cls, fGodMode, terminal, user_command):
        """
        This routine parses the user input and create the corresponding
        CLI Object
        @param fGodMode: Whether the call is made in God Mode
        @type fGodMode: Boolean
        @param terminal: The instance of the terminal to use
        @type terminal: MyTerminal
        @type user_command: String
        @param user_command: String input by the user
        @return: The corresponding CLI Context or the CLI Object.
                 Both will NOT be set.
        """
        cli_context, command, cli_context_string = cls.get_cli_context(fGodMode, user_command, '')
        cli_type, command = cli_context.get_cli_command(fGodMode, command)
        #if cli_context == cli_type and command == '':
        #    #Change in context- and there is no remaining command
        #    return cli_context, None
        #Ignore the Exit CLI
        if cli_type == cli_base.cli_generic_exit:
            return cls, None
        #Actual command in the context
        cli_object = None
        cli_params = command.split()
        try:
            try:
                cli_object = cli_type()
                cli_object.params = cli_params
                cli_object.terminal = terminal
                cli_base.get_all_params(cli_object, terminal, cli_context, cli_type, cli_type.command_history)
                #complete sub command
                command = ''.join((str(cli_object.params[i])+' ') for i in range(len(cli_object.params)))
                command = command.rstrip()
                #Add to the particular cli type history
                if cli_type.input_char_show:
                    cli_type.command_history.add_string(command)
            except Exception, ex:
                print 'ERROR: %s\r'%(ex)
                print 'Expected Command>> %s\r'%(cli_type.command_example)
                cli_object = None
        finally:
            return None, cli_object

