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

import socket
import struct

import threading
from threading import Lock
from threading import Timer

from object_collection import DpsCollection
from object_collection import DOVEStatus
from object_collection import DpsLogLevels

import dcslib

class DPSRetransmitHandler:
    '''
    This class represents the Retransmit Handler
    '''
    max_retransmit = 3
    max_queue_len = 8192

    def __init__(self):
        '''
        Class Initializer
        '''
        self.lock = threading.Lock()
        self.Query_ID_Hash = {}
        self.timer = Timer(5, self.Timer_Routine)
        self.timer.start()
        self.msg_size_total = 0
        self.retransmit_total = 0

    def Queue(self, query_id, msg, msg_size):
        '''
        This routine queues the message to the hash table
        @param query_id: The Query ID
        @type query_id: Integer
        @param msg: The Python Object containing the Message
        @type msg: PyObject
        @return: 0 on success, -1 on failure
        '''
        status = -1
        self.lock.acquire()
        while True:
            if len(self.Query_ID_Hash) > self.max_queue_len:
                #log.warning('Retransmit Queue Length %s exceeds max of %s\r', 
                #            len(self.Query_ID_Hash), self.max_queue_len)
                break
            try:
                exists = self.Query_ID_Hash[query_id]
                message = 'Query ID %s already exists in Timer Module'%query_id
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            except Exception:
                msg_tuple = (msg, self.max_retransmit, msg_size)
                self.Query_ID_Hash[query_id] = msg_tuple
                self.msg_size_total += msg_size
                status = 0
            break
        self.lock.release()
        return status

    def DeQueue(self, query_id):
        '''
        This routine returns the message object corresponding to query id and
        removes query_id from hash
        @param query_id: The Query ID
        @type query_id: Integer
        '''
        status = -1
        self.lock.acquire()
        try:
            msg_tuple = self.Query_ID_Hash[query_id]
            msg = msg_tuple[0]
            msg_size = msg_tuple[2]
            self.msg_size_total -= msg_size
            del self.Query_ID_Hash[query_id]
            status = 0
        except Exception, ex:
            msg = None
        self.lock.release()
        return (status, msg)

    def Generate_Query_Id(self):
        '''
        This routine generates a system wide unique id
        '''
        DpsCollection.global_lock.acquire()
        query_id = DpsCollection.generate_query_id()
        DpsCollection.global_lock.release()
        return query_id

    def Timer_Routine(self):
        '''
        This routine runs periodically and retransmits the pending messages
        and removes those message which have reached their expiration
        '''
        expired_msgs = []
        #print 'Timer_Routine: Total Messages %s, Total Size %s\r'%(len(self.Query_ID_Hash), self.msg_size_total)
        self.lock.acquire()
        for key in self.Query_ID_Hash.keys():
            try:
                msg_tuple = self.Query_ID_Hash[key]
                msg = msg_tuple[0]
                count = msg_tuple[1]
                msg_size = msg_tuple[2]
                if msg_tuple[1] == 0:
                    expired_msgs.append(msg_tuple[0])
                    self.msg_size_total -= msg_size
                    del self.Query_ID_Hash[key]
                else:
                    dcslib.retransmit_data(msg_tuple[0]) 
                    self.retransmit_total += 1
                    self.Query_ID_Hash[key] = (msg, count-1, msg_size)
            except Exception:
                continue
        self.lock.release()
        #Call the timeout on all expired messages
        for msg in expired_msgs:
            dcslib.retransmit_timeout(msg)
        self.timer = Timer(5, self.Timer_Routine)
        self.timer.start()
        return

    def Show(self):
        '''
        Show the details of the Retransmit Handler
        '''
        print '--------------- Retransmit Query IDs --------------\r'
        self.lock.acquire()
        try:
            query_ids = self.Query_ID_Hash.keys()
            if len(query_ids) > 0:
                print '--------------- Retransmit Query IDs --------------\r'
            for query_id in query_ids:
                try:
                    msg_tuple = self.Query_ID_Hash[query_id]
                    print 'Query ID: %s, Remaining Transmits %s, Size %s\r'%(query_id, msg_tuple[1], msg_tuple[2])
                except Exception, ex:
                    print 'Show: Exception %s\r'%ex
                    pass
            print 'Total Messages in Retransmit Queue %s\r'%len(self.Query_ID_Hash)
            print 'Total Number of Retransmitted Messages %s\r'%self.retransmit_total
        except Exception:
            pass
        self.lock.release()
        return
