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

import time

from threading import Timer

import socket
from object_collection import DpsCollection
from object_collection import DOVEStatus
from object_collection import DOVEGatewayTypes
from object_collection import DpsTransactionType
from object_collection import IPSUBNETMode
from object_collection import DpsLogLevels
from object_collection import mac_show
from dcs_objects.IPAddressLocation import IPAddressLocation

import struct

import dcslib

class DpsClientHandler(object):
    '''
    This class handles requests from the DPS Clients. This code should handle 
    all Data Object Requests based on the DPS Client Server protocol.
    This class has routines which can be called from the C code. Routines
    that are not supposed to called from the C code MUST be marked with the
    following:
    @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
    '''

    #######################################################################
    #Error Codes based on the following structure: dps_resp_status_t
    #Described in dps_client_common.h
    #######################################################################
    #typedef enum {
    #} dps_resp_status_t;
    dps_error_none = 0
    dps_error_invalid_src_ip = 1
    dps_error_invalid_dst_ip = 2
    dps_error_invalid_src_dvg = 3
    dps_error_invalid_dst_dvg = 4
    dps_error_invalid_euid = 5
    dps_error_invalid_policy_id = 6
    dps_error_policy_tracking_mismatch = 7
    dps_error_invalid_domain_id = 8
    dps_error_no_memory = 9
    dps_error_no_internal_gw = 10
    dps_error_invalid_query_id = 11
    dps_error_no_broadcast_list = 12
    dps_bad_operation = 13
    dps_no_response = 14
    dps_error_endpoint_conflict = 15
    dps_error_retry = 16
    dps_error_no_route = 17
    dps_error_invalid_tunnel = 18

    #List of DPS Client Protocol Message Types
    DPS_ENDPOINT_LOC_REQ = 1
    DPS_ENDPOINT_LOC_REPLY = 2
    DPS_POLICY_REQ = 3
    DPS_POLICY_REPLY = 4
    DPS_POLICY_INVALIDATE = 5
    DPS_ENDPOINT_UPDATE = 6
    DPS_ENDPOINT_UPDATE_REPLY = 7
    DPS_ADDR_RESOLVE = 8
    DPS_ADDR_REPLY = 9
    DPS_INTERNAL_GW_REQ = 10
    DPS_INTERNAL_GW_REPLY = 11
    DPS_UNSOLICITED_VNID_POLICY_LIST = 12
    DPS_BCAST_LIST_REQ = 13
    DPS_BCAST_LIST_REPLY = 14
    DPS_VM_MIGRATION_EVENT = 15
    DPS_MCAST_SENDER_REGISTRATION = 16
    DPS_MCAST_SENDER_DEREGISTRATION = 17
    DPS_MCAST_RECEIVER_JOIN = 18
    DPS_MCAST_RECEIVER_LEAVE = 19
    DPS_MCAST_RECEIVER_DS_LIST = 20
    DPS_UNSOLICITED_BCAST_LIST_REPLY = 21
    DPS_UNSOLICITED_INTERNAL_GW_REPLY = 22
    DPS_GENERAL_ACK = 23
    DPS_UNSOLICITED_EXTERNAL_GW_LIST = 24
    DPS_UNSOLICITED_VLAN_GW_LIST = 25
    DPS_TUNNEL_REGISTER = 26,
    DPS_TUNNEL_DEREGISTER = 27
    DPS_TUNNEL_REG_DEREGISTER_ACK = 28
    DPS_EXTERNAL_GW_LIST_REQ = 29
    DPS_EXTERNAL_GW_LIST_REPLY = 30
    DPS_VLAN_GW_LIST_REQ = 31
    DPS_VLAN_GW_LIST_REPLY = 32
    DPS_UNSOLICITED_ENDPOINT_LOC_REPLY = 33
    DPS_VNID_POLICY_LIST_REQ = 34
    DPS_VNID_POLICY_LIST_REPLY = 35
    DPS_MCAST_CTRL_GW_REQ = 36
    DPS_MCAST_CTRL_GW_REPLY = 37
    DPS_UNSOLICITED_VNID_DEL_REQ = 38
    DPS_CTRL_PLANE_HB = 39
    DPS_GET_DCS_NODE = 40
    DPS_UNSOLICITED_INVALIDATE_VM = 41

    #List of destination endpoint data type
    dst_endpoint_data_type_vmac = 1
    dst_endpoint_data_type_vip = 2
    dst_endpoint_data_type_euid = 3

    #Timer Frequency
    Timer_Frequency = 5
    #Heartbeat Frequency
    Heartbeat_Frequency = 2

    def __init__(self):
        '''
        Constructor:
        '''
        self.started = False
        #Collection of Domains
        self.Domain_Hash = DpsCollection.Domain_Hash
        self.VNID_Hash = DpsCollection.VNID_Hash
        #Collection of Updates
        self.VNID_Broadcast_Updates = DpsCollection.VNID_Broadcast_Updates
        self.VNID_Broadcast_Updates_To = DpsCollection.VNID_Broadcast_Updates_To
        self.Policy_Updates_To = DpsCollection.Policy_Updates_To
        self.Gateway_Updates_To = DpsCollection.Gateway_Updates_To
        self.VNID_Multicast_Updates = DpsCollection.VNID_Multicast_Updates
        self.VNID_Multicast_Updates_To = DpsCollection.VNID_Multicast_Updates_To
        self.Address_Resolution_Requests = DpsCollection.Address_Resolution_Requests
        self.Address_Resolution_Requests_To = DpsCollection.Address_Resolution_Requests_To
        self.Conflict_Detection_Requests = DpsCollection.Conflict_Detection_Requests
        self.Endpoint_Expiration_Timer_Queue = DpsCollection.Endpoint_Expiration_Timer_Queue
        self.lock = DpsCollection.global_lock
        self.ip_get_val_from_packed = {socket.AF_INET6: self.ipv6_get_val_from_packed,
                                       socket.AF_INET: self.ipv4_get_val_from_packed}
        #List of VNIDs to query controller about
        self.vnid_query_controller = {}

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

    def Stop(self):
        '''
        This routine tells the protocol handler to stop processing new requests
        '''
        self.Delete()
        return

    def Start(self):
        '''
        This routine restarts the Procotol Handler
        '''
        self.lock.acquire()
        while True:
            if self.started:
                break
            #Start the Timer routine
            self.started = True
            self.timer = Timer(self.Timer_Frequency, self.Protocol_Timer_Routine)
            self.timer.start()
            break
        self.lock.release()
        return

    def Delete(self):
        '''
        This routine deletes all objects.
        '''
        self.lock.acquire()
        self.started = False
        self.VNID_Broadcast_Updates.clear()
        self.VNID_Broadcast_Updates_To.clear()
        self.Policy_Updates_To.clear()
        self.Gateway_Updates_To.clear() 
        self.VNID_Multicast_Updates.clear()
        self.VNID_Multicast_Updates_To.clear()
        self.Address_Resolution_Requests.clear()
        self.Address_Resolution_Requests_To.clear()
        self.Conflict_Detection_Requests.clear()
        self.lock.release()
        return

    def Protocol_Timer_Routine(self):
        '''
        This routine is invoked perodically and handles all timer routines
        related to the DPS Client Server Protocol Handler
        '''
        if not self.started:
            return
        self.timer = Timer(self.Timer_Frequency, self.Protocol_Timer_Routine)
        self.timer.start()
        return
