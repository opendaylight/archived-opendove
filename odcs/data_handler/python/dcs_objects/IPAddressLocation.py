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

class IPAddressLocation:
    '''
    Represents an IP Address and a Port i.e. a Service Endpoint. In some cases
    the port may be non-existent in which case this structure just 
    represents an IP Address
    '''

    #Represents the format for the IPv4 and IPv6 addresses
    #IPv6 is represented as a character string of 16 bytes
    #IPv4 is represented as an Integer
    fmts = {socket.AF_INET6: '16s',
            socket.AF_INET: 'I'}

    def __init__(self, inet_type, ip_value, port):
        '''
        Constructor:
        @param inet_type: socket type i.e. AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: IPv4 (Network Order)or IPv6 IP Address
        @type ip_value: IPv4 (Network Order) integer, IPv6 String
        @param port: The Port of Location 1-65535 (Host Order)
        @type port: Integer 
        '''
        if inet_type != socket.AF_INET6 and inet_type != socket.AF_INET:
            raise Exception('Not Valid IP Type, only IPv4 and IPv6 are supported')
        self.inet_type = inet_type
        self.ip_value = ip_value
        #IP Mode Integer (0 = Dedicated, 1 = Shared)
        self.mode = 0
        #Store the IP Address in packed format
        try:
            self.ip_value_packed = struct.pack(IPAddressLocation.fmts[self.inet_type],
                                               self.ip_value)
            self.port = port
            self.fValid = True
        except Exception:
            self.ip_value_packed = struct.pack('I',0)
            self.port = 0
            self.fValid = False

    def show(self):
        '''
        Returns the string version of the (IP, Port) for this instance of IP
        Address Location
        '''
        if self.fValid:
            #inet_ntop is fine: ABiswas (but Eclipse View complaining)
            ip_string = socket.inet_ntop(self.inet_type, self.ip_value_packed)
            return '(%s, %s)'%(ip_string, self.port)
        else:
            return 'Invalid'

    def show_ip(self):
        '''
        Returns a string version of IP only
        '''
        if self.fValid:
            return socket.inet_ntop(self.inet_type, self.ip_value_packed)
        else:
            return '0.0.0.0'

    @staticmethod
    def Equal(IP1, IP2):
        '''
        Checks if 2 IPAddress Locations are equal
        '''
        if IP1.ip_value == IP2.ip_value and IP1.port == IP2.port:
            return True
        else:
            return False
