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
from object_collection import DOVEGatewayTypes
from object_collection import DpsCollection
from object_collection import DOVEStatus
from object_collection import AssociatedType
from object_collection import IPSUBNETAssociatedType
from object_collection import DpsLogLevels

import struct
import time

import dcslib

class DpsControllerHandler(object):
    '''
    This class handles requests from the DOVE Controller. This code should 
    handle all Data Object Requests based DPS <-> Controller REST-based
    interactions.
    This class has routines which can be called from the C code. Routines
    that are not supposed to called from the C code MUST be marked with the
    following:
    @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
    '''
    #At max 4 domains can be recovered by one node. Each domain recovery
    #will have it's own thread
    Domain_Recovery_Max = 20
    #The controller protocol handler timer interval
    TIMER_INTERVAL = 5
    #Maximum time to complete a mass transfer
    MASS_TRANSFER_COMPLETE_MAX = TIMER_INTERVAL * 60 #5 minutes

    def __init__(self):
        '''
        Constructor:
        '''
        self.started = False
        #Collection of Domains Handled by this Node
        self.Domain_Hash = DpsCollection.Domain_Hash
        #Collection of VNIDs
        self.VNID_Hash = DpsCollection.VNID_Hash
        #Collection of All Domains and corresponding VNIDs
        self.Domain_Hash_Global = DpsCollection.Domain_Hash_Global
        #Collection of All Domains being recovered by this node
        self.Domain_Recovery = {}
        #Collection of All VNIDs being recovered by this Node
        self.VNID_Recovery = {}
        #Collection of All VNIDs which need trigger to DOVE switches to re-register
        self.VNID_Reregistration = {}
        #Collection of domains that have been marked for mass transfer receipt of
        #a domain.
        self.MassTransfer_Get_Ready = DpsCollection.MassTransfer_Get_Ready
        #Reference to Global Lock
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

    def Delete(self):
        '''
        This routine deletes all objects.
        '''
        #Mark self as deleted
        self.started = False
        self.VNID_Reregistration.clear()
        self.Domain_Delete_All_Local()
        self.lock.acquire()
        self.VNID_Hash.clear()
        self.Domain_Hash_Global.clear()
        self.cluster_db = None
        self.lock.release()
        return

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
            self.timer = Timer(5, self.Controller_Timer_Routine)
            self.timer.start()
            break
        self.lock.release()
        return

    def Controller_Timer_Routine(self):
        '''
        This routine is invoked perodically and handles all timer routines
        related to the DPS Controller Protocol Handler
        '''
        if not self.started:
            return
        self.VNID_Recovery_Timeout()
        self.Mass_Transfer_Get_Ready_Timeout()
        if self.started:
            self.timer = Timer(5, self.Controller_Timer_Routine)
            self.timer.start()
        return
