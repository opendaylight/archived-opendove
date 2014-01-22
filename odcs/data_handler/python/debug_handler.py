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

from threading import Timer

import socket
from object_collection import DpsCollection
from object_collection import DOVEStatus
from object_collection import DpsClientType
from object_collection import DOVEGatewayTypes
from object_collection import DpsTransactionType
from object_collection import IPSUBNETMode

import struct

from dcs_objects.Dvg import DVG
from dcs_objects.Policy import Policy
from dcs_objects.TunnelEndpoint import DPSClient
from dcs_objects.TunnelEndpoint import TunnelEndpoint
from dcs_objects.Endpoint import Endpoint
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.Multicast import Multicast

import dcslib

class DpsDebugHandler(object):
    '''
    This class handles requests for debug information from the REST Client
    '''

    def __init__(self):
        '''
        Constructor:
        '''
        #Collection of Domains
        self.Domain_Hash = DpsCollection.Domain_Hash
        self.Domain_Hash_Global = DpsCollection.Domain_Hash_Global
        self.VNID_Hash = DpsCollection.VNID_Hash
        self.lock = DpsCollection.global_lock
        self.ip_get_val_from_packed = {socket.AF_INET6: self.ipv6_get_val_from_packed,
                                       socket.AF_INET: self.ipv4_get_val_from_packed}

    def ipv4_get_val_from_packed(self, ip_packed):
        '''
        This routine get IPv4 address from packed data. The packed data is
        usually 16bytes (union of IPv4 and IPv6) address
        @param ip_packed: Packed IP data
        @type ip_packed: ByteArray
        '''
        ip_val = struct.unpack('I', ip_packed[:4])
        return ip_val[0]

    def ipv6_get_val_from_packed(self, ip_packed):
        '''
        This routine get IPv4 address from packed data. The packed data is
        usually 16bytes (union of IPv4 and IPv6) address
        @param ip_packed: Packed IP data
        @type ip_packed: ByteArray
        '''
        ip_val = struct.unpack('16z', ip_packed[:16])
        return ip_val[0]

    def Get_DVG_Object(self, vnid):
        '''
        This routine returns the DVG Object given the VNID
        @param vnid: The VNID 
        @type vnid: Integer
        @return: DVG
        @rtype: DVG
        '''
        dvg = None
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
                domain = self.Domain_Hash[domain_id]
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                break
            break
        self.lock.release()
        return dvg

    def Get_VNID_Endpoints(self, vnid):
        '''
        This routine returns a list of Endpoints in this VNID. Each Endpoint
        returned will be a tuple of consisting of following:
        (vMac, pIP, [vIP1, vIP2, vIP3])
        @param vnid: The VNID 
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        list_endpoints = []
        while True:
            dvg = self.Get_DVG_Object(vnid)
            if dvg is None:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            self.lock.acquire()
            try:
                for endpoint in dvg.Endpoint_Hash_MAC.values():
                    pIP = endpoint.tunnel_endpoint.primary_ip()
                    HostIP = endpoint.tunnel_endpoint.dps_client.location
                    if len(endpoint.vIP_set) == 0:
                        list_endpoints.append((endpoint.vMac, HostIP.ip_value_packed, pIP.ip_value_packed, DpsCollection.Invalid_IP_Packed))
                    else:
                        for vIP in endpoint.vIP_set_show.values():
                            list_endpoints.append((endpoint.vMac, HostIP.ip_value_packed, pIP.ip_value_packed, vIP.ip_value_packed))
            except Exception:
                pass
            self.lock.release()
            break
        return (ret_val, list_endpoints)

    def Get_VNID_DoveSwitches(self, vnid):
        '''
        This routine returns a list of DOVE Switches in this VNID. Each Tunnel
        returned will be a list of consisting of following i.e.
        [pIPa1, pIPa2, pIPa3], [pIPb1], [pIPc1, pIPc2]...]
        where each pIPx is a Integer (AF_INET) or String (AF_INET6)
        @param vnid: The VNID
        @type vnid: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        list_tunnels = []
        while True:
            dvg = self.Get_DVG_Object(vnid)
            if dvg is None:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            tunnel_set = {}
            self.lock.acquire()
            try:
                for client_type in DpsClientType.types.keys():
                    for tunnel in dvg.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                        tunnel_set[tunnel] = True
                    for tunnel in dvg.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                        tunnel_set[tunnel] = True
                for tunnel in tunnel_set.keys():
                    HostIP = tunnel.dps_client.location
                    ip_list = []
                    for ip in tunnel.ip_listv4.ip_list:
                        ip_list.append(ip)
                    for ip in tunnel.ip_listv6.ip_list:
                        ip_list.append(ip)
                    list_tunnels.append((HostIP.ip_value_packed,ip_list))
            except Exception:
                pass
            self.lock.release()
            break
        return (ret_val, list_tunnels)

    def Get_Domain_VNID_Mapping(self):
        '''
        This routine shows the Domain --> VNID mapping
        & the VNID --> Domain mapping
        @return: Domain Global Mapping, VNID Mapping
        @rtype: {}, {}
        '''
        #print 'self.VNID_Hash %s\r'%self.VNID_Hash
        return (self.Domain_Hash_Global, self.VNID_Hash)

    def Get_VNID_Allow_Policies(self, vnid):
        '''
        This routine returns all the VNIDs this vnid can talk to
        @param vnid: The VNID
        @type vnid: Integer
        @return: (status, String, String)
        @rtype: (Integer, "src1:dst1,src2:dst2" etc, "src1:dst1,src2:dst2" etc
        '''
        #Get the DVG object
        status = DOVEStatus.DOVE_STATUS_OK
        unicast_policies = ""
        multicast_policies = ""
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            #Unicast policies where this vnid is the source
            dst_vnids = dvg.Policy_Source[0].keys()
            for dst_vnid in dst_vnids:
                try:
                    policy = dvg.Policy_Source[0][dst_vnid]
                    if policy.action_connectivity != Policy.action_forward:
                        continue
                except Exception:
                    continue
                unicast_policies += "%s:%s,"%(vnid,dst_vnid)
            #Unicast policies where this vnid is the destination
            src_vnids = dvg.Policy_Destination[0].keys()
            for src_vnid in src_vnids:
                if src_vnid == vnid:
                    #Already covered earlier
                    continue
                try:
                    policy = dvg.Policy_Destination[0][src_vnid]
                    if policy.action_connectivity != Policy.action_forward:
                        continue
                except Exception:
                    continue
                unicast_policies += "%s:%s,"%(src_vnid,vnid)
            unicast_policies = unicast_policies.rstrip(",")
            #Multicast policies where this vnid is the source
            dst_vnids = dvg.Policy_Source[1].keys()
            for dst_vnid in dst_vnids:
                try:
                    policy = dvg.Policy_Source[0][dst_vnid]
                    if policy.action_connectivity != Policy.action_forward:
                        continue
                except Exception:
                    continue
                multicast_policies += "%s:%s,"%(vnid,dst_vnid)
            #Multicast policies where this vnid is the destination
            src_vnids = dvg.Policy_Destination[1].keys()
            for src_vnid in src_vnids:
                if src_vnid == vnid:
                    #Already covered earlier
                    continue
                try:
                    policy = dvg.Policy_Destination[1][src_vnid]
                    if policy.action_connectivity != Policy.action_forward:
                        continue
                except Exception:
                    continue
                multicast_policies += "%s:%s,"%(src_vnid,vnid)
            multicast_policies = multicast_policies.rstrip(",")
            break
        return (status, unicast_policies, multicast_policies)

    def Get_VNID_Subnets(self, vnid):
        '''
        This routine returns all the Subnets associated with this VNID
        @param vnid: The VNID
        @type vnid: Integer
        @return: (Mode, IP4, Mask, Gateway)
        @rtype: (String, Integer, Integer, Integer)
        '''
        #Get the DVG object
        status = DOVEStatus.DOVE_STATUS_OK
        subnet_list = []
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                status = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            #Each element of the subnet list contains 4 integer tuple
            subnets = dvg.IP_Subnet_List.PyList.values()
            for subnet in subnets:
                if subnet.mode == 0:
                    mode_string = 'Dedicated'
                else:
                    mode_string = 'Shared'
                subnet_list.append((mode_string, subnet.ip_value,
                                    subnet.mask_value, subnet.ip_gateway))
            break
        #print '%s\r'%subnet_list
        return (status, subnet_list)

