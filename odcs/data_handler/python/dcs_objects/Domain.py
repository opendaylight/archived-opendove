'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas and jinghe
'''
import logging
import time
from logging import getLogger
log = getLogger(__name__)

import struct
import socket
import Queue #renamed to queue in Python 3.0
import threading
from threading import Thread

from object_collection import DpsCollection
from object_collection import DpsClientType
from object_collection import DOVEStatus
from dcs_objects import dcs_object
from dcs_objects.IPAddressList import IPAddressList
from dcs_objects.IPSubnetList import IPSubnetList
from dcs_objects.Multicast import Multicast
from dcs_objects.AddressResolution import AddressResolution
from dcs_objects.ConflictDetection import ConflictDetection
from dcs_objects.DPSClientHost import DPSClientHost
from object_collection import DpsLogLevels

import dcslib

class Domain(dcs_object):
    '''
    This represents the Domain Object in DOVE
    '''

    def __init__(self, domain_id, active):
        '''
        Constructor:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param active: Whether the Domain is active
        @type active: Boolean
        '''
        if domain_id < 0 or domain_id > 16777215:
            raise Exception ('Domain ID %s not valid'%(domain_id))
        self.IP_Subnet_List = IPSubnetList()
        self.active = active
        self.replication_factor = 1
        self.mass_transfer = None
        #Set of nodes to forward updates after Mass Transfer is complete
        #and waiting for all other nodes to see this new DPS Node
        #as part of the domain.
        self.mass_transfer_forward_nodes = {}
        #Create Timer for the Domain Object
        #Create the Replication Dictionary
        #The Replication dictionary
        self.replication_query_id_server = {}
        self.replication_query_id_client = {}
        #Based on Object Model Chapter 5.1: Domain Object
        #Collection of DVGs hashed by DVG id.
        #TODO: Add to Domain Object in Domain Object Model
        self.DVG_Hash = {}
        #############################################################
        #Endpoint Object Tables (Requirement 1.1 and 1.2)
        #############################################################
        #Endpoint Object Hash Table (IP Address)
        #Separating IPv4 and IPv6 into different Hashes since one
        #will be hashed by integer, the other string
        self.Endpoint_Hash_IPv4 = {}
        self.Endpoint_Hash_IPv6 = {}
        #Endpoint Object Hash Table (MAC Address)
        self.Endpoint_Hash_MAC = {}
        #############################################################
        #Policy Object Hash Table (Requirement 1.2)
        #The first index is for unicast policy, the second for multicast
        #############################################################
        self.Policy_Hash_DVG = [{}, {}]
        #############################################################
        #DPS Client Hash Tables (Requirement 1.5)
        #############################################################
        #This is to store the DPS Client IPs Address (Different from 
        #the DOVE Switch/Gateway IP address)
        self.DPSClients_Hash_IPv4 = {}
        self.DPSClients_Hash_IPv6 = {}
        #This is to store the Tunnel Endpoint IP Address
        self.Tunnel_Endpoints_Hash_IPv4 = {}
        self.Tunnel_Endpoints_Hash_IPv6 = {}
        #List of Implicit Gateway IP Address for this Domain
        self.ImplicitGatewayIPListv4 = IPAddressList(socket.AF_INET)
        self.ImplicitGatewayIPListv6 = IPAddressList(socket.AF_INET6)
        #Multicast Object
        self.Multicast = Multicast(self, domain_id)
        #Address Resolution
        self.AddressResolution = AddressResolution(domain_id)
        self.ConflictDetection = ConflictDetection(domain_id)
        #############################################################
        #Add the domain into DPS Collection Hash Table
        #############################################################
        DpsCollection.Domain_Hash[domain_id] = self
        #DpsCollection.VNID_Hash[domain_id] = domain_id
        #############################################################
        #Statistics 
        #############################################################
        self.Stats_Array = []
        curr_time = time.time()
        self.Stats_Array.append([curr_time, 0, 0, 0])

        self.Endpoint_Update_Count = 0
        self.Endpoint_Update_Count_Delta = 0
        self.Endpoint_Lookup_Count = 0
        self.Endpoint_Lookup_Count_Delta = 0
        self.Policy_Lookup_Count = 0
        self.Policy_Lookup_Count_Delta = 0
        self.Multicast_Lookup_Count = 0
        self.Internal_Gateway_Lookup_Count = 0
        #############################################################
        #To be finished
        #############################################################
        # Initialize the common part
        dcs_object.__init__(self, domain_id)

    def dvg_add(self, dvg):
        '''
        Adds a DVG to the Domain Collection
        @param dvg: The DVG Object
        @type dvg: DVG
        '''
        self.DVG_Hash[dvg.unique_id] = dvg
        return

    def dvg_del(self, dvg):
        '''
        Deletes a DVG to the Domain Collection
        @param dvg: The DVG Object
        @type dvg: DVG
        '''
        #Remove VNID from AddressResolution
        self.AddressResolution.vnid_delete(dvg)
        try:
            dvg = self.DVG_Hash[dvg.unique_id]
            del self.DVG_Hash[dvg.unique_id]
            dvg.delete()
        except Exception:
            pass
        return

    def vnid_random_get(self):
        '''
        This routine gets a random (active) VNID in a domain
        '''
        try:
            vnid = self.DVG_Hash.keys()[0]
        except Exception:
            vnid = 0
        return vnid

    def policy_add(self, policy):
        '''
        Adds a Policy to the Domain Collection
        @param dvg: The Policy Object
        @type dvg: Policy
        '''
        self.Policy_Hash_DVG[policy.traffic_type][policy.key] = policy
        return

    def policy_del(self, policy):
        '''
        Deletes a DVG to the Domain Collection
        @param dvg: The Policy Object
        @type dvg: Policy
        '''
        try:
            del self.Policy_Hash_DVG[policy.traffic_type][policy.key]
        except Exception:
            pass
        return

    def endpoint_add(self, endpoint):
        '''
        Adds an Endpoint to the Domain Collection
        @param endpoint: Endpoint Object
        @type endpoint: Endpoint
        '''
        self.Endpoint_Hash_MAC[endpoint.vMac] = endpoint
        for vIP_key in endpoint.vIP_set.keys():
            self.endpoint_vIP_add(endpoint, endpoint.vIP_set[vIP_key])
        return

    def endpoint_del(self, endpoint):
        '''
        Removes an Endpoint from the Domain Collection
        @param endpoint: Endpoint Object
        @type endpoint: Endpoint
        '''
        try:
            del self.Endpoint_Hash_MAC[endpoint.vMac]
        except Exception:
            pass
        for vIP_key in endpoint.vIP_set.keys():
            self.endpoint_vIP_del(endpoint.vIP_set[vIP_key])
        return

    def endpoint_vIP_add(self, endpoint, vIP):
        '''
        Adds an Endpoint (Virtual) IP Address to the collection
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
        Deletes an Endpoint (Virtual) IP Address from the Domain collection
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

    def tunnel_endpoint_add(self, tunnel_endpoint):
        '''
        Adds a Tunnel Endpoint to the Domain Collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        '''
        for ip_value in tunnel_endpoint.ip_listv4.ip_list:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv4
            ip_hash[ip_value] = tunnel_endpoint
        for ip_value in tunnel_endpoint.ip_listv6.ip_list:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv6
            ip_hash[ip_value] = tunnel_endpoint
        return

    def tunnel_endpoint_delete(self, tunnel_endpoint):
        '''
        Deletes a Tunnel Endpoint from the Domain Collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        '''
        for ip_value in tunnel_endpoint.ip_listv4.ip_list:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv4
            try:
                del ip_hash[ip_value]
            except Exception:
                pass
        for ip_value in tunnel_endpoint.ip_listv6.ip_list:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv6
            try:
                del ip_hash[ip_value]
            except Exception:
                pass
        return

    def tunnel_endpoint_add_IP(self, tunnel_endpoint, inet_type, ip_value):
        '''
        Adds a Tunnel Endpoint Physical IP Address to the Domain Collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        '''
        #TODO: Should we add all IPs to this hash or only the primary IP
        if inet_type == socket.AF_INET:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv4
        else:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv6
        ip_hash[ip_value] = tunnel_endpoint
        return

    def tunnel_endpoint_delete_IP(self, inet_type, ip_value):
        '''
        Deletes a Tunnel Endpoint Physical IP Address from the Domain Collection.
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        '''
        if inet_type == socket.AF_INET:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv4
        else:
            ip_hash = self.Tunnel_Endpoints_Hash_IPv6
        try:
            del ip_hash[ip_value]
        except Exception:
            pass
        return

    def dps_client_add(self, dps_client):
        '''
        Adds a DPS Client to the collection
        @param dps_client: The DPS Client
        @type dps_client: DpsClient Object
        '''
        if dps_client.location.inet_type == socket.AF_INET:
            ip_hash = self.DPSClients_Hash_IPv4
        else:
            ip_hash = self.DPSClients_Hash_IPv6
        ip_hash[dps_client.location.ip_value] = dps_client
        return

    def dps_client_delete(self, dps_client):
        '''
        Deletes a DPS Client from the collection
        @param dps_client: The DPS Client
        @type dps_client: DpsClient Object
        '''
        #Remove DPS Client from AddressResolution
        self.AddressResolution.dps_client_delete(dps_client)
        if dps_client.location.inet_type == socket.AF_INET:
            ip_hash = self.DPSClients_Hash_IPv4
        else:
            ip_hash = self.DPSClients_Hash_IPv6
        try:
            del ip_hash[dps_client.location.ip_value]
        except Exception:
            pass
        return

    def delete(self, fdelete):
        '''
        Deletes the objects from all the Hash Tables
        @param fdelete: True if the Domain has been deleted from DOVE
        @type fdelete: Boolean
        '''
        if not self.valid:
            return
        self.valid = False
        #Unlink from DPS Clients
        DPSClientHost.Domain_Deleted_Locally(self)
        #Unlink from Address_Resolution_Requests
        try:
            del DpsCollection.Address_Resolution_Requests[self.unique_id]
        except Exception:
            pass
        #Unlink self from DpsCollection
        try:
            del DpsCollection.Domain_Hash[self.unique_id]
        except Exception:
            pass
        #Delete all DVGs
        if fdelete:
            #Only delete dvg structures if the domain is deleted
            for dvg in self.DVG_Hash.values():
                try:
                    dvg.delete()
                except Exception:
                    pass
            #Delete all endpoints
            for endpoint in self.Endpoint_Hash_MAC.values():
                try:
                    endpoint.delete()
                except Exception:
                    pass
            for endpoint in self.Endpoint_Hash_IPv4.values():
                try:
                    endpoint.delete()
                except Exception:
                    pass
            for endpoint in self.Endpoint_Hash_IPv6.values():
                try:
                    endpoint.delete()
                except Exception:
                    pass
            for tunnel in self.Tunnel_Endpoints_Hash_IPv4.values():
                try:
                    tunnel.delete()
                except Exception:
                    pass
            for tunnel in self.Tunnel_Endpoints_Hash_IPv6.values():
                try:
                    tunnel.delete()
                except Exception:
                    pass
            for dps_client in self.DPSClients_Hash_IPv4.values():
                try:
                    dps_client.delete()
                except Exception:
                    pass
            for dps_client in self.DPSClients_Hash_IPv6.values():
                try:
                    dps_client.delete()
                except Exception:
                    pass
            #Delete all policies
            for policy in self.Policy_Hash_DVG[0].values():
                try:
                    policy.delete()
                except Exception:
                    pass
            for policy in self.Policy_Hash_DVG[1].values():
                try:
                    policy.delete()
                except Exception:
                    pass
        self.DVG_Hash.clear()
        self.Endpoint_Hash_MAC.clear()
        self.Endpoint_Hash_IPv4.clear()
        self.Endpoint_Hash_IPv6.clear()
        #Delete all tunnels
        self.Tunnel_Endpoints_Hash_IPv4.clear()
        self.Tunnel_Endpoints_Hash_IPv6.clear()
        #Delete all dps clients
        self.DPSClients_Hash_IPv4.clear()
        self.DPSClients_Hash_IPv6.clear()
        self.Policy_Hash_DVG[0].clear()
        self.Policy_Hash_DVG[1].clear()
        #Destroy IP subnet list
        self.IP_Subnet_List.destroy()
        #Destroy Address Resolution
        self.AddressResolution.delete()
        #Destroy Conflict Detection
        self.ConflictDetection.delete()
        #Destroy Multicast
        self.Multicast.delete()
        return

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
        ret_val = self.IP_Subnet_List.add(ip_value, mask_value, gateway, mode)
        if ret_val == DOVEStatus.DOVE_STATUS_OK:
            #Add to implicit gateway collection
            self.ImplicitGatewayIPListv4.add(socket.AF_INET, gateway)
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
            #Get the gateway associated with this subnet
            ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = self.ip_subnet_get(ip_value, mask_value)
            if ret_val != DOVEStatus.DOVE_STATUS_OK:
                break
            #Remove the gateway from the Implicit Gateway collection
            self.ImplicitGatewayIPListv4.remove(socket.AF_INET, subnet_gateway)
            #Remove the Subnet
            ret_val = self.IP_Subnet_List.delete(ip_value, mask_value)
            break
        return ret_val

    def ip_subnet_lookup(self, ip_value):
        '''
        Check if a given IP falls into a subnet in the list and get the subnet
        @param ip_value: IP Address
        @type ip_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode, subnet_gateway
        @rtype: Integer, Integer, Integer, Integer, Integer
        '''
        return self.IP_Subnet_List.lookup(ip_value)

    def ip_subnet_get(self, ip_value, mask_value):
        '''
        Get a exact subnet according to IP and Mask pair
        @param ip_value: IP Address
        @type ip_value: Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: status, subnet_ip, subnet_mask, subnet_mode, subnet_gateway
        @rtype: Integer, Integer, Integer, Integer, Integer
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

    def process_lookup_not_found(self, dps_client_IP_type, dps_client_IP_val,
                                 source_dvg, vIP_type, vIP_value, vIP_packed):
        '''
        This routine handles the case of vIP lookup failure in the list of Endpoints
        in a domain. The client protocol handler sends out the External Gateway
        as the answer, but in the background this routine checks with all relevant
        DPS Client in every VNID in this domain to resolve the Address
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_val: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_val: Integer
        @param source_dvg: The Source VNID
        @type source_dvg: DVG
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_val: IPv6 or IPv4 Address
        @type vIP_val: String or Integer
        @param vIP_packed: vIP packed
        @type vIP_packed: ByteArray
        '''
        if not self.valid:
            return
        if ((self.AddressResolution.total >= self.AddressResolution.total_max) or 
            (len(self.AddressResolution.endpoint_resolution) >= self.AddressResolution.vIP_not_found_max)):
            return
        while True:
            try:
                if dps_client_IP_type == socket.AF_INET:
                    dps_client = self.DPSClients_Hash_IPv4[dps_client_IP_val]
                else:
                    dps_client = self.DPSClients_Hash_IPv6[dps_client_IP_val]
            except Exception:
                break
            #Store in Address Resolution Structure
            fSendAddressResolution = self.AddressResolution.process_lookup_not_found(dps_client, source_dvg, vIP_type, vIP_value)
            #Queue Work Item to Send Address Resolution Queries
            if fSendAddressResolution:
                DpsCollection.address_resolution_queue.put((self.send_resolution_work, (vIP_value, vIP_packed)))
            break
        return

    def send_resolution_work(self, vIP_tuple):
        '''
        This routine sends address resolution queries to all dps clients in all DVGs
        @attention: Do not call this routine with global lock held
        @param vIP_tuple: (vIP_value, vIP_packed)
        @type vIP_tuple: Tuple of (String or Integer, ByteArray)
        '''
        #log.warning('send_resolution_work: Enter\r')
        vIP_value = vIP_tuple[0]
        vIP_packed = vIP_tuple[1]
        DpsCollection.global_lock.acquire()
        while True:
            if not self.valid:
                break
            try:
                dvg_list = self.DVG_Hash.values()
            except Exception:
                dvg_list = []
            #Send Address Resolution to all DVGS
            #Drop Lock in between DVGs to allow other threads to run
            DpsCollection.global_lock.release()
            for dvg in dvg_list:
                DpsCollection.global_lock.acquire()
                try:
                    ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = dvg.ip_subnet_lookup(vIP_value)
                    if ret_val == DOVEStatus.DOVE_STATUS_OK:
                        dvg.send_address_resolution(vIP_packed)
                except Exception:
                    pass
                DpsCollection.global_lock.release()
            DpsCollection.global_lock.acquire()
            #Store self in Global Timer
            DpsCollection.Address_Resolution_Requests[self.unique_id] = self
            break
        DpsCollection.global_lock.release()
        #log.warning('send_resolution_work: Exit\r')
        return

    def mass_transfer_finish(self, dps_location, fcomplete, weight):
        '''
        This routine will be called by the Mass Transfer Callback when mass transfer
        of data to remote node is done.
        @param dps_location: The location of the remote node
        @type dps_location: IPAddressLocation
        @param fcomplete: Indicates if the transfer was complete or had to be cancelled
                          due to some issue
        @type fcomplete: False
        @param weight: The weight of the mass transfer
        @type weight: Integer
        '''
        message = 'Domain %d: Mass Transfer finished to %s, completed %s'%(self.unique_id,
                                                                           dps_location.show_ip(),
                                                                           fcomplete)
        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
        mass_transfer_complete = False
        DpsCollection.global_lock.acquire()
        while True:
            try:
                if self.mass_transfer is None:
                    break
                #Don't make self.mass_transfer None till activate has been exchanged
                #self.mass_transfer = None
                DpsCollection.mass_transfer_lock.acquire()
                try:
                    DpsCollection.MassTransfer_Active -= weight
                    if DpsCollection.MassTransfer_Active < 0:
                        message = 'Domain %s: Mass Transfer finished, accounting problem with mass transfer count %s'
                        dcslib.dps_cluster_write_log(DpsLogLevels.CRITICAL, message)
                except Exception:
                    pass
                DpsCollection.mass_transfer_lock.release()
                if fcomplete and self.valid:
                    self.mass_transfer_forward_nodes[dps_location.ip_value] = dps_location
                    mass_transfer_complete = True
            except Exception:
                pass
            break
        DpsCollection.global_lock.release()
        if mass_transfer_complete:
            message = 'Domain %s: Mass Transfer Complete, sending Activate message to remote Node %s'%(self.unique_id, 
                                                                                                       dps_location.show_ip())
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            ret = dcslib.dps_domain_activate_on_node(dps_location.ip_value_packed, 
                                                     self.unique_id, 
                                                     self.replication_factor)
            if ret != 0:
                message = 'Domain %s: Activate on Node %s failed, clearing local forwarding cache'%(self.unique_id, 
                                                                                                    dps_location.show_ip())
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                try:
                    del self.mass_transfer_forward_nodes[dps_location.ip_value]
                except Exception:
                    pass
        else:
            message = 'Domain %s: Mass Transfer canceled, sending Deactivate message to remote Node %s'%(self.unique_id, 
                                                                                                         dps_location.show_ip())
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            ret = dcslib.dps_domain_deactivate_on_node(dps_location.ip_value_packed, 
                                                       self.unique_id)
            if ret != 0:
                message = 'Domain %s: Deactivate on Node %s Failed'%(self.unique_id,
                                                                     dps_location.show_ip())
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
        DpsCollection.global_lock.acquire()
        self.mass_transfer = None
        DpsCollection.global_lock.release()
        return

    def mass_transfer_processing(self):
        '''
        This routine determines if the local node is in the midst of mass
        transfer processing
        @return - True if mass transfer is underway
        @rtype - Boolean
        '''
        if self.mass_transfer is not None or len(self.mass_transfer_forward_nodes) > 0:
            return True
        else:
            return False

    def send_vm_migration_update_to(self, dps_client, endpoint, vnid, query_id):
        if not self.active:
            return
        if query_id == 0:
            query_id_use = DpsCollection.generate_query_id()
        else:
            query_id_use = query_id
        # to send to dcslib.send_all_vm_migration_update ()
        vipv4_list = []
        vipv6_list = []
        pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
        pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
        for vIP_obj in endpoint.vIP_set.values():
            if vIP_obj.fValid:
                if vIP_obj.inet_type == socket.AF_INET:
                    vipv4_list.append(vIP_obj.ip_value)
                elif vIP_obj.inet_type == socket.AF_INET6:
                    vipv6_list.append(vIP_obj.ip_value)
        ret_val = dcslib.send_all_vm_migration_update(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                      dps_client.location.port, #DPS Client Port
                                                      vnid, #VNID ID
                                                      query_id_use,
                                                      endpoint.dvg.unique_id,
                                                      endpoint.version,
                                                      pipv4_list,
                                                      pipv6_list,
                                                      endpoint.vMac,
                                                      vipv4_list,
                                                      vipv6_list)
        if ret_val != 0: 
            message = 'send_all_vm_migration_update FAILED with value %d'%(ret_val)
            dcslib.dps_data_write_log(DpsLogLevels.NOTICE, message)
        return

    def send_vm_migration_update(self, endpoint, vnid):
        '''
        This routine is used to send a vm migration update to all the DPS clients
        handling the domain.
        '''
        if not self.active:
            return
        DpsCollection.global_lock.acquire()
        #Create a set of dps_clients
        dps_clients = {}
        for dps_client in self.DPSClients_Hash_IPv4.values():
            dps_clients[dps_client] = True
        for dps_client in self.DPSClients_Hash_IPv6.values():
            dps_clients[dps_client] = True
        for dps_client in dps_clients.keys():
            self.send_vm_migration_update_to(dps_client, endpoint, vnid, 0)
        dps_clients.clear()
        DpsCollection.global_lock.release()
        return

    def show(self, fdetails):
        '''
        Display Contents of a Domain
        @param fdetails: Whether to show detailed information
        @type fdetails: Integer (0 = False, 1 = True)
        '''
        print '------------------------------------------------------------------\r'
        print 'Showing Domain: %s\r'%(self.unique_id)
        #############################################################
        #Show list of DVGs.
        #############################################################
        dvg_list = ''
        for dvg in self.DVG_Hash.values():
            dvg_list += str(dvg.unique_id) + ', '
        dvg_list = dvg_list.rstrip()
        dvg_list = dvg_list.rstrip(',')
        print 'VNIDs: [%s]\r'%(dvg_list)
        ##############################################################
        ##Show number of DPS Clients
        ##############################################################
        dps_client_count = len(self.DPSClients_Hash_IPv4) + len(self.DPSClients_Hash_IPv6)
        if fdetails and dps_client_count > 0:
            print '------------------------- DPS Clients ----------------------------\r'
            for dps_clients in self.DPSClients_Hash_IPv4.values():
                dps_clients.show()
            for dps_clients in self.DPSClients_Hash_IPv6.values():
                dps_clients.show()
            print '------------------------------------------------------------------\r'
        else:
            print 'DPS Clients: Total %s\r'%dps_client_count
        # #############################################################
        # #Show number of tunnels.
        # #############################################################
        # tunnel_count = len(self.Tunnel_Endpoints_Hash_IPv4)+ len(self.Tunnel_Endpoints_Hash_IPv6)
        # if fdetails and tunnel_count > 0:
        #     tunnel_set = {}
        #     for tunnel in self.Tunnel_Endpoints_Hash_IPv4.values():
        #         tunnel_set[tunnel] = True
        #     for tunnel in self.Tunnel_Endpoints_Hash_IPv6.values():
        #         tunnel_set[tunnel] = True
        #     print '---------------------- Tunnel Endpoints --------------------------\r'
        #     for tunnel in tunnel_set.keys():
        #         tunnel.show_details()
        #     tunnel_set.clear()
        #     print '------------------------------------------------------------------\r'
        # else:
        #     print 'Tunnels: Total %s\r'%tunnel_count
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
        #############################################################
        #Show Implicit Gateways.
        #############################################################
        count = len(self.ImplicitGatewayIPListv4.ip_hash) + len(self.ImplicitGatewayIPListv6.ip_hash)
        if fdetails and count > 0:
            print '------------------------- Implicit Gateways ----------------------\r'
            if len(self.ImplicitGatewayIPListv4.ip_hash) > 0:
                self.ImplicitGatewayIPListv4.show()
            if len(self.ImplicitGatewayIPListv6.ip_hash) > 0:
                self.ImplicitGatewayIPListv6.show()
        else:
            print 'Implicit Gateways: Total %s\r'%count
        ##############################################################
        ##Show Dedicated Subnet Lists
        ##############################################################
        if self.IP_Subnet_List.count > 0:
            print 'Subnet List: Total %s\r'%self.IP_Subnet_List.count
            self.ip_subnet_show()
        #############################################################
        #Show Policies.
        #############################################################
        count = len(self.Policy_Hash_DVG[0])
        if fdetails and count > 0:
            print '----------------------- Unicast Policies -------------------------\r'
            for policy in self.Policy_Hash_DVG[0].values():
                if policy.src_dvg == policy.dst_dvg and policy.action_connectivity == policy.action_forward:
                    continue
                print '%s\r'%policy.show()
        else:
            print 'Policy Count %s\r'%count
        #############################################################
        #Show Nodes to whom requests are being forwarded and
        #Mass Transfer
        #############################################################
        if self.mass_transfer is not None:
            print '\r'
            print 'Ongoing mass transfer to %s\r'%self.mass_transfer.remote_location.show()
        if len(self.mass_transfer_forward_nodes) > 0:
            for remote_location in self.mass_transfer_forward_nodes.values():
                print 'Forwarding Requests to DPS Node %s\r'%remote_location.show()
        return

    def show_mass_transfer(self):
        '''
        '''
        if self.mass_transfer is not None:
            print '\r'
            print 'Ongoing mass transfer to %s, Stage %s\n'%(self.mass_transfer.remote_location.show_ip(),
                                                             self.mass_transfer.stage_get())
            self.mass_transfer.show()
        if len(self.mass_transfer_forward_nodes) > 0:
            for remote_location in self.mass_transfer_forward_nodes.values():
                print 'Forwarding Requests to DPS Node %s\r'%remote_location.show()
        return
#Add Class to List of supported Classes
Domain.add_class()
