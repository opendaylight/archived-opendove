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

import socket

from object_collection import DpsCollection
from object_collection import DpsClientType
from object_collection import DpsTransactionType
from object_collection import DOVEGatewayTypes
from dcs_objects import dcs_object
from dcs_objects.Domain import Domain
from object_collection import DOVEStatus
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.IPAddressList import IPAddressList
from dcs_objects.IPSubnetList import IPSubnetList
from object_collection import DpsLogLevels

import dcslib

class DVG(dcs_object):
    '''
    This represents the DVG/VNID Object in within a domain
    '''

    def __init__(self, domain, dvg_id):
        '''
        Constructor:
        @param domain: The Domain Object
        @type domain: Domain
        @param dvg_id: The VNID/DVG ID
        @type dvg_id: Integer
        '''
        if dvg_id < 0 or dvg_id > 16777215:
            raise DOVEStatus.DOVE_STATUS_INVALID_DVG
        self.domain = domain
        #Endpoint Object Hash Table (IP Address)
        #Separating IPv4 and IPv6 into different Hashes since one
        #will be hashed by integer, the other string
        self.Endpoint_Hash_IPv4 = {}
        self.Endpoint_Hash_IPv6 = {}
        #Endpoint Object Hash Table (MAC Address)
        self.Endpoint_Hash_MAC = {}
        #This is to store the Tunnel Endpoint IP Address
        self.Tunnel_Endpoints_Hash_IPv4 = {}
        for client_type in DpsClientType.types.keys():
            self.Tunnel_Endpoints_Hash_IPv4[client_type] = {}
        self.Tunnel_Endpoints_Hash_IPv6 = {}
        for client_type in DpsClientType.types.keys():
            self.Tunnel_Endpoints_Hash_IPv6[client_type] = {}
        #List of External Gateway IP Addresses for this Domain
        self.ExternalGatewayIPListv4 = IPAddressList(socket.AF_INET)
        self.ExternalGatewayIPListv6 = IPAddressList(socket.AF_INET6)
        #List of Implicit Gateway IP Address for this DVG/VNID
        self.ImplicitGatewayIPListv4 = IPAddressList(socket.AF_INET)
        self.ImplicitGatewayIPListv6 = IPAddressList(socket.AF_INET6)
        #Policies related to this DVG/VNID where this is the Source VNID
        #The first index is for unicast policy, the second for multicast
        self.Policy_Source = [{}, {}]
        #Policies related to this DVG/VNID where this is the Destination VNID
        #The first index is for unicast policy, the second for multicast
        self.Policy_Destination = [{}, {}]
        #Initialize Subnet List to the Domain's IP Subnet List 
        #self.IP_Subnet_List = domain.IP_Subnet_List
        self.IP_Subnet_List = IPSubnetList()
        #############################################################
        dcs_object.__init__(self, dvg_id)
        Domain.dvg_add(domain, self)
        #Add to VNID Collection
        DpsCollection.VNID_Hash[dvg_id] = domain.unique_id

    def endpoint_add(self, endpoint):
        '''
        Adds an Endpoint to the DVG/VNID Collection
        @param endpoint: Endpoint Object
        @type endpoint: Endpoint
        '''
        self.Endpoint_Hash_MAC[endpoint.vMac] = endpoint
        for vIP_key in endpoint.vIP_set.keys():
            try:
                self.endpoint_vIP_add(endpoint, endpoint.vIP_set[vIP_key])
            except Exception:
                pass
        return

    def endpoint_del(self, endpoint):
        '''
        Removes an Endpoint from the DVG/VNID Collection
        @param endpoint: Endpoint Object
        @type endpoint: Endpoint
        '''
        try:
            del self.Endpoint_Hash_MAC[endpoint.vMac]
        except Exception:
            pass
        for vIP_key in endpoint.vIP_set.keys():
            try:
                self.endpoint_vIP_del(endpoint.vIP_set[vIP_key])
            except Exception:
                pass
        return

    def endpoint_vIP_add(self, endpoint, vIP):
        '''
        Adds an Endpoint (Virtual) IP Address to the DVG/VNID collection
        @param endpoint: Endpoint Object
        @type endpoint: Endpoint
        @param vIP: IP Address
        @type vIP: IPAddressLocation
        '''
        if vIP.inet_type == socket.AF_INET:
            ip_hash = self.Endpoint_Hash_IPv4
        else:
            ip_hash = self.Endpoint_Hash_IPv6
        ip_hash[vIP.ip_value] = endpoint
        return

    def endpoint_vIP_del(self, vIP):
        '''
        Deletes an Endpoint (Virtual) IP Address from the DVG/VNID collection
        @param vIP: IP Address
        @type vIP: IPAddressLocation
        '''
        if vIP.inet_type == socket.AF_INET:
            ip_hash = self.Endpoint_Hash_IPv4
        else:
            ip_hash = self.Endpoint_Hash_IPv6
        try:
            del ip_hash[vIP.ip_value]
        except Exception:
            pass
        return

    def tunnel_endpoint_add(self, fregister, tunnel_endpoint, client_type, transaction_type):
        '''
        Adds a Tunnel Endpoint to the DVG/VNID Collection
        @param fregister: Whether this is an explicit tunnel register
        @param fregister: Boolean
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        '''
        while True:
            try:
                client_type_value = DpsClientType.types[client_type]
            except Exception:
                break
            new_tunnel = fregister
            for ip_value in tunnel_endpoint.ip_listv4.ip_list:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv4[client_type]
                try:
                    tunnel = ip_hash[ip_value]
                    if tunnel != tunnel_endpoint:
                        IP = IPAddressLocation(socket.AF_INET, ip_value, 0)
                        message = 'DVG.tunnel_endpoint_add. Tunnel IP %s migrated without informing DCS Server'%IP.show()
                        dcslib.dps_data_write_log(DpsLogLevels.WARNING, message)
                        ip_hash[ip_value] = tunnel_endpoint
                        new_tunnel = True
                except Exception:
                    ip_hash[ip_value] = tunnel_endpoint
                    new_tunnel = True
            for ip_value in tunnel_endpoint.ip_listv6.ip_list:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv6[client_type]
                try:
                    tunnel = ip_hash[ip_value]
                    if tunnel != tunnel_endpoint:
                        IP = IPAddressLocation(socket.AF_INET6, ip_value, 0)
                        message = 'DVG.tunnel_endpoint_add. Tunnel IP %s migrated without informing DCS Server'%IP.show()
                        dcslib.dps_data_write_log(DpsLogLevels.WARNING, message)
                        ip_hash[ip_value] = tunnel_endpoint
                        new_tunnel = True
                except Exception:
                    ip_hash[ip_value] = tunnel_endpoint
                    new_tunnel = True
            if (new_tunnel and (transaction_type == DpsTransactionType.normal) and 
                self.domain.active and (self.unique_id != DpsCollection.Shared_VNID)):
                DpsCollection.VNID_Broadcast_Updates[self.unique_id] = self
            if new_tunnel and self.domain.active and self.unique_id != DpsCollection.Shared_VNID:
                if client_type == DpsClientType.vlan_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_VLAN))
                if client_type == DpsClientType.external_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            break
        return

    def tunnel_endpoint_delete(self, tunnel_endpoint, client_type):
        '''
        Deletes a Tunnel Endpoint from the DVG/VNID Collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        '''
        while True:
            try:
                client_type_value = DpsClientType.types[client_type]
            except Exception:
                break
            tunnel_deleted = False
            for ip_value in tunnel_endpoint.ip_listv4.ip_list:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv4[client_type]
                try:
                    del ip_hash[ip_value]
                    tunnel_deleted = True
                except Exception:
                    pass
                if client_type == DpsClientType.external_gateway:
                    try:
                        self.ExternalGatewayIPListv4.remove(socket.AF_INET, ip_value)
                    except Exception:
                        pass
            for ip_value in tunnel_endpoint.ip_listv6.ip_list:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv4[client_type]
                try:
                    del ip_hash[ip_value]
                    tunnel_deleted = True
                except Exception:
                    pass
                if client_type == DpsClientType.external_gateway:
                    try:
                        self.ExternalGatewayIPListv6.remove(socket.AF_INET6, ip_value)
                    except Exception:
                        pass
            if tunnel_deleted and self.domain.active and self.unique_id != DpsCollection.Shared_VNID:
                DpsCollection.VNID_Broadcast_Updates[self.unique_id] = self
                self.domain.Multicast.tunnel_delete(self, tunnel_endpoint)
            if tunnel_deleted and self.domain.active and self.unique_id != DpsCollection.Shared_VNID:
                if client_type == DpsClientType.vlan_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_VLAN))
                elif client_type == DpsClientType.external_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            break
        return

    def tunnel_endpoint_add_IP(self, fregister, tunnel_endpoint, client_type, 
                               inet_type, ip_value, transaction_type):
        '''
        Adds a Tunnel Endpoint Physical IP Address to the Domain Collection
        @param fregister: Whether this is an explicit tunnel register
        @param fregister: Boolean
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        '''
        #TODO: Should we add all IPs to this hash or only the primary IP
        while True:
            mass_transfer_needed = False
            try:
                client_type_value = DpsClientType.types[client_type]
            except Exception:
                break
            if inet_type == socket.AF_INET:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv4[client_type]
            else:
                ip_hash = self.Tunnel_Endpoints_Hash_IPv6[client_type]
            new_tunnel = fregister
            try:
                tunnel = ip_hash[ip_value]
                if tunnel != tunnel_endpoint:
                    IP = IPAddressLocation(socket.AF_INET, ip_value, 0)
                    message = 'tunnel_endpoint_add_IP. Tunnel IP %s migrated without informing DCS Server'%(IP.show_ip())
                    dcslib.dps_data_write_log(DpsLogLevels.NOTICE, message)
                    ip_hash[ip_value] = tunnel_endpoint
                    new_tunnel = True
            except Exception:
                ip_hash[ip_value] = tunnel_endpoint
                mass_transfer_needed = True
                new_tunnel = True
            if (new_tunnel and (transaction_type == DpsTransactionType.normal) and 
                self.domain.active and (self.unique_id != DpsCollection.Shared_VNID)):
                DpsCollection.VNID_Broadcast_Updates[self.unique_id] = self
            if new_tunnel and self.domain.active and (self.unique_id != DpsCollection.Shared_VNID):
                if client_type == DpsClientType.vlan_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_VLAN))
                elif client_type == DpsClientType.external_gateway:
                    DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            if mass_transfer_needed:
                if self.domain.mass_transfer is not None:
                    try:
                        self.domain.mass_transfer.register_tunnel(self.unique_id,
                                                                  client_type, 
                                                                  True, 
                                                                  tunnel_endpoint.dps_client.location, 
                                                                  [(inet_type, ip_value)])
                    except Exception:
                        pass
            break
        return

    def tunnel_endpoint_delete_IP(self, client_type, inet_type, ip_value):
        '''
        Deletes a Tunnel Endpoint Physical IP Address from the Domain Collection.
        The Tunnel Endpoint should have a primary physical
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        '''
        while True:
            try:
                if inet_type == socket.AF_INET:
                    ip_hash = self.Tunnel_Endpoints_Hash_IPv4[client_type]
                else:
                    ip_hash = self.Tunnel_Endpoints_Hash_IPv6[client_type]
            except Exception:
                break
            try:
                del ip_hash[ip_value]
            except Exception:
                break
            try:
                if self.domain.active and (self.unique_id != DpsCollection.Shared_VNID):
                    if client_type == DpsClientType.vlan_gateway:
                        DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_VLAN))
                    elif client_type == DpsClientType.external_gateway:
                        DpsCollection.gateway_update_queue.put((self, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            except Exception:
                pass
            break
        return

    def dps_clients_get(self):
        '''
        This routine returns a list of dps_clients in a DVG
        @attention - This routine should be called with the global lock held
        '''
        dps_client_set = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                dps_client_set[tunnel.dps_client] = True
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                dps_client_set[tunnel.dps_client] = True
        return dps_client_set.keys()

    def policy_add_src(self, dst_dvg_id, policy):
        '''
        This routine adds a policy where this VNID is the Source
        @param dst_dvg_id: The Destination DVG ID
        @type dst_dvg_id: Integer
        @param policy: Policy Object
        @type policy: Policy
        '''
        self.Policy_Source[policy.traffic_type][dst_dvg_id] = policy
        if self.domain.active:
            DpsCollection.policy_update_queue.put((self, policy.traffic_type))

    def policy_add_dst(self, src_dvg_id, policy):
        '''
        This routine adds a policy where this VNID is the Destination
        @param dst_dvg_id: The Destination DVG ID
        @type dst_dvg_id: Integer
        @param policy: Policy Object
        @type policy: Policy
        '''
        self.Policy_Destination[policy.traffic_type][src_dvg_id] = policy
        #Policy check done at source, no need to send to destination
        #if self.domain.active
        #    DpsCollection.policy_update_queue.put((self, policy.traffic_type))

    def policy_del_src(self, dst_dvg_id, traffic_type):
        '''
        This routine deletes a policy where this VNID is the Source
        @param dst_dvg_id: The Destination DVG ID
        @type dst_dvg_id: Integer
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        '''
        try:
            del self.Policy_Source[traffic_type][dst_dvg_id]
        except Exception:
            pass
        if self.domain.active:
            DpsCollection.policy_update_queue.put((self, traffic_type))

    def policy_del_dst(self, src_dvg_id, traffic_type):
        '''
        This routine deletes a policy where this VNID is the Destination
        @param dst_dvg_id: The Destination DVG ID
        @type dst_dvg_id: Integer
        '''
        try:
            del self.Policy_Destination[traffic_type][src_dvg_id]
        except Exception:
            pass
        #Policy check done at source, no need to send to destination
        #if self.domain.active
        #    DpsCollection.policy_update_queue.put((self, traffic_type))

    def policy_allowed_vnids(self, fSource, traffic_type):
        '''
        This routine gets all VNIDs (including self) which are allowed to 
        communicate with this VNID.
        @param fSource: If this VNID is the source
        @type fSource: Boolean
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @return: List of all VNIDs
        @rtype: []
        '''
        vnids = []
        if fSource:
            policies = self.Policy_Source[traffic_type]
        else:
            policies = self.Policy_Destination[traffic_type]
        for vnid in policies.keys():
            try:
                policy = policies[vnid]
            except Exception:
                pass
            if policy.type != policy.type_connectivity:
                continue
            if policy.action_connectivity != policy.action_forward:
                continue
            vnids.append(vnid)
        return vnids

    def ip_subnet_add(self, ip_value, mask_value, gateway, mode):
        '''
        Adds a IP Subnet to List
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @param gateway: Gateway of Subnet
        @type gateway: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        while True:
            ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
            fFirstSubnet = False
            if self.IP_Subnet_List.count == 0:
                fFirstSubnet = True
            #Create a new Subnet list for self if necessary
            #if self.IP_Subnet_List == self.domain.IP_Subnet_List:
            #    self.IP_Subnet_List = IPSubnetList()
            ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.ip_subnet_get(ip_value, mask_value)
            if ret_val != DOVEStatus.DOVE_STATUS_OK:
                ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.domain.ip_subnet_get(ip_value, mask_value)
                if ret_val == DOVEStatus.DOVE_STATUS_OK:
                    if subnet_mode != mode or subnet_gateway != gateway:
                        return DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
                ret_val = self.IP_Subnet_List.add(ip_value, mask_value, gateway, mode)
                if ret_val == DOVEStatus.DOVE_STATUS_OK:
                    #Add to implicit gateway collection
                    self.ImplicitGatewayIPListv4.add(socket.AF_INET, gateway)
                    #Add to domain
                    self.domain.ip_subnet_add(ip_value, mask_value, gateway, mode)
            #Clear endpoints vIPs which don't belong in this subnet (but existed earlier)
            if fFirstSubnet:
                for endpoint in self.Endpoint_Hash_MAC.values():
                    for vIP in endpoint.vIP_set.values():
                        #Check if vip_value belonged in the subnet
                        ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.ip_subnet_lookup(vIP.ip_value)
                        print 'ret_val %s, subnet_ip %s, ip_value %s\r'%(ret_val, subnet_ip, ip_value)
                        if subnet_ip == ip_value:
                            continue
                        endpoint.vIP_del(vIP)
            break
        return ret_val

    def ip_subnet_delete(self, ip_value, mask_value):
        '''
        Deletes a IP Subnet from List
        @param ip_value: IP Address of Subnet
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        while True:
            #Delete all vIPs associated with this subnet
            for endpoint in self.Endpoint_Hash_MAC.values():
                for vIP in endpoint.vIP_set.values():
                    #Check if vip_value belonged in the subnet
                    ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.ip_subnet_lookup(vIP.ip_value)
                    #print 'ret_val %s, subnet_ip %s, ip_value %s\r'%(ret_val, subnet_ip, ip_value)
                    if ret_val != DOVEStatus.DOVE_STATUS_OK:
                        continue
                    if subnet_ip != ip_value:
                        continue
                    endpoint.vIP_del(vIP)
            #Get the gateway associated with this subnet
            ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.ip_subnet_get(ip_value, mask_value)
            if ret_val != DOVEStatus.DOVE_STATUS_OK:
                break
            #Remove the gateway from the Implicit Gateway collection
            self.ImplicitGatewayIPListv4.remove(socket.AF_INET, subnet_gateway)
            #Remove the Subnet
            ret_val = self.IP_Subnet_List.delete(ip_value, mask_value)
            #if self.IP_Subnet_List.count <= 0:
            #    #Replace with Domain's List if empty
            #    self.IP_Subnet_List.destroy()
            #    self.IP_Subnet_List = self.domain.IP_Subnet_List
            if ret_val == DOVEStatus.DOVE_STATUS_OK:
                #Remove from domain
                self.domain.ip_subnet_delete(ip_value, mask_value)
            #Inform all DOVE Switches
            print 'Inform all DOVE Switches to re-register on VNID %s\r'%self.unique_id
            break
        return ret_val

    def ip_subnet_lookup(self, ip_value):
        '''
        Check if a given IP falls into a subnet in the list and get the subnet
        @param ip_value: IP Address
        @type ip_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode
        @rtype: Integer, Integer, Integer, Integer
        '''
        return self.IP_Subnet_List.lookup(ip_value)

    def ip_subnet_get(self, ip_value, mask_value):
        '''
        Get a exact subnet according to IP and Mask pair
        @param ip_value: IP Address
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode
        @rtype: Integer, Integer, Integer, Integer
        '''
        return self.IP_Subnet_List.get(ip_value, mask_value)

    def ip_subnet_show(self):
        '''
        List all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        return self.IP_Subnet_List.show()

    def ip_subnet_flush(self):
        '''
        Flush all IP Subnets from List
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        return self.IP_Subnet_List.flush()

    def send_policies_to(self, dps_client, solicited, query_id):
        '''
        This routine must be called with global lock held. This routine
        will drop in between but return with lock held.
        Sends the list of policies in the Domain to the DPS Client
        @param dps_client: DPSClient
        @type dps_client: DPSClient
        @param solicited: Whether the DPS Client asked for this (1, 0)
        @type solicited: Integer
        @param query_id: If solicited, the query ID sent by the DPS Client
        @type query_id: Integer
        '''
        #log.info('VNID %s send_all_policies_to: Enter', self.unique_id)
        if not self.valid or not self.domain.active:
            return
        policy_list = []
        #Only Unicast Policies need to be sent
        for policy in self.Policy_Source[0].values():
            if policy.type != policy.type_connectivity:
                #log.debug('Not connectivity')
                continue
            if policy.action_connectivity != policy.action_forward:
                #log.debug('Not action_forward')
                continue
            #Only send "allow" policies to the DPS Clients
            policy_list.append((policy.src_dvg.unique_id, policy.dst_dvg.unique_id))
        ##NO Need to send destination related policies
        #for policy in self.Policy_Destination[0].values():
        #    if policy.type != policy.type_connectivity:
        #        log.debug('Not connectivity')
        #        continue
        #    if policy.action_connectivity != policy.action_forward:
        #        log.debug('Not action_forward')
        #        continue
        #    #Only send "allow" policies to the DPS Clients
        #    policy_list.append((policy.src_dvg.unique_id, policy.dst_dvg.unique_id))
        if solicited == 0:
            query_id_use = DpsCollection.generate_query_id()
        else:
            query_id_use = query_id
        ret_val = dcslib.send_all_connectivity_policies(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                        dps_client.location.port, #DPS Client Port
                                                        solicited, #If the DPS Client asked for this
                                                        self.unique_id, #VNID ID
                                                        query_id_use,
                                                        len(policy_list), #Number of Policies
                                                        policy_list
                                                        )
        if solicited == 0 and ret_val != 0 and len(DpsCollection.Policy_Updates_To) < DpsCollection.Max_Pending_Queue_Size:
            #Insert into a retry queue to be tried in the next iteration
            key = '%s:%s'%(self.unique_id, dps_client.location.ip_value)
            DpsCollection.Policy_Updates_To[key] = (self, dps_client)
        #log.info('VNID %s send_all_policies_to: Done', self.unique_id)
        return

    def send_policy_update(self, traffic_type):
        '''
        This routine must NOT be held with global lock held. It can be called
        from the following places
            1. A timer routine
        This routine sends the new policy to every DPS client which that has
        endpoints in that VNID
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        '''
        if not self.valid:
            return
        DpsCollection.global_lock.acquire()
        #Create a set of dps_clients
        dps_clients = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        if traffic_type == 0: #UNICAST
            for dps_client in dps_clients.keys():
                self.send_policies_to(dps_client, 0, 0)
        elif traffic_type == 1: #MULTICAST
            ext_v4, ext_v6 = self.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL)
            vlan_v4, vlan_v6 = self.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_VLAN)
            for dps_client in dps_clients.keys():
                self.send_gateways_to(dps_client, 
                                      DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL, 
                                      ext_v4, 
                                      ext_v6)
                self.send_gateways_to(dps_client, 
                                      DOVEGatewayTypes.GATEWAY_TYPE_VLAN, 
                                      vlan_v4, 
                                      vlan_v6)
            #Send updates to all Multicast Senders in this DVG
            self.domain.Multicast.policy_update(self)
        dps_clients.clear()
        DpsCollection.global_lock.release()
        return

    def send_gateways_to(self, dps_client, gwy_type, v4_gwys, v6_gwys):
        '''
        This routine must be called with global lock held.
        Sends the list of VLAN gateways in the VNID to the DPS Client
        @param dps_client: The DPS Client
        @type dps_client: DPS Client
        @param gwy_type: The TYPE of Gateway
        @type gwy_type: DOVEGatewayTypes
        @param v4_gwys: List of (vnid, IPv4) gateways
        @type v4_gwys: List of Tuples i.e. [(v1,ip1), (v2,ip2)]
        @param v6_gwys: List of (vnid, IPv6) gateways
        @param v6_gwys: List of Tuples i.e. [(v1,ip1), (v2,ip2)]
        '''
        if not self.valid or not self.domain.active:
            return
        query_id = DpsCollection.generate_query_id()
        ret_val = dcslib.send_gateways(dps_client.location.ip_value_packed, #DPS Client Location IP
                                       dps_client.location.port, #DPS Client Port
                                       self.unique_id, #VNID ID
                                       query_id, #Query ID
                                       gwy_type,
                                       v4_gwys,
                                       v6_gwys
                                       )
        if ret_val != 0 and len(DpsCollection.Gateway_Updates_To) < DpsCollection.Max_Pending_Queue_Size:
            #Insert into a retry queue to be tried in the next iteration
            key = '%s:%s:%s'%(self.unique_id, dps_client.location.ip_value, gwy_type)
            DpsCollection.Gateway_Updates_To[key] = (self, dps_client, gwy_type)
        return

    def send_gateway_update(self, gwy_type, v4_gwys, v6_gwys):
        '''
        Send Gateway update to all the DPS Clients to all relevant DVGs
        @attention: This routine must be called with global lock held.
        @param gwy_type: The TYPE of Gateway
        @type gwy_type: DOVEGatewayTypes
        @param v4_gwys: List of (vnid, IPv4) gateways
        @type v4_gwys: List of Tuples i.e. [(v1,ip1), (v2,ip2)]
        @param v6_gwys: List of (vnid, IPv6) gateways
        @param v6_gwys: List of Tuples i.e. [(v1,ip1), (v2,ip2)]
        '''
        #Create a set of dps_clients
        if not self.valid:
            return
        dps_clients = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        for dps_client in dps_clients.keys():
            self.send_gateways_to(dps_client,
                                  gwy_type,
                                  v4_gwys,
                                  v6_gwys)
        dps_clients.clear()
        return

    def send_multicast_update_to(self, dps_client, receiver_dict, mac, inet_type, ip_packed, global_scope):
        '''
        Sends the list of VLAN gateways in the VNID to the DPS Client
        @attention: This routine must be called with global lock held. 
        @param dps_client: The DPS Client
        @type dps_client: DPS Client
        @param receiver_dict: The TYPE of Gateway
        @type receiver_dict: {}
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_packed: IP Address in Packed Format
        @type ip_packed: ByteArray
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        '''
        if not self.valid or not self.domain.active:
            return
        query_id = DpsCollection.generate_query_id()
        #DpsCollection.global_lock.release()
        ret_val = dcslib.send_multicast_tunnels(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                dps_client.location.port, #DPS Client Port
                                                self.unique_id, #VNID ID
                                                query_id, #Query ID
                                                mac,#MAC
                                                inet_type,#AF_INET or AF_INET6
                                                ip_packed,#ByteArray
                                                receiver_dict
                                                )
        if ret_val != 0 and len(DpsCollection.VNID_Multicast_Updates_To) < DpsCollection.Max_Pending_Queue_Size:
            #Insert into a retry queue to be tried in the next iteration
            key = '%s:%s:%s:%s:%s'%(self.unique_id, dps_client.location.ip_value, mac, ip_packed, global_scope)
            DpsCollection.VNID_Multicast_Updates_To[key] = (self, dps_client, mac, inet_type, ip_packed, global_scope)
        return

    def send_multicast_update(self, dps_client_list, receiver_dict, mac, inet_type, ip_packed, global_scope):
        '''
        This routine sends the multicast receiver list to every sender in this dvg.
        @attention: This routine must be called with global lock held.
        @param dps_client_list: List of dps clients
        @type dps_client: DPSClient
        @param receiver_dict: A dictionary of receivers
        @type receiver_dict: {}
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_packed: IP Address in Packed Format
        @type ip_packed: ByteArray
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        '''
        if not self.valid:
            return
        for dps_client in dps_client_list:
            self.send_multicast_update_to(dps_client, receiver_dict, mac, inet_type, ip_packed, global_scope)
        return

    def send_address_resolution_to(self, dps_client, vIP_packed):
        '''
        This routine sends address resolution request to a particular DPS Client
        @param dps_client: The DPS Client
        @type dps_client: DPS Client
        @param vIP_packed: The Address being resolved
        @type vIP_packed: ByteArray
        '''
        if not self.valid:
            return
        query_id = DpsCollection.generate_query_id()
        ret_val = dcslib.send_address_resolution(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                 dps_client.location.port, #DPS Client Port
                                                 self.unique_id, #VNID ID
                                                 query_id, #Query ID
                                                 vIP_packed,
                                                 DpsCollection.Invalid_MAC_Bytes
                                                 )
        if ret_val != 0 and len(DpsCollection.Address_Resolution_Requests_To) < DpsCollection.Max_Pending_Queue_Size:
            #Insert into a retry queue to be tried in the next iteration
            key = '%s:%s:%s'%(self.unique_id, dps_client.location.ip_value, vIP_packed)
            DpsCollection.Address_Resolution_Requests_To[key] = (self, dps_client, vIP_packed)
        return

    def send_address_resolution(self, vIP_packed):
        '''
        This routine sends address resolution messages to every dps client in this DVG
        @param vIP_packed: vIP packed
        @type vIP_packed: ByteArray
        '''
        if not self.valid:
            return
        dps_list = self.dps_clients_get()
        for dps_client in dps_list:
            self.send_address_resolution_to(dps_client, vIP_packed)

    def send_endpoint_location_reply_to(self, dps_client_ip_packed, dps_client_port,
                                        vIP_vnid, vIP_version,
                                        pipv4_list, pipv6_list,
                                        vMac, vIP_packed):
        '''
        This routine sends an unsolicited Endpoint Reply to dps client
        @param dps_client_ip_packed: The DPS Client IP
        @type dps_client_ip_packed: Packed Structure
        @param dps_client_port: DPS Client Port
        @type dps_client_port: Integer
        @param vIP_vnid: The Endpoint's VNID
        @type vIP_vnid: Integer
        @param vIP_version: The Endpoint's version
        @type vIP_version: Integer
        @param pipv4_list: List of IPv4 Endpoint's Tunnel Address
        @type pipv4_list: []
        @param pipv6_list: List of IPv6 Endpoint's Tunnel Address
        @type pipv6_list: []
        @param vMac: Virtual MAC address
        @type vMac: ByteArray
        @param vIP_packed: The Virtual IP Address
        @type vIP_packed: ByteArray
        '''
        if not self.valid:
            return
        query_id = DpsCollection.generate_query_id()
        #Don't care if message was not sent
        dcslib.send_endpoint_reply(dps_client_ip_packed,
                                   dps_client_port,
                                   self.unique_id,
                                   query_id,
                                   vIP_vnid,
                                   vIP_version,
                                   pipv4_list,
                                   pipv6_list,
                                   vMac,
                                   vIP_packed)
        return

    def send_vnid_deletion_to(self, dps_client):
        '''
        This routine sends address resolution request to a particular DPS Client
        @param dps_client: The DPS Client
        @type dps_client: DPS Client
        '''
        query_id = DpsCollection.generate_query_id()
        dcslib.send_vnid_deletion(dps_client.location.ip_value_packed, #DPS Client Location IP
                                  dps_client.location.port, #DPS Client Port
                                  self.unique_id, #VNID ID
                                  query_id
                                  )
        return

    def send_vnid_deletion(self):
        '''
        This routine sends vnid deletion to every client
        @param vIP_packed: vIP packed
        @type vIP_packed: ByteArray
        '''
        dps_list = self.dps_clients_get()
        for dps_client in dps_list:
            self.send_vnid_deletion_to(dps_client)

    def communication_gateway_list(self, fSource, gwy_type):
        '''
        This routine MUST be called with the global lock held
        This routine determines the list of gateways this VNID can communicate 
        with.
        @param fSource: If this VNID is the source
        @type fSource: Boolean
        @param gwy_type: The Gateway Type
        @type gwy_type: DOVEGatewayTypes
        @return: List of IPv4 gateways (vnid, ip) and IPv6 gateways (vnid, ip)
        @rtype: ([(vnid,ipv4)], [vnid,ipv6])
        '''
        if not self.valid:
            return ([],[])
        #Needed for Multicast Control Traffic
        vnids = self.policy_allowed_vnids(fSource, 1)
        gateways_v4 = []
        gateways_v6 = []
        for vnid in vnids:
            try:
                dvg = self.domain.DVG_Hash[vnid]
            except Exception:
                continue
            if gwy_type == DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL:
                ipv4_values = dvg.Tunnel_Endpoints_Hash_IPv4[DpsClientType.external_gateway].keys()
                ipv6_values = dvg.Tunnel_Endpoints_Hash_IPv6[DpsClientType.external_gateway].keys()
            elif gwy_type == DOVEGatewayTypes.GATEWAY_TYPE_VLAN:
                ipv4_values = dvg.Tunnel_Endpoints_Hash_IPv4[DpsClientType.vlan_gateway].keys()
                ipv6_values = dvg.Tunnel_Endpoints_Hash_IPv6[DpsClientType.vlan_gateway].keys()
            else:
                continue
            for ipv4_val in ipv4_values:
                gateways_v4.append((vnid, ipv4_val))
            for ipv6_val in ipv6_values:
                gateways_v6.append((vnid, ipv6_val))
        return (gateways_v4, gateways_v6)

    def gateway_list_modified(self, gwy_type):
        '''
        This routine must NOT be held with global lock held. It can be called
        from the following places
            1. A timer routine
        This is the routine that should be called when a gateway
        is added/deleted to the VNID/DVG. This routine will send updates to ALL
        relevant DOVE Switches that need to send MULTICAST messages to
        this VLAN gateway. To determine this the following algorithm is
        used:
        1. Determine all source DVGs/VNIDs allowed to send traffic to this VNID.
        2. For each of the allowed source DVGs, we have to send a list of all 
           VLAN Gateways it can send messages to i.e.
           2a. Determine all destination DVGs and corresponding VLAN Gateways in
               them.
        @param gwy_type: The Gateway Type
        @type gwy_type: DOVEGatewayTypes
        '''
        #Step 1: Determine all source DVGs/VNIDs allowed to send to this VNID
        if not self.valid:
            return
        DpsCollection.global_lock.acquire()
        #Needed for Multicast Control Traffic
        src_vnids = self.policy_allowed_vnids(False, 1)
        for src_vnid in src_vnids:
            try:
                src_dvg = self.domain.DVG_Hash[src_vnid]
            except Exception:
                continue
            gateways_v4, gateways_v6 = src_dvg.communication_gateway_list(True, gwy_type)
            src_dvg.send_gateway_update(gwy_type, gateways_v4, gateways_v6)
        DpsCollection.global_lock.release()
        return

    def send_broadcast_table_to(self, dps_client):
        '''
        This routine must be called with global lock held.
        This routine sends the Broadcast Table list to DPS Client
        @param dps_client: DPSClient
        @type dps_client: DPSClient
        '''
        if not self.valid or not self.domain.active:
            return
        ipv4_set = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            #ipv4_list += self.Tunnel_Endpoints_Hash_IPv4[client_type].keys()
            for ipv4 in self.Tunnel_Endpoints_Hash_IPv4[client_type].keys():
                ipv4_set[ipv4] = True
        ipv4_list = ipv4_set.keys()
        ipv6_set = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            #ipv6_list += self.Tunnel_Endpoints_Hash_IPv6[client_type].keys()
            for ipv6 in self.Tunnel_Endpoints_Hash_IPv6[client_type].keys():
                ipv6_set[ipv6] = True
        ipv6_list = ipv6_set.keys()
        query_id = DpsCollection.generate_query_id()
        ret_val = dcslib.send_broadcast_table(dps_client.location.ip_value_packed, #DPS Client Location IP
                                              dps_client.location.port, #DPS Client Port
                                              self.unique_id,#VNID ID
                                              query_id, #Query ID
                                              ipv4_list,
                                              ipv6_list
                                              )
        if ((ret_val != 0) and 
            (len(DpsCollection.VNID_Broadcast_Updates_To) < DpsCollection.Max_Pending_Queue_Size)):
            #Insert into a retry queue to be tried in the next iteration
            key = '%s:%s'%(self.unique_id, dps_client.location.ip_value)
            DpsCollection.VNID_Broadcast_Updates_To[key] = (self, dps_client)
        return

    def send_broadcast_table_update(self):
        '''
        This routine must be called with global lock held.
        This routine sends the Broadcast Table list to every DPS Client in that
        VNID.
        '''
        if not self.valid or not self.domain.active:
            return
        #Send to all IPv4 DPS Clients
        ipv4_set = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            #ipv4_list += self.Tunnel_Endpoints_Hash_IPv4[client_type].keys()
            for ipv4 in self.Tunnel_Endpoints_Hash_IPv4[client_type].keys():
                ipv4_set[ipv4] = True
        ipv4_list = ipv4_set.keys()
        ipv6_set = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            #ipv6_list += self.Tunnel_Endpoints_Hash_IPv6[client_type].keys()
            for ipv6 in self.Tunnel_Endpoints_Hash_IPv6[client_type].keys():
                ipv6_set[ipv6] = True
        ipv6_list = ipv6_set.keys()
        #Create a set of dps_clients
        dps_clients = {}
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                dps_client = tunnel.dps_client
                dps_clients[dps_client] = True
        for dps_client in dps_clients.keys():
            query_id = DpsCollection.generate_query_id()
            ret_val = dcslib.send_broadcast_table(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                  dps_client.location.port, #DPS Client Port
                                                  self.unique_id,#VNID ID
                                                  query_id, #Query ID
                                                  ipv4_list,
                                                  ipv6_list
                                                  )
            if ((ret_val != 0) and 
                (len(DpsCollection.VNID_Broadcast_Updates_To) < DpsCollection.Max_Pending_Queue_Size)):
                #Insert into a retry queue to be tried in the next iteration
                key = '%s:%s'%(self.unique_id, dps_client.location.ip_value)
                DpsCollection.VNID_Broadcast_Updates_To[key] = (self, dps_client)
        dps_clients.clear()
        return

    def delete(self):
        '''
        Destructor:
        '''
        if not self.valid:
            return
        self.valid = False
        #Send vnid Deletion to All DOVE Switches
        #TODO: Don't do this in final product
        self.send_vnid_deletion()
        #Remove from Broadcast Collection
        try:
            del DpsCollection.VNID_Broadcast_Updates[self.unique_id]
        except Exception:
            pass
        #Remove self from VNID_Broadcast_Updates_To
        for key in DpsCollection.VNID_Broadcast_Updates_To.keys():
            try:
                tuple_value = DpsCollection.VNID_Broadcast_Updates_To[key]
                dvg = tuple_value[0]
                if dvg == self:
                    del DpsCollection.VNID_Broadcast_Updates_To[key]
            except Exception:
                pass
        #Remove from DPS Client Updates
        for key in DpsCollection.Policy_Updates_To.keys():
            try:
                tuple_value = DpsCollection.Policy_Updates_To[key]
                dvg = tuple_value[0]
                if dvg == self:
                    del DpsCollection.Policy_Updates_To[key]
            except Exception:
                pass
        #Remove from Gateway_Updates_To
        for key in DpsCollection.Gateway_Updates_To.keys():
            try:
                tuple_value = DpsCollection.Gateway_Updates_To[key]
                dvg = tuple_value[0]
                if dvg == self:
                    del DpsCollection.Gateway_Updates_To[key]
            except Exception:
                pass
        #Remove from Multicast Updates
        for key in DpsCollection.VNID_Multicast_Updates.keys():
            try:
                vnid_tuple = DpsCollection.VNID_Multicast_Updates[key]
                dvg = vnid_tuple[0]
                if dvg == self:
                    del DpsCollection.VNID_Multicast_Updates[key]
            except Exception:
                continue
        #Remove from VNID_Multicast_Updates_To
        for key in DpsCollection.VNID_Multicast_Updates_To.keys():
            try:
                vnid_tuple = DpsCollection.VNID_Multicast_Updates_To[key]
                dvg = vnid_tuple[0]
                if dvg == self:
                    del DpsCollection.VNID_Multicast_Updates[key]
            except Exception:
                continue
        #Remove from Address_Resolution_Requests_To
        for key in DpsCollection.Address_Resolution_Requests_To.keys():
            try:
                vnid_tuple = DpsCollection.Address_Resolution_Requests_To[key]
                dvg = vnid_tuple[0]
                if dvg == self:
                    del DpsCollection.VNID_Multicast_Updates[key]
            except Exception:
                continue
        #Remove from VNID Collection
        try:
            del DpsCollection.VNID_Hash[self.unique_id]
        except Exception:
            pass
        #Delete all policies
        for policy_key in self.Policy_Destination[0].keys():
            try:
                policy = self.Policy_Destination[0][policy_key]
                Domain.policy_del(self.domain, policy)
                del self.Policy_Destination[0][policy_key]
                policy.delete()
            except Exception:
                pass
        self.Policy_Destination[0].clear()
        for policy_key in self.Policy_Destination[1].keys():
            try:
                policy = self.Policy_Destination[1][policy_key]
                Domain.policy_del(self.domain, policy)
                del self.Policy_Destination[1][policy_key]
                policy.delete()
            except Exception:
                pass
        self.Policy_Destination[1].clear()
        for policy_key in self.Policy_Source[0].keys():
            try:
                policy = self.Policy_Source[0][policy_key]
                Domain.policy_del(self.domain, policy)
                del self.Policy_Source[0][policy_key]
                policy.delete()
            except Exception:
                pass
        self.Policy_Source[0].clear()
        for policy_key in self.Policy_Source[1].keys():
            try:
                policy = self.Policy_Source[1][policy_key]
                Domain.policy_del(self.domain, policy)
                del self.Policy_Source[1][policy_key]
                policy.delete()
            except Exception:
                pass
        self.Policy_Source[1].clear()
        #Delete all Endpoints
        for endpoint in self.Endpoint_Hash_MAC.values():
            try:
                endpoint.delete()
            except Exception:
                pass
        self.Endpoint_Hash_MAC.clear()
        for endpoint in self.Endpoint_Hash_IPv4.values():
            try:
                endpoint.delete()
            except Exception:
                pass
        self.Endpoint_Hash_IPv4.clear()
        for endpoint in self.Endpoint_Hash_IPv6.values():
            try:
                endpoint.delete()
            except Exception:
                pass
        self.Endpoint_Hash_IPv6.clear()
        #Delete all tunnels
        for client_type in self.Tunnel_Endpoints_Hash_IPv4.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[client_type].values():
                try:
                    tunnel.delete()
                except Exception:
                    pass
                self.Tunnel_Endpoints_Hash_IPv4[client_type].clear()
        for client_type in self.Tunnel_Endpoints_Hash_IPv6.keys():
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[client_type].values():
                try:
                    tunnel.delete()
                except Exception:
                    pass
                self.Tunnel_Endpoints_Hash_IPv6[client_type].clear()
        #if self.IP_Subnet_List != self.domain.IP_Subnet_List:
        #    self.IP_Subnet_List.destroy()
        self.IP_Subnet_List.destroy()
        #Remove from Domain
        self.domain.dvg_del(self)

    def show(self, fdetails):
        '''
        Display Contents of a DVG
        @param fdetails: Whether to show detailed information
        @type fdetails: Integer (0 = False, 1 = True)
        '''
        print '------------------------------------------------------------------\r'
        print 'Showing VNID: %s\r'%(self.unique_id)
        #############################################################
        #Show number of tunnels.
        #############################################################
        tunnel_count = len(self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.dove_switch])+ len(self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.dove_switch])
        if fdetails and tunnel_count > 0:
            tunnel_set = {}
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.dove_switch].values():
                tunnel_set[tunnel] = True
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.dove_switch].values():
                tunnel_set[tunnel] = True
            print '---------------------- DOVE Switches -------------------------\r'
            for tunnel in tunnel_set.keys():
                tunnel.show()
            tunnel_set.clear()
            print '--------------------------------------------------------------\r'
        else:
            print 'DOVE Switches: Total %s\r'%tunnel_count
        tunnel_count = len(self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.vlan_gateway])+ len(self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.vlan_gateway])
        if fdetails and tunnel_count > 0:
            tunnel_set = {}
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.vlan_gateway].values():
                tunnel_set[tunnel] = True
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.vlan_gateway].values():
                tunnel_set[tunnel] = True
            print '---------------------- VLAN Gateways -------------------------\r'
            for tunnel in tunnel_set.keys():
                tunnel.show()
            tunnel_set.clear()
            print '--------------------------------------------------------------\r'
        else:
            print 'VLAN GATEWAYS: Total %s\r'%tunnel_count
        tunnel_count = len(self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.external_gateway])+ len(self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.external_gateway])
        if fdetails and tunnel_count > 0:
            tunnel_set = {}
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4[DpsClientType.external_gateway].values():
                tunnel_set[tunnel] = True
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6[DpsClientType.external_gateway].values():
                tunnel_set[tunnel] = True
            print '---------------------- EXTERNAL GATEWAY ----------------------\r'
            for tunnel in tunnel_set.keys():
                tunnel.show()
            tunnel_set.clear()
            print '--------------------------------------------------------------\r'
        else:
            print 'EXTERNAL GATEWAYS: Total %s\r'%tunnel_count
        #############################################################
        #Show number of endpoints.
        #############################################################
        endpoint_count = len(self.Endpoint_Hash_MAC)
        if fdetails and endpoint_count > 0:
            print '-------------------------- Endpoints -----------------------------\r'
            for endpoint in self.Endpoint_Hash_MAC.values():
                endpoint.show()
        else:
            print 'Endpoints: Total %s\r'%endpoint_count
            endpoint_ip = len(self.Endpoint_Hash_IPv4) + len(self.Endpoint_Hash_IPv6)
            print 'Endpoints: Total Number of vIP Addresses %s\r'%endpoint_ip
        ##############################################################
        ##Show Dedicated Subnet Lists
        ##############################################################
        if self.IP_Subnet_List != self.domain.IP_Subnet_List and self.IP_Subnet_List.count > 0:
            print 'Subnet List: Total %s\r'%self.IP_Subnet_List.count
            self.ip_subnet_show()
        ##############################################################
        ##Show Implicit Gateway List
        ##############################################################
        count = self.ImplicitGatewayIPListv4.count() + self.ImplicitGatewayIPListv6.count()
        if fdetails and count > 0:
            print '------------------------- Implicit Gateways ----------------------\r'
            if self.ImplicitGatewayIPListv4.count() > 0:
                self.ImplicitGatewayIPListv4.show()
            if self.ImplicitGatewayIPListv6.count() > 0:
                self.ImplicitGatewayIPListv6.show()
        else:
            print 'Implicit Gateways: Total %s\r'%count
        #############################################################
        #Show Policies.
        #############################################################
        if fdetails:
            print '----------------------- Unicast Policies -------------------------\r'
            print 'Allowed Destination VNIDs %s\r'%self.policy_allowed_vnids(True, 0)
            print 'Allowed Source VNIDs      %s\r'%self.policy_allowed_vnids(False, 0)
        ############################################################
        #To be finished
        return

DVG.add_class()
