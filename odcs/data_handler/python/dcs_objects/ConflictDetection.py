'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas
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
from object_collection import DpsTransactionType

class ConflictDetectionvIP:
    '''
    Represents the Conflict Detection for a Virtual IP Address
    '''

    def __init__(self, domain_id, vIP_type, vIP_value):
        '''
        Constructor:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_value: The Virtual IP Address
        @type vIP_value: String (IPv6), Integer (IPv4)
        '''
        self.domain_id = domain_id
        self.vIP = IPAddressLocation(vIP_type, vIP_value, 0)
        #Set of Endpoints that claim to have this vIP
        self.endpoints_claim = {}
        #Set of Endpoint which actually have this vIP
        self.endpoints_registered = {}
        #Retry is decremented in every cycle
        self.retry = 2
        #Who owns the endpoint at this time
        self.endpoint_owner = None
        #log.warning('ConflictDetectionvIP: for IP %s\r', self.vIP.show())

    def endpoint_claim(self, endpoint, transaction_type):
        '''
        This routine adds a Endpoint that claims to have this vIP previously. This routine
        should be called from Endpoint_Find_and_Detect_Conflict routine
        @attention: This routine assumes the global lock is held when this routine
                    is called.
        @param endpoint: The Endpoint
        @type endpoint: Endpoint
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        '''
        #log.warning('endpoint_claim: vIP %s, transaction_type %s\r', 
        #            self.vIP.show(), transaction_type)
        #endpoint.show()
        while True:
            tunnel = endpoint.tunnel_endpoint
            if tunnel is None:
                break
            dps_client = tunnel.dps_client
            self.endpoints_claim[endpoint] = endpoint
            #Let the previous owner be the one to claim this IP.
            self.endpoint_owner = endpoint
            if transaction_type != DpsTransactionType.normal:
                break
            #Send address resolution request to the DPS Client
            query_id = DpsCollection.generate_query_id()
            ret_val = dcslib.send_address_resolution(dps_client.location.ip_value_packed, #DPS Client Location IP
                                                     dps_client.location.port, #DPS Client Port
                                                     endpoint.vnid, #VNID ID
                                                     query_id, #Query ID
                                                     self.vIP.ip_value_packed, #IP packed
                                                     endpoint.vMac #vMac
                                                     )
            if ret_val != 0 and len(DpsCollection.Address_Resolution_Requests_To) < DpsCollection.Max_Pending_Queue_Size:
                #Insert into a retry queue to be tried in the next iteration. In this case it
                #will be sent without a MAC address, but it's ok since this is a rare condition
                key = '%s:%s:%s'%(self.unique_id, dps_client.location.ip_value, self.vIP.ip_value_packed)
                DpsCollection.Address_Resolution_Requests_To[key] = (self, dps_client, self.vIP.ip_value_packed)
            break
        return

    def endpoint_register(self, endpoint):
        '''
        This routine should be called whenever an endpoint is registered with
        the vIP address
        @param endpoint: The Endpoint
        @type endpoint: Endpoint
        @return True: If this conflict processing is complete
        @rtype: Boolean
        '''
        #log.warning('endpoint_register: vIP %s\r', self.vIP.show())
        #endpoint.show()
        try:
            del self.endpoints_claim[endpoint]
        except Exception:
            pass
        self.endpoints_registered[endpoint] = endpoint
        if len(self.endpoints_claim) == 0:
            self.complete()
            return True
        else:
            return False

    def endpoint_delete(self, endpoint):
        '''
        This routine must be called whenever a endpoint is deleted from the
        domain
        @param endpoint: The Endpoint
        @type endpoint: Endpoint
        '''
        try:
            del self.endpoints_claim[endpoint]
        except Exception:
            pass
        try:
            del self.endpoints_registered[endpoint]
        except Exception:
            pass
        return

    def timeout(self):
        '''
        Handle timeout
        @return: True if the conflict detection timer has expired
        '''
        #log.warning('timeout: vIP %s\r', self.vIP.show())
        if self.retry <= 0:
            self.complete()
            return True
        else:
            self.retry -= 1
            return False

    def complete(self):
        '''
        This routine should be called when the timer has expired. A conflict
        is detected only if there are 2 or more endpoints which have registered
        the vIP AFTER there was a hint of Conflict.
        '''
        #log.warning('complete: vIP %s\r', self.vIP.show())
        #for endpoint in self.endpoints_registered.keys():
        #    endpoint.show()
        #Let the owner retain the endpoint
        endpoint_owner = self.endpoint_owner
        if len(self.endpoints_registered) > 1:
            #Conflict detected
            endpoints = self.endpoints_registered.keys()
            dcslib.report_endpoint_conflict(self.domain_id, #Domain ID
                                            endpoints[0].vnid, #VNID of 1st Endpoint
                                            endpoints[1].vnid, #VNID ID of 2nd Endpoint
                                            0, #Version - Unused
                                            self.vIP.ip_value_packed,
                                            endpoints[0].vMac,
                                            endpoints[0].tunnel_endpoint.primary_ip().ip_value_packed, 
                                            endpoints[0].tunnel_endpoint.dps_client.location.ip_value_packed,
                                            endpoints[1].vMac,
                                            endpoints[1].tunnel_endpoint.primary_ip().ip_value_packed, 
                                            endpoints[1].tunnel_endpoint.dps_client.location.ip_value_packed,
                                            )
            #Delete the vIP from all the other endpoints
            for endpoint in endpoints:
                if endpoint == endpoint_owner:
                    continue
                endpoint.vIP_del(self.vIP)
        elif len(self.endpoints_registered) == 1:
            endpoint_owner = self.endpoints_registered.keys()[0]
            #Give the vIP to the endpoint which registered the vIP
            endpoint = self.endpoints_registered.keys()[0]
            endpoint.vIP_add(self.vIP)
        #For the endpoints that were not registered with that vIP, 
        #remove the vIP from that endpoint
        for endpoint in self.endpoints_claim.keys():
            if endpoint == endpoint_owner:
                continue
            endpoint.vIP_del(self.vIP)
        #Give the vIP to the endpoint to the owner
        if endpoint_owner is not None:
            endpoint_owner.vIP_add(self.vIP)
        #Clear all entries
        self.endpoints_claim.clear()
        self.endpoints_registered.clear()
        return

    def delete(self):
        '''
        This routine
        '''
        #Clear all entries
        self.endpoints_claim.clear()
        self.endpoints_registered.clear()
        return

class ConflictDetection:
    '''
    Represents the Conflict Detection and Validation for a Domain.
    Possible conflicts are detected by the DPS Client Server Protocol in
    the Endpoint_Find_and_Detect_Conflict routine - At this point the
    conflict detection is simply a hint that there may be a problem.
    The Conflict needs to be verified by sending Address Resolution
    Requests to both the VMs which claim to have the vMac/vIp. If
    the answer returns from both VMs, there there is a conflict.
    '''

    def __init__(self, domain_id):
        '''
        Constructor:
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        self.domain_id = domain_id
        #Set of Conflict Detections waiting to be resolved.
        #Keyed by IP value
        self.conflict_detection_IP = {}
        ##Keyed by MAC values
        #self.conflict_detection_MAC = {}
        #Max amount of conflicts allowed
        self.total_max = 64
        #Mark as valid
        self.valid = True

    def endpoint_claim_vIP(self, endpoint, vIP_type, vIP_val, transaction_type):
        '''
        This routine should be called when an endpoint "claims" a vIP.
        An Endpoint "claims" a vIP if the vIP was registered on that
        endpoint before the "hint" of conflict happened.
        @param endpoint: The Endpoint Claiming this vIP
        @type endpoint: Endpoint
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_val: The IP address of the Endpoint
        @type vIP_val: Integer or String
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        '''
        while True:
            try:
                conflict = self.conflict_detection_IP[vIP_val]
            except Exception:
                if len(self.conflict_detection_IP) > self.total_max:
                    break
                conflict = ConflictDetectionvIP(self.domain_id, vIP_type, vIP_val)
                self.conflict_detection_IP[vIP_val] = conflict
            conflict.endpoint_claim(endpoint, transaction_type)
            #Add Domain to global collection waiting for conflict resolution
            DpsCollection.Conflict_Detection_Requests[self.domain_id] = self
            break
        return

    def endpoint_register_vIP(self, endpoint, vIP_type, vIP_val):
        '''
        This routine should be called when an endpoint register a vIP.
        An Endpoint register a vIP if the vIP was registered on that
        endpoint after the "hint" of conflict happened.
        @attention: There MUST be a conflict "claim" before a registration
        '''
        while True:
            try:
                conflict = self.conflict_detection_IP[vIP_val]
            except Exception:
                break
            complete = conflict.endpoint_register(endpoint)
            if complete:
                try:
                    del self.conflict_detection_IP[vIP_val]
                except Exception:
                    pass
            break
        return

    def endpoint_delete(self, endpoint):
        '''
        This routine must be called whenever a endpoint is deleted from the
        domain
        @param endpoint: The Endpoint
        @type endpoint: Endpoint
        '''
        for conflict in self.conflict_detection_IP.values():
            conflict.endpoint_delete(endpoint)
        return

    def process_timeout(self):
        '''
        This routine processes timeouts and removes resolutions that have been waiting a
        long time for an answer
        '''
        for key in self.conflict_detection_IP.keys():
            try:
                conflict = self.conflict_detection_IP[key]
                deleted = conflict.timeout()
                if deleted:
                    del self.conflict_detection_IP[key]
            except Exception:
                pass
        if len(self.conflict_detection_IP) > 0:
            #Add Domain to global collection waiting for conflict resolution
            DpsCollection.Conflict_Detection_Requests[self.domain_id] = self
        return

    def delete(self):
        '''
        Delete self
        '''
        if not self.valid:
            return
        self.valid = False
        for key in self.conflict_detection_IP.keys():
            try:
                conflict = self.conflict_detection_IP[key]
                conflict.delete()
                del self.conflict_detection_IP[key]
            except Exception:
                pass
        try:
            del DpsCollection.Conflict_Detection_Requests[self.domain_id]
        except Exception:
            pass
        return
