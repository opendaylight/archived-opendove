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
        ret_val, subnet_list = dcslib.create_ipsubnetlist()
        if ret_val != DOVEStatus.DOVE_STATUS_OK:
            raise Exception ('Fail to create IP subnet list')
        #The C object list for Subnet
        self.CList = subnet_list
        #The Python list for Subnet
        self.PyList = {}
        self.count = 0

    def add(self, ip_value, mask_value, ip_gateway, mode):
        '''
        Adds a IP Subnet to List
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @param ip_gateway: The Gateway associated with the subnet
        @type ip_gateway: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG
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
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG
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
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG, 0, 0, 0, 0

        ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.IPSubnetLookup(ip_value)

        return ret_val, socket.htonl(subnet_ip), socket.htonl(subnet_mask), subnet_mode, socket.htonl(subnet_gateway)

    def IPSubnetLookup(self,ip_val):
	'''
	This function will do a lookup on Python Subnet Dictionary for an IP and returns
	the subnet details.
	'''
        try:
            key_list = self.PyList.keys()
            for key in key_list:
                ipmask = key.split(':')
                ip_str = ipmask[0]
                mask_str = ipmask[1]
                ip = struct.unpack('!L',socket.inet_aton(ip_str))[0]
                mask = struct.unpack('!L',socket.inet_aton(mask_str))[0]
                lookupip = struct.unpack('!L',socket.inet_aton(ip_val))[0]
                if (ip & mask) == (lookupip & mask):
                    subnet = self.PyList[key]
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
        if self.CList is None:
            raise Exception('Invalid')
        ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.lookup(ip_value)
        if ret_val == DOVEStatus.DOVE_STATUS_OK:
            return subnet_mode
        else:
            raise Exception('Invalid')

    def get(self, ip_value, mask_value):
        '''
        Get a exact subnet according to IP and Mask pair
        @param ip_value: IP Address
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode, subnet_gateway
        @rtype: Integer, Integer, Integer, Integer, Integer
        '''
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG, 0, 0, 0, 0
        c_struct = struct.pack('III',
                               IPSUBNETListOpcode.IP_SUBNET_LIST_OPCODE_GET, 
                               socket.ntohl(ip_value), socket.ntohl(mask_value))
        ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = dcslib.process_ipsubnet(self.CList, c_struct)
        return ret_val, socket.htonl(subnet_ip), socket.htonl(subnet_mask), subnet_mode, socket.htonl(subnet_gateway)

    def show(self):
        '''
        List all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG
        c_struct = struct.pack('I', IPSUBNETListOpcode.IP_SUBNET_LIST_OPCODE_LIST)
        ret_val = dcslib.process_ipsubnet(self.CList, c_struct)
        return ret_val

    def flush(self):
        '''
        Flush all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG
        c_struct = struct.pack('I', IPSUBNETListOpcode.IP_SUBNET_LIST_OPCODE_FLUSH)
        ret_val = dcslib.process_ipsubnet(self.CList, c_struct)
        self.count = 0
        self.PyList.clear()
        return ret_val

    def getallids(self):
        '''
        Get all IDs from list
        @return: status, subnet_ids
        @rtype: Integer, String
        '''
        if self.CList is None:
            return DOVEStatus.DOVE_STATUS_INVALID_DVG, ''
        c_struct = struct.pack('I', IPSUBNETListOpcode.IP_SUBNET_LIST_OPCODE_GETALLIDS)
        ret_val, subnet_ids = dcslib.process_ipsubnet(self.CList, c_struct)
        return ret_val, subnet_ids

    def destroy(self):
        '''
        Destructor all IP Subnets from List
        '''
        if self.CList is None:
            return
        c_struct = struct.pack('I', IPSUBNETListOpcode.IP_SUBNET_LIST_OPCODE_DESTROY)
        ret_val = dcslib.process_ipsubnet(self.CList, c_struct)
        if ret_val != DOVEStatus.DOVE_STATUS_OK:
            message = 'IPSubnetList.destroy: Failed return value %s'%ret_val
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.count = 0
        self.CList = None
        self.PyList.clear()
        return
