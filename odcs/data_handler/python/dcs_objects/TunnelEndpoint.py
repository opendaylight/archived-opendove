'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas and jinghe
'''

import logging
import socket
from logging import getLogger
log = getLogger(__name__)
import struct

from dcs_objects import dcs_object
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.IPAddressList import IPAddressList
from dcs_objects.Domain import Domain
from dcs_objects.Dvg import DVG
from dcs_objects.DPSClientHost import DPSClientHost
from object_collection import DpsCollection
from object_collection import DpsClientType
from object_collection import DpsTransactionType
from object_collection import DpsLogLevels

import dcslib

class DPSClient:
    '''
    This represents the DPS Client which is communicating via the DPS
    Client Server Protocol. Each DPS Client is associated with one
    or more Tunnel Endpoints.
    NOTE: Each DPSClient will have multiple instances every every Domain
          it hosts TunnelEndpoint on. In other words if a DPSClient hosts
          TunnelEndpoint on Domain A and Domain B, then there will be 2
          independent instances of that DPSClient, 1 in Domain A and 1
          in Domain B.
    '''

    #A maximum of "max_failure" failures is tolerated from a DPS Client. After
    #that the DPS Client will have been deemed to be down and will be removed
    #from the collection
    failure_count_max = 4

    def __init__(self, domain, ip_type, ip_val, port):
        '''
        Constructor:
        This routine assumes that the IP address of the DPS Client 
        uniquely identifies the DPS Client.
        @param domain: The Domain object
        @type domain: Domain
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param port: The DPS Client Port (for UDP communication)
        @type port: Integer
        '''
        self.domain = domain
        #DPS Client Location i.e. the UDP Port and Location
        self.location = IPAddressLocation(ip_type, ip_val, port)
        #List of Tunnel Endpoints
        #This is to store the Tunnel Endpoint IP Address
        self.Tunnel_Endpoints_Hash_IPv4 = {}
        self.Tunnel_Endpoints_Hash_IPv6 = {}
        ##Failure count
        self.failure_count = self.failure_count_max
        self.version = 0
        self.valid = True
        #Add to Domain collection
        Domain.dps_client_add(self.domain, self)
        #self.show(None)

    def tunnel_endpoint_add(self, tunnel_endpoint):
        '''
        Add a Tunnel Endpoint to the collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        '''
        ip_list = tunnel_endpoint.ip_listv4.ip_list
        for ip_value in ip_list:
            self.Tunnel_Endpoints_Hash_IPv4[ip_value] = tunnel_endpoint
        ip_list = tunnel_endpoint.ip_listv6.ip_list
        for ip_value in ip_list:
            self.Tunnel_Endpoints_Hash_IPv6[ip_value] = tunnel_endpoint
        return

    def tunnel_endpoint_delete(self, tunnel_endpoint):
        '''
        Removes a Tunnel Endpoint from the collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
        '''
        #log.info('DPSClient.tunnel_endpoint_delete %s', tunnel_endpoint.primary_ip().show())
        ip_list = tunnel_endpoint.ip_listv4.ip_list
        for ip_value in ip_list:
            try:
                del self.Tunnel_Endpoints_Hash_IPv4[ip_value]
            except Exception:
                pass
        ip_list = tunnel_endpoint.ip_listv6.ip_list
        for ip_value in ip_list:
            try:
                del self.Tunnel_Endpoints_Hash_IPv6[ip_value]
            except Exception:
                pass
        if ((len(self.Tunnel_Endpoints_Hash_IPv4) == 0) and 
            (len(self.Tunnel_Endpoints_Hash_IPv6) == 0)):
            #log.info('DPSClient.tunnel_endpoint_delete: No tunnel endpoints')
            #No other tunnels attached
            Domain.dps_client_delete(self.domain, self)
        #self.show(None)

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
        Deletes a Tunnel Endpoint Physical IP Address to the Domain Collection
        @param tunnel_endpoint: The Tunnel Endpoint
        @type tunnel_endpoint: TunnelEndpoint Object
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
        if ((len(self.Tunnel_Endpoints_Hash_IPv4) == 0) and 
            (len(self.Tunnel_Endpoints_Hash_IPv6) == 0)):
            #log.info('DPSClient.tunnel_endpoint_delete: No tunnel endpoints')
            #No other tunnels attached
            Domain.dps_client_delete(self.domain, self)
        return

    def delete(self):
        '''
        Destructor:
        '''
        if not self.valid:
            return
        self.valid = False
        #Remove from GlobalCollection
        DPSClientHost.Host_Delete_All(self.domain, self.location.ip_value)
        #Remove self from VNID_Broadcast_Updates_To
        for key in DpsCollection.VNID_Broadcast_Updates_To.keys():
            try:
                tuple_value = DpsCollection.VNID_Broadcast_Updates_To[key]
                dps_client = tuple_value[1]
                if dps_client == self:
                    del DpsCollection.VNID_Broadcast_Updates_To[key]
            except Exception:
                pass
        #Remove self from DPS Client Updates
        for key in DpsCollection.Policy_Updates_To.keys():
            try:
                tuple_value = DpsCollection.Policy_Updates_To[key]
                dps_client = tuple_value[1]
                if dps_client == self:
                    del DpsCollection.Policy_Updates_To[key]
            except Exception:
                pass
        #Remove from Gateway_Updates_To
        for key in DpsCollection.Gateway_Updates_To.keys():
            try:
                tuple_value = DpsCollection.Gateway_Updates_To[key]
                dps_client = tuple_value[1]
                if dps_client == self:
                    del DpsCollection.Gateway_Updates_To[key]
            except Exception:
                pass
        #Remove from VNID_Multicast_Updates_To
        for key in DpsCollection.VNID_Multicast_Updates_To.keys():
            try:
                tuple_value = DpsCollection.VNID_Multicast_Updates_To[key]
                dps_client = tuple_value[1]
                if dps_client == self:
                    del DpsCollection.VNID_Multicast_Updates_To[key]
            except Exception:
                pass
        #Remove from Address_Resolution_Requests_To
        for key in DpsCollection.Address_Resolution_Requests_To.keys():
            try:
                tuple_value = DpsCollection.Address_Resolution_Requests_To[key]
                dps_client = tuple_value[1]
                if dps_client == self:
                    del DpsCollection.Address_Resolution_Requests_To[key]
            except Exception:
                continue
        for tunnel in self.Tunnel_Endpoints_Hash_IPv4.values():
            tunnel.delete()
        for tunnel in self.Tunnel_Endpoints_Hash_IPv6.values():
            tunnel.delete()
        Domain.dps_client_delete(self.domain, self)

    def show(self):
        '''
        Display Details
        '''
        print 'DPS Client: %s\r'%(self.location.show())
        print '---------------------- Tunnel Endpoints --------------------------\r'
        #############################################################
        #Show list of tunnels.
        #############################################################
        #First create a unique tunnel list
        tunnel_set = {}
        for tunnel in self.Tunnel_Endpoints_Hash_IPv4.values():
            tunnel_set[tunnel] = True
        for tunnel in self.Tunnel_Endpoints_Hash_IPv6.values():
            tunnel_set[tunnel] = True
        for tunnel in tunnel_set.keys():
            tunnel.show()
        tunnel_set.clear()
        print '------------------------------------------------------------------\r'
        return

class TunnelEndpoint:
    '''
    This represents a DOVE Underlay Tunnel in the DOVE environment -
    i.e DOVE Switch or VLAN Gateway.
    NOTE: Each Tunnel Endpoint will have multiple instances every every Domain
          it hosts Endpoints on. In other words if a TunnelEndpoint hosts
          endpoints on Domain A and Domain B, then there will be 2
          independent instances of that Tunnel Endpoint, 1 in Domain A and 1
          in Domain B.
    '''

    def __init__(self, fregister, domain, dvg, client_type, transaction_type, dps_client, pip_tuple_list):
        '''
        Constructor:
        This routine assumes that the 1st IP address in the tuple uniquely
        identifies a DOVE Switch/VLAN Gateway within the Domain.
        @param fregister: Whether this is an explicit tunnel register
        @param fregister: Boolean
        @param domain: The Domain object
        @type domain: Domain
        @param dvg: The DVG/VNID object
        @type dvg: DVG
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param dps_client: The DPS Client associated with this Tunnel
        @type dps_client: DPSClient
        @type dps_client_port: Integer
        @param pip_tuple_array: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_array: List of tuples
        '''
        if len(pip_tuple_list) == 0:
            raise Exception('No IP Address Provided: Invalid DOVE Switch')
        try:
            client_string = DpsClientType.types[client_type]
        except Exception:
            raise Exception('Not a supported Tunnel Type')
        self.domain = domain
        #DPS Client Location i.e. the UDP Port and Location
        self.dps_client = dps_client
        #############################################################
        #Based on Object Model Chapter 5.3: DOVE Switch
        #############################################################
        #TODO: SQL Table of Policies: Requirement 1.3
        #Hash List of all Endpoints hashed by MAC: Requirement 1.4
        self.Endpoint_Hash_MAC = {}
        #A dictionary indexed by VNID IDs, the value is the number
        #of Endpoints on that DOVE Switch in a particular VNID/DVG
        self.DVG_Hash = {}
        inet_type = pip_tuple_list[0][0]
        ip_value = pip_tuple_list[0][1]
        #ip_location = IPAddressLocation(inet_type, ip_value, 0)
        #print 'Tunnel %s created DVG_Hash\r'%ip_location.show_ip()
        for ctype in DpsClientType.types.keys():
            self.DVG_Hash[ctype] = {}
        #Key IP_value: Value VNID
        self.IP_Hash_VNID = {}
        #print 'Tunnel %s created IP_Hash_VNID\r'%ip_location.show_ip()
        for ctype in DpsClientType.types.keys():
            self.IP_Hash_VNID[ctype] = {}
        #Key VNID: Value Hash of IPs
        self.VNID_Hash_IP = {}
        for ctype in DpsClientType.types.keys():
            self.VNID_Hash_IP[ctype] = {}
        #print 'Tunnel %s created VNID_Hash_IP\r'%ip_location.show_ip()
        #List of Physical IP Addresses
        self.ip_listv4 = IPAddressList(socket.AF_INET)
        self.ip_listv6 = IPAddressList(socket.AF_INET6)
        for i in range(len(pip_tuple_list)):
            inet_type = pip_tuple_list[i][0]
            ip_value = pip_tuple_list[i][1]
            self.ip_add(dvg, fregister, client_type, inet_type, ip_value, transaction_type)
        #############################################################
        #To be finished
        #############################################################
        #dcs_object.__init__(self, self.primary_ip().show())
        self.version = 0
        self.valid = True
        DPSClientHost.Host_Add(domain, dps_client.location.inet_type, 
                               dps_client.location.ip_value, dps_client.location.port, 
                               client_type)
        #print 'Tunnel %s created\r'%ip_location.show_ip()

    def endpoint_add(self, endpoint, transaction_type):
        '''
        Adds an Endpoint to the DOVE Tunnel
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        '''
        #log.info('Tunnel.endpoint_add')
        #self.show(None)
        self.Endpoint_Hash_MAC[endpoint.vMac] = endpoint
        #self.show()
        return

    def endpoint_del(self, endpoint):
        '''
        Deletes an Endpoint from the DOVE Tunnel Endpoint
        '''
        try:
            del self.Endpoint_Hash_MAC[endpoint.vMac]
        except Exception:
            pass
        #self.show()
        if self.domain.unique_id == 0:
            #Remove the Tunnel Endpoint if no other Endpoints exist for Domain 0
            self.delete_if_empty()
        return

    def ip_add(self, dvg, fregister, client_type, inet_type, ip_value, transaction_type):
        '''
        This adds a tunnel IP address
        @param dvg: The DVG on which this registration occurred
        @type dvg: DVG
        @param fregister: Whether this is an explicit tunnel register
        @param fregister: Boolean
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        '''
        while True:
            try:
                client_type_value = DpsClientType.types[client_type]
            except Exception:
                break
            self.DVG_Hash[client_type][dvg.unique_id] = dvg
            #Add to Tunnels List even though it may have come through other DVGs as well
            if inet_type == socket.AF_INET:
                if not self.ip_listv4.search(ip_value):
                    self.ip_listv4.add(inet_type, ip_value)
            elif inet_type == socket.AF_INET6:
                if not self.ip_listv4.search(ip_value):
                    self.ip_listv6.add(inet_type, ip_value)
            #log.info('ip_add: Adding to DPS Client %s', ip_value)
            #Add DVG to self.
            #Add IP to DPS Client
            DPSClient.tunnel_endpoint_add_IP(self.dps_client, self, inet_type, ip_value)
            #Add IP to this DVGs
            DVG.tunnel_endpoint_add_IP(dvg, fregister, self, client_type, inet_type, ip_value, transaction_type)
            try:
                vnid_hash_ip = self.VNID_Hash_IP[client_type][dvg.unique_id]
            except Exception:
                self.VNID_Hash_IP[client_type][dvg.unique_id] = {}
                vnid_hash_ip = self.VNID_Hash_IP[client_type][dvg.unique_id]
            vnid_hash_ip[ip_value] = True
            #Add DVGs to IP hash
            try:
                ip_hash_vnid = self.IP_Hash_VNID[client_type][ip_value]
            except Exception:
                self.IP_Hash_VNID[client_type][ip_value] = {}
                ip_hash_vnid = self.IP_Hash_VNID[client_type][ip_value]
            ip_hash_vnid[dvg.unique_id] = True 
            #Add IP to Domain
            #log.info('ip_add: Adding to domain %s', self.domain.unique_id)
            Domain.tunnel_endpoint_add_IP(self.domain, self, inet_type, ip_value)
            break
        return

    def ip_delete(self, dvg, client_type, inet_type, ip_value):
        '''
        This deletes a tunnel IP address from a DVG and client type
        @param dvg: The DVG on which this registration occurred
        @type dvg: DVG
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        '''
        fdeleteVNID = False
        fdeleteIP = False
        while True:
            try:
                client_type_value = DpsClientType.types[client_type]
            except Exception:
                break
            try:
                vnid_hash_ip = self.VNID_Hash_IP[client_type][dvg.unique_id]
                del vnid_hash_ip[ip_value]
                if len(vnid_hash_ip) == 0:
                    del self.VNID_Hash_IP[client_type][dvg.unique_id]
                    fdeleteVNID = True
            except Exception:
                pass
            try:
                ip_hash_vnid = self.IP_Hash_VNID[client_type][ip_value]
                del ip_hash_vnid[dvg.unique_id]
                if len(ip_hash_vnid) == 0:
                    del self.IP_Hash_VNID[client_type][ip_value]
                    fdeleteIP = True
            except Exception:
                pass
            if fdeleteVNID:
                #Remove Tunnel from DVG
                DVG.tunnel_endpoint_delete(dvg, self, client_type)
                try:
                    del self.DVG_Hash[client_type][dvg.unique_id]
                    if len(self.DVG_Hash[client_type]) == 0:
                        #print 'Delete all Endpoints with client_type %s\r'%client_type_value
                        self.delete_client_type(client_type)
                except Exception:
                    pass
            if not fdeleteIP:
                break
            if fdeleteIP:
                #Remove only the IP from the DVG
                DVG.tunnel_endpoint_delete_IP(dvg, client_type, inet_type, ip_value)
            #Check if IP is present in any client type
            ip_present = False
            for ctype in self.IP_Hash_VNID.keys():
                try:
                    ip_hash_vnid = self.IP_Hash_VNID[ctype][ip_value]
                    ip_present = True
                    break
                except Exception:
                    continue
            if ip_present:
                break
            #Actually delete the IP from Domain and DPS Clients since no DVGs have it
            if inet_type == socket.AF_INET:
                self.ip_listv4.remove(inet_type, ip_value)
            elif inet_type == socket.AF_INET6:
                self.ip_listv6.remove(inet_type, ip_value)
            #Remove IP to DPS Client
            DPSClient.tunnel_endpoint_delete_IP(self.dps_client, inet_type, ip_value)
            #Remove IP from Domain
            Domain.tunnel_endpoint_delete_IP(self.domain, inet_type, ip_value)
            #Delete Self if not IP Addresses are present
            if self.ip_listv4.count() == 0 and self.ip_listv6.count() == 0:
                self.delete()
            break
        return

    def ip_clear(self, inet_type, ip_value):
        '''
        This deletes a tunnel IP address from this Tunnel and all DVGS and types
        @param dvg: The DVG on which this registration occurred
        @type dvg: DVG
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param inet_type: AF_INET or AF_INET6
        @type inet_type: Integer
        @param ip_value: The IP address value
        @type ip_value: Integer or String
        '''
        while True:
            for client_type in DpsClientType.types.keys():
                #Delete IP from all matching VNIDs and Client Type
                for vnid in self.DVG_Hash[client_type].keys():
                    fRemoveVNID = False
                    try:
                        vnid_hash_ip = self.VNID_Hash_IP[client_type][vnid]
                        del vnid_hash_ip[ip_value]
                        if len(vnid_hash_ip) == 0:
                            del self.VNID_Hash_IP[client_type][vnid]
                            fRemoveVNID = True
                    except Exception:
                        pass
                    #Remove the IP from the DVG
                    try:
                        dvg = self.DVG_Hash[client_type][vnid]
                        DVG.tunnel_endpoint_delete_IP(dvg, client_type, inet_type, ip_value)
                        if fRemoveVNID:
                            del self.DVG_Hash[client_type][vnid]
                            if len(self.DVG_Hash[client_type]) == 0:
                                #print 'Delete all Endpoints with client_type %s\r'%client_type
                                self.delete_client_type(client_type)
                    except Exception:
                        pass
                #Delete IP completly from Tunnel
                try:
                    ip_hash_vnid = self.IP_Hash_VNID[client_type][ip_value]
                    ip_hash_vnid.clear()
                    del self.IP_Hash_VNID[client_type][ip_value]
                except Exception:
                    pass
            #Delete the IP from Domain and DPS Clients since no DVGs have it
            if inet_type == socket.AF_INET:
                self.ip_listv4.remove(inet_type, ip_value)
            elif inet_type == socket.AF_INET6:
                self.ip_listv6.remove(inet_type, ip_value)
            #Remove IP to DPS Client
            DPSClient.tunnel_endpoint_delete_IP(self.dps_client, inet_type, ip_value)
            #Remove IP from Domain
            Domain.tunnel_endpoint_delete_IP(self.domain, inet_type, ip_value)
            #Delete Self if not IP Addresses are present
            if self.ip_listv4.count() == 0 and self.ip_listv6.count() == 0:
                self.delete()
            break
        return

    def delete_if_empty(self):
        '''
        This routine checks if a DPS Client no longer hosts any Endpoint.
        If it doesn't, it unlinks itself from all the domain
        '''
        if len(self.Endpoint_Hash_MAC) != 0:
            return
        if not self.valid:
            return
        self.valid = False
        #Remove self from all DVGs
        for client_type in DpsClientType.types.keys():
            for dvg in self.DVG_Hash[client_type].values():
                DVG.tunnel_endpoint_delete(dvg, self, client_type)
        #Clear the VNID/DVG Hash
        self.DVG_Hash.clear()
        #Remove from DPS Client Collection
        DPSClient.tunnel_endpoint_delete(self.dps_client, self)
        #Remove from Domain Collection
        Domain.tunnel_endpoint_delete(self.domain, self)
        #Clear IPs
        self.ip_listv4.clear()
        self.ip_listv6.clear()
        return

    def delete(self):
        '''
        Destructor:
        '''
        #Lose references to all endpoint objects
        for endpoint in self.Endpoint_Hash_MAC.values():
            endpoint.delete()
        self.Endpoint_Hash_MAC.clear()
        #Now call delete_if_empty
        self.delete_if_empty()
        return

    def delete_client_type(self, client_type):
        '''
        Remove association with a particular client type
        '''
        for endpoint_key in self.Endpoint_Hash_MAC.keys():
            try:
                endpoint = self.Endpoint_Hash_MAC[endpoint_key]
            except Exception:
                continue
            if endpoint.client_type == client_type:
                endpoint.delete()
        return

    def primary_ip(self):
        '''
        This routine shows any one IP Address to represent the Tunnel
        '''
        try:
            ip = self.ip_listv4.get_primary()
            return IPAddressLocation(socket.AF_INET, ip, 0)
        except Exception:
            pass
        try:
            ip = self.ip_listv6.get_primary()
            return IPAddressLocation(socket.AF_INET6, ip, 0)
        except Exception:
            pass
        return IPAddressLocation(socket.AF_INET, 0, 0)

    def tunnel_types_supported(self):
        '''
        This routine lists the tunnel type hosted by this tunnel
        '''
        tunnel_types = []
        for tunnel_type in DpsClientType.types.keys():
            try:
                dvg_hash = self.DVG_Hash[tunnel_type]
                if len(dvg_hash) == 0:
                    continue
                tunnel_types.append(tunnel_type)
            except Exception:
                pass
        return tunnel_types

    @staticmethod
    def merge_tunnels(tunnel_primary, tunnel_secondary):
        '''
        This routine merges the content of secondary tunnel into the primary tunnel
        and deletes the secondary tunnel
        '''
        #Copy Endpoints
        tunnel_primary.Endpoint_Hash_MAC = dict(tunnel_primary.Endpoint_Hash_MAC.items() + tunnel_secondary.Endpoint_Hash_MAC.items())
        tunnel_secondary.Endpoint_Hash_MAC.clear()
        #Copy DVGs
        for ctype in DpsClientType.types.keys():
            tunnel_primary.DVG_Hash[ctype] = dict(tunnel_primary.DVG_Hash[ctype].items() + tunnel_secondary.DVG_Hash[ctype].items())
            tunnel_secondary.DVG_Hash[ctype].clear()
        #Copy 
        for ctype in DpsClientType.types.keys():
            tunnel_primary.IP_Hash_VNID[ctype] = dict(tunnel_primary.IP_Hash_VNID[ctype].items() + tunnel_secondary.IP_Hash_VNID[ctype].items())
            tunnel_secondary.IP_Hash_VNID[ctype].clear()
        #Key VNID: Value Hash of IPs
        for ctype in DpsClientType.types.keys():
            tunnel_primary.VNID_Hash_IP[ctype] = dict(tunnel_primary.VNID_Hash_IP[ctype].items() + tunnel_secondary.VNID_Hash_IP[ctype].items())
            tunnel_secondary.VNID_Hash_IP[ctype].clear()
        #List of Physical IP Addresses
        ipv4_values = tunnel_secondary.ip_listv4.ip_hash.keys()
        tunnel_secondary.ip_listv4.clear()
        for ip_value in ipv4_values:
            for ctype in DpsClientType.types.keys():
                for dvg in tunnel_primary.DVG_Hash[ctype].values():
                    tunnel_primary.ip_add(dvg, False, ctype, socket.AF_INET, ip_value, DpsTransactionType.normal)
        ip6_values = tunnel_secondary.ip_listv6.ip_hash.keys()
        tunnel_secondary.ip_listv6.clear()
        for ip_value in ip6_values:
            for ctype in DpsClientType.types.keys():
                for dvg in tunnel_primary.DVG_Hash[ctype].values():
                    tunnel_primary.ip_add(dvg, False, ctype, socket.AF_INET, ip_value, DpsTransactionType.normal)
        #Delete the Secondary Tunnel
        tunnel_secondary.delete()
        return

    @staticmethod
    def register(domain, dvg, client_type, transaction_type, dps_client, pip_tuple_list):
        '''
        This routine should be called when a set of tunnel endpoint IP Addresses
        are registered by the DPS
        Client Protocol Handler.
        @param domain: The Domain object
        @type domain: Domain
        @param dvg: The DVG/VNID object
        @type dvg: DVG
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param dps_client: The DPS Client associated with this Tunnel
        @type dps_client: DPSClient
        @type dps_client_port: Integer
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        '''
        #Find if a tunnel exists with existing pIP
        if len(pip_tuple_list) == 0:
            return
        tunnel_set = {}
        if ((client_type == DpsClientType.dove_switch) or 
            (client_type == DpsClientType.vlan_gateway) or
            (client_type == DpsClientType.external_gateway)):
            for pip_tuple in pip_tuple_list:
                inet_type = pip_tuple[0]
                pip_value = pip_tuple[1]
                #ip_location = IPAddressLocation(inet_type, pip_value, 0)
                #print 'Tunnel Registration: VNID %s, IP %s\r'%(dvg.unique_id, ip_location.show_ip())
                if inet_type == socket.AF_INET:
                    ip_hash = domain.Tunnel_Endpoints_Hash_IPv4
                else:
                    ip_hash = domain.Tunnel_Endpoints_Hash_IPv6
                try:
                    tunnel = ip_hash[pip_value]
                    if tunnel.dps_client != dps_client:
                        message = 'Tunnel %s migrated from DPS Client %s to %s\r'%(tunnel.primary_ip().show_ip(),
                                                                                   tunnel.dps_client.location.show_ip(),
                                                                                   dps_client.location.show_ip())
                        dcslib.dps_data_write_log(DpsLogLevels.NOTICE, message)
                        #Delete the Tunnel IP since it no longer belongs with this
                        #Tunnel object
                        tunnel.ip_clear(inet_type, pip_value)
                        continue
                    tunnel_set[tunnel] = True
                except Exception:
                    continue
            if len(tunnel_set) > 1:
                #More than 1 tunnels found corresponding to the IP Addresses
                #Merge them into a single tunnel
                tunnels = tunnel_set.keys()
                tunnel_primary = tunnels[0]
                tunnels_secondary = tunnels[1:]
                for tunnel_secondary in tunnels_secondary:
                    print 'TunnelEndpoint.merge tunnel_primary %s, tunnel_secondary %s\r'%(tunnel_primary, tunnel_primary)
                    TunnelEndpoint.merge_tunnels(tunnel_primary, tunnel_secondary)
                tunnel = tunnel_primary
            elif len(tunnel_set) == 1:
                tunnel = tunnel_set.keys()[0]
            else:
                tunnel = None
            if tunnel is not None:
                tunnel.DVG_Hash[client_type][dvg.unique_id] = dvg
                for pip_tuple in pip_tuple_list:
                    inet_type = pip_tuple[0]
                    pip_value = pip_tuple[1]
                    tunnel.ip_add(dvg, False, client_type, inet_type, pip_value, transaction_type)
                DPSClientHost.Host_Add(domain, dps_client.location.inet_type, 
                                       dps_client.location.ip_value, dps_client.location.port, 
                                       client_type)
            else:
                #Allocate a new tunnel
                tunnel = TunnelEndpoint(True, domain, dvg, client_type, transaction_type, dps_client, pip_tuple_list)
            #if tunnel is not None:
            #    tunnel.show_details()
        if client_type == DpsClientType.external_gateway:
            for pip_tuple in pip_tuple_list:
                inet_type = pip_tuple[0]
                pip_value = pip_tuple[1]
                try:
                    if inet_type == socket.AF_INET:
                        IPList = dvg.ExternalGatewayIPListv4
                    else:
                        IPList = dvg.ExternalGatewayIPListv6
                    IPList.add(inet_type, pip_value)
                except Exception:
                    pass
        return

    @staticmethod
    def unregister(domain, dvg, client_type, pip_tuple_list):
        '''
        This routine should be called when a set of tunnel endpoint IP 
        Addresses are unregistered by the DPS
        Client Protocol Handler.
        @param domain: The Domain object
        @type domain: Domain
        @param dvg: The DVG/VNID object
        @type dvg: DVG
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        '''
        #TODO: See if we need a DPS Client validation check
        if ((client_type == DpsClientType.dove_switch) or 
            (client_type == DpsClientType.vlan_gateway) or
            (client_type == DpsClientType.external_gateway)):
            for pip_tuple in pip_tuple_list:
                inet_type = pip_tuple[0]
                pip_value = pip_tuple[1]
                #Find the Tunnel from Domain Collection
                if inet_type == socket.AF_INET:
                    ip_hash = domain.Tunnel_Endpoints_Hash_IPv4
                else:
                    ip_hash = domain.Tunnel_Endpoints_Hash_IPv6
                try:
                    tunnel = ip_hash[pip_value]
                    try:
                        TunnelEndpoint.ip_delete(tunnel, dvg, client_type, inet_type, pip_value)
                        #tunnel.show_details()
                    except:
                        message = 'Tunnel.Unregister: Problem deleting IP %s'%pip_value
                        dcslib.dps_data_write_log(DpsLogLevels.WARNING, message)
                except Exception:
                    pass
        if client_type == DpsClientType.external_gateway:
            for pip_tuple in pip_tuple_list:
                inet_type = pip_tuple[0]
                pip_value = pip_tuple[1]
                try:
                    if inet_type == socket.AF_INET:
                        IPList = dvg.ExternalGatewayIPListv4
                    else:
                        IPList = dvg.ExternalGatewayIPListv6
                    IPList.remove(inet_type, pip_value)
                except Exception:
                    pass
        #TODO: Send update to all other Tunnels in Domain about this IP List unregister
        return

    def ip_tuple_list_get(self):
        '''
        Returns the List of IPs as IP Tuples
        @return - A list of tuples of Physical IP Addresses, each tuple is of 
                  the form (inet_type, ip_value).
                  The inet_type = socket type i.e. AF_INET or AF_INET6
                  The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        '''
        ip_tuple_list = []
        for ip in self.ip_listv4.ip_list:
            ip_tuple_list.append((socket.AF_INET, ip))
        for ip in self.ip_listv6.ip_list:
            ip_tuple_list.append((socket.AF_INET6, ip))
        return ip_tuple_list

    def ip_list_get(self):
        '''
        Returns List of IPs as a string
        '''
        string_ipv4 = self.ip_listv4.toString()
        string_ipv4 = string_ipv4.rstrip()
        string_ipv4 = string_ipv4.rstrip(',')
        string_ipv6 = self.ip_listv6.toString()
        string_ipv6 = string_ipv6.rstrip()
        string_ipv6 = string_ipv6.rstrip(',')
        string_ip_list = string_ipv4 + ', ' + string_ipv6
        string_ip_list = string_ip_list.rstrip()
        string_ip_list = string_ip_list.rstrip(',')
        return string_ip_list

    def show(self):
        '''
        Display Contents of a Tunnel
        '''
        try:
            string_ip = 'Tunnel IPs [%s]'%(self.ip_list_get())
            string_info = 'DPS Client %s: Endpoints %s'%(self.dps_client.location.show(), len(self.Endpoint_Hash_MAC))
            print '%s [%s]\r'%(string_ip, string_info)
        except Exception:
            pass
        return

    def show_details_client_type(self, client_type):
        '''
        Display the details of a Tunnel given a client_type
        '''
        try:
            vnid_hash = self.DVG_Hash[client_type]
            vnid_hash_ip = self.VNID_Hash_IP[client_type]
            ip_hash_vnid = self.IP_Hash_VNID[client_type]
            client_type_string = DpsClientType.types[client_type]
            print 'Tunnel Type %s\r'%client_type_string
            print 'VNIDS %s\r'%vnid_hash.keys()
            for vnid, vnid_ips in vnid_hash_ip.items():
                ip_list = IPAddressList(socket.AF_INET)
                for vnid_ip in vnid_ips:
                    try:
                        ip_list.add(socket.AF_INET, vnid_ip)
                    except Exception:
                        pass
                print 'VNID %s, PIPv4s %s\r'%(vnid, ip_list.toString())
            for ip, ip_vnids in ip_hash_vnid.items():
                try:
                    ip_packed = struct.pack(IPAddressList.fmts[socket.AF_INET], ip)
                    ip_string = socket.inet_ntop(socket.AF_INET, ip_packed)
                except Exception:
                    pass
                print 'PIPv4 %s, VNIDs %s\r'%(ip_string, ip_vnids.keys())
        except Exception:
            pass
        return

    def show_details(self):
        '''
        Display the details of a Tunnel
        '''
        try:
            print '------------------------------------------------------------------\r'
            string_ip = 'Tunnel IPs [%s]'%(self.ip_list_get())
            string_info = '     DPS Client %s: Endpoint Count %s'%(self.dps_client.location.show(), len(self.Endpoint_Hash_MAC))
            print '%s\r'%string_ip
            print '%s\r'%string_info
            for client_type in DpsClientType.types.keys():
                client_type_string = DpsClientType.types[client_type]
                if len(self.DVG_Hash[client_type]) == 0:
                    if len(self.VNID_Hash_IP[client_type]) != 0:
                        print 'ALERT! self.VNID_Hash_IP for %s exists but no VNID in tunnel\r'%client_type_string
                    if len(self.IP_Hash_VNID[client_type]) != 0:
                        print 'ALERT! self.IP_Hash_VNID for %s exists but no VNID in tunnel\r'%client_type_string
                    continue
                self.show_details_client_type(client_type)
        except Exception, ex:
            pass
        print '------------------------------------------------------------------\r'
