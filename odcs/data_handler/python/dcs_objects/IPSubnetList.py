'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas and jinghe
'''

import logging
from logging import getLogger
log = getLogger(__name__)

import struct
import socket

from object_collection import DOVEStatus
from object_collection import IPSUBNETListOpcode
from object_collection import DpsLogLevels

import dcslib

class IPSubnet:
    '''
    Represents a SUBNET in PYTHON
    '''
    def __init__(self, ip_value, mask_value, ip_gateway, mode):
        '''
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @param ip_gateway: The Gateway associated with the subnet
        @type ip_gateway: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        '''
        self.ip_value = ip_value
        self.mask_value = mask_value
        self.ip_gateway = ip_gateway
        self.mode = mode

class IPSubnetList:
    '''
    Represents an List of IP Subnet Ranges.
    Note that currently on IPv4 subnet ranges are supported
    '''
    #Represents the format for the IPv4 and IPv6 addresses
    #IPv6 is represented as a character string of 16 bytes
    #IPv4 is represented as an Integer
    fmts = {socket.AF_INET6: '16s',
            socket.AF_INET: 'I'}

    def __init__(self):
        '''
        Constructor:
        @param inet_type: socket type i.e. AF_INET or AF_INET6
        @type inet_type: Integer
        '''
        #The Python list for Subnet
        self.PyList = {}
        self.count = 0

    def add(self, ip_value, mask_value, ip_gateway, mode):
        '''
        Adds a IP Subnet to List
        @param ip_value: IP Address of Subnet (Network Byte Order)
        @type ip_value: Integer
        @param mask_value: Mask of Subnet (Network Byte Order)
        @type mask_value: Integer
        @param ip_gateway: The Gateway associated with the subnet (Network Byte Order)
        @type ip_gateway: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        #Add to Python List
        key = '%s:%s'%(ip_value, mask_value)
        self.PyList[key] = IPSubnet(ip_value, mask_value, ip_gateway, mode)
        self.count += 1
        return DOVEStatus.DOVE_STATUS_OK

    def delete(self, ip_value, mask_value):
        '''
        Deletes a IP Subnet from List
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        key = '%s:%s'%(ip_value, mask_value)
        try:
            del self.PyList[key]
        except Exception:
            pass
        self.count -= 1
        if self.count < 0:
            message = 'IPSubnetList.delete: Count %s < 0 - Counting Error'%self.count
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return DOVEStatus.DOVE_STATUS_OK

    def lookup(self, ip_value):
        '''
        Check if a given IP falls into a subnet in the list and get the subnet
        @param ip_value: IP Address
        @type ip_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode, subnet_gateway
        @rtype: Integer, Integer, Integer, Integer, Integer
        '''
        try:
            subnet_list = self.PyList.values()
            for subnet in subnet_list:
                if (subnet.ip_value & subnet.mask_value) == (ip_val & subnet.mask_value):
                    return DOVEStatus.DOVE_STATUS_OK, subnet.ip_value, subnet.mask_value, subnet.mode, subnet.ip_gateway
        except Exception:
            return DOVEStatus.DOVE_STATUS_NOT_FOUND, 0, 0, 0, 0

    def valid_ip(self, ip_value):
        '''
        Check if a given IP falls in one of the Subnets in the Subnet List
        @param ip_value: IP Address
        @type ip_value: Integer
        @return: True or False
        @rtype: Boolean
        '''
        ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.lookup(ip_value)
        if ret_val == DOVEStatus.DOVE_STATUS_OK:
            return subnet_mode
        else:
            raise Exception('Invalid')

    def get(self, ip_value, mask_value):
        '''
        Get a exact subnet according to IP and Mask pair
        @param ip_value: IP Address (Network Byte Order)
        @type ip_value: Integer
        @param mask_value: Mask of Subnet (Network Byte Order)
        @type mask_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode, subnet_gateway
        @rtype: Integer, Integer, Integer, Integer, Integer
        '''
        try:
            subnet_list = self.PyList.values()
            for subnet in subnet_list:
                if (subnet.ip_value & subnet.mask_value) == (ip_val & subnet_mask):
                    return DOVEStatus.DOVE_STATUS_OK, subnet.ip_value, subnet.mask_value, subnet.mode, subnet.ip_gateway
        except Exception:
            return DOVEStatus.DOVE_STATUS_NOT_FOUND, 0, 0, 0, 0

    def show(self):
        '''
        List all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        try:
            subnet_list = self.PyList.values()
            for subnet in subnet_list:
                ip_value_packed = struct.pack('I', subnet.ip_value)
                mask_value_packed = struct.pack('I', subnet.mask_value)
                print 'IP: %s, Mask: %s\r'%(socket.inet_ntop(socket.AF_INET, ip_value_packed),
                                            socket.inet_ntop(socket.AF_INET, mask_value_packed))
        except Exception:
            return DOVEStatus.DOVE_STATUS_NOT_FOUND

    def flush(self):
        '''
        Flush all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.count = 0
        self.PyList.clear()
        return ret_val

    def destroy(self):
        '''
        Destructor all IP Subnets from List
        '''
        self.count = 0
        self.CList = None
        self.PyList.clear()
        return
