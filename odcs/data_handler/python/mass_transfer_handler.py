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

import socket
import struct
import time
import threading
from threading import Lock
from threading import Timer

from object_collection import DOVEStatus
from object_collection import DpsCollection
from object_collection import DpsClientType
from object_collection import IPSUBNETMode
from object_collection import IPSUBNETAssociatedType
from object_collection import DpsLogLevels
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.Endpoint import Endpoint
from dcs_objects.Policy import Policy
from object_collection import mac_show
from cluster_database import ClusterDatabase
from client_protocol_handler import DpsClientHandler

import dcslib

class DPSVNIDMassTransfer:
    '''
    This PYTHON class represents a set of VNIDs in the mass transfer
    '''
    MAX_VNIDS_PER_LIST = 10000
    def __init__(self, vnids, fadd):
        '''
        @param vnids: List of VNIDs
        @type tunnel: Policy
        @param fadd: Whether it's a add or delete
        @type fadd: Boolean
        '''
        self.vnids = vnids
        self.fadd = fadd

    def show(self):
        '''
        '''
        return self.vnids

class DPSPolicyMassTransfer:
    '''
    This PYTHON class represents an Policy in the mass transfer
    '''
    MAX_POLICIES_PER_LIST = 4000
    def __init__(self, policies, fadd):
        '''
        @param policy: Set of Policy Objects
        @type policy: []
        @param fadd: Whether it's a add or delete
        @type fadd: Boolean
        '''
        self.policies = policies
        self.fadd = fadd

    def show(self):
        '''
        '''
        return self.policies

class DPSSubnetMassTransfer:
    '''
    This PYTHON class represents an Subnet in the mass transfer
    '''
    MAX_SUBNET_PER_LIST = 4000
    def __init__(self, subnets, fadd):
        '''
        @param subnets: Array of subnets
        @type subnets: List
        @param fadd: Whether it's a add or delete
        @type fadd: Boolean
        '''
        self.fadd = fadd
        self.subnets = subnets

    def show(self):
        '''
        '''
        return self.subnets

class DPSTunnelMassTransfer:
    '''
    This PYTHON class represents an Tunnel in the mass transfer
    '''
    def __init__(self, client_type, fregister, host_location, pip_tuple_list):
        '''
        @param client_type: The Tunnel Type (Switch or Gateway)
        @type client_type: DpsClientType
        @param fregister: Whether it's a register or de-register
        @type fregister: Boolean
        @param host_location: The location of Host(DPS Client) hosting this tunnel
        @type host_location: IPAddressLocation
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        '''
        self.client_type = client_type
        self.fregister = fregister
        self.host_location = host_location
        self.pip_tuple_list = []
        for pip_tuple in pip_tuple_list:
            inet_type = pip_tuple[0]
            ip_value = pip_tuple[1]
            self.pip_tuple_list.append((inet_type, ip_value))

    def show(self):
        '''
        '''
        client_string = DpsClientType.types[self.client_type]
        try:
            #print 'pip_tuple_list %s\r'%self.pip_tuple_list
            pip_tuple = self.pip_tuple_list[0]
            #print 'pip_tuple %s\r'%pip_tuple
            inet_type = pip_tuple[0]
            ip_value = pip_tuple[1]
            ip_value_packed = struct.pack(IPAddressLocation.fmts[inet_type],ip_value)
            ip_string = socket.inet_ntop(inet_type, ip_value_packed)
        except Exception, ex:
            message = 'Tunnel.show exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ip_string ='0.0.0.0'
        return '%s: IP %s'%(client_string, ip_string)

class DPSEndpointMassTransfer:
    '''
    This PYTHON class represents an Endpoint in the mass transfer
    '''
    def __init__(self, endpoint, operation, vIP_type, vIP_val):
        '''
        @param endpoint: The Endpoint Object
        @type endpoint: Endpoint
        @param operation: The type of Endpoint operation
        @type operation: Integer
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_val: The IP address of the Endpoint
        @type vIP_val: Integer or String
        '''
        self.endpoint = endpoint
        self.operation = operation
        self.vIP = IPAddressLocation(vIP_type, vIP_val, 0)

    def show(self):
        '''
        '''
        return mac_show(self.endpoint.vMac)

class DPSMulticastMassTransfer:
    '''
    This PYTHON class represents a Multicast Sender or Receiver Registration
    during Mass Transfer
    '''
    def __init__(self, domain_id, vnid, fsender, fregister, client_type, global_scope,
                 multicast_mac, multicast_ip_family, multicast_ip_packed,
                 tunnel_ip_packed):
        '''
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param fsender: Whether this is Sender or Receiver
        @type fsender: Boolean
        @param fregister: Whether this is registration or un-register
        @type fregister: Boolean
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        self.domain_id = domain_id
        self.vnid = vnid
        self.fsender = fsender
        self.fregister = fregister
        self.client_type = client_type
        if global_scope:
            self.global_scope = 1
        else:
            self.global_scope = 0
        self.multicast_mac = multicast_mac
        self.multicast_ip_family = multicast_ip_family
        self.multicast_ip_packed = multicast_ip_packed
        self.tunnel_ip_packed = tunnel_ip_packed

    def show(self):
        '''
        '''
        try:
            multicast_ip_string = socket.inet_ntop(self.multicast_ip_family, self.multicast_ip_packed)
        except Exception:
            multicast_ip_string = '0.0.0.0'
        return multicast_ip_string

class DPSDomainMassTransfer:
    '''
    This PYTHON class handles the Mass Transfer of objects related
    to a domain from one DPS Node to another. This mass transfer
    handler assumes that the Domain and DVGs have already been
    created at the destination DPS with the appropriate setting.
    @attention: Lock Hierarchy
                1. DpsCollection.global_lock
                2. DPSDomainMassTransfer Object Lock self.lock
                3. DpsCollection.mass_transfer_lock
    '''
    #Different stages of transfer. A transfer stage cannot finish
    #till all objects in that state are transferred. Think of this
    #as a finite state machine
    transfer_start = 0
    transfer_vnids = 1
    transfer_policies = 2
    transfer_subnets = 3
    transfer_tunnels = 4
    transfer_endpoints = 5
    transfer_multicasts = 6
    transfer_finished = 7
    MAX_RETRIES_PER_OBJECT = 20
    TIMER_SLEEP = 2

    transfer_vnid_list_per_sec = 10
    transfer_policies_per_sec = 10
    transfer_subnets_per_sec = 10
    transfer_tunnels_per_sec = 250
    transfer_endpoints_per_sec = 500
    transfer_multicast_per_sec = 250

    transfer_string = {transfer_start: 'Start',
                       transfer_vnids: 'VNIDs',
                       transfer_policies: 'Policy',
                       transfer_subnets: 'Subnet',
                       transfer_tunnels: 'Tunnel',
                       transfer_endpoints: 'Endpoint',
                       transfer_multicasts: 'Multicast',
                       transfer_finished: 'Finished'}

    #If the number of remaining registrations are less than this number
    #then force retry
    remaining_registration_retry = 500

    def __init__(self, domain, inet_type, ip_value, port, finish_callback, weight):
        '''
        This routine initializes the object associated with Mass Transfer
        for a particular Domains Objects. Once such an object is initialized
        for a domain, the domain MUST invoke the appropriate calls when
        VM/Tunnel/Multicast registration occurs in this domain while the
        mass transfer is happening
        @param domain: The Domain Object
        @type domain: Domain
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The DPS IP Address to transfer to
        @type ip_value: String/Integer
        @param dps_port: The DPS Port
        @type dps_port: USHORT
        @param finish_callback: The Routine to invoke when the Mass Transfer
                                is complete
        @type finish_callback: PYTHON routine
        @param weight: The weight associated with this transfer
        @type weight: Integer
        '''
        #The set of unacknowledged transfers indexed by query IDs.
        #Key = Query ID, Value = Endpoint OR Tunnel OR Multicast Object
        self.valid = True
        self.cluster_db = ClusterDatabase()
        self.unacked = {}
        self.transfer_stage = self.transfer_start
        self.domain = domain
        self.remote_location = IPAddressLocation(inet_type, ip_value, port)
        self.finish_callback = finish_callback
        self.invoked_callback = False
        self.lock = threading.Lock()
        self.weight = weight
        #Each object transferred consists of 2 sets and the max unacked value
        # 1. The 1st set/dictionary contains objects which need to be transferred.
        #    Key = ID+object, Value = (ID, object, query_id)
        #    where ID depends on object. For e.g. tunnels have id as vnid
        #    while policies have ID as Policy ID
        # 2. The 2nd set contains objects that have been sent but not acknowledged
        #    Key = query_id, Value = (ID, object)
        self.transfer = {}
        self.transfer[self.transfer_vnids] = ({}, {}, self.transfer_vnid_list_per_sec)
        self.transfer[self.transfer_policies] = ({}, {}, self.transfer_policies_per_sec)
        self.transfer[self.transfer_subnets] = ({}, {}, self.transfer_subnets_per_sec)
        self.transfer[self.transfer_tunnels] = ({}, {}, self.transfer_tunnels_per_sec)
        self.transfer[self.transfer_endpoints] = ({}, {}, self.transfer_endpoints_per_sec)
        self.transfer[self.transfer_multicasts] = ({}, {}, self.transfer_multicast_per_sec)
        self.transfer_routine = {self.transfer_vnids: self.vnid_send,
                                 self.transfer_policies: self.policy_send,
                                 self.transfer_subnets: self.subnet_send,
                                 self.transfer_tunnels: self.tunnel_send,
                                 self.transfer_multicasts: self.multicast_send,
                                 self.transfer_endpoints: self.endpoint_send}
        self.ip_get_val_from_packed = {socket.AF_INET6: self.ipv6_get_val_from_packed,
                                       socket.AF_INET: self.ipv4_get_val_from_packed}

    @staticmethod
    def domain_mass_transfer_weight(domain):
        '''
        This routine computes the weight of a domain being mass transferred
        @param domain: The Domain Object
        @type domain: Domain.
        '''
        weight = 1 #Start with 1 for the Domain
        #Number of VNIDs
        vnids = len(domain.DVG_Hash)
        weight += vnids/(DPSVNIDMassTransfer.MAX_VNIDS_PER_LIST * DPSDomainMassTransfer.transfer_vnid_list_per_sec)
        #Number of Policies
        policies = len(domain.Policy_Hash_DVG[0])
        policies += len(domain.Policy_Hash_DVG[1])
        weight += policies/(DPSDomainMassTransfer.transfer_policies_per_sec)
        #Number of Subnets (assume 1 per vnid)
        subnets = len(domain.DVG_Hash)
        weight += subnets/(DPSDomainMassTransfer.transfer_subnets_per_sec)
        #Number of Tunnels
        tunnels = len(domain.Tunnel_Endpoints_Hash_IPv4) + len(domain.Tunnel_Endpoints_Hash_IPv6)
        if tunnels > 0:
            weight += 1 + tunnels/(DPSDomainMassTransfer.transfer_tunnels_per_sec)
        #Number of Endpoints
        endpoints = len(domain.Endpoint_Hash_MAC)
        if endpoints > 0:
            weight += 1 + endpoints/(DPSDomainMassTransfer.transfer_endpoints_per_sec)
        #Number of Multicast (assume 
        multicasts = domain.Multicast.size()
        if multicasts > 0:
            weight += 1 + multicasts/(DPSDomainMassTransfer.transfer_multicast_per_sec)
        return weight

    def remaining_registration_items(self):
        '''
        Count of the remaining items in the mass transfer
        '''
        count = 0
        for i in range(self.transfer_tunnels, self.transfer_finished):
            count += len(self.transfer[i][0])
            count += len(self.transfer[i][1])
        #print 'remaining_items %s\r'%count
        return count

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

    def vnid_send(self, query_id, vnid_key, vnid_transfer):
        '''
        This routine transfers a set of VNID
        @attention: DO NOT IMPORT from other PYTHON modules
        @param vnid_key: The VNID key for this 
        @type vnid_key: Integer
        @param policy_transfer: The Policy Transfer Object
        @type policy_transfer: DPSPolicyMassTransfer
        '''
        vnids = vnid_transfer.vnids
        if vnid_transfer.fadd:
            add = 1
        else:
            add = 0
        status = dcslib.dps_vnids_replicate(add,
                                            self.domain.unique_id,
                                            vnids,
                                            self.remote_location.ip_value_packed
                                            )
        #Synchronous Send. Remove from the UnAcked queue immediately
        if status == 0:
            vnid_unacked = self.transfer[self.transfer_vnids][1]
            try:
                del vnid_unacked[query_id]
            except Exception:
                pass
        else:
            self.valid = False
        return

    def policy_send(self, query_id, policy_key, policy_transfer):
        '''
        This routine transfers policies
        @attention: DO NOT IMPORT from other PYTHON modules
        @param id: The policy id for this registration
        @type id: Integer
        @param policy_transfer: The Policy Transfer Object
        @type policy_transfer: DPSPolicyMassTransfer
        '''
        if policy_transfer.fadd:
            add = 1
        else:
            add = 0
        status = dcslib.dps_policy_bulk_replicate(add,
                                                  self.domain.unique_id,
                                                  policy_transfer.policies,
                                                  self.remote_location.ip_value_packed
                                                  )
        #Synchronous Send. Remove from the UnAcked queue immediately
        if status == 0:
            policy_unacked = self.transfer[self.transfer_policies][1]
            try:
                del policy_unacked[query_id]
            except Exception:
                pass
        else:
            self.valid = False
        return

    def subnet_send(self, query_id, associated_id, subnet_transfer):
        '''
        This routine transfers subnets
        @attention: DO NOT IMPORT from other PYTHON modules
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param tunnel: The Tunnel Object
        @type tunnel: TunnelEndpoint
        '''
        #print 'DPSDomainMassTransfer.subnet_send: query_id %s, subnet IP %s, subnet mask %s, gateway %s\r'%(query_id,
        #                                                                                                    socket.inet_ntop(subnet_transfer.inet_type, subnet_transfer.ip_value_packed),
        #                                                                                                    socket.inet_ntop(subnet_transfer.inet_type, subnet_transfer.mask_value_packed),
        #                                                                                                    socket.inet_ntop(subnet_transfer.inet_type, subnet_transfer.ip_gateway_packed))
        status = dcslib.dps_ipsubnet_bulk_replicate(self.domain.unique_id,
                                                    subnet_transfer.fadd,
                                                    subnet_transfer.subnets,
                                                    self.remote_location.ip_value_packed
                                                   )
        #Synchronous Send. Remove from the UnAcked queue immediately
        if status == 0:
            subnet_unacked = self.transfer[self.transfer_subnets][1]
            try:
                del subnet_unacked[query_id]
            except Exception:
                pass
        else:
            self.valid = False
        return

    def tunnel_send(self, query_id, vnid, tunnel_transfer):
        '''
        This routine transfers tunnels
        @attention: DO NOT IMPORT from other PYTHON modules
        @param query_id: The Query ID to use for this transfer
        @param query_id: Integer
        @param vnid: The vnid for this registration
        @type vnid: Integer
        @param tunnel_transfer: The DPSTunnelMassTransfer Object
        @type tunnel_transfer: DPSTunnelMassTransfer
        '''
        #print 'tunnel_send %s Enter\r'%tunnel_transfer.show()
        DpsCollection.mass_transfer_lock.acquire()
        DpsCollection.MassTransfer_QueryID_Mapping[query_id] = self
        DpsCollection.mass_transfer_lock.release()
        #Create the pip list. The C code expects list of addresses (not tuples)
        pip_list = []
        for pip_tuple in tunnel_transfer.pip_tuple_list:
            pip_list.append(pip_tuple[1])
        status = dcslib.mass_transfer_tunnel(self.remote_location.ip_value_packed,
                                             self.remote_location.port,
                                             tunnel_transfer.host_location.ip_value_packed,
                                             tunnel_transfer.host_location.port,
                                             vnid,
                                             query_id,
                                             vnid, #Will this work for Domain 0, DVG 0?
                                             tunnel_transfer.client_type,
                                             tunnel_transfer.fregister,
                                             pip_list,
                                             )
        if status != 0:
            #print 'transfer_tunnel failed\r'
            #self.valid = False
            DpsCollection.mass_transfer_lock.acquire()
            try:
                del DpsCollection.MassTransfer_QueryID_Mapping[query_id]
            except Exception:
                pass
            DpsCollection.mass_transfer_lock.release()
        #print 'tunnel_send %s Exit\r'%tunnel_transfer.show()
        return

    def endpoint_send(self, query_id, vnid, endpoint_transfer):
        '''
        This routine transfers endpoints
        @attention: DO NOT IMPORT from other PYTHON modules
        @param query_id: The Query ID to use for this transfer
        @param query_id: Integer
        @param vnid: The vnid for this registration
        @type vnid: Integer
        @param endpoint_transfer: The DPSEndpointMassTransfer Object
        @type endpoint_transfer: DPSEndpointMassTransfer
        '''
        DpsCollection.mass_transfer_lock.acquire()
        DpsCollection.MassTransfer_QueryID_Mapping[query_id] = self
        DpsCollection.mass_transfer_lock.release()
        endpoint = endpoint_transfer.endpoint
        operation = endpoint_transfer.operation
        vIP = endpoint_transfer.vIP
        try:
            pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
        except Exception, ex:
            message = 'MassTransfer.endpoint_send: ip_listv4 Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            pipv4_list = []
        try:
            pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
        except Exception, ex:
            message = 'MassTransfer.endpoint_send: ip_listv6 Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            pipv6_list = []
        #TODO: Support multiple vIP. Currently due to generation of query id 
        #      being gated by DpsCollection Global Lock issue and hierarchy
        #      only a single vIP can be sent. It would be better to send
        #      all vIPs in a single message.
        status = dcslib.mass_transfer_endpoint(self.remote_location.ip_value_packed,
                                               self.remote_location.port,
                                               endpoint.tunnel_endpoint.dps_client.location.ip_value_packed,
                                               endpoint.tunnel_endpoint.dps_client.location.port,
                                               vnid,
                                               query_id,
                                               endpoint.dvg.unique_id,
                                               endpoint.client_type,
                                               endpoint.version,
                                               operation,
                                               pipv4_list,
                                               pipv6_list,
                                               endpoint.vMac,
                                               vIP.ip_value_packed
                                               )
        if status != 0:
            #print 'transfer_endpoint failed\r'
            #self.valid = False
            DpsCollection.mass_transfer_lock.acquire()
            try:
                del DpsCollection.MassTransfer_QueryID_Mapping[query_id]
            except Exception:
                pass
            DpsCollection.mass_transfer_lock.release()
        return

    def multicast_send(self, query_id, vnid, multicast_transfer):
        '''
        This routine transfers tunnels
        @attention: DO NOT IMPORT from other PYTHON modules
        @param vnid: The vnid for this registration
        @type vnid: Integer
        @param tunnel: The Tunnel Object
        @type tunnel: TunnelEndpoint
        @param key_unacked: The unacked set key
        @type key_unacked: String or Integer
        '''
        DpsCollection.mass_transfer_lock.acquire()
        DpsCollection.MassTransfer_QueryID_Mapping[query_id] = self
        DpsCollection.mass_transfer_lock.release()
        if multicast_transfer.fsender:
            status = dcslib.mass_transfer_multicast_sender(self.remote_location.ip_value_packed,
                                                           self.remote_location.port,
                                                           multicast_transfer.vnid,
                                                           query_id,
                                                           multicast_transfer.fregister,
                                                           multicast_transfer.client_type,
                                                           multicast_transfer.multicast_mac,
                                                           multicast_transfer.multicast_ip_family,
                                                           multicast_transfer.multicast_ip_packed,
                                                           multicast_transfer.tunnel_ip_packed
                                                          )
        else:
            status = dcslib.mass_transfer_multicast_receiver(self.remote_location.ip_value_packed,
                                                             self.remote_location.port,
                                                             multicast_transfer.vnid,
                                                             query_id,
                                                             multicast_transfer.fregister,
                                                             multicast_transfer.client_type,
                                                             multicast_transfer.global_scope,
                                                             multicast_transfer.multicast_mac,
                                                             multicast_transfer.multicast_ip_family,
                                                             multicast_transfer.multicast_ip_packed,
                                                             multicast_transfer.tunnel_ip_packed
                                                            )
        if status != 0:
            #self.valid = False
            DpsCollection.mass_transfer_lock.acquire()
            try:
                del DpsCollection.MassTransfer_QueryID_Mapping[query_id]
            except Exception:
                pass
            DpsCollection.mass_transfer_lock.release()
        return

    def transfer_start_get_vnids(self):
        '''
        This routine gets the vnids which need to be transferred
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
        '''
        #print 'transfer_start_get_vnids: Enter\r'
        vnid_set = self.transfer[self.transfer_vnids][0]
        key_fmt = 'vnids-section-%d'
        vnids = self.domain.DVG_Hash.keys()
        vnids_total = len(vnids)
        vnids_processed = 0
        key_index = 0
        while vnids_processed < vnids_total:
            vnid_key = key_fmt%(key_index)
            vnid_start = vnids_processed
            vnid_end = vnids_processed + DPSVNIDMassTransfer.MAX_VNIDS_PER_LIST
            if vnid_end > vnids_total:
                vnid_end = vnids_total
            vnids_list = vnids[vnid_start:vnid_end]
            #print 'transfer_start_get_vnids [%s:%s]\r'%(vnid_start,vnid_end)
            query_id = DpsCollection.generate_query_id()
            vnid_transfer = DPSVNIDMassTransfer(vnids_list, True)
            vnid_set[vnid_key] = (vnid_key, vnid_transfer, query_id)
            #print 'transfer_vnid: key %s, query_id %s, vnids %s\r'%(vnid_key, query_id, vnids_list)
            vnids_processed = vnid_end
            key_index += 1
        #print 'transfer_start_get_vnids: Exit\r'
        return

    def transfer_start_get_policies(self):
        '''
        This routine gets the policies which need to be transferred.
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
        '''
        #PyArg_Parse(pyPolicy, "IIIIII", &traffic_type, &type, &sdvg, &ddvg, &ttl, &action))
        #Get the Policy Set
        policy_set = self.transfer[self.transfer_policies][0]
        key_fmt = 'policies-section-%d'
        policy_list = []
        #Get the complete policy set first
        for i in range(2):
            for policy in self.domain.Policy_Hash_DVG[i].values():
                if policy.src_dvg == policy.dst_dvg and policy.action_connectivity == policy.action_forward:
                    #Default: No need to transfer
                    continue
                if policy.type == Policy.type_connectivity:
                    action = policy.action_connectivity
                else:
                    #TODO: ??? 
                    action = 0
                #PyArg_Parse(pyPolicy, "IIIIII", &traffic_type, &type, &sdvg, &ddvg, &ttl, &action))
                policy_list.append((policy.traffic_type, policy.type, policy.src_dvg.unique_id,
                                    policy.dst_dvg.unique_id, policy.ttl, action))
        policies_total = len(policy_list)
        policies_processed = 0
        key_index = 0
        while policies_processed < policies_total:
            policy_key = key_fmt%(key_index)
            policy_start = policies_processed
            policy_end = policies_processed + DPSPolicyMassTransfer.MAX_POLICIES_PER_LIST
            if policy_end > policies_total:
                policy_end = policies_total
            policy_sub_list = policy_list[policy_start:policy_end]
            #print 'transfer_start_get_vnids [%s:%s]\r'%(vnid_start,vnid_end)
            query_id = DpsCollection.generate_query_id()
            policy_transfer = DPSPolicyMassTransfer(policy_sub_list, True)
            policy_set[policy_key] = (policy_key, policy_transfer, query_id)
            #print 'transfer_policy: key %s, query_id %s, policies %s\r'%(policy_key, query_id, policy_sub_list)
            policies_processed = policy_end
            key_index += 1
        #print 'transfer_start_get_policies: Exit\r'
        return

    def transfer_start_get_subnets(self):
        '''
        This routine gets the subnets which need to be transferred.
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
        '''
        subnet_set = self.transfer[self.transfer_subnets][0]
        key_fmt = 'subnets-section-%d'
        # PyArg_ParseTuple(pysubnet, "Issss",
        #                  vnid, &ip_string, &mask_string, &gw_string, &mode_string))
        subnet_list = []
        for dvg in self.domain.DVG_Hash.values():
            for subnet in dvg.IP_Subnet_List.PyList.values():
                try:
                    ip_packed = struct.pack('I', subnet.ip_value)
                    ip_string = socket.inet_ntop(socket.AF_INET, ip_packed)
                except Exception:
                    ip_string = '0.0.0.0'
                try:
                    mask_packed = struct.pack('I', subnet.mask_value)
                    mask_string = socket.inet_ntop(socket.AF_INET, mask_packed)
                except Exception:
                    mask_string = '0.0.0.0'
                try:
                    gwy_packed = struct.pack('I', subnet.ip_gateway)
                    gwy_string = socket.inet_ntop(socket.AF_INET, gwy_packed)
                except Exception:
                    gwy_string = '0.0.0.0'
                if subnet.mode == IPSUBNETMode.IP_SUBNET_MODE_SHARED:
                    mode_string = 'shared'
                else:
                    mode_string = 'dedicated'
                subnet_tuple = (dvg.unique_id, ip_string, mask_string, gwy_string, mode_string)
                subnet_list.append(subnet_tuple)
        #Create sub lists if necessary
        subnets_total = len(subnet_list)
        subnets_processed = 0
        key_index = 0
        while subnets_processed < subnets_total:
            subnet_key = key_fmt%(key_index)
            subnet_start = subnets_processed
            subnet_end = subnets_processed + DPSSubnetMassTransfer.MAX_SUBNET_PER_LIST
            if subnet_end > subnets_total:
                subnet_end = subnets_total
            subnet_sub_list = subnet_list[subnet_start:subnet_end]
            #print 'transfer_start_get_vnids [%s:%s]\r'%(vnid_start,vnid_end)
            query_id = DpsCollection.generate_query_id()
            subnet_transfer = DPSSubnetMassTransfer(subnet_sub_list, True)
            subnet_set[subnet_key] = (subnet_key, subnet_transfer, query_id)
            subnets_processed = subnet_end
            key_index += 1
        #print 'transfer_start_get_policies: Exit\r'
        return

    def transfer_start_get_tunnels(self):
        '''
        This routine gets the tunnels which need to be transferred.
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
       '''
        #Get the Tunnel Set
        tunnel_set = self.transfer[self.transfer_tunnels][0]
        for dvg in self.domain.DVG_Hash.values():
            for client_type in dvg.Tunnel_Endpoints_Hash_IPv4.keys():
                for tunnel in dvg.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                    key = '%s:%s:%s'%(dvg.unique_id, client_type, tunnel.primary_ip().ip_value)
                    try:
                        exists = tunnel_set[key]
                        continue
                    except Exception:
                        pass
                    query_id = DpsCollection.generate_query_id()
                    tunnel_transfer = DPSTunnelMassTransfer(client_type, True,
                                                            tunnel.dps_client.location,
                                                            tunnel.ip_tuple_list_get())
                    #print 'transfer_tunnel: key %s, query_id %s, tunnel %s\r'%(key, query_id, tunnel_transfer.show())
                    tunnel_set[key] = (dvg.unique_id, tunnel_transfer, query_id)
            for client_type in dvg.Tunnel_Endpoints_Hash_IPv6.keys():
                for tunnel in dvg.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                    key = '%s:%s:%s'%(dvg.unique_id, client_type, tunnel.primary_ip().ip_value)
                    try:
                        exists = tunnel_set[key]
                        continue
                    except Exception:
                        pass
                    query_id = DpsCollection.generate_query_id()
                    tunnel_transfer = DPSTunnelMassTransfer(client_type, True, 
                                                            tunnel.dps_client.location,
                                                            tunnel.ip_tuple_list_get())
                    #print 'transfer_tunnel: key %s, query_id %s, tunnel %s\r'%(key, query_id, tunnel_transfer.show())
                    tunnel_set[key] = (dvg.unique_id, tunnel_transfer, query_id)
        #print 'Mass Transfer Tunnels Count %s\r'%len(tunnel_set)
        return

    def transfer_start_get_endpoints(self):
        '''
        This routine gets the endpoints which need to be transferred.
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
       '''
        #Get the Endpoint Set
        endpoint_set = self.transfer[self.transfer_endpoints][0]
        for endpoint in self.domain.Endpoint_Hash_MAC.values():
            if endpoint.in_migration:
                print 'Domain %s: Mass Transfer Endpoint [%s] in Migration or Deleted state\r'%(self.domain.unique_id,
                                                                                                endpoint.get_string())
                operation = Endpoint.op_update_delete
            else:
                operation = Endpoint.op_update_add
            vIPs = endpoint.vIP_set.values()
            if len(vIPs) == 0:
                key = '%s:%s:%s'%(endpoint.dvg.unique_id, endpoint, 0)
                query_id = DpsCollection.generate_query_id()
                endpoint_transfer = DPSEndpointMassTransfer(endpoint,
                                                            operation,
                                                            socket.AF_INET,
                                                            0)
                endpoint_set[key] = (endpoint.dvg.unique_id, endpoint_transfer, query_id)
                #print 'transfer_endpoint: key %s, query_id %s, endpoint %s\r'%(key, query_id, endpoint_transfer.show())
            else:
                for vIP in vIPs:
                    key = '%s:%s:%s'%(endpoint.dvg.unique_id, endpoint, vIP.ip_value)
                    query_id = DpsCollection.generate_query_id()
                    endpoint_transfer = DPSEndpointMassTransfer(endpoint,
                                                                operation,
                                                                vIP.inet_type, 
                                                                vIP.ip_value)
                    endpoint_set[key] = (endpoint.dvg.unique_id, endpoint_transfer, query_id)
                    #print 'transfer_endpoint: key %s, query_id %s, endpoint %s\r'%(key, query_id, endpoint_transfer.show())
        #print 'Mass Transfer Endpoints Count %s\r'%len(endpoint_set)
        return

    def transfer_start_get_multicast_vnid_group(self, mac, vnid, mcast_vnid_group):
        '''
        This routine gets all the Multicast registration for a specific
        Multicast VNID group
        @param mac: The MAC Address of the Multicast Registration
        @type mac: ByteArray
        @param vnid: The VNID associated with the Multicast Registration
        @type vnid: Integer
        @param mcast_vnid_group: The Multicast VNID Group
        @type mcast_vnid_group: Multicast_VNID_Group
        '''
        multicast_set = self.transfer[self.transfer_multicasts][0]
        #All Receivers
        for tunnel in mcast_vnid_group.receiver_all.keys():
            tunnel_ip = tunnel.primary_ip()
            for client_type in tunnel.tunnel_types_supported():
                key = '%s:%s:%s:%s:%s'%(vnid, client_type, mac, 0, tunnel_ip.ip_value)
                query_id = DpsCollection.generate_query_id()
                multicast_transfer = DPSMulticastMassTransfer(self.domain.unique_id,
                                                              vnid,
                                                              False,
                                                              True,
                                                              client_type,
                                                              mcast_vnid_group.global_scope,
                                                              mac,
                                                              0,
                                                              DpsCollection.Invalid_IP_Packed,
                                                              tunnel_ip.ip_value_packed)
                multicast_set[key] = (vnid, multicast_transfer, query_id)
        #Specific Receivers
        for ip_value in mcast_vnid_group.receiver_iphash.keys():
            try:
                ip_hash = mcast_vnid_group.receiver_iphash[ip_value]
            except Exception:
                continue
            if type(ip_value) == type(1):
                inet_type = socket.AF_INET
            else:
                inet_type = socket.AF_INET6
            ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],
                                    ip_value)
            for tunnel in ip_hash.keys():
                tunnel_ip = tunnel.primary_ip()
                for client_type in tunnel.tunnel_types_supported():
                    key = '%s:%s:%s:%s:%s'%(vnid, client_type, mac, ip_value, tunnel_ip.ip_value)
                    query_id = DpsCollection.generate_query_id()
                    multicast_transfer = DPSMulticastMassTransfer(self.domain.unique_id,
                                                                  vnid,
                                                                  False,
                                                                  True,
                                                                  client_type,
                                                                  mcast_vnid_group.global_scope,
                                                                  mac,
                                                                  inet_type,
                                                                  ip_packed,
                                                                  tunnel_ip.ip_value_packed)
                    multicast_set[key] = (vnid, multicast_transfer, query_id)
        #All Senders
        for tunnel in mcast_vnid_group.sender_all.keys():
            tunnel_ip = tunnel.primary_ip()
            for client_type in tunnel.tunnel_types_supported():
                key = '%s:%s:%s:%s:%s'%(vnid, client_type, mac, 0, tunnel_ip.ip_value)
                query_id = DpsCollection.generate_query_id()
                multicast_transfer = DPSMulticastMassTransfer(self.domain.unique_id,
                                                              vnid,
                                                              True,
                                                              True,
                                                              client_type,
                                                              mcast_vnid_group.global_scope,
                                                              mac,
                                                              0,
                                                              DpsCollection.Invalid_IP_Packed,
                                                              tunnel_ip.ip_value_packed)
                multicast_set[key] = (vnid, multicast_transfer, query_id)
        #Specific Senders
        for ip_value in mcast_vnid_group.sender_iphash.keys():
            try:
                ip_hash = mcast_vnid_group.sender_iphash[ip_value]
            except Exception:
                continue
            if type(ip_value) == type(1):
                inet_type = socket.AF_INET
            else:
                inet_type = socket.AF_INET6
            ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],
                                    ip_value)
            for tunnel in ip_hash.keys():
                tunnel_ip = tunnel.primary_ip()
                for client_type in tunnel.tunnel_types_supported():
                    key = '%s:%s:%s:%s:%s'%(vnid, client_type, mac, ip_value, tunnel_ip.ip_value)
                    query_id = DpsCollection.generate_query_id()
                    multicast_transfer = DPSMulticastMassTransfer(self.domain.unique_id,
                                                                  vnid,
                                                                  True,
                                                                  True,
                                                                  client_type,
                                                                  mcast_vnid_group.global_scope,
                                                                  mac,
                                                                  inet_type,
                                                                  ip_packed,
                                                                  tunnel_ip.ip_value_packed)
                    multicast_set[key] = (vnid, multicast_transfer, query_id)
        return

    def transfer_start_get_multicast_group(self, mcast_group):
        '''
        This routine gets all the Multicast registration for a specific
        Multicast group
        @param mcast_group: The Multicast Group
        @type mcast_group: Multicast_Group
        '''
        for vnid in mcast_group.VNID_Hash.keys():
            try:
                mcast_vnid_group = mcast_group.VNID_Hash[vnid]
            except Exception:
                continue
            self.transfer_start_get_multicast_vnid_group(mcast_group.mac, vnid, mcast_vnid_group)
        return

    def transfer_start_get_multicasts(self):
        '''
        This routine gets all the Multicast registrations in the Domain.
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
        '''
        mcast = self.domain.Multicast
        self.transfer_start_get_multicast_group(mcast.Global_Scope)
        self.transfer_start_get_multicast_group(mcast.All)
        for mcast_group in mcast.MAC_Group.values():
            self.transfer_start_get_multicast_group(mcast_group)
        return

    def show_object_set(self, obj_string, obj_set):
        '''
        '''
        print 'show_object_set: Enter [%s]\r'%obj_string
        for key, obj_tuple in obj_set.items():
            query_id = obj_tuple[2]
            obj = obj_tuple[1]
            print 'New Object %s, Query Id %s, Obj %s\r'%(obj_string, query_id, obj.show())
        print 'show_object_set: Exit [%s]\r'%obj_string
        return

    def show_object_set_unacked(self, obj_string, obj_set):
        '''
        '''
        print 'show_object_set_unacked: Enter [%s]\r'%obj_string
        for query_id, obj_tuple in obj_set.items():
            obj = obj_tuple[1]
            print 'Unacked Object %s, Query Id %s, Obj %s\r'%(obj_string, query_id, obj.show())
        print 'show_object_set_unacked: Exit [%s]\r'%obj_string
        return

    def transfer_objects(self):
        '''
        This routine transfer objects
        @attention: DO NOT IMPORT from other PYTHON modules
        @attention: This routine assumes that the calling routine has the 
                    object lock held.
        @return: If the transfer is complete
        @rtype: Success
        '''
        #print 'transfer_objects: Enter\r'
        fFinished = False
        while True:
            object_string = self.transfer_string[self.transfer_stage]
            try:
                set_tuple = self.transfer[self.transfer_stage]
                obj_set = set_tuple[0]
                obj_set_unacked = set_tuple[1]
                unacked_max = set_tuple[2]
                time_sleep = 1/unacked_max
                #self.show_object_set(object_string, obj_set)
                #self.show_object_set_unacked(object_string, obj_set_unacked)
            except Exception:
                fFinished = True
                break
            #Figure out the maximum objects to transfer
            max_new_objects = unacked_max - len(obj_set_unacked)
            max_new_objects = min(max_new_objects, len(obj_set))
            if self.domain.unique_id == DpsCollection.Shared_DomainID:
                domain_name = '[Shared]'
            else:
                domain_name = self.domain.unique_id
            message = 'Domain %s: Mass Transfer Stage %s: New Objects %s, Unacked %s'%(domain_name,
                                                                                       object_string,
                                                                                       max_new_objects,
                                                                                       len(obj_set_unacked))
            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
            if max_new_objects < 0:
                message = 'DPSDomainMassTransfer: Accounting issue unacked_max %s, len(obj_set_unacked) %s\r'%(unacked_max, len(obj_set_unacked))
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            else:
                for i in range(max_new_objects):
                    try:
                        key, obj_tuple = obj_set.popitem()
                        #Get a query id
                        query_id = obj_tuple[2]
                        #print 'obj_set: Popped new item query id %s\r'%(query_id, obj_tuple[1].show())
                        obj_set_unacked[query_id] = (obj_tuple[0], obj_tuple[1], 0)
                        #print 'Adding obj_set_unacked[%s]: %s\r'%(query_id, obj_tuple[1].show())
                    except Exception, ex:
                        #print 'Exception in popping %s\r'%ex
                        break
            if len(obj_set_unacked) == 0:
                #Go to next transfer stage
                self.transfer_stage += 1
                continue
            #Transfer this stage
            #print'Transferring %s %s unacked Registrations for Domain %d to DPS %s\r'%(len(obj_set_unacked), 
            #                                                                           self.transfer_string[self.transfer_stage],
            #                                                                           self.domain.unique_id, 
            #                                                                           self.remote_location.show())
            query_ids = obj_set_unacked.keys()
            query_ids.sort()
            for query_id in query_ids:
                try:
                    obj_tuple = obj_set_unacked[query_id]
                    obj_id = obj_tuple[0]
                    obj = obj_tuple[1]
                    tries = obj_tuple[2]
                    if tries > self.MAX_RETRIES_PER_OBJECT:
                        #Reached max number of retries per object
                        self.valid = False
                        raise Exception('Reached %s tries'%(self.MAX_RETRIES_PER_OBJECT))
                    obj_set_unacked[query_id] = (obj_id, obj, tries+1)
                    #print 'Transferring Unacked query_id %s, obj_id %s, obj %s\r'%(query_id, obj_id, obj.show())
                    self.transfer_routine[self.transfer_stage](query_id, obj_id, obj)
                    #Add a little delay between each transfer
                    time.sleep(time_sleep)
                except Exception, ex:
                    message = 'Domain %s: Mass Transfer Objects stage %s Exception [%s]'%(domain_name,
                                                                                          self.stage_get(),
                                                                                          ex)
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                    try:
                        del obj_set_unacked[query_id]
                    except Exception:
                        pass
                    break
            if not self.valid:
                break
            if len(obj_set_unacked) == 0:
                #Go to next transfer stage
                self.transfer_stage += 1
                continue
            break
        #print 'transfer_objects: Exit\r'
        return fFinished

    def registration_allow(self):
        '''
        This routine determines if newer registrations show be allowed while
        mass transfer is taking place.
        @attention: DO NOT IMPORT from other PYTHON modules
        '''
        #Currently we allow registrations if there are objects in the main queue
        allow = True
        #if self.remaining_registration_items() < self.remaining_registration_retry:
        #    allow = False
        while True:
            if not self.domain.valid:
                allow = False
                break
            if ((len(self.transfer[self.transfer_tunnels][0]) == 0) and 
                (len(self.transfer[self.transfer_endpoints][0]) == 0) and 
                (len(self.transfer[self.transfer_multicasts][0]) == 0)):
                allow = False
            break
        return allow

    def register_delta(self, transfer_type, obj_key, obj_tuple):
        '''
        This routine should be called when an object is registered while mass
        transfer is going on in the domain.
        @attention: MUST NOT BE INVOKED from other PYTHON modules
        @attention: The global lock must be held when this routine is called
        @param transfer_type: Should be a key in self.transfer_valid
        @type transfer_type: Integer
        @param obj_key : The Key with which to insert into the transfer set
        @type obj_key: String
        @param obj_tuple: (key, obj_transfer, query_id)
        @type obj_tuple: Tuple
        @return: The status of the operation
        @rtype: DOVEStatus
        '''
        status = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            if not self.registration_allow():
                status = DOVEStatus.DOVE_STATUS_RETRY
                break
            try:
                set_tuple = self.transfer[transfer_type]
                obj_set = set_tuple[0]
                obj_set[obj_key] = obj_tuple
            except Exception:
                status = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
                break
            #Move stage index back if necessary
            try:
                if transfer_type < self.transfer_stage:
                    message = 'register_delta: going back from stage %s to stage %s'%(self.transfer_string[self.transfer_stage],
                                                                                      self.transfer_string[transfer_type])
                    dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                    self.transfer_stage = transfer_type
            except Exception:
                pass
            #Use this thread to send out more requests
            #self.transfer_objects()
            break
        self.lock.release()
        return status

    def transfer_thread(self):
        '''
        This routine is the thread which check for un-acked messages and re-transmits
        them.
        @attention: DO NOT IMPORT from other PYTHON modules
        '''
        #Check if self is valid
        if not self.valid:
            message = 'Domain %s: Problem with Mass Transfer quitting, Stage %s'%(self.domain.unique_id,
                                                                                  self.stage_get())
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            self.finish_callback(self.remote_location, False, self.weight)
            self.delete()
            return
        #print 'Mass Transfer Thread for Domain %s active\r'%self.domain.unique_id
        if not self.domain.valid:
            message = 'Domain %s: Deleted, invoking transfer complete, Stage %s\r'%(self.domain.unique_id,
                                                                                    self.stage_get())
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            self.finish_callback(self.remote_location, False, self.weight)
            self.delete()
            return
        #Check if remote node is still up
        if not self.cluster_db.cluster.node_is_up(self.remote_location.ip_value):
            message = 'Domain %s: Invoking transfer complete callback for Node %s Down, Stage %s\r'%(self.domain.unique_id,
                                                                                                     self.remote_location.show_ip(),
                                                                                                     self.stage_get())
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            self.finish_callback(self.remote_location, False, self.weight)
            self.delete()
            return
        self.lock.acquire()
        try:
            fInvokeCallback = False
            fFinished = self.transfer_objects()
            if fFinished and not self.invoked_callback:
                self.invoked_callback = True
                fInvokeCallback = True
        except Exception:
            fFinished = False
            pass
        self.lock.release()
        #Invoke next timer thread if there are un-acked objects
        if not fFinished:
            timer = Timer(self.TIMER_SLEEP, self.transfer_thread)
            timer.start()
        #Invoke callback if needed
        elif fInvokeCallback:
            #print 'Invoking transfer complete callback for Domain %s\r'%self.domain.unique_id
            self.finish_callback(self.remote_location, True, self.weight)
        #print 'Mass Transfer Thread for Domain %s finish\r'%self.domain.unique_id
        return

    def transfer_start(self):
        '''
        This routine starts the transfer for this domain. 
        @attention: CAN BE INVOKED from other PYTHON modules
        @attention: This routine assumes that the global database lock is held
                    so that the domain objects don't change during this routine.
        '''
        #Get the VNID Set
        self.transfer_start_get_vnids()
        #Get the Policy Set
        self.transfer_start_get_policies()
        #Get the Subnet Set
        self.transfer_start_get_subnets()
        #Get the Tunnel Set
        self.transfer_start_get_tunnels()
        #Get the Endpoint Set
        self.transfer_start_get_endpoints()
        #Get the Multicast Set
        self.transfer_start_get_multicasts()
        #Start the Mass Transfer Thread
        self.transfer_stage = self.transfer_vnids
        #self.show()
        timer = Timer(0, self.transfer_thread)
        timer.start()
        return

    def transfer_ack(self, query_id, status):
        '''
        This routine should be called when a mass transfer operation is ACK(ed) by the remote end
        @attention: CAN BE INVOKED from other PYTHON modules
        @attention: NO Lock must be held when this routine is called
        @param query_id: The query_id that was completed
        @type query_id: Integer
        @param status: The status of the transfer
        @type status: Integer
        '''
        if status == DpsClientHandler.dps_error_retry:
            return
        self.lock.acquire()
        tunnel_unacked = self.transfer[self.transfer_tunnels][1]
        endpoint_unacked = self.transfer[self.transfer_endpoints][1]
        multicast_unacked = self.transfer[self.transfer_multicasts][1]
        try:
            del tunnel_unacked[query_id]
            #print 'transfer_ack: Tunnel ACK %s\r'%query_id
        except Exception:
            pass
        try:
            del endpoint_unacked[query_id]
            #print 'transfer_ack: Endpoint ACK %s\r'%query_id
        except Exception:
            pass
        try:
            del multicast_unacked[query_id]
            #print 'transfer_ack: Multicast ACK %s\r'%query_id
        except Exception:
            pass
        #while True:
        #try:
        #    obj_unacked = self.transfer[self.transfer_stage][1]
        #    if (len(obj_unacked)) == 0:
        #        #Transfer remaining items if the we are at half point
        #        self.transfer_objects()
        #except Exception:
        #    #log.warning('DPSDomainMassTransfer.transfer_ack found stage to be %s\n\r. Check code\n\r',
        #    #            self.transfer_stage)
        #    pass
        #break
        self.lock.release()
        return

    def register_tunnel(self, vnid, client_type, fregister, host_location, pip_tuple_list):
        '''
        This routine registers tunnels during mass transfer
        @attention: CAN BE INVOKED from other PYTHON modules
        @attention: The global lock must be held when this routine is called
        @param vnid: The VNID associated with the Update
        @type vnid: Integer
        @param client_type: The Tunnel Type (Switch or Gateway)
        @type client_type: DpsClientType
        @param fregister: Whether it's a register or de-register
        @type fregister: Boolean
        @param host_location: The location of Host(DPS Client) hosting this tunnel
        @type host_location: IPAddressLocation
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        '''
        if len(pip_tuple_list) == 0:
            return DOVEStatus.DOVE_STATUS_OK
        #print 'Mass Transfer: register_tunnel Enter %s\r'%(pip_tuple_list)
        query_id = DpsCollection.generate_query_id()
        tunnel_transfer = DPSTunnelMassTransfer(client_type, fregister, host_location, pip_tuple_list)
        pip_value = pip_tuple_list[0][1]
        #Must match with transfer_start_get_tunnels
        tunnel_key = '%s:%s:%s:%s'%(vnid, client_type, pip_value, fregister)
        #tunnel_key = '%s'%query_id
        tunnel_tuple = (vnid, tunnel_transfer, query_id)
        #print 'Mass Transfer: register_tunnel key %s, query_id %s, tunnel %s\r'%(tunnel_key, query_id, tunnel_transfer.show())
        status = self.register_delta(self.transfer_tunnels, tunnel_key, tunnel_tuple)
        #print 'Mass Transfer: register_tunnel Exit\r'
        return status

    def register_endpoint(self, vnid, endpoint, operation, vIP_type, vIP_val):
        '''
        This routine registers endpoints during mass transfer
        @attention: CAN BE INVOKED from other PYTHON modules
        @attention: The global lock must be held when this routine is called
        @param vnid: The VNID associated with the Update
        @type vnid: Integer
        @param endpoint: The Endpoint Object
        @type endpoint: Endpoint
        @param operation: The type of Endpoint operation
        @type operation: Integer
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_val: The IP address of the Endpoint
        @type vIP_val: Integer or String
        '''
        #print 'Mass Transfer: register_endpoint Enter\r'
        endpoint_transfer = DPSEndpointMassTransfer(endpoint, operation, vIP_type, vIP_val)
        query_id = DpsCollection.generate_query_id()
        #Must match with transfer_start_get_endpoints
        endpoint_key = '%s:%s:%s'%(endpoint.dvg.unique_id, endpoint, vIP_val)
        #endpoint_key = '%s'%query_id
        endpoint_tuple = (vnid, endpoint_transfer, query_id)
        #print 'Mass Transfer: register_endpoint key %s, query_id %s, endpoint %s\r'%(endpoint_key, query_id, endpoint_transfer.show())
        status = self.register_delta(self.transfer_endpoints, endpoint_key, endpoint_tuple)
        #print 'Mass Transfer: register_endpoint Exit\r'
        return status

    def register_multicast(self, domain_id, vnid, fsender, fregister, client_type, global_scope,
                           multicast_mac, multicast_ip_family, multicast_ip_packed,
                           tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine registers endpoints during mass transfer
        @attention: CAN BE INVOKED from other PYTHON modules
        @attention: The global lock must be held when this routine is called
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param fsender: Whether this is Sender or Receiver
        @type fsender: Boolean
        @param fregister: Whether this is registration or un-register
        @type fregister: Boolean
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        multicast_transfer = DPSMulticastMassTransfer(domain_id, vnid, fsender, fregister, client_type,
                                                      multicast_mac, multicast_ip_family, multicast_ip_packed,
                                                      tunnel_ip_packed)
        try:
            tunnel_IP_val = self.ip_get_val_from_packed[tunnel_ip_family](tunnel_ip_packed)
        except Exception:
            tunnel_IP_val = 0
        try:
            multicast_IP_val = self.ip_get_val_from_packed[multicast_ip_family](multicast_ip_packed)
        except Exception:
            multicast_IP_val = 0
        query_id = DpsCollection.generate_query_id()
        #Must match with transfer_start_get_multicasts
        #multicast_key = '%s:%s:%s:%s:%s'%(vnid, client_type, multicast_mac, multicast_IP_val, tunnel_IP_val)
        multicast_key = '%s'%query_id
        multicast_tuple = (vnid, multicast_transfer, query_id)
        status = self.register_delta(self.transfer_multicasts, multicast_key, multicast_tuple)
        return status

    def register_vnid(self, vnid, fadd):
        '''
        This routine registers VNID during mass transfer
        @param vnid: The VNID
        @type vnid: vnid
        @param fadd: Whether it's an add or delete
        @type fadd: Boolean
        '''
        #print 'Mass Transfer: register_vnid Enter\r'
        vnid_transfer = DPSVNIDMassTransfer([vnid], fadd)
        query_id = DpsCollection.generate_query_id()
        #vnid_key = 'vnid-%d:'%vnid
        vnid_key = '%s'%query_id
        #Must match with transfer_set_get_policies
        vnid_tuple = (vnid_key, vnid_transfer, query_id)
        status = self.register_delta(self.transfer_vnids, vnid_key, vnid_tuple)
        #print 'Mass Transfer: register_vnid Exit\r'
        return status

    def register_policy(self, policy, fadd):
        '''
        This routine registers policies during mass transfer
        @param policy: The Policy Object
        @type policy: Policy
        @param fadd: Whether it's an add or delete
        @type fadd: Boolean
        '''
        if policy.type == Policy.type_connectivity:
            action = policy.action_connectivity
        else:
            #TODO: ??? 
            action = 0
        #PyArg_Parse(pyPolicy, "IIIIII", &traffic_type, &type, &sdvg, &ddvg, &ttl, &action))
        policy_tuple = (policy.traffic_type, policy.type, policy.src_dvg.unique_id,
                        policy.dst_dvg.unique_id, policy.ttl, action)
        policy_transfer = DPSPolicyMassTransfer([policy_tuple], fadd)
        query_id = DpsCollection.generate_query_id()
        key_fmt = '%d:'+Policy.key_fmt
        #Must match with transfer_set_get_policies
        policy_key = key_fmt%(policy.traffic_type, policy.src_dvg.unique_id, policy.dst_dvg.unique_id)
        #policy_key = '%s'%query_id
        policy_tuple = (policy_key, policy_transfer, query_id)
        status = self.register_delta(self.transfer_policies, policy_key, policy_tuple)
        return status

    def register_subnet(self, fadd, associated_type, associated_id, ip_value, mask_value, ip_gateway, mode):
        '''
        This routine registers policies during mass transfer
        @param fadd: Whether it's an add or delete
        @type fadd: Boolean
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @param ip_gateway: The Gateway associated with the subnet
        @type ip_gateway: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        '''
        try:
            ip_packed = struct.pack('I', ip_value)
            ip_string = socket.inet_ntop(socket.AF_INET, ip_packed)
        except Exception:
            ip_string = '0.0.0.0'
        try:
            mask_packed = struct.pack('I', mask_value)
            mask_string = socket.inet_ntop(socket.AF_INET, mask_packed)
        except Exception:
            mask_string = '0.0.0.0'
        try:
            gwy_packed = struct.pack('I', ip_gateway)
            gwy_string = socket.inet_ntop(socket.AF_INET, gwy_packed)
        except Exception:
            gwy_string = '0.0.0.0'
        if mode == IPSUBNETMode.IP_SUBNET_MODE_SHARED:
            mode_string = 'shared'
        else:
            mode_string = 'dedicated'
        if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
            vnid = 0
        else:
            vnid = associated_id
        subnet_tuple = (vnid, ip_string, mask_string, gwy_string, mode_string)
        subnet_transfer = DPSSubnetMassTransfer(fadd, [subnet_tuple])
        query_id = DpsCollection.generate_query_id()
        #Must match with transfer_set_get_subnets
        subnet_key = '%s:%s:%s'%(associated_type, associated_id, ip_value)
        #subnet_key = '%s'%query_id
        subnet_register_tuple = (vnid, subnet_transfer, query_id)
        return self.register_delta(self.transfer_subnets, subnet_key, subnet_register_tuple)

    def stage_get(self):
        '''
        Returns the current mass transfer stage
        '''
        try:
            return self.transfer_string[self.transfer_stage]
        except Exception:
            return 'Error: Unknown Stage'

    def delete(self):
        for i in range(1,self.transfer_finished):
            try:
                transfers = self.transfer[i]
                set1 = transfers[0]
                set1.clear()
                set2 = transfers[1]
                set2.clear()
            except Exception:
                pass
        return

    def show(self):
        '''
        Show contents
        @attention: CAN BE INVOKED from other PYTHON modules
        '''
        print '------------------------ Tunnels ----------------------\r'
        set_tuple = self.transfer[self.transfer_tunnels]
        obj_set = set_tuple[0]
        obj_set_unacked = set_tuple[1]
        print 'To Be Transferred %s\r'%len(obj_set)
        for vnid_obj in obj_set.values():
            vnid = vnid_obj[0]
            tunnel_transfer = vnid_obj[1]
            for pip_tuple in tunnel_transfer.pip_tuple_list:
                ip_location = IPAddressLocation(pip_tuple[0], pip_tuple[1], 0)
                print '    VNID %s: Tunnel %s\r'%(vnid, ip_location.show())
        print 'To Be ACKED %s\r'%len(obj_set_unacked)
        for vnid_obj in obj_set_unacked.values():
            vnid = vnid_obj[0]
            tunnel_transfer = vnid_obj[1]
            for pip_tuple in tunnel_transfer.pip_tuple_list:
                ip_location = IPAddressLocation(pip_tuple[0], pip_tuple[1], 0)
                print '    VNID %s: Tunnel %s\r'%(vnid, ip_location.show())
        print '------------------------- Endpoints -------------------\r'
        set_tuple = self.transfer[self.transfer_endpoints]
        obj_set = set_tuple[0]
        obj_set_unacked = set_tuple[1]
        print 'To Be Transferred %s\r'%len(obj_set)
        for vnid_obj in obj_set.values():
            endpoint_vIP = vnid_obj[1].vIP
            print '    %s\r'%endpoint_vIP.show()
        print 'To Be ACKED %s\r'%len(obj_set_unacked)
        for vnid_obj in obj_set_unacked.values():
            endpoint_vIP = vnid_obj[1].vIP
            print '    %s\r'%endpoint_vIP.show()
        return

class DPSMassTransfer:
    '''
    This is the global class which handles mass transfer. The C code should
    invoke routines from this Class.
    '''

    def __init__(self):
        '''
        Initialization Routine
        '''
        #Mapping of Query IDs to DPSDomainMassTransfer Objects
        self.QueryID_Mapping = DpsCollection.MassTransfer_QueryID_Mapping

    def Transfer_Ack(self, query_id, status):
        '''
        Handle transfer ACK
        @attention: This routine MUST be called from C code only
        @param query_id: The Query ID associated with the message
        @type query_id: Integer
        @param status: The status of the transfer
        @type status: Integer
        '''
        while True:
            DpsCollection.mass_transfer_lock.acquire()
            try:
                domain_transfer = self.QueryID_Mapping[query_id]
            except Exception:
                break
            DpsCollection.mass_transfer_lock.release()
            domain_transfer.transfer_ack(query_id, status)
            break
        return
