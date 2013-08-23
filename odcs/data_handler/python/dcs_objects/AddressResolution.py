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
import random

import dcslib

from dcs_objects.IPAddressLocation import IPAddressLocation
from object_collection import DpsCollection
from object_collection import DpsLogLevels

class AddressResolutionvIP:
    '''
    Represents the Address Resolution for a Virtual IP Address
    '''

    def __init__(self, vIP_type, vIP_value):
        '''
        Constructor:
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_value: The Virtual IP Address
        @type vIP_value: String (IPv6), Integer (IPv4)
        '''
        self.vIP = IPAddressLocation(vIP_type, vIP_value, 0)
        #List of Waiters which are waiting for answer on this Resolution
        #Key (DPS Client)
        self.dps_clients = {}
        #Total number of waiters
        self.total = 0
        #Retry is decremented in every cycle
        self.retry = 2

    def add(self, dps_client, dvg):
        '''
        This routine adds a DPS Client waiting for this vIP resolution
        @param dps_client: The DPS Client
        @type dps_client: DPSClient
        @param vnid: The DVG that asked for this endpoint
        @type vnid: Integer
        @return: True if waiter was actually added
        @return: Boolean
        '''
        try:
            waiter = self.dps_clients[dps_client]
        except:
            self.dps_clients[dps_client] = {}
            waiter = self.dps_clients[dps_client]
        try:
            exists = waiter[dvg.unique_id]
            return False
        except Exception:
            waiter[dvg.unique_id] = dvg
            self.total += 1
            return True

    def dps_client_delete(self, dps_client):
        '''
        This routine must be called whenever a dps_client is deleted from the
        domain
        @param dps_client: The DPS Client
        @type dps_client: DPSClient
        @return: The number of waiters deleted
        @rtype: Integer
        '''
        deleted = 0
        try:
            waiter = self.dps_clients[dps_client]
            deleted = len(waiter)
            self.total -= deleted
            waiter.clear()
            del self.dps_clients[dps_client]
        except Exception:
            pass
        return deleted

    def vnid_delete(self, dvg):
        '''
        This routine must be called whenever a VNID is deleted in the domain
        @param vnid: The VNID that asked for this endpoint
        @type vnid: DVG
        @return: The number of waiters deleted
        @rtype: Integer
        '''
        deleted = 0
        for waiter in self.dps_clients.values():
            try:
                del waiter[dvg.unique_id]
                deleted += 1
            except Exception:
                pass
        self.total -= deleted
        return deleted

    def resolved_work_item(self, endpoint):
        '''
        This is the actual routine which sends the Resolved message to all relevant
        DOVE Switches
        @param endpoint: The Endpoint Object Looked up
        @type endpoint: Endpoint
        '''
        pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
        pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
        for dps_client in self.dps_clients.keys():
            try:
                client = self.dps_clients[dps_client]
                dvgs = client.values()
            except Exception:
                pass
            for dvg in dvgs:
                DpsCollection.global_lock.acquire()
                if not dvg.valid:
                    DpsCollection.global_lock.release()
                    continue
                try:
                    dvg.send_endpoint_location_reply_to(dps_client.location.ip_value_packed,
                                                        dps_client.location.port,
                                                        endpoint.dvg.unique_id,
                                                        endpoint.version,
                                                        pipv4_list,
                                                        pipv6_list,
                                                        endpoint.vMac,
                                                        self.vIP.ip_value_packed #vIP
                                                       )
                except Exception, ex:
                    message = 'Address Resolution: Send location reply to Exception [%s]'%ex
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING,
                                                 message)
                DpsCollection.global_lock.release()
        self.dps_clients.clear()
        return

    def resolved(self, endpoint):
        '''
        This routine sends unsolicted endpoint location replies to all
        waiters
        @param endpoint: The Endpoint Object Looked up
        @type endpoint: Endpoint
        @return: The Number of entries that were resolved
        @rtype: Integer
        '''
        #Confirm that this is the correct match
        try:
            vIP_location = endpoint.vIP_set[self.vIP.ip_value]
        except Exception:
            raise Exception('Incorrect Endpoint')
        #Put work item on the queue
        DpsCollection.address_resolution_queue.put((self.resolved_work_item, endpoint))
        return self.total

    def show(self):
        '''
        Show details of waiters
        '''
        print 'Waiting for vIP %s resolution\r'%self.vIP.show()
        i = 0
        for dps_client in self.dps_clients.keys():
            try:
                client = self.dps_clients[dps_client]
                vnids = client.keys()
            except Exception:
                pass
            print '    [%d] DPS Client %s, VNIDs %s\r'%(i, dps_client.location.show(), vnids)
        print '--------------------------------------\r'

    def timeout(self):
        '''
        Handle timeout
        @return: (True, number of entries) if deleted
        @rtype: Tuple (Boolean, Integer)
        '''
        if self.retry <= 0:
            entries = self.delete()
            return (True, entries)
        else:
            self.retry -= 1
            return (False, 0)

    def delete(self):
        '''
        Delete self
        '''
        total = self.total
        self.dps_clients.clear()
        self.retry = 0
        self.total = 0
        return total

class AddressResolution:
    '''
    Represents Address Resolution object for a Domain.
    '''

    def __init__(self, domain_id):
        '''
        Constructor:
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        self.domain_id = domain_id
        #Set of Endpoint Resolutions waiting to be resolved.
        #Keyed by IP value
        self.endpoint_resolution = {}
        #Total amount of waiting clients
        self.total = 0
        #Max amount of waiters allowed
        self.total_max = 32
        #Max amount of non-found vIP addresses
        self.vIP_not_found_max = 8
        #Mark as valid
        self.valid = True

    def process_lookup_not_found(self, dps_client, dvg, vIP_type, vIP_val):
        '''
        This routine process a Virtual IP Address that couldn't be found in
        the list of Endpoint
        @param dps_client: The DPS Client
        @type dps_client: DPSClient
        @param dvg: The sending VNID
        @type dvg: Integer
        @param vIP_val: Virtual IP Address
        @type vIP_val: Integer and String
        '''
        try:
            try:
                resolution_obj = self.endpoint_resolution[vIP_val]
            except Exception:
                resolution_obj = AddressResolutionvIP(vIP_type, vIP_val)
                self.endpoint_resolution[vIP_val] = resolution_obj
            added = resolution_obj.add(dps_client, dvg)
            if added:
                self.total += 1
        except Exception:
            pass
        return

    def dps_client_delete(self, dps_client):
        '''
        This routine must be called whenever a dps_client is deleted from the
        domain
        @param dps_client: The DPS Client
        @type dps_client: DPSClient
        '''
        for resolution_obj in self.endpoint_resolution.values():
            self.total -= resolution_obj.dps_client_delete(dps_client)
        if self.total < 0:
            message = 'AddressResolution.dps_client_delete: Problem with accounting'
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def vnid_delete(self, dvg):
        '''
        This routine must be called whenever a VNID is deleted in the domain
        @param dvg: The VNID that asked for this endpoint
        @type dvg: DVG
        '''
        for resolution_obj in self.endpoint_resolution.values():
            self.total -= resolution_obj.vnid_delete(dvg)
        if self.total < 0:
            message = 'AddressResolution.vnid_delete: Problem with accounting'
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def process_endpoint_update(self, endpoint, vIP_val):
        '''
        This routine checks which DPS Client have been waiting for this endpoint
        update and sends resolution to those DPS Clients
        @param endpoint: The Endpoint being updated
        @type endpoint: Endpoint
        @param vIP_val: The vIP Value
        @type vIP_val: Integer or String
        '''
        while True:
            try:
                resolution_obj = self.endpoint_resolution[vIP_val]
                del self.endpoint_resolution[vIP_val]
            except Exception:
                break
            try:
                self.total -= resolution_obj.resolved(endpoint)
            except Exception, ex:
                message = 'AddressResolution.process_endpoint_update Problem Resolving Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            break
        if self.total < 0:
            message = 'AddressResolution.process_endpoint_update: Problem with accounting'
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def process_timeout(self):
        '''
        This routine processes timeouts and removes resolutions that have been waiting a
        long time for an answer
        '''
        for key in self.endpoint_resolution.keys():
            try:
                obj = self.endpoint_resolution[key]
                deleted, count = obj.timeout()
                if deleted:
                    self.total -= count
                    del self.endpoint_resolution[key]
            except Exception:
                pass
        return

    def delete(self):
        '''
        Delete self
        '''
        if not self.valid:
            return
        self.valid = False
        for key in self.endpoint_resolution.keys():
            try:
                obj = self.endpoint_resolution[key]
                self.total -= obj.delete()
                del self.endpoint_resolution[key]
            except Exception:
                pass
        if self.total != 0:
            message = 'AddressResolution.delete: Accounting Problem'
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def show(self):
        '''
        This routine shows all the waiters for this domain
        '''
        print '---------- Domain %s Address Resolution ---------\r'%(self.domain_id)
        for resolution_obj in self.endpoint_resolution.values():
            resolution_obj.show()
