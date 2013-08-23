'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas
'''

import logging
import socket
from logging import getLogger
log = getLogger(__name__)
import time

from dcs_objects.IPAddressLocation import IPAddressLocation
from object_collection import DpsCollection
from object_collection import DpsClientType

import dcslib

class DPSClientHost:
    '''
    This represents the DPS Client which is communicating via the DPS
    Client Server Protocol.
    @attention: A DCSHostClient is not associated with any specific domain
                (unlike the DPSClient object). This object is maintained to 
                keep in contact with that Host via heartbeat messages
    '''
    Collection = {}

    #The frequency (seconds) at which to send heartbeat to DPS Clients
    heartbeat_frequency = {DpsClientType.dove_switch: 60,
                           DpsClientType.external_gateway: 3,
                           DpsClientType.vlan_gateway: 20
                           }

    #A maximum of no contact time before DPS Client is marked as down
    heartbeat_no_contact = {DpsClientType.dove_switch: 3600,
                            DpsClientType.external_gateway: 20,
                            DpsClientType.vlan_gateway: 600}

    def __init__(self, domain, ip_type, ip_val, port, client_type):
        '''
        Constructor:
        This routine assumes that the IP address of the DPS Client 
        uniquely identifies the DPS Client.
        @param domain: The Domain object for which this DPSHostClient was first 
                       created
        @type domain: Domain
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param port: The DPS Client Port (for UDP communication)
        @type port: Integer
        @param client_type: What kind of Client is it
        @type client_type: Should be one of DpsClientType
        '''
        self.frequency = self.heartbeat_frequency[client_type]
        self.domains = {}
        self.domains[domain.unique_id] = domain
        #DPS Client Location i.e. the UDP Port and Location
        self.location = IPAddressLocation(ip_type, ip_val, port)
        self.client_type = client_type
        self.client_types = {}
        self.client_types[client_type] = True
        self.last_contact = time.time()
        #print '%s added client_type %s\r'%(self.location.show_ip(), DpsClientType.types[client_type])
        #Add self to global collection
        try:
            DPSClientHost.Collection[ip_val] = self
        except Exception:
            pass
        self.valid = True

    def update_add(self, domain, port, client_type):
        '''
        This routine updates the dps client with the parameters
        @param domain: The Domain object for which this DPSHostClient was first 
                       created
        @type domain: Domain
        @param port: The DPS Client Port (for UDP communication)
        @type port: Integer
        @param client_type: What kind of Client is it
        @type client_type: Should be one of DpsClientType
        '''
        if not self.valid:
            return
        self.domains[domain.unique_id] = domain
        self.location.port = port
        self.client_types[client_type] = True
        #Get the client type with the highest (lowest in time difference) frequency heartbeat requirement
        try:
            if DPSClientHost.heartbeat_frequency[client_type] < DPSClientHost.heartbeat_frequency[self.client_type]:
                self.client_type = client_type
                print '%s changing client_type to %s\r'%(self.location.show_ip(), DpsClientType.types[client_type])
        except Exception:
            self.client_type = client_type
        return

    def update_delete(self, domain, client_type):
        '''
        This routine updates the dps client with the parameters
        @param domain: The Domain object for which this DPSHostClient was first 
                       created
        @type domain: Domain
        @param port: The DPS Client Port (for UDP communication)
        @type port: Integer
        @param client_type: What kind of Client is it
        @type client_type: Should be one of DpsClientType
        '''
        if not self.valid:
            return
        try:
            del self.domains[domain.unique_id]
        except Exception:
            pass
        try:
            del self.client_types[client_type]
        except Exception:
            pass
        if len(self.domains) == 0 or len(self.client_types) == 0:
            self.delete()
            return
        #Get the client type with the highest (lowest in time difference) frequency heartbeat requirement
        try:
            client_types = self.client_types.keys()
            self.client_type = client_types[0]
            client_types = client_types[1:]
            for client_type in client_types:
                if DPSClientHost.heartbeat_frequency[client_type] < DPSClientHost.heartbeat_frequency[self.client_type]:
                    self.client_type = client_type
        except Exception:
            pass
        return

    def touch(self):
        '''
        This routine touches the DPS Client
        '''
        self.last_contact = time.time()
        return

    def domain_add(self, domain):
        '''
        This routine add a domain to the DPSClientHost
        @param domain: The Domain object
        @type domain: Domain
        '''
        self.domains[domain.unique_id] = domain
        return

    def domain_delete(self, domain):
        '''
        This routine deletes the domain from the DPSClientHost
        @param domain: The Domain object
        @type domain: Domain
        '''
        try:
            del self.domains[domain.unique_id]
        except Exception:
            pass
        if len(self.domains) == 0:
            self.delete()
        return

    def vnid_random_get(self):
        '''
        This gets a random vnid to send the heartbeat to.
        @raise exception: If no VNID is found
        '''
        domain = self.domains.values()[0]
        vnid = domain.vnid_random_get()
        return vnid

    def delete(self):
        '''
        This routine deletes the object
        '''
        if not self.valid:
            return
        self.valid = False
        domains = self.domains.values()
        self.domains.clear()
        for domain in domains:
            try:
                if self.location.inet_type == socket.AF_INET:
                    dps_client_domain = domain.DPSClients_Hash_IPv4[self.location.ip_value]
                else:
                    dps_client_domain = domain.DPSClients_Hash_IPv6[self.location.ip_value]
                dps_client_domain.delete()
            except Exception:
                continue
        try:
            del DPSClientHost.Collection[self.location.ip_value]
        except Exception:
            pass
        return

    @staticmethod
    def Host_Add(domain, ip_type, ip_val, port, client_type):
        '''
        This is the routine calling routines should invoke to add a DPS Client
        to the collection
        @attention: This routine must be called with the global lock held
        @param domain: The Domain object
        @type domain: Domain
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param port: The DPS Client Port (for UDP communication)
        @type port: Integer
        @param client_type: What kind of Client is it
        @type client_type: Should be one of DpsClientType
        '''
        #print 'Host_Add: Enter\r'
        try:
            dps_client = DPSClientHost.Collection[ip_val]
            dps_client.update_add(domain, port, client_type)
        except Exception:
            dps_client = DPSClientHost(domain, ip_type, ip_val, port, client_type)
        #print 'Host_Add: Exit\r'
        return

    @staticmethod
    def Host_Delete(domain, ip_val, client_type):
        '''
        This is the routine calling routines should invoke to add a DPS Client
        to the collection
        @attention: This routine must be called with the global lock held
        @param domain: The Domain object
        @type domain: Domain
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param client_type: What kind of Client is it
        @type client_type: Should be one of DpsClientType
        '''
        try:
            dps_client = DPSClientHost.Collection[ip_val]
            dps_client.update_delete(domain, client_type)
        except Exception:
            pass
        return

    @staticmethod
    def Host_Delete_All(domain, ip_val):
        '''
        This is the routine calling routines should invoke to add a DPS Client
        to the collection
        @attention: This routine must be called with the global lock held
        @param domain: The Domain object
        @type domain: Domain
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        '''
        try:
            dps_client = DPSClientHost.Collection[ip_val]
            dps_client.delete()
        except Exception:
            pass
        return

    @staticmethod
    def Host_Touch(ip_val, port):
        '''
        This routine touches the Host
        @param ip_val: The value
        @param port: The port of the DPS Client
        '''
        try:
            dps_client = DPSClientHost.Collection[ip_val]
            if port != 0:
                dps_client.location.port = port
            dps_client.touch()
        except Exception:
            pass
        return

    @staticmethod
    def Domain_Deleted_Locally(domain):
        '''
        This routine should be called when a domain is deleted or de-activated
        from the local DPS Server Node
        @attention: This routine must be called with the global lock held
        @param domain: The Domain object
        @type domain: Domain
        '''
        try:
            dps_clients = DPSClientHost.Collection.values()
            for dps_client in dps_clients:
                try:
                    dps_client.domain_delete(domain)
                except Exception:
                    pass
        except Exception:
            pass
        return

    @staticmethod
    def Send_Heartbeat():
        '''
        This routine sends Heartbeats to all DPS Client which have not been
        contacted for the specified amount of time. 
        '''
        curr_time = time.time()
        try:
            dps_clients = DPSClientHost.Collection.values()
            for dps_client in dps_clients:
                try:
                    time_diff = int(curr_time - dps_client.last_contact)
                    client_type = dps_client.client_type
                    time_no_contact = DPSClientHost.heartbeat_no_contact[client_type]
                    time_heartbeat_send = DPSClientHost.heartbeat_frequency[client_type]
                    if time_diff > time_no_contact:
                        print 'No contact from DPS Client %s for %s seconds. Deleting!!!\r'%(dps_client.location.show_ip(),
                                                                                             time_diff)
                        dps_client.delete()
                    elif time_diff >= time_heartbeat_send:
                        vnid = dps_client.vnid_random_get()
                        query_id = DpsCollection.generate_query_id()
                        dcslib.send_heartbeat(dps_client.location.ip_value_packed,
                                              dps_client.location.port,
                                              vnid, query_id)
                except Exception:
                    continue
        except Exception:
            pass
        return

    @staticmethod
    def show():
        curr_time = time.time()
        print '%s %s\r'%('DCS Clients'.rjust(20), 'Age Time'.rjust(20))
        try:
            dps_clients = DPSClientHost.Collection.values()
            for dps_client in dps_clients:
                time_diff = int(curr_time - dps_client.last_contact)
                print '%s %s\r'%(dps_client.location.show_ip().rjust(20),str(time_diff).rjust(20))
        except Exception:
            pass
        return

