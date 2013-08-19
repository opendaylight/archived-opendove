
'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: Amitabha Biswas
'''

import logging
from logging import getLogger
log = getLogger(__name__)

import struct
import socket
import random

class IPAddressList:
    '''
    Represents an IP Address list.
    '''

    #Represents the format for the IPv4 and IPv6 addresses
    #IPv6 is represented as a character string of 16 bytes
    #IPv4 is represented as an Integer
    fmts = {socket.AF_INET6: '16s',
            socket.AF_INET: 'I'}

    def __init__(self, inet_type):
        '''
        Constructor:
        @param inet_type: socket type i.e. AF_INET or AF_INET6
        @type inet_type: Integer
        '''
        if inet_type != socket.AF_INET6 and inet_type != socket.AF_INET:
            raise Exception('Not Valid IP Type, only IPv4 and IPv6 are supported')
        self.inet_type = inet_type
        self.ip_hash = {}
        self.ip_list = []

    def add(self, inet_type, ip_value):
        '''
        @param inet_type: socket type i.e. AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: IPv4 or IPv6 IP Address in Network Byte Order
        @type ip_value: IPv4 integer, IPv6 String
        '''
        if inet_type != self.inet_type:
            raise Exception('Not Match IP Type')
        try:
            old_value = self.ip_hash[ip_value]
        except Exception:
            self.ip_hash[ip_value] = ip_value
            self.ip_list.append(ip_value)
        return 0

    def remove(self, inet_type, ip_value):
        '''
        Removes an IP Address from the collection
        @param inet_type: socket type i.e. AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: IPv4 or IPv6 IP Address in network byte order
        @type ip_value: IPv4 integer, IPv6 String
        '''
        if inet_type != self.inet_type:
            raise Exception('Not Match IP Type')
        try:
            del self.ip_hash[ip_value]
            self.ip_list.remove(ip_value)
        except Exception:
            pass
        return

    def search(self, ip_value):
        '''
        Searches for an IP Address in the list
        @param ip_value: IPv4 or IPv6 IP Address in Network Byte Order
        @type ip_value: IPv4 integer, IPv6 String
        @return - True if found, False if not found
        @rtype - Boolean
        '''
        try:
            ip = self.ip_hash[ip_value]
            return True
        except Exception:
            return False

    def count(self):
        '''
        Returns the number of elements in the collection
        '''
        return len(self.ip_hash)

    def clear(self):
        '''
        Clears all IP Address entries from the list
        '''
        self.ip_hash.clear()
        del self.ip_list[:]
        return

    def get_primary(self):
        '''
        Returns the 1st value from the list. If there is no IP Address available
        an exception is raised
        '''
        return self.ip_list[0]

    def get_random(self):
        '''
        Gets a random value from the list. If there is no IP Address available
        an exception is raised
        '''
        if len(self.ip_list) == 0:
            raise Exception('No IP Address')
        else:
            return self.ip_list[random.randint( 0, (len(self.ip_list)-1) )]

    def show(self):
        '''
        Returns the string version of the (IP, Port) for this instance of IP
        Address Location
        '''
        if len(self.ip_list) > 0:
            ip_string_list = ''
            for ip_value in self.ip_list:
                ip_packed = struct.pack(IPAddressList.fmts[self.inet_type], ip_value)
                ip_string = socket.inet_ntop(self.inet_type, ip_packed)
                ip_string_list += '%s, '%(ip_string)
            ip_string_list.rstrip()
            ip_string_list.rstrip(',')
        else:
            ip_string_list = 'None'
        print '%s\r'%ip_string_list

    def toString(self):
        '''
        Returns the string version of the (IP, Port) for this instance of IP
        Address Location
        '''
        if len(self.ip_list) > 0:
            ip_string_list = ''
            for ip_value in self.ip_list:
                ip_packed = struct.pack(IPAddressList.fmts[self.inet_type], ip_value)
                ip_string = socket.inet_ntop(self.inet_type, ip_packed)
                ip_string_list += '%s, '%(ip_string)
            ip_string_list=ip_string_list.rstrip()
            ip_string_list=ip_string_list.rstrip(',')
        else:
            ip_string_list = ''
        return ip_string_list;
