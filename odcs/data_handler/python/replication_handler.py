'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: hpadhye
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
from object_collection import DpsTransactionType
from object_collection import DOVEStatus
from cluster_database import ClusterDatabase
from client_protocol_handler import DpsClientHandler
from object_collection import DpsLogLevels

import dcslib

class DPSReplicationRequest:
    '''
    This class represents a single Replicated Request
    '''

    #Represents the number of times the timer can expire on this request
    Timer_Expiration_Count = 3

    def __init__(self, replication_query_id_list, reply_message):
        '''
        Initialization routine for the DPSReplicationRequest
        @param replication_query_id_list: The DCS Server Replication Query ID List
        @type replication_query_id_list: List []
        @param reply_message: The Reply message to be sent back to DPS Client 
                              associated with this Replication request
        @type reply_message: Packed Structure
        @param dps_server_list: List of Tuple (inet_type, ip_value_packed, port)
        @type dps_server_list: []
        '''
        self.timer_expiration_count = self.Timer_Expiration_Count
        self.query_ids = {}
        self.list = []
        for i in range(len(replication_query_id_list)):
            self.query_ids[replication_query_id_list[i]] = True
        self.reply_message = reply_message

    def process_replication_reply(self, query_id, error):
        '''
        This routine processes the replication reply
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
        @attention: This routine must be called with the global lock held
        @param query_id: The Query ID in the reply
        @type query_id: Integer
        @param error: Error in the DPS Client Server Message
        @type error: Integer
        @return: The List of query_ids which should be removed from the List
        @rtype: List
        '''
        send_reply_message = False
        list_remove = []
        resp_error = error
        if resp_error == DpsClientHandler.dps_error_none:
            try:
                del self.query_ids[query_id]
                list_remove.append(query_id)
                self.timer_expiration_count -= self.Timer_Expiration_Count
            except Exception:
                pass
            if len(self.query_ids) == 0:
                #All expected replies have been received
                send_reply_message = True
                self.timer_expiration_count = 0
        else:
            #Any other error send reply immediately
            send_reply_message = True
            #Remove all query IDs
            list_remove = self.query_ids.keys()
            self.query_ids.clear()
            ##Set response error to RETRY
            #resp_error = DpsClientHandler.dps_error_retry
        if send_reply_message and self.reply_message is not None:
            #log.warning('Sending reply for Query IDs %s\r', list_remove)
            dcslib.send_message_and_free(self.reply_message, resp_error)
            self.reply_message = None
        return list_remove

    def timer(self):
        '''
        This routine decides if this object needs to be deleted
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
        @attention: This routine must be called with the global lock held
        @return: list of query ids to delete from domain tracker
        @rtype: List
        '''
        self.timer_expiration_count -= 1
        if self.timer_expiration_count <= 0:
            if self.reply_message is not None:
                #log.warning('Timer Expiration: Sending reply for Query IDs %s\r', self.query_ids.keys())
                dcslib.send_message_and_free(self.reply_message, 
                                             DpsClientHandler.dps_error_retry)
            self.reply_message = None
            return self.query_ids.keys()
        else:
            return []

    def show(self):
        '''
        This routine shows the replication request
        '''
        return 'Replication Query IDs [%s]'%(self.query_ids.keys())

class DPSDomainReplicationTracker:
    '''
    This class initializes an instance of a per domain replication
    tracking.
    Note: A Client Query that is retried by the DPS client can 
          generate multiple replications. So we cap the number
          of outstanding requests per domain. We could have been
          more fancy and tried to coalesce the same client query
          into the same DPSReplicationRequest, but that would mean
          that if the Domain Mapping Changed, we would have to
          remove the old Domain Mapping form the DPSReplication
          request and added the new Mapping in.
          For the current scheme to work, the DCS Servers must
          ignore Replication requests which have a version lower
          that what they are expecting and simply return SUCCESS.
    '''

    #The total number of outstanding replication requests
    max_replication_requests = 4000

    def __init__(self, domain_id):
        self.domain_id = domain_id
        #A Hash to locate a Replication Request by Server Query ID
        self.Server_QueryID_Hash = {}

    def add_replication_request(self, replication_query_id_list, reply_message):
        '''
        This routine adds a DPSReplicationRequest to the Queue
        use the Replication Query ID to send out the replicated requests
        @param replication_query_id_list: The DCS Server Replication Query ID List
        @type replication_query_id_list: List []
        @param reply_message: A Pointer to the Reply Message which will be
                              used to send reply to the Client.
        @type reply_message: PyObject * (PYTHON OBJECT)
        @raise - If the Server Replication Key exists
        '''
        #Create the replication request first
        if len(replication_query_id_list) > self.max_replication_requests:
            raise Exception('add_replication_request: Too many outstanding requests')
        replication_request = DPSReplicationRequest(replication_query_id_list, reply_message)
        for i in range(len(replication_query_id_list)):
            try:
                existing_request = self.Server_QueryID_Hash[replication_query_id_list[i]]
                message = 'Server Replication Key %s Already exists - MAJOR Problem'%replication_query_id_list[i]
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                raise Exception('add_replication_request: Replication Query ID %s already exists'%(replication_query_id_list[i]))
            except Exception:
                pass
            self.Server_QueryID_Hash[replication_query_id_list[i]] = replication_request
        return

    def process_replication_reply(self, replication_query_id, error):
        '''
        This routine processes the replication reply
        @param replication_query_id: The Query ID of the Replication Message
        @type replication_query_id: Integer
        @param error: Error in the DPS Client Server Message
        @type error: Integer
        @return: None
        '''
        try:
            replication_request = self.Server_QueryID_Hash[replication_query_id]
            list_remove = replication_request.process_replication_reply(replication_query_id, error)
            #Remove the replication request since the message has been sent
            for query_id in list_remove:
                try:
                    del self.Server_QueryID_Hash[query_id]
                    #log.warning('Removing Query ID %s from List\r', query_id)
                except Exception:
                    pass
        except Exception:
            pass
        return

    def delete(self):
        '''
        The destroy routine clears all the messages in the waiting queue
        '''
        for query_id in self.Server_QueryID_Hash.keys():
            self.process_replication_reply(query_id, DOVEStatus.DOVE_STATUS_INVALID_DOMAIN)
        if len(self.Server_QueryID_Hash) > 0:
            #log.info('ALERT: DPSDomainReplicationTracker.delete did not clear automatically')
            self.Server_QueryID_Hash.clear()
        return

    def timer(self):
        '''
        This routine runs the replication timer for a particular Domain
        '''
        for query_id in self.Server_QueryID_Hash.keys():
            try:
                replication_object = self.Server_QueryID_Hash[query_id]
                list_remove = replication_object.timer()
                for rid in list_remove:
                    try:
                        del self.Server_QueryID_Hash[rid]
                        #log.warning('Removing Query ID %s from List\r', rid)
                    except Exception:
                        pass
            except Exception:
                pass

    def show(self):
        print '------------------------------------------------------------------\r'
        print 'Showing pending Replication requests for Domain %s'%(self.domain_id)
        for query_id in self.Server_QueryID_Hash.keys():
            print '%s\r'%(query_id)
        print '------------------------------------------------------------------\r'

class DPSReplicationTracker(object):
    '''
    This class stores the replication queries made and corresponding responses
    received. The queries are indexed based on queryId + dove switch IP.
    '''

    def __init__(self):
        #Get a reference to an instance of the Cluster Database
        self.cluster_db = ClusterDatabase()
        #Dictionary of DPSDomainReplicationTracker Object
        #When a domain is created or deleted, DPSDomainReplicationTracker
        #will be added or removed from the dictionary
        self.domain_tracker_set = DpsCollection.Domain_Replication_Requests
        while True:
            #Create a Global (All Domain Tracker Set if necessary)
            domain_id = DpsCollection.Shared_DomainID
            try:
                domain_tracker = self.domain_tracker_set[domain_id]
                break
            except Exception:
                pass
            self.domain_tracker_set[domain_id]= DPSDomainReplicationTracker(domain_id)
            break
        self.lock = DpsCollection.global_lock
        self.timer = Timer(6, self.ReplicationTimerProcess)
        self.timer.start()

    def ReplicationDetermineDPSNodesPYTHON(self, msg_domain_id, transaction_type):
        '''
        This routine will determine which DPS Nodes (including Local) should
        this message be sent to for replication purposes.
        DpsTransactionType.normal - Send to all nodes in the cluster handling
                                    this domain + other nodes to whom mass transfer
                                    has finished but other nodes don't know about
                                    it i.e. domain.mass_transfer_forward_nodes[]
        DpsTransactionType.replication - If the are nodes in domain.mass_transfer_forward_nodes[]
                                         then reply with RETRY, otherwise store on
                                         local node. This can be changed in the future
                                         to actually forward (and wait) request to
                                         mass_transfer_forward_nodes and reply SUCCESS.
                                         But for now RETRY is fine.
        DpsTransactionType.mass_transfer - Return local node
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        @param msg_domain_id: The Domain ID in the message
        @type msg_domain_id: Integer
        @param transaction_type: DpsTransactionType
        @type transaction_type: DpsTransactionType
        @return: (DOVEStatus, List of DCS Servers to send request to (replicate))
        @return: Each DCS Server in list is a tuple of (inet_type, ip_packed, port)
        '''
        nodes = []
        try:
            domain = DpsCollection.Domain_Hash[msg_domain_id]
        except Exception:
            domain = None
        ret_val = DpsClientHandler.dps_error_none
        if transaction_type == DpsTransactionType.normal:
            node_set = {}
            #Get Cluster Nodes
            try:
                cluster_nodes = self.cluster_db.cluster.Domain_Hash[msg_domain_id].values()
            except Exception, ex:
                #print 'Replication: Cannot get cluster_nodes for Domain %s\r'%msg_domain_id
                raise Exception(ex)
            for cluster_node in cluster_nodes:
                #Ignore nodes which are down
                if cluster_node.is_down():
                    continue
                node_set[cluster_node.location.ip_value] = (cluster_node.location.inet_type,
                                                            cluster_node.location.ip_value_packed,
                                                            cluster_node.location.port)
            #Get the nodes if any to foward to (after Mass Transfer)
            if domain is not None:
                forward_nodes = domain.mass_transfer_forward_nodes.values()
                for forward_node in forward_nodes:
                    node_set[forward_node.ip_value] = (forward_node.inet_type,
                                                       forward_node.ip_value_packed,
                                                       forward_node.port)
            nodes = node_set.values()
        elif transaction_type == DpsTransactionType.replication:
            if domain is not None:
                if len(domain.mass_transfer_forward_nodes) > 0:
                    #Force retry. We don't want the node to wait for 
                    #replies from the forwarded nodes.
                    message ='DCS: In forwarding mode after mass transfer, force registration retry'
                    dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                    ret_val = DpsClientHandler.dps_error_retry
                else:
                    #Return local node
                    local_node = self.cluster_db.cluster.Local.get()
                    nodes.append((local_node.location.inet_type,
                                  local_node.location.ip_value_packed,
                                  local_node.location.port))
        elif transaction_type == DpsTransactionType.mass_transfer:
            #Return local node
            local_node = self.cluster_db.cluster.Local.get()
            nodes.append((local_node.location.inet_type,
                          local_node.location.ip_value_packed,
                          local_node.location.port))
        return (ret_val, nodes)

    def ReplicationQueryIDGenerate(self, msg_domain_id, transaction_type, reply_message):
        '''
        This routine generates a Replication Query ID to be used by the Server
        for Client Requests which need to be replicated. The DCS Server must
        use the Replication Query ID to send out the replicated requests
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param msg_domain_id: The Domain ID in the message
        @type msg_domain_id: Integer
        @param transaction_type: DpsTransactionType
        @type transaction_type: DpsTransactionType
        @param reply_message: A Pointer to the Reply Message which will be
                              used to send reply to the Client. This message
                              can no longer be used by the C code till the
                              send_message_and_free is called to
                              send and free the message.
        @type reply_message: PyObject * (PYTHON OBJECT)
        @return: (DOVEStatus,
                  List of DCS Servers for replication, 
                  List of Query IDs to use)
                  DOVE_STATUS_OK: Replicate
                  DOVE_STATUS_EXISTS: The Client ID already exists, so the DPS
                                      Server shouldn't pro
                Any other Status: Failure (Do not replicate)
        '''
        domain_id = DpsCollection.Shared_DomainID
        status = DpsClientHandler.dps_error_none
        dps_nodes = []
        query_ids = []
        self.lock.acquire()
        while True:
            try:
                domain_tracker = self.domain_tracker_set[domain_id]
                status, dps_nodes = self.ReplicationDetermineDPSNodesPYTHON(msg_domain_id, transaction_type)
                for i in range(len(dps_nodes)):
                    query_ids.append(DpsCollection.generate_query_id())
                domain_tracker.add_replication_request(query_ids, reply_message)
                status = DpsClientHandler.dps_error_none
            except Exception, ex:
                message ='DCS: Force registration retry [%s]'%(ex)
                #print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                status = DpsClientHandler.dps_error_retry
                dps_nodes = []
                query_ids = []
                #print 'ReplicationQueryIDGenerate exception %s\r'%ex
            break
        self.lock.release()
        return (status, dps_nodes, query_ids)

    def ReplicationReplyProcess(self, replication_query_id, dps_protocol_status):
        '''
        This routine process the Replication Reply Message
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param replication_query_id: The Query ID of the Replication Message
        @type replication_query_id: Integer
        @param dps_protocol_status: The DPS Protocol Status received in the
                                    message
        @type dps_protocol_status: Integer
        @return None
        '''
        domain_id = DpsCollection.Shared_DomainID
        self.lock.acquire()
        try:
            self.domain_tracker_set[domain_id].process_replication_reply(replication_query_id,
                                                                         dps_protocol_status)
        except Exception:
            pass
        self.lock.release()
        return

    def ReplicationTimerProcess(self):
        '''
        This routine implements the timer routine that goes through all the
        domain replication trackers
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        '''
        self.lock.acquire()
        for domain_id in self.domain_tracker_set.keys():
            try:
                domain_tracker = self.domain_tracker_set[domain_id]
                domain_tracker.timer()
            except Exception:
                continue
        self.lock.release()
        #Restart the Timer
        self.timer = Timer(6, self.ReplicationTimerProcess)
        self.timer.start()
        return 0
