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

from object_collection import DpsCollection
from object_collection import DOVEStatus
from object_collection import DpsLogLevels

from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.DPSNodeAttributes import DPSNodeAttributes
from dcs_objects.DPSNodeStatistics import DPSNodeStatistics
from Utilities.ArrayInteger import ArrayInteger

import struct
import socket
import time
import random
from threading import Timer
import dcslib

class DPSNodeState:
    '''
    This represents the different states of a DPS Node
    '''
    up = 1
    hint_down = 2
    down = 3
    inactive = 4
    strings = {up: 'Up',
               hint_down :'Hint_Down',
               down : 'Down',
               inactive: 'Inactive'}

class DPSNodeClusterState:
    '''
    This represents the cluster state of a DCS Node
    '''
    down = 0
    active = 1
    inactive = 2
    string = {down: 'Down', active: 'Active', inactive: 'Inactive'}

class DPSNodePartitionState:
    '''
    This represents the partition state of the DPS Node
    '''
    same = 1 #In same partition as local node
    different_hint = 2 #MAY have been in a different partition
    different = 3 #Was in a different partition
    strings = {same:'Same', 
               different_hint:'Possibly Different', 
               different:'Recovering from Different'}

class DPSNode:
    '''
    This represents an DPS Node in the Cluster.
    '''
    #9 seconds after hint down
    MAX_NO_CONTACT = 15#second
    #After 6 seconds of no contact, node goes into "hint_no_contact" state
    HINT_NO_CONTACT = 10#second
    #3 sec timer interval to check for nodes which cannot be contacted
    TIMEOUT_INTERVAL = 5#second
    #How often to compute load
    LOAD_COMPUTATION_TIMEOUT_COUNT = 12
    #The Number of Timeout Counts to keep a Domain Deleted For
    DOMAIN_INVALIDATE_TIMEOUT_COUNT = 3
    #The maximum amount of continuous load computation cycles a 
    #node can be heavily loaded
    HEAVY_LOAD_TIMEOUT_COUNT = 6
    #Try to fix a heavily loaded node within HEAVY_LOAD_ATTEMPT_FIX of
    #detection
    HEAVY_LOAD_ATTEMPT_FIX = 180 #seconds
    #The time between retrying to fix a heavily loaded node
    HEAVY_LOAD_FIX_RETRY = 300 #seconds
    #How quickly to attempt to fix if replication factor is not met.
    REPLICATION_FACTOR_ATTEMPT_FIX = 15 #seconds
    #The time between retrying to meeting replication factor
    REPLICATION_FACTOR_FIX_RETRY = 30 #seconds
    #The default replication factor
    REPLICATION_FACTOR_DEFAULT = 2
    #The time between retrying to recover a domain
    DOMAIN_RECOVER_RETRY = 120 #seconds

    def __init__(self, cluster, flocal, ip_type, ip_val, port):
        '''
        Constructor:
        @param cluster: Reference to the Cluster
        @type cluster: Cluster
        @param fLocal: If this is the local Node
        @type fLocal: Boolean
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        '''
        self.cluster = cluster
        self.flocal = flocal
        self.leader = False
        self.dmc_config_version = 0L
        self.location = IPAddressLocation(ip_type, ip_val, port)
        self.domains = {}
        self.domains_deactivated = {}
        self.statistics = DPSNodeStatistics(self.location)
        self.last_contacted_secs = time.time()
        self.last_contacted_ascii = time.asctime(time.localtime())
        self.hint_down_secs = time.time()
        self.state = DPSNodeState.up
        self.partition_state = DPSNodePartitionState.same

    def domain_add(self, domain_id, replication_factor):
        '''
        Adds a domain
        @param domain_id: Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @type replication_factor: Integer
        @return: True if network paritition was detected, False otherwise
        @rtype: Boolean
        '''
        fPartition = False
        if ((self.partition_state == DPSNodePartitionState.different_hint) or 
            (self.partition_state == DPSNodePartitionState.different)):
            #Nodes may have been partitioned. Deal with it
            self.partition_state = DPSNodePartitionState.different
            if domain_id != DpsCollection.Shared_DomainID:
                message = 'ALERT! DCS Node %s has come back from a different network partition or reboot'%self.location.show_ip()
                print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            self.partition_state = DPSNodePartitionState.same
            fPartition = True
        self.domains[domain_id] = replication_factor
        self.statistics.domain_add(domain_id)
        return fPartition

    def domain_delete(self, domain_id):
        '''
        Remove a domain
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        try:
            del self.domains[domain_id]
        except Exception:
            pass
        try:
            del self.domains_deactivated[domain_id]
        except Exception:
            pass
        self.statistics.domain_delete(domain_id)
        return

    def domain_delete_all(self):
        '''
        Remove all domains from this node
        '''
        self.domains.clear()
        self.domains_deactivated.clear()
        self.statistics.domain_delete_all()
        return

    def domains_get(self):
        '''
        Returns all domain ids
        '''
        return self.domains.keys()

    def domain_supported(self, domain_id, replication_factor):
        '''
        Whether this domain id is present in the domain
        '''
        try:
            replication_factor_exists = self.domains[domain_id]
            self.domains[domain_id] = replication_factor
            return True
        except Exception:
            return False

    def domains_get_deactivated(self):
        '''
        Returns all domain ids
        '''
        return self.domains_deactivated.keys()

    def domain_is_deactivated(self, domain_id):
        '''
        Whether this domain is deactivated
        '''
        try:
            value = self.domains_deactivated[domain_id]
            return True
        except Exception:
            return False

    def domain_deactivate(self, domain_id):
        '''
        Deactivate the domain
        '''
        try:
            rep_factor = self.domains[domain_id]
            self.domains_deactivated[domain_id] = rep_factor
        except Exception:
            pass
        return

    def domain_activate(self, domain_id):
        '''
        Deactivate the domain
        '''
        try:
            del self.domains_deactivated[domain_id]
        except Exception:
            pass
        return

    def touch(self):
        '''
        This routine should be called whenever the DPS Node is contacted
        @return: True if status changed to up from down
        @rtype: Boolean
        '''
        self.last_contacted_secs = time.time()
        self.last_contacted_ascii = time.asctime(time.localtime())
        if self.state == DPSNodeState.inactive:
            #Inactive nodes don't change state
            return
        old_state = self.state
        if self.state != DPSNodeState.up:
            if self.is_down():
                message = 'DCSNode %s back online'%self.location.show_ip()
                print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            self.state = DPSNodeState.up
        if old_state == DPSNodeState.down:
            return True
        else:
            return False

    def hint_down(self, status_change_allow):
        '''
        This routine should be called when the DPS Node is detected possibly
        down. This should be called by
        1. The Leader Timer routine if the node hasn't been received a heartbeat
           from this node.
        2. The Leader Node notified the DPS about the latest node status list and
           this node was marked down
        @param status_change_allow: Whether to allow Node status to change
        @type status_change_allow: Boolean
        '''
        if self.state == DPSNodeState.up:
            self.hint_down_secs = time.time()
            if status_change_allow:
                #print 'DCSNode %s may have gone down\r'%self.location.show_ip()
                self.state = DPSNodeState.hint_down
        return

    def timeout_non_leader(self, status_change_allow):
        '''
        This timeout routine on this node should be called when the current node 
        is not the leader
        @param status_change_allow: Whether to allow Node status to change
        @type status_change_allow: Boolean
        @return: True if status changed to Down
        @rtype: Boolean
        '''
        if self.state == DPSNodeState.inactive:
            #Inactive nodes don't change state
            return False
        if not status_change_allow:
            return False
        current_secs = time.time()
        if self.state == DPSNodeState.hint_down:
            if ((current_secs - self.hint_down_secs > self.MAX_NO_CONTACT) and 
                status_change_allow):
                self.state = DPSNodeState.down
                message = 'DCS Node %s is DOWN!'%self.location.show_ip()
                print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                return True
        elif self.state == DPSNodeState.up:
            #Update hint_down to current time but don't change state.
            #When the local node is not the leader, the hint_down comes
            #from hint_down routine case
            self.hint_down_secs = current_secs
            return False
        else: #DPSNodeState.down
            return False

    def is_up(self):
        '''
        This routine returns True if this node is up
        '''
        if self.state == DPSNodeState.up:
            return True
        else:
            return False

    def is_inactive(self):
        '''
        This routine True if the state of the node is inactive
        '''
        if self.state == DPSNodeState.inactive:
            return True
        else:
            return False

    def is_down(self):
        '''
        This routine returns True if this node is not down
        '''
        if self.state == DPSNodeState.down:
            return True
        else:
            return False

    def lost_contact(self):
        '''
        This routine determine is the node has lost contact with the
        current node.
        '''
        if self.state == DPSNodeState.down or self.state == DPSNodeState.hint_down:
            return True
        else:
            return False

    def timeout_leader(self, status_change_allow):
        '''
        This routine should be called for timeout processing ONLY if the LOCAL
        NODE is the leader OR if the Local is Processing the Leader
        @param status_change_allow: Whether to allow Node status to change
        @type status_change_allow: Boolean
        @return: True if status changed to Down
        @rtype: Boolean
        '''
        if self.state == DPSNodeState.inactive:
            #Inactive nodes don't change state
            return False
        if not status_change_allow:
            return False
        current_secs = time.time()
        if self.state == DPSNodeState.hint_down:
            if ((current_secs - self.hint_down_secs > self.MAX_NO_CONTACT) and 
                status_change_allow):
                self.state = DPSNodeState.down
                message = 'DCS Node %s is DOWN!'%self.location.show_ip()
                print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                return True
        elif self.state == DPSNodeState.up:
            if ((current_secs - self.last_contacted_secs > self.HINT_NO_CONTACT) and
                status_change_allow):
                self.state = DPSNodeState.hint_down
                self.hint_down_secs = current_secs
                #print 'Node %s state changed to Hint Down!\r'%self.location.show_ip()
                #Hint Down is not a state change
                return False
        else: #DPSNodeState.down
            return False

    def show_domains(self):
        '''
        Returns the content of DPS Node
        '''
        #############################################################
        #To be finished
        return '%s'%(self.domains.keys())

class DPSNodeLeader:
    '''
    This represent the DPS Node Leader Object
    '''
    def __init__(self):
        self.dps_node = None

    def set(self, dps_node):
        '''
        Assign a new dps_node to be the leader
        @param dps_node: The DPS Node which is the leader
        '''
        self.dps_node = dps_node

    def get(self):
        return self.dps_node

class DPSNodeLocal:
    '''
    This represent the Local DPS Node Object
    '''
    def __init__(self):
        self.dps_node = None

    def set(self, dps_node):
        '''
        Assign a new dps_node to be the leader
        @param dps_node: The DPS Node which is the leader
        '''
        self.dps_node = dps_node

    def get(self):
        return self.dps_node

class DPSNodeCollectionClass:
    '''
    This represents the Node Collections
    '''
    valid = False
    #Collection of All Nodes, Hashed on IP value
    DPS_Nodes = {}
    #Collection of All Domains, Hashed on Domain ID
    Domain_Hash = {}
    #Collection of Domain Replication Factors
    Domain_Replication_Hash = {}
    #Collection of Domains_Deleted
    Domain_Deleted = {}
    #Collection of Domain Not meeting Replication Factor
    #Each element consists of (domain_id, number of nodes, detection time, last fix time)
    Domain_Not_Meeting_Replication_Factors = {}
    #Collection of Domain which don't have any DCS Nodes
    Domain_With_No_Nodes = {}
    #Collection of Nodes which are heavily loaded
    Nodes_Heavy_Load = {}
    #Collection of Nodes to whom to send heartbeat request to
    Nodes_Send_Heartbeart_Request_To = {}
    #The Variable name explains it...
    ITERATIONS_BEFORE_REPRINT = 120
    #Local Node
    local = DPSNodeLocal()
    #An instance of the DPS Node Leader Object
    leader = DPSNodeLeader()
    #An instance of Cluster Attributes
    #attribute = DPSNodeAttributes()
    #Timer thread started
    timer_thread_started = False
    #Stop Timer thread
    timer_thread_stop = False
    #Domain re-create thread
    domain_recreate_thread_started = False
    domain_recreate_thread_stop = False
    #DMC Config Version
    dmc_config_version = 0L
    #Allow Node State to Change
    state_change_allow = True
    #Number of iterations for Shared Domain to not exist
    Shared_Domain_Not_Present_Count_Max = 3
    #Number of iterations max to trigger leader election
    Leader_Not_Present_Count_Max = 3
    Leader_Election_Trigger = False
    #List of nodes which were in possibly in a different Network Partition
    Nodes_In_Different_Partition_Hint = {}
    #The number of Timeout to remove the Nodes from different Partition
    Nodes_In_Different_Partition_Hint_Timeout = 3
    #The maximum number of mass transfers a node can be involved in at 1 time
    NODE_MAX_TRANSFER_MAX = 50
    ##Collection of Nodes involved in mass transfer
    Nodes_In_Mass_Transfer = {}
    ##Maximum amount of time a node can be involved in 5 mass transfers
    Nodes_In_Mass_Transfer_Secs = 10
    #Exchange Domain Mapping
    Exchange_Domain_Mapping = False
    #Last time Domain Mapping was exchanged
    Exchange_Domain_Mapping_Time = 0
    Exchange_Domain_Mapping_Time_Max = 120 #seconds

class ClusterLog:
    '''
    This represents Cluster Logs. We don't want log messages to be
    printed all the time. So this routines stores all messages in a
    hash and prints them at given time intervals. If a message has
    not been touched in a while it will be removed.
    Each message is stored as a tuple(last log, last print)
    '''

    messages = {}
    @staticmethod
    def log_message(message):
        '''
        This record a message in the Logs
        '''
        try:
            message_tuple = ClusterLog.messages[message]
            last_print = message_tuple[1]
            if last_print >= DPSNodeCollectionClass.ITERATIONS_BEFORE_REPRINT:
                #print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE,
                                             message)
                last_print = 0
            ClusterLog.messages[message] = (0, last_print)
        except Exception:
            try:
                #print '%s\r'%message
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE,
                                             message)
                ClusterLog.messages[message] = (0, 0)
            except Exception:
                pass
        return

    @staticmethod
    def timeout():
        '''
        This routine performs 2 operations:
        1. Decide whether to print the message on console
        2. Whether to remove the message.
        '''
        for message in ClusterLog.messages.keys():
            try:
                message_tuple = ClusterLog.messages[message]
            except Exception:
                continue
            last_log = message_tuple[0] + 1
            last_print = message_tuple[1] + 1
            if last_log >= DPSNodeCollectionClass.ITERATIONS_BEFORE_REPRINT:
                try:
                    del ClusterLog.messages[message]
                except Exception:
                    pass
                continue
            ClusterLog.messages[message] = (last_log, last_print)
            continue
        return

    @staticmethod
    def show():
        for message, message_tuple in ClusterLog.messages.items():
            print "Message '%s': last log %s, last print %s\r"%(message, message_tuple[0], message_tuple[1])
        return

class Cluster:
    '''
    This represents the DPS Cluster
    '''
    DOMAIN_RECREATE_TIMEOUT = 10 #seconds

    def __init__(self):
        '''
        Constructor
        '''
        #Collection of all Nodes
        self.Node_Hash = DPSNodeCollectionClass.DPS_Nodes
        #Collection of all Domains
        self.Domain_Hash = DPSNodeCollectionClass.Domain_Hash
        #Collection of Domain Replication Factors
        self.Domain_Replication_Hash = DPSNodeCollectionClass.Domain_Replication_Hash
        #Collection of Domain Deleted
        self.Domain_Deleted = DPSNodeCollectionClass.Domain_Deleted
        #Collection of Domain Not meeting Replication Factor
        self.Domain_Not_Meeting_Replication_Factors = DPSNodeCollectionClass.Domain_Not_Meeting_Replication_Factors
        #Collection of Domain which don't have any DCS Nodes
        self.Domain_With_No_Nodes = DPSNodeCollectionClass.Domain_With_No_Nodes
        #Collection of Nodes that are heavily loaded
        self.Nodes_Heavy_Load = DPSNodeCollectionClass.Nodes_Heavy_Load
        #Collection of Nodes to send heartbeat request to
        self.Nodes_Send_Heartbeart_Request_To = DPSNodeCollectionClass.Nodes_Send_Heartbeart_Request_To
        #Local Node
        self.Local = DPSNodeCollectionClass.local
        #The Leader Location
        self.Leader = DPSNodeCollectionClass.leader
        #Node Attributes
        #self.Attribute = DPSNodeCollectionClass.attribute
        #Global Lock
        self.lock = DpsCollection.global_lock
        #Domain 0 created or not
        self.Shared_Domain_Not_Present_Count = DPSNodeCollectionClass.Shared_Domain_Not_Present_Count_Max
        #Leader present or not
        self.Leader_Not_Present_Count = 0
        #Number of times the timer routine has run
        self.timer_run = 0
        #List of nodes which were in possibly in a different Network Partition
        self.Nodes_In_Different_Partition_Hint = DPSNodeCollectionClass.Nodes_In_Different_Partition_Hint
        #Whether re-registration is necessary
        self.Reregister_Endpoints = False
        #Wait 10 iterations before asking for re-registration
        self.Reregister_Endpoints_Iterations_Max_Wait = 10
        self.Reregister_Endpoints_Iterations = 0
        self.Nodes_In_Mass_Transfer = DPSNodeCollectionClass.Nodes_In_Mass_Transfer
        #Start a single(ton) timer thread
        #if not DPSNodeCollectionClass.timer_thread_started:
        #    self.timer = Timer(DPSNode.TIMEOUT_INTERVAL, self.Timeout_Timer)
        #    DPSNodeCollectionClass.timer_thread_started = True
        #    self.timer.start()
        #else:
        #    self.timer = None
        ##Start a single(ton) domain recreate thread
        #if not DPSNodeCollectionClass.domain_recreate_thread_started:
        #    self.domain_recreate_timer = Timer(self.DOMAIN_RECREATE_TIMEOUT,
        #                                       self.Domain_Recreate_Timer)
        #    DPSNodeCollectionClass.domain_recreate_thread_started = True
        #    self.domain_recreate_timer.start()
        #else:
        #    self.domain_recreate_timer = None

    def node_status_change_allow_set(self, status_change_allow):
        '''
        @param status_change_allow: Whether to allow Node status to change
        @type status_change_allow: Boolean
        '''
        DPSNodeCollectionClass.state_change_allow = status_change_allow

    def node_status_change_allow_get(self):
        '''
        @return Whether to allow Node status to change
        @rtype Boolean
        '''
        return DPSNodeCollectionClass.state_change_allow

    def dmc_config_version_set(self, dmc_config_version):
        '''
        @param dmc_config_version: The configuration version as seen by the Cluster
        @type dmc_config_version: Long Integer
        '''
        DPSNodeCollectionClass.dmc_config_version = dmc_config_version
        dcslib.dps_cluster_set_cluster_config_version(dmc_config_version)
        return

    def dmc_config_version_get(self):
        '''
        @return: The configuration version as seen by the Cluster
        @rtype: Long Integer
        '''
        return DPSNodeCollectionClass.dmc_config_version

    def node_update_local(self, ip_type, ip_val, port):
        '''
        This routine updates the local DPS Node with the new values
        '''
        local_node = self.Local.get()
        if local_node is not None:
            #print 'Update Local: Existing Local Node %s\r'%local_node.location.show_ip()
            local_node.flocal = True
            ip_val_old = local_node.location.ip_value
            try:
                del self.Node_Hash[ip_val_old]
            except Exception:
                pass
            #self.Attribute.del_node_from_node_domain_table(ip_val_old)
            local_node.location = IPAddressLocation(ip_type, ip_val, port)
        else:
            local_node = DPSNode(self, True, ip_type, ip_val, port)
        message = 'Local DCS Service IP Address: %s'%local_node.location.show_ip()
        ClusterLog.log_message(message)
        self.Node_Hash[local_node.location.ip_value] = local_node
        #self.Attribute.add_node_to_node_domain_table(ip_val)
        self.Local.set(local_node)
        #Update Statistics Location
        local_node.statistics.location = local_node.location
        return local_node

    def node_add(self, ip_type, ip_val, port):
        '''
        This routine adds a remote dps node to the collection. This routine
        doesn't check if the node already existed.
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        '''
        if not DPSNodeCollectionClass.valid:
            return None
        dps_node = DPSNode(self, False, ip_type, ip_val, port)
        self.Node_Hash[dps_node.location.ip_value] = dps_node
        #Add the node to the attribute table
        #self.Attribute.add_node_to_node_domain_table(ip_val)
        if self.leader_local():
            #If local node is the leader, send heartbeat request to new node
            dps_node.state = DPSNodeState.hint_down
            message = 'Leader sending Heartbeat Request To %s'%dps_node.location.show_ip()
            ClusterLog.log_message(message)
            dcslib.dps_cluster_send_heartbeat_request_to(dps_node.location.ip_value_packed)
            #Send again 3 seconds later
            self.Nodes_Send_Heartbeart_Request_To[dps_node] = True
        return dps_node

    def node_delete(self, ip_val):
        '''
        This routine adds a dps node to the collection. This routine
        doesn't check if the node already existed.
        @param ip_val: The Value of the IP Address
        @type ip_val: String (IPv6) or Integer
        '''
        fLeaderDeleted = False
        try:
            dps_node = self.Node_Hash[ip_val]
            try:
                del self.Nodes_Send_Heartbeart_Request_To[dps_node]
            except Exception:
                pass
            if self.Leader.get() == dps_node:
                fLeaderDeleted = True
            try:
                del self.Nodes_In_Mass_Transfer[dps_node]
            except Exception:
                pass
            #Delete all Domains from the Node
            self.node_delete_all_domains(dps_node)
            try:
                del self.Nodes_In_Different_Partition_Hint[dps_node]
            except Exception:
                pass
            try:
                del self.Node_Hash[ip_val]
            except Exception:
                pass
            #self.Attribute.del_node_from_node_domain_table(ip_val)
            if fLeaderDeleted:
                #Run the leader election
                self.leader_election_run()
        except Exception:
            pass

    def mass_transfer_complete_synchronous(self, dps_node, domain_id):
        '''
        This routine handles the case when Domain Activate results in
        the node exchange domain mapping with every other node in the 
        cluster explicitly instead of relying on the clustering procotol.
        In such the case, the node exchanges mapping WITH EVERY OTHER NODE
        BEFORE exchanging with the node that sent the Domain Activate.
        So if the local node sent the Domain Activate to a remote node 
        and it receives a domain mapping from the remote node, it means
        every other node already has the mapping so it can stop forwarding
        updates.
        @return - True if 
        '''
        ret_val = False
        while True:
            try:
                domain = DpsCollection.Domain_Hash[domain_id]
                del domain.mass_transfer_forward_nodes[dps_node.location.ip_value]
                message= 'Domain %s: Mass Transfer to DPS Node %s ACKED\r'%(domain_id,
                                                                            dps_node.location.show_ip())
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                ret_val = True
            except Exception:
                break
        return ret_val

    def mass_transfer_complete_asynchronous(self, dps_node, domain_id, version):
        '''
        This routine handles the case when Domain Activate results in
        the node exchanges domain using an asynchronous clustering procotol.
        In such nodes in the cluster may receive the new mapping from the
        destination node of MASS TRANSFER in any sequence. The local node
        must 
        '''
        message = 'ALERT!!! mass_transfer_complete_asynchronous: NOT YET IMPLEMENTED'
        dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
        return False

    def mass_transfer_complete_check(self, dps_node, domain_id, version):
        '''
        Check to see if this version update results in completion of Mass Transfer
        from point of view of the Local Node. When the Local Node finished
        mass transfer of data for a domain to the remote node, it sent a Domain 
        Activate to the remote node. From this point on the local node must
        continue "forwarding" updates to this node, till it assured of the
        fact that every other node in the network is aware is aware of that
        the domain is now (also) hosted on another node.
        This routine determines if the local can indeed deem that the domain id
        mapping was communicated to every other node in the cluster.
        '''
        while True:
            #Synchronous
            if not self.mass_transfer_complete_synchronous(dps_node, domain_id):
                break
            ##Asynchronous
            #if not self.mass_transfer_complete_asynchronous(dps_node, domain_id, version):
            #    break
            break
        return

    def node_add_domain(self, dps_node, domain_id, replication_factor):
        '''
        This routine adds a domain to a node based on the domain mapping received
        from the remote end.
        @param dps_node: DPSNode
        @type dps_node: DPS Node
        @param domain_id: Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @type replication_factor: Integer
        '''
        #Check if Domain has been deleted
        if not DPSNodeCollectionClass.valid:
            return
        #For non-local node, don't add back deleted domain immediately. We
        #may be getting stale information from them in mapping exchange
        if dps_node != self.Local.get():
            try:
                domain_deleted = self.Domain_Deleted[domain_id]
                return
            except Exception:
                pass
        version = 100
        #Add domain to Node
        fPartition = dps_node.domain_add(domain_id, replication_factor)
        if fPartition:
            #Remove from Network Partition "Hint" State List
            try:
                del self.Nodes_In_Different_Partition_Hint[dps_node]
            except Exception:
                pass
            self.leader_election_run()
            if self.leader_local():
                self.Reregister_Endpoints = True
        #Add Node to domain
        try:
            domain_mapping = self.Domain_Hash[domain_id]
        except Exception:
            self.Domain_Hash[domain_id] = {}
            domain_mapping = self.Domain_Hash[domain_id]
        domain_mapping[dps_node.location.ip_value] = dps_node
        #self.Attribute.add_domain_to_node(dps_node.location.ip_value, domain_id,
        #                                  version)
        #Add the Domain to the collection
        self.Domain_Replication_Hash[domain_id] = replication_factor
        #Remove from Domain_With_No_Nodes
        try:
            del self.Domain_With_No_Nodes[domain_id]
            message = 'Domain %s now hosted on node %s'%dps_node.location.show_ip()
            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
            #print 'Domain %s now hosted on node %s\r'%dps_node.location.show_ip()
        except Exception:
            pass
        self.mass_transfer_complete_check(dps_node, domain_id, version)
        return

    def node_delete_domain(self, dps_node, domain_id,version=100):
        '''
        @param dps_node: DPSNode
        @type dps_node: DPS Node
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        if not DPSNodeCollectionClass.valid:
            return
        #Remove node from domain
        try:
            domain_mapping = self.Domain_Hash[domain_id]
            del domain_mapping[dps_node.location.ip_value]
            if len(domain_mapping) == 0:
                del self.Domain_Hash[domain_id]
        except Exception:
            pass
        #Remove domain from node
        dps_node.domain_delete(domain_id)
        #self.Attribute.del_domain_from_node(dps_node.location.ip_value, domain_id,
        #                                    version)
        return

    def node_delete_all_domains(self, dps_node,version=100):
        '''
        @param dps_node: DPSNode
        @type dps_node: DPS Node
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        if not DPSNodeCollectionClass.valid:
            return
        #Remove node from each domain
        for domain_id in dps_node.domains_get():
            try:
                domain_mapping = self.Domain_Hash[domain_id]
                del domain_mapping[dps_node.location.ip_value]
                if len(domain_mapping) == 0:
                    del self.Domain_Hash[domain_id]
            except Exception:
                pass
        #Clear the Node's Domain Collection
        dps_node.domain_delete_all()
        #self.Attribute.del_all_domains_from_node(dps_node.location.ip_value,
        #                                         version)
        return

    def node_is_up(self, ip_value):
        '''
        This routine checks if the node is up
        '''
        is_up = False
        while True:
            if not DPSNodeCollectionClass.valid:
                break
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            is_up = dps_node.is_up()
            break
        return is_up

    def node_touch(self, ip_value):
        '''
        This routine should be called under the following circumstances
        1. Every time a REST request/reply is received from the ip address.
        2. In addition it may be called when the node status list is received from
           the leader and the leader has marked a node as up
        @attention - This routine should be called with the global lock held
        @param ip_value: The IP address of DCS Node
        @type ip_value: Integer or String
        '''
        while True:
            if not DPSNodeCollectionClass.valid:
                break
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            changed = dps_node.touch()
            local = self.Local.get()
            if dps_node == local:
                #No need to process local node
                break
            if not changed:
                break
            if self.leader_local():
                message = 'Leader sending Heartbeat Request To %s'%dps_node.location.show_ip()
                ClusterLog.log_message(message)
                dcslib.dps_cluster_send_heartbeat_request_to(dps_node.location.ip_value_packed)
            #This remote node has rejoined the cluster. It may be coming back
            #from a different partition. Need to check.
            dps_node.partition_state = DPSNodePartitionState.different_hint
            self.Nodes_In_Different_Partition_Hint[dps_node] = 0
            #Exchange Domain Mapping with the node
            #Calling routine should have the lock acquired.
            self.lock.release()
            self.domain_mapping_exchange([dps_node])
            self.lock.acquire()
            break
        return

    def node_inactivate(self, ip_value):
        '''
        This routine inactivates a DCS node
        @param ip_value: IP Address
        @type ip_value: String or Integer
        '''
        while True:
            if not DPSNodeCollectionClass.valid:
                break
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            dps_node.state = DPSNodeState.inactive
            break
        return

    def node_inactivate_allow(self, dps_node):
        '''
        This routine determines if the dps node can be inactivated
        @param dps_node: DPS Node
        @type dps_node:
        @return: If the Local Node can be put in the desired state 1: Allow, 0: Deny
        @rtype: Integer
        '''
        #Get the domains in the node
        fallow = True
        domains = dps_node.domains.keys()
        #For every domain determine if it hosted by another dps node
        for domain in domains:
            if domain == DpsCollection.Shared_DomainID:
                #TODO: Remove this check
                #Don't care about Shared Domain for Now
                continue
            try:
                domain_hash = self.Domain_Hash[domain]
            except Exception:
                continue
            other_active_nodes = 0
            nodes = domain_hash.values()
            for node in nodes:
                if node == dps_node:
                    continue
                if node.is_up():
                    other_active_nodes += 1
            if other_active_nodes == 0:
                fallow = False
                break
        return fallow

    #def node_activate(self, ip_value):
    #    '''
    #    This routine activates a DCS node
    #    @param ip_value: IP Address
    #    @type ip_value: String or Integer
    #    '''
    #    while True:
    #        try:
    #            dps_node = self.Node_Hash[ip_value]
    #        except Exception:
    #            break
    #        dps_node.state = DPSNodeState.up
    #        break
    #    return

    def node_hint_down(self, ip_value):
        '''
        This routine should be called when node status list is received from
        the leader and the leader has marked a node as down
        @attention - This routine should be called with the global lock held
        @param ip_value: The IP address of DCS Node
        @type ip_value: Integer or String
        '''
        while True:
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            if dps_node == self.Local.get():
                #Local node cannot be down
                break
            dps_node.hint_down(self.node_status_change_allow_get())
            break
        return

    def nodes_running_get_statistics_array(self):
        '''
        This routine gets the array of statistics for all nodes that are up and running
        i.e. state "up"
        '''
        node_stats_array = []
        for dps_node in self.Node_Hash.values():
            if not dps_node.is_up():
                continue
            node_stats_array.append(dps_node.statistics)
        return node_stats_array

    def domain_get_nodes(self, domain_id):
        '''
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        try:
            values = self.Domain_Hash[domain_id].values()
        except Exception:
            values = []
        return values

    def domain_get_replication_status(self, domain_id):
        '''
        This routine returns the replication factor and the number of
        active nodes currently hosting this domain
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        try:
            dps_nodes = self.domain_get_nodes(domain_id)
            num_nodes = 0
            for dps_node in dps_nodes:
                if not dps_node.is_down():
                    num_nodes += 1
        except Exception:
            num_nodes = 0
        try:
            replication = self.Domain_Replication_Hash[domain_id]
        except Exception:
            replication = 0
        return (num_nodes, replication)

    def domain_delete(self, domain_id):
        '''
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        while True:
            if not DPSNodeCollectionClass.valid:
                break
            try:
                del self.Domain_Hash[domain_id]
            except Exception:
                pass
            try:
                del self.Domain_Replication_Hash[domain_id]
            except Exception:
                pass
            try:
                del self.Domain_With_No_Nodes[domain_id]
            except Exception:
                pass
            try:
                for dps_node in self.Node_Hash.values():
                    self.node_delete_domain(dps_node, domain_id)
                self.Domain_Deleted[domain_id] = DPSNode.DOMAIN_INVALIDATE_TIMEOUT_COUNT
            except Exception:
                pass
            break
        return

    def domain_add_node(self, domain_id):
        '''
        This routine determines a node to host a domain and then transfers
        the data to it.
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        fInitiated = False
        node_stats_array = self.nodes_running_get_statistics_array()
        node_stats_array_domain = []
        for node_stats in node_stats_array:
            try:
                domain_stats = node_stats.domain_statistics[domain_id]
            except Exception:
                node_stats_array_domain.append(node_stats)
        #Get 3 nodes not hosting this domain
        try:
            nodes = DPSNodeStatistics.load_available_nodes(node_stats_array_domain, 3, False)
            random.shuffle(nodes)
        except Exception, ex:
            message = 'Cluster.lowest_loaded_nodes_not_hosting: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            nodes = []
        while True:
            #Get the lowest node possible
            dps_node_new = None
            for node_ip in nodes:
                try:
                    dps_node_new = self.Node_Hash[node_ip]
                    #Check to make sure this node is not involved in Mass Transfer
                    try:
                        mass_transfer = self.Nodes_In_Mass_Transfer[dps_node_new]
                        mass_transfer_count = mass_transfer[1]
                        if mass_transfer_count > DPSNodeCollectionClass.NODE_MAX_TRANSFER_MAX:
                            dps_node_new = None
                            continue
                    except Exception:
                        pass
                    if dps_node_new.is_up():
                        break
                    dps_node_new = None
                except Exception:
                    continue
            if dps_node_new is None:
                #message = 'Cannot find a node to move domain %s to'%domain_id
                break
            #Get any node hosting this domain
            dps_node_current = None
            try:
                dps_nodes = self.Domain_Hash[domain_id].values()
                for dps_node in dps_nodes:
                    try:
                        mass_transfer = self.Nodes_In_Mass_Transfer[dps_node]
                        mass_transfer_count = mass_transfer[1]
                        if mass_transfer_count > DPSNodeCollectionClass.NODE_MAX_TRANSFER_MAX:
                            continue
                    except Exception:
                        pass
                    if dps_node.is_up():
                        dps_node_current = dps_node
                        break
            except Exception:
                pass
            if dps_node_current is None:
                #message = 'Cannot find a node to move domain %s from'%domain_id
                #ClusterLog.log_message(message)
                break
            message = 'Domain %s: Initiating Mass Transfer of from %s to %s'%(domain_id,
                                                                              dps_node_current.location.show_ip(),
                                                                              dps_node_new.location.show_ip())
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            curr_time = time.time()
            try:
                mass_transfer = self.Nodes_In_Mass_Transfer[dps_node_current]
                mass_transfer_count = mass_transfer[1] + 1
                self.Nodes_In_Mass_Transfer[dps_node_current] = (curr_time, mass_transfer_count)
            except Exception:
                self.Nodes_In_Mass_Transfer[dps_node_current] = (curr_time, 1)
            try:
                mass_transfer = self.Nodes_In_Mass_Transfer[dps_node_new]
                mass_transfer_count = mass_transfer[1] + 1
                self.Nodes_In_Mass_Transfer[dps_node_new] = (curr_time, mass_transfer_count)
            except Exception:
                self.Nodes_In_Mass_Transfer[dps_node_new] = (curr_time, 1)
            #Perform the actual transfer
            dcslib.dps_cluster_initiate_mass_transfer(dps_node_current.location.ip_value_packed,
                                                      dps_node_new.location.ip_value_packed,
                                                      domain_id)
            fInitiated = True
            break
        return fInitiated

    def domain_delete_node(self, domain_id):
        '''
        This routine determines a node to (un)host a domain and then deactivates
        it from that node.
        @param domain_id: Domain ID
        @type domain_id: Integer
        '''
        node_stats_array = self.nodes_running_get_statistics_array()
        node_stats_array_domain = []
        for node_stats in node_stats_array:
            try:
                domain_stats = node_stats.domain_statistics[domain_id]
                node_stats_array_domain.append(node_stats)
            except Exception:
                pass
        #Get the 2 highest loaded node hosting this domain
        try:
            nodes = DPSNodeStatistics.load_available_nodes(node_stats_array_domain, 2, True)
        except Exception, ex:
            message = 'Cluster.domain_delete_node: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            nodes = []
        while True:
            if len(nodes) < 2:
                #What's the point of de-activating when there isn't even 2 nodes
                #hosting the domain at least
                break
            #Get the highest node possible
            dps_node = None
            for node_ip in nodes:
                try:
                    dps_node = self.Node_Hash[node_ip]
                    break
                except Exception:
                    continue
            if dps_node is None:
                #print 'Cannot find a node to remove domain %s from\r'%domain_id
                break
            message = 'Deactivating domain %s from %s'%(domain_id,
                                                        dps_node.location.show_ip())
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            #Perform the actual transfer
            dcslib.dps_domain_deactivate_on_node(dps_node.location.ip_value_packed, domain_id)
            break
        return

    # def node_get_domains(self, nodeIP):
    #     '''
    #     Return domains handled by a node.
    #     '''
    #     try:
    #         dps_node = self.Node_Hash[nodeIP]
    #         domains = dps_node.domains_get()
    #         #domains = self.Attribute.get_domains_for_node(nodeIP)
    #     except Exception:
    #         domains = []
    #     return domains

    # def node_set_domain_mapping(self, nodeIP, version, domain_list):
    #     '''
    #     Sets the list of domains (python list) handled by a node.
    #     '''
    #     try:
    #         self.Attribute.set_domain_list_for_node(nodeIP, version, domain_list)
    #     except Exception:
    #         pass

    def cluster_delete_all_nodes(self):
        '''
        This routine deletes all dps nodes in the collection. 
        '''
        for node_id in self.Node_Hash.keys():
            try:
                self.node_delete(node_id)
            except Exception:
                pass
        return

    def domain_mapping_exchange(self, dps_nodes):
        '''
        This routine invokes exchanges the local mapping with the dps nodes
        specified in the list.
        @attention: This routine must not be called with the global lock held
        @param dps_nodes: List of DPS Nodes with whom to exchange the 
                          domain mapping.
        @type dps_nodes: [DPSNodes]
        '''
        while True:
            try:
                local_ip_val = self.Local.dps_node.location.ip_value
            except Exception:
                message = 'Domain_mapping_exchange: cannot get Local IP'
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                break
            self.lock.acquire()
            try:
                #local_domain_list = self.node_get_domains(local_ip_val)
                #print 'domain_mapping_exchange: local_domain_list %s\r'%local_domain_list
                #local_domain_str = ArrayInteger.GenerateFromList(local_domain_list, True)
                local_domain_str = str(self.Local.dps_node.domains)
                #print 'domain_mapping_exchange: local_domain_list %s\r'%local_domain_str
            except Exception, ex:
                self.lock.release()
                message = 'Domain_mapping_exchange: cannot create Local Domain String [%s]'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                break
            self.lock.release()
            node_ips = []
            for dps_node in dps_nodes:
                if dps_node.is_down():
                    continue
                if dps_node.flocal:
                    continue
                node_ips.append(dps_node.location.ip_value)
            dcslib.dps_cluster_send_local_mapping_to(node_ips, local_domain_str)
            break
        return

    def domain_mapping_exchange_timer(self):
        '''
        This routine send domain mapping to all nodes if necessary
        '''
        while True:
            curr_time = time.time()
            time_diff = curr_time - DPSNodeCollectionClass.Exchange_Domain_Mapping_Time
            if time_diff > DPSNodeCollectionClass.Exchange_Domain_Mapping_Time_Max:
                DPSNodeCollectionClass.Exchange_Domain_Mapping = True
            if not DPSNodeCollectionClass.Exchange_Domain_Mapping:
                break
            self.lock.acquire()
            try:
                DPSNodeCollectionClass.Exchange_Domain_Mapping = False
                nodes = self.Node_Hash.values()
            except Exception:
                nodes = []
                break
            self.lock.release()
            self.domain_mapping_exchange(nodes)
            DPSNodeCollectionClass.Exchange_Domain_Mapping_Time = curr_time
            break
        return

    def leader_amongst_2(self, node1, node2):
        '''
        This routine determines which DCS node is the leader amongst the 2
        @param node1: DPSNode 1
        @type node1: DPSNode
        @param node2: DPSNode 2
        @type node2: DPSNode
        @return: The Leader amongst the 2
        @rtype: DPSNode
        '''
        if node1.location.show_ip() == node2.location.show_ip():
            message = 'ALERT! 2 nodes have same IP Address %s'%node1.location.show_ip()
            print '%s\r'%message
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
        if node1.location.show_ip() < node2.location.show_ip():
            return node1
        else:
            return node2

    def leader_local(self):
        '''
        Determines if the local node is the leader
        @return: Returns whether local node is the leader
        @rtype: Boolean
        '''
        if self.Local.get() == self.Leader.get():
            return True
        else:
            return False

    def timer_verify_replication_factor(self):
        '''
        This routine verifies if domain are meeting their replication
        factor. If a domain is not meeting their replication factor
        that domain is put on a watch list:
        '''
        for domain in self.Domain_Replication_Hash.keys():
            if domain == DpsCollection.Shared_DomainID:
                domain_name = "[Shared]"
            else:
                domain_name = domain
            num_nodes, replication = self.domain_get_replication_status(domain)
            if num_nodes == replication:
                try:
                    del self.Domain_Not_Meeting_Replication_Factors[domain]
                    message = 'Domain %s: Now meeting replication factor %s, nodes %s'%(domain_name, replication, num_nodes)
                    ClusterLog.log_message(message)
                except Exception:
                    pass
            else:
                try:
                    replication_tuple = self.Domain_Not_Meeting_Replication_Factors[domain]
                    detection_time = replication_tuple[2]
                    last_fix_time = replication_tuple[3]
                    self.Domain_Not_Meeting_Replication_Factors[domain] = (num_nodes, domain, detection_time, last_fix_time)
                except Exception:
                    #Tuple representing (when detected, when last attempt to fix was made)
                    self.Domain_Not_Meeting_Replication_Factors[domain] = (num_nodes, domain, time.time(), 0)
                message = "Domain %s: Not meeting replication factor %s, nodes %s"%(domain_name, replication, num_nodes)
                ClusterLog.log_message(message)
        #Only leader should attempt to fix Replication Factor issues
        if not self.leader_local():
            return
        #This node is the leader.
        #List of Domain that need to be given an additional node
        domain_add = []
        #List of Domain that need to be number of nodes hosting them
        #to be reduced
        domain_deactivate = []
        try:
            replication_tuples = self.Domain_Not_Meeting_Replication_Factors.values()
            replication_tuples.sort()
        except Exception:
            replication_tuples = []
        #print 'Cluster: Replication Tuples %s\r'%replication_tuples
        for replication_tuple in replication_tuples:
            curr_time = time.time()
            domain = replication_tuple[1]
            detection_time = replication_tuple[2]
            last_fix_time = replication_tuple[3]
            #print 'Domain %s, curr_time %s, detection_time %s, last_fix_time %s\r'%(domain, curr_time, detection_time, last_fix_time)
            #Update to current time
            if curr_time - detection_time < DPSNode.REPLICATION_FACTOR_ATTEMPT_FIX:
                #print 'curr_time - detection_time < DPSNode.REPLICATION_FACTOR_ATTEMPT_FIX\r'
                continue
            if curr_time - last_fix_time < DPSNode.REPLICATION_FACTOR_FIX_RETRY:
                #print 'curr_time - last_fix_time < DPSNode.REPLICATION_FACTOR_FIX_RETRY\r'
                continue
            num_nodes, replication = self.domain_get_replication_status(domain)
            #Check one more time
            if num_nodes == replication:
                try:
                    del self.Domain_Not_Meeting_Replication_Factors[domain]
                except Exception:
                    pass
                continue
            if num_nodes > replication:
                domain_deactivate.append(domain)
            elif num_nodes != 0:
                domain_add.append(domain)
            elif num_nodes == 0:
                self.Domain_Not_Meeting_Replication_Factors[domain] = (num_nodes, domain, detection_time, curr_time)
                try:
                    exists = self.Domain_With_No_Nodes[domain]
                except Exception:
                    self.Domain_With_No_Nodes[domain] = (curr_time, 0)
        #Process the Adds first
        #print 'domain_add %s\r'%domain_add
        for domain in domain_add:
            fInitiated = self.domain_add_node(domain)
            if fInitiated:
                try:
                    replication_tuple = self.Domain_Not_Meeting_Replication_Factors[domain]
                    num_nodes = replication_tuple[0]
                except Exception:
                    num_nodes = 1
                self.Domain_Not_Meeting_Replication_Factors[domain] = (num_nodes, domain, detection_time, curr_time)
        for domain in domain_deactivate:
            try:
                replication_tuple = self.Domain_Not_Meeting_Replication_Factors[domain]
                num_nodes = replication_tuple[0]
            except Exception:
                num_nodes = 2
            self.Domain_Not_Meeting_Replication_Factors[domain] = (num_nodes, domain, detection_time, curr_time)
            self.domain_delete_node(domain)
        return

    def leader_election_trigger_set(self, fStart):
        '''
        Inform the cluster database to run in cluster node and
        start leader election process.
        @param fStart: Whether to start the process
        @type fStart: Boolean
        '''
        DPSNodeCollectionClass.Leader_Election_Trigger = fStart

    def leader_timer_load_computation(self):
        '''
        This routine computes the load on the nodes and returns a list of nodes
        that need mitigation of load.
        '''
        curr_time = time.time()
        for dps_node in self.Node_Hash.values():
            fheavy_load, load = dps_node.statistics.heavy_load()
            if fheavy_load:
                message = 'DCS Node %s is under heavy load, Load = %.2f'%(dps_node.location.show_ip(),
                                                                          load)
                ClusterLog.log_message(message)
                #Check if node was already in the Heavy Loaded Hash
                try:
                    heavy_load_tuple = self.Nodes_Heavy_Load[dps_node.location.ip_value]
                except Exception:
                    self.Nodes_Heavy_Load[dps_node.location.ip_value] = (curr_time, 0)
            else:
                #Remove from Heavy Load
                try:
                    del self.Nodes_Heavy_Load[dps_node.location.ip_value]
                except Exception:
                    pass
        nodes_needing_mitigation = []
        #Determine the node which need
        for dps_ip in self.Nodes_Heavy_Load.keys():
            #Only do max 2 nodes at a time
            if len(nodes_needing_mitigation) >= 2:
                break
            try:
                dps_node = self.Node_Hash[dps_ip]
                heavy_load_tuple = self.Nodes_Heavy_Load[dps_ip]
            except Exception:
                continue
            curr_time = time.time()
            detection_time = heavy_load_tuple[0]
            last_fix_time = heavy_load_tuple[1]
            #Update to current time
            if curr_time - detection_time < DPSNode.HEAVY_LOAD_ATTEMPT_FIX:
                #print 'curr_time - detection_time < DPSNode.HEAVY_LOAD_ATTEMPT_FIX\r'
                continue
            if curr_time - last_fix_time < DPSNode.HEAVY_LOAD_FIX_RETRY:
                #print 'curr_time - last_fix_time < DPSNode.HEAVY_LOAD_FIX_RETRY\r'
                continue
            #Reset the detection and last fix time.
            self.Nodes_Heavy_Load[dps_ip] = (detection_time, curr_time)
            message = 'Adding DCS Node %s to List of Heavily Nodes needing Mitigation'%dps_node.location.show_ip()
            ClusterLog.log_message(message)
            nodes_needing_mitigation.append(dps_node)
        return nodes_needing_mitigation

    def leader_timer_heavy_load_mitigate(self, dps_nodes_heavy):
        '''
        This routine tries to mitigate the load on the list of dps nodes
        supplied as a parameter
        @param dps_nodes_heavy: The List of DPS nodes needing mitigation.
        @type dps_nodes_heavy: []
        '''
        #Find the 2 lowest loaded node in the cluster
        while True:
            if len(dps_nodes_heavy) <= 0:
                break
            stats_array = self.nodes_running_get_statistics_array()
            try:
                node_stats_low = DPSNodeStatistics.load_available_nodes(stats_array, 2, False)
            except Exception, ex:
                message = 'leader_timer_heavy_load_mitigate: Exception [%s]'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                #No point in trying
                break
            dps_nodes_low = []
            for node_ip in node_stats_low:
                try:
                    dps_node = self.Node_Hash[node_ip]
                    fheavy_load, dps_node_load = dps_node.statistics.heavy_load()
                    if fheavy_load:
                        continue
                    dps_nodes_low.append(dps_node)
                except Exception:
                    continue
            if len(dps_nodes_low) <= 0:
                message = 'ALERT! No viable node found for mitigating load. Inform DOVE Administrator'
                ClusterLog.log_message(message)
                break
            low_node_index = 0
            low_nodes_length = len(dps_nodes_low)
            for dps_node_heavy in dps_nodes_heavy:
                dps_node_low = dps_nodes_low[low_node_index]
                #Find a good domain to move.
                try:
                    domains = DPSNodeStatistics.load_balance_domain(dps_node_heavy.statistics,
                                                                   dps_node_low.statistics)
                except Exception, ex:
                    message = 'Exception [%s] in trying to mitigate load on DCS Node %s, will try again in 10 minutes\r'%(ex,
                                                                                                                      dps_node_heavy.location.show_ip())
                    dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                    continue
                for domain in domains:
                    message = 'Trying to mitigate load on DCS Node %s by moving a domain %s to DCS Node %s\r'%(dps_node_heavy.location.show_ip(),
                                                                                                               domain,
                                                                                                               dps_node_low.location.show_ip())
                    dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                    #Perform the actual transfer
                    dcslib.dps_cluster_initiate_mass_transfer(dps_node_heavy.location.ip_value_packed,
                                                              dps_node_low.location.ip_value_packed,
                                                              domain)
                #The Cluster will increase the replication factor of the domain first
                #and then later on remove the domain from the heavier loaded system.
                low_node_index += 1
                if low_node_index >= low_nodes_length:
                    low_node_index = 0
            break
        return

    def leader_check_shared_domain(self):
        '''
        This routine checks if Shared Domain needs to be created
        '''
        nodes, replication = self.domain_get_replication_status(DpsCollection.Shared_DomainID)
        if nodes == 0 or replication == 0:
            if self.Shared_Domain_Not_Present_Count >= DPSNodeCollectionClass.Shared_Domain_Not_Present_Count_Max:
                dcslib.dps_cluster_create_shared_domain()
                self.Shared_Domain_Not_Present_Count = 0
            self.Shared_Domain_Not_Present_Count += 1
        else:
            self.Shared_Domain_Not_Present_Count = 0
        return

    def leader_reregister_endpoints(self):
        '''
        This routine determines if the leader needs to ask for endpoint
        registration from DOVE Switches.
        '''
        while True:
            if not self.Reregister_Endpoints:
                break
            self.Reregister_Endpoints_Iterations += 1
            if self.Reregister_Endpoints_Iterations <= self.Reregister_Endpoints_Iterations_Max_Wait:
                break
            message = 'Clustering: Ask DMC to request re-registration from all DOVE Switches'
            ClusterLog.log_message(message)
            ret = dcslib.dps_cluster_reregister_endpoints(0, 0)
            if ret == 0:
                self.Reregister_Endpoints_Iterations = 0
                self.Reregister_Endpoints = False
                break
            break
        return

    def leader_timer_routine(self):
        '''
        This is the timer routine which detects if nodes have not been
        contacted in a while. If 'lower' nodes are deemed to be uncontactable
        try and reach out to them.
        @attention - This routine should only be called if the Local Node is
                     the leader
        @attention - This routine should NOT be called with the global lock held
        '''
        self.leader_reregister_endpoints()
        unreachable_nodes = []
        #Check Shared Domain Existence
        self.leader_check_shared_domain()
        #Process Statistics
        self.lock.acquire()
        try:
            #Send heartbeat request to
            for dps_node in self.Nodes_Send_Heartbeart_Request_To.keys():
                try:
                    del self.Nodes_Send_Heartbeart_Request_To[dps_node]
                    message = 'Leader sending Heartbeat Request To %s'%dps_node.location.show_ip()
                    ClusterLog.log_message(message)
                    dcslib.dps_cluster_send_heartbeat_request_to(dps_node.location.ip_value_packed)
                except Exception:
                    pass
            local = self.Local.get()
            for dps_node in self.Node_Hash.values():
                if dps_node == local:
                    continue
                dps_node.timeout_leader(self.node_status_change_allow_get())
                #if self.leader_amongst_2(local, dps_node) != local:
                #    #Only contact lower nodes
                #    continue
                if dps_node.lost_contact():
                    #If I think I am the leader continue to request heartbeat from lower
                    #nodes which may be down.
                    unreachable_nodes.append(dps_node)
        except Exception, ex:
            message = 'Leader Timer Routine: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
        self.lock.release()
        for dps_node in unreachable_nodes:
            message = 'Leader sending Heartbeat Request To %s'%dps_node.location.show_ip()
            ClusterLog.log_message(message)
            dcslib.dps_cluster_send_heartbeat_request_to(dps_node.location.ip_value_packed)
        #Deal with Load
        if self.timer_run % DPSNode.LOAD_COMPUTATION_TIMEOUT_COUNT == 0:
            nodes_needing_mitigation = self.leader_timer_load_computation()
            self.leader_timer_heavy_load_mitigate(nodes_needing_mitigation)
        return

    def non_leader_timer_routine(self):
        '''
        This is the timer routine if the local node is not the leader.
        @attention - This routine should only be called if the Local Node is NOT
                     the leader
        @attention - This routine should NOT be called with the global lock held
        '''
        #Only leader re-registers endpoints
        self.Reregister_Endpoints = False
        self.Reregister_Endpoints_Iterations = 0
        fRunLeaderElection = False
        self.lock.acquire()
        try:
            self.Nodes_Send_Heartbeart_Request_To.clear()
            local = self.Local.get()
            leader = self.Leader.get()
            for dps_node in self.Node_Hash.values():
                if dps_node == local:
                    continue
                if leader != dps_node:
                    dps_node.timeout_non_leader(self.node_status_change_allow_get())
                    continue
                #The dps node is the "leader"
                #Send heartbeat to existing leader
                if local.is_inactive():
                    factive = 0
                else:
                    factive = 1
                dcslib.dps_cluster_send_heartbeat_to(factive,
                                                     local.dmc_config_version,
                                                     leader.location.ip_value_packed)
                changed = leader.timeout_leader(self.node_status_change_allow_get())
                if not changed:
                    continue
                #Lost contact with the leader
                #Re-run the leader election algorithm
                fRunLeaderElection = True
            if fRunLeaderElection:
                self.leader_election_run()
        except Exception, ex:
            message = 'Non Leader Timer Routine: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
        self.lock.release()
        return

    def leader_send_hearbeat_request(self):
        '''
        This routine sends "Heartbeat Request" message to all necessary (lower) nodes.
        @attention - This routine should NOT be called with the global lock held
        '''
        while True:
            if not self.leader_local():
                break
            dps_nodes = []
            self.lock.acquire()
            try:
                local = self.Local.get()
                for dps_node in self.Node_Hash.values():
                    if dps_node == local:
                        continue
                    #if self.leader_amongst_2(local, dps_node) != local:
                    #    continue
                    dps_nodes.append(dps_node)
            except Exception, ex:
                message = 'Leader sending Heartbeat Request: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            self.lock.release()
            for dps_node in dps_nodes:
                message = 'Leader sending Heartbeat Request To %s'%dps_node.location.show_ip()
                ClusterLog.log_message(message)
                #print '%s\r'%message
                dcslib.dps_cluster_send_heartbeat_request_to(dps_node.location.ip_value_packed)
            break
        return

    def leader_send_node_list_status_to(self, dps_node):
        '''
        This routine sends the list of nodes and their status to the dps_node
        @attention - This routine should NOT be called with the global lock held
        @param dps_node: The dps node to send this information to
        @type dps_node: DPSNode
        '''
        while True:
            if not self.leader_local():
                break
            if dps_node == self.Local.get():
                break
            dps_nodes = []
            self.lock.acquire()
            try:
                for node in self.Node_Hash.values():
                    if node.lost_contact():
                        #Down
                        dps_nodes.append((node.location.ip_value_packed, DPSNodeClusterState.down))
                    elif node.is_up():
                        #Up
                        dps_nodes.append((node.location.ip_value_packed, DPSNodeClusterState.active))
                    else:
                        #Inactive
                        dps_nodes.append((node.location.ip_value_packed, DPSNodeClusterState.inactive))
            except Exception, ex:
                message = 'Leader sending Node Status: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            self.lock.release()
            dcslib.dps_cluster_send_nodes_status_to(self.dmc_config_version_get(),
                                                    dps_node.location.ip_value_packed, 
                                                    dps_nodes)
            break
        return

    def leader_election_run(self):
        '''
        This routine runs the leader election algorithm on the set of
        DPS Nodes and return the leader node to the calling routine
        @attention: This routine assumes that the global lock is held
        '''
        new_leader = None
        for dps_node in self.Node_Hash.values():
            if dps_node.is_down() or dps_node.is_inactive():
                continue
            if new_leader is None:
                new_leader = dps_node
            else:
                new_leader = self.leader_amongst_2(new_leader, dps_node)
        if new_leader == None:
            return
        old_leader = self.Leader.get()
        self.Leader.set(new_leader)
        local = self.Local.get()
        if local == new_leader:
            #Report self as new leader to DMC
            dcslib.dps_cluster_set_leader(1,
                                          new_leader.location.ip_value_packed,
                                          new_leader.location.port)
            #Send request for heartbeat messages to all (lower) nodes
            #print 'Sending Heartbeat request to all nodes\r'
            DpsCollection.heartbeat_request_queue.put((self))
            if old_leader != new_leader:
                message = 'This node %s, elected as new leader'%self.Local.dps_node.location.show_ip()
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                if DPSNodeCollectionClass.dmc_config_version == 0L:
                    self.Reregister_Endpoints = True
        else:
            #Report new leader to DMC
            dcslib.dps_cluster_set_leader(0,
                                          new_leader.location.ip_value_packed,
                                          new_leader.location.port)
            #print 'leader_election_run: Sending Heartbeat to Leader %s\r'%new_leader.location.show_ip()
            if local.is_inactive():
                factive = 0
            else:
                factive = 1
            dcslib.dps_cluster_send_heartbeat_to(factive,
                                                 local.dmc_config_version,
                                                 self.Leader.dps_node.location.ip_value_packed)
            message = 'leader_election_run: Leader %s'%self.Leader.dps_node.location.show_ip()
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
        return

    def node_update_domain_stats(self, ip_value, domain_id, 
                                 endpoints, tunnels, 
                                 endpoint_updates, endpoint_lookups, policy_lookups):
        '''
        This routine updates the Domain Statistics in a Node
        @param ip_value: DPS Node IP Address
        @type ip_value: IP or String
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param endpoints: The number of endpoints in that domain
        @type endpoints: Integer
        @param tunnels: The number of tunnels in that domain
        @type tunnels: Integer
        @param endpoint_updates: The number of endpoint updates that have happened
                                 since the last statistics update
        @type endpoint_updates: Integer
        @param endpoint_lookups: The number of endpoint lookups that have happened
                                 since the last statistics update
        @type endpoint_lookups: Integer
        @param policy_lookups: The number of policy lookups that have happened
                               since the last statistics update
        @type policy_lookups: Integer
        '''
        while True:
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            dps_node.statistics.update_statistics(domain_id, 
                                                  endpoints, tunnels, 
                                                  endpoint_updates, endpoint_lookups, policy_lookups)
            break
        return

    def node_show_load(self, ip_value):
        '''
        This routine shows the load on a particular node
        @param ip_value: DPS Node IP Address
        @type ip_value: IP or String
        '''
        while True:
            try:
                dps_node = self.Node_Hash[ip_value]
            except Exception:
                break
            dps_node.statistics.show()
            break
        return

    def lowest_loaded_nodes(self, num):
        '''
        This routine return "num" lowest loaded nodes
        @param num: The number of nodes needed
        @type num: Integer
        @return: List of lowest loaded nodes - maximum number "num"
        @rtype:[ip_packed1, ip_packed2]...
        '''
        node_stats_array = self.nodes_running_get_statistics_array()
        #print 'lowest_loaded_nodes: node_stats_array %s\r'%(node_stats_array)
        try:
            nodes = DPSNodeStatistics.load_available_nodes_packed(node_stats_array, num, False)
            random.shuffle(nodes)
        except Exception, ex:
            message = 'lowest_loaded_nodes: Exception [%s]'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            nodes = []
        return nodes

    def lowest_loaded_nodes_not_hosting(self, num, domain_id):
        '''
        This routine returns "num" lowest loaded nodes not hosting a domain
        @param num: The number of nodes needed
        @type num: Integer
        @param domain_id: The Domain ID that the node shouldn't be hosting
        @type domain_id: Integer
        @return: List of lowest loaded nodes not hosting domain - 
                 maximum number "num"
        @rtype:[ip_packed1, ip_packed2]...
        '''
        node_stats_array = self.nodes_running_get_statistics_array()
        node_stats_array_domain = []
        for node_stats in node_stats_array:
            try:
                domain_stats = node_stats.domain_statistics[domain_id]
            except Exception:
                node_stats_array_domain.append(node_stats)
        try:
            nodes = DPSNodeStatistics.load_available_nodes_packed(node_stats_array_domain, num, False)
            random.shuffle(nodes)
        except Exception, ex:
            message = 'lowest_loaded_nodes_not_hosting: Exception [%s]'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            nodes = []
        return nodes

    def highest_loaded_nodes_hosting(self, num, domain_id):
        '''
        This routine returns "num" highest loaded nodes hosting a domain
        @param num: The number of nodes needed
        @type num: Integer
        @param domain_id: The Domain ID that the node shouldn't be hosting
        @type domain_id: Integer
        @return: List of lowest loaded nodes not hosting domain - 
                 maximum number "num"
        @rtype:[ip_packed1, ip_packed2]...
        '''
        node_stats_array = self.nodes_running_get_statistics_array()
        node_stats_array_domain = []
        for node_stats in node_stats_array:
            try:
                domain_stats = node_stats.domain_statistics[domain_id]
                node_stats_array_domain.append(node_stats)
            except Exception:
                pass
        try:
            nodes = DPSNodeStatistics.load_available_nodes_packed(node_stats_array_domain, num, True)
        except Exception, ex:
            message = 'highest_loaded_nodes_hosting: Exception [%s]'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            nodes = []
        return nodes

    def highest_loaded_nodes(self, num):
        '''
        This routine return "num" lowest loaded nodes
        @param num: The number of nodes needed
        @type num: Integer
        @return: List of lowest loaded nodes - maximum number "num"
        @rtype:[ip_packed1, ip_packed2]...
        '''
        node_stats_array = self.nodes_running_get_statistics_array()
        return DPSNodeStatistics.load_available_nodes_packed(node_stats_array, num, True)

    def Timeout_Clear_Domains_From_Down_Nodes(self):
        '''
        This routine clears domains from those nodes which have gone down
        '''
        local = self.Local.get()
        for dps_node in self.Node_Hash.values():
            if local == dps_node:
                continue
            if dps_node.is_down():
                self.node_delete_all_domains(dps_node)
        return

    def Timeout_Clear_Nodes_From_Mass_Transfer(self):
        '''
        This routine clears nodes from mass transfer:
        '''
        curr_time = time.time()
        for node, mass_tranfer in self.Nodes_In_Mass_Transfer.items():
            time_start = mass_tranfer[0]
            if curr_time - time_start > DPSNodeCollectionClass.Nodes_In_Mass_Transfer_Secs:
                try:
                    del self.Nodes_In_Mass_Transfer[node]
                except Exception:
                    pass
        return

    def Timeout_Recycle_Deleted_Domains(self):
        '''
        This routine recycles deleted domains
        '''
        self.lock.acquire()
        try:
            domains = self.Domain_Deleted.keys()
            for domain in domains:
                try:
                    count = self.Domain_Deleted[domain]
                    if count <= 0:
                        del self.Domain_Deleted[domain]
                        continue
                    self.Domain_Deleted[domain] = count - 1
                except Exception:
                    continue
        except Exception, ex:
            message = 'Timeout Recycle Deleted Domains: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return

    def Timeout_Network_Partition(self):
        '''
        This routine nodes from list of nodes which could have been
        partitioned. If a "possible" partitioned node doesn't exchange
        valid Domains within a certain time period, in 99.999% chance
        it didn't host any domains so it can be considered to have
        rebooted.
        '''
        self.lock.acquire()
        try:
            dps_nodes = self.Nodes_In_Different_Partition_Hint.keys()
            for dps_node in dps_nodes:
                #print 'Timeout_Network_Partition: dps_node %s\r'%dps_node.location.show_ip()
                #Check if the dps node is still considered to be in a different
                #partition hint state
                if dps_node.partition_state != DPSNodePartitionState.different_hint:
                    try:
                        del self.Nodes_In_Different_Partition_Hint[dps_node]
                    except Exception:
                        pass
                    continue
                try:
                    value = self.Nodes_In_Different_Partition_Hint[dps_node]
                    if value >= DPSNodeCollectionClass.Nodes_In_Different_Partition_Hint_Timeout:
                        dps_node.partition_state = DPSNodePartitionState.same
                        del self.Nodes_In_Different_Partition_Hint[dps_node]
                    else:
                        value += 1
                        self.Nodes_In_Different_Partition_Hint[dps_node] = value
                except Exception:
                    pass
        except Exception, ex:
            message = 'Timeout_Network_Partition: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
        self.lock.release()
        return

    def Timeout_Leader_Election(self):
        '''
        Elect Leader if one is not present
        '''
        self.lock.acquire()
        while True:
            try:
                if DPSNodeCollectionClass.Leader_Election_Trigger == False:
                    #print 'Leader Election Not Triggered\r'
                    self.Leader_Not_Present_Count = 0
                    break
    #            if len(self.Node_Hash) < 2:
    #                print 'Not enough nodes to run leader election\r'
    #                self.Leader_Not_Present_Count = 0
    #                break
                leader = self.Leader.get()
                if leader is not None:
                    #print 'Leader Already present %s\r'%leader.location.show_ip()
                    self.Leader_Not_Present_Count = 0
                    break
                if self.Leader_Not_Present_Count >= DPSNodeCollectionClass.Leader_Not_Present_Count_Max:
                    self.leader_election_run()
                    self.Leader_Not_Present_Count = 0
                    break
                self.Leader_Not_Present_Count += 1
                #print 'Leader_Not_Present_Count %s\r'%self.Leader_Not_Present_Count
            except Exception, ex:
                message = 'Timeout Leader Election: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            break
        self.lock.release()
        return

    def Timeout_Timer(self):
        '''
        This is the timer routine.
        '''
        self.timer_run += 1
        if DPSNodeCollectionClass.timer_thread_stop:
            #Someone wants to stop this thread
            DPSNodeCollectionClass.timer_thread_stop = False
            DPSNodeCollectionClass.timer_thread_started = False
            return
        #Remove Nodes from Network Partition Hint 
        self.Timeout_Network_Partition()
        #Check if Leader needs to be elected
        self.Timeout_Leader_Election()
        self.Timeout_Clear_Domains_From_Down_Nodes()
        self.Timeout_Clear_Nodes_From_Mass_Transfer()
        #Recycle Logs
        #ClusterLog.show()
        ClusterLog.timeout()
        #Recycle Deleted Domains
        self.Timeout_Recycle_Deleted_Domains()
        #Exchange Domain Mapping if necessary
        self.domain_mapping_exchange_timer()
        if self.leader_local():
            self.leader_timer_routine()
        else:
            self.non_leader_timer_routine()
        #Verify Replication Factors
        self.timer_verify_replication_factor()
        #Restart the Timer Thread
        self.timer = Timer(DPSNode.TIMEOUT_INTERVAL, self.Timeout_Timer)
        DPSNodeCollectionClass.timer_thread_started = True
        self.timer.start()
        return

    def Domain_Recreate_Timer(self):
        '''
        This is the timer routine that will handle re-creating a domain
        on the DCS that is not handled by any node.
        '''
        while True:
            if DPSNodeCollectionClass.domain_recreate_thread_stop:
                #Someone wants to stop this thread
                DPSNodeCollectionClass.domain_recreate_thread_stop = False
                DPSNodeCollectionClass.domain_recreate_thread_started = False
                return
            domains = self.Domain_With_No_Nodes.keys()
            if len(domains) == 0:
                break
            dps_nodes = self.lowest_loaded_nodes(4)
            if len(dps_nodes) == 0:
                break
            #For now just log the Domains which don't have any Nodes
            node_index = 0
            for domain in domains:
                message = 'ALERT! Domain %s is not hosted by any Node'%domain
                ClusterLog.log_message(message)
                try:
                    recover_tuple = self.Domain_With_No_Nodes[domain]
                except Exception:
                    continue
                curr_time = time.time()
                detect_time = recover_tuple[0]
                last_fix_time = recover_tuple[1]
                if curr_time - last_fix_time < DPSNode.DOMAIN_RECOVER_RETRY:
                    continue
                #Try to recover this domain.
                message = 'Trying to Recover Domain %d'%domain
                ClusterLog.log_message(message)
                try:
                    replication_factor = self.Domain_Replication_Hash[domain]
                except Exception:
                    replication_factor = self.REPLICATION_FACTOR_DEFAULT
                self.Domain_With_No_Nodes[domain] = (detect_time, curr_time)
                dps_node = dps_nodes[node_index]
                #Initiate the recovery
                dcslib.dps_domain_recover_on_node(dps_node, domain, replication_factor)
                node_index += 1
                node_index = node_index % len(dps_nodes)
            break
        #Restart the Timer Thread
        self.domain_recreate_timer = Timer(self.DOMAIN_RECREATE_TIMEOUT, self.Domain_Recreate_Timer)
        DPSNodeCollectionClass.domain_recreate_thread_started = True
        self.domain_recreate_timer.start()
        return

    def Delete(self):
        '''
        This routine deletes all objects.
        @attention: This routine must be called with the global lock held
        '''
        try:
            #Clear all nodes except local node
            for node_key in self.Node_Hash.keys():
                try:
                    dps_node = self.Node_Hash[node_key]
                    dps_node.domains.clear()
                except Exception:
                    continue
                if dps_node == self.Local.get():
                    continue
                try:
                    del self.Node_Hash[node_key]
                except Exception:
                    continue
            self.Node_Hash.clear()
            self.Domain_Hash.clear()
            self.Domain_Replication_Hash.clear()
            self.Domain_Not_Meeting_Replication_Factors.clear()
            self.Domain_With_No_Nodes.clear()
            self.Nodes_Heavy_Load.clear()
            self.Nodes_In_Different_Partition_Hint.clear()
            self.Nodes_In_Mass_Transfer.clear()
            #self.Attribute.clear()
            self.Leader.set(None)
            self.Domain_Deleted.clear()
            DPSNodeCollectionClass.valid = False
            DPSNodeCollectionClass.dmc_config_version = 0L
        except Exception:
            pass

    def Stop_Threads(self):
        '''
        This routine stops the cluster timer threads
        @attention: This routine must be called with the global lock held
        '''
        DPSNodeCollectionClass.timer_thread_stop = True
        DPSNodeCollectionClass.domain_recreate_thread_stop = True
        DPSNodeCollectionClass.timer_thread_started = False
        DPSNodeCollectionClass.domain_recreate_thread_started = False
        DPSNodeCollectionClass.Leader_Election_Trigger = False
        self.Delete()
        return

    def Start_Threads(self):
        '''
        This routine restarts the cluster timer threads
        @attention: This routine must be called with the global lock held
        '''
        DPSNodeCollectionClass.valid = True
        if not DPSNodeCollectionClass.timer_thread_started:
            self.timer = Timer(DPSNode.TIMEOUT_INTERVAL, self.Timeout_Timer)
            DPSNodeCollectionClass.timer_thread_started = True
            DPSNodeCollectionClass.timer_thread_stop = False
            self.timer.start()
        else:
            self.timer = None
        #Start a single(ton) domain recreate thread
        if not DPSNodeCollectionClass.domain_recreate_thread_started:
            self.domain_recreate_timer = Timer(self.DOMAIN_RECREATE_TIMEOUT,
                                               self.Domain_Recreate_Timer)
            DPSNodeCollectionClass.domain_recreate_thread_started = True
            DPSNodeCollectionClass.domain_recreate_thread_stop = False
            self.domain_recreate_timer.start()
        else:
            self.domain_recreate_timer = None
        return

    def show(self):
        '''
        Display Contents of a Cluster
        '''
        print '------------------------------------------------------------------\r'
        print 'DCS Node List\r'
        time_secs = time.time()
        for dps_node in self.Node_Hash.values():
            if dps_node == self.Local.get():
                last_contact = 0
            else:
                last_contact = int(time_secs - dps_node.last_contacted_secs)
            print'%s: State %s, DMC Version %s, Last Contact %s secs ago\r'%(dps_node.location.show_ip(),
                                                                             DPSNodeState.strings[dps_node.state],
                                                                             dps_node.dmc_config_version, 
                                                                             last_contact)
        print '------------------------------------------------------------------\r'
        #print 'Showing Node --> Domain Mapping:\r'
        #for dps_node in self.Node_Hash.values():
        #    print'%s: %s\r'%(dps_node.location.show_ip(), dps_node.show_domains())
        #print '------------------------------------------------------------------\r'
        print 'Showing Domain --> Node Mapping:\r'
        for domain_id in self.Domain_Hash.keys():
            #Form the output string
            str_out = ''
            for node in self.Domain_Hash[domain_id].values():
                str_out += '%s, '%(node.location.show_ip())
            str_out = str_out.rstrip()
            str_out = str_out.rstrip(',')
            print '%s: [%s]\r'%(domain_id, str_out)
        print '------------------------------------------------------------------\r'
        #print'node_domain_table == >> %s\r'%(self.Attribute.node_domain_table)
        #print '------------------------------------------------------------------\r'
        #print 'Node->Domain Table (*new*):\r'
        #for node_ip in self.Attribute.node_domain_table.keys():
        #    str_out = ''
        #    row = self.Attribute.node_domain_table[node_ip]
        #    node_ip_packed = struct.pack('I',node_ip)
        #    str_out += socket.inet_ntop(socket.AF_INET, node_ip_packed)
        #    str_out += ' -->> '
        #    str_out += str(row[1])
        #    print '%s\r'%str_out
        #print '------------------------------------------------------------------\r'
        #print 'node_version_table == >> %s\r'%(self.Attribute.node_version_table)
        if self.Leader.dps_node is not None:
            print "cluster **LEADER** -->> [%s]\r"%self.Leader.dps_node.location.show_ip()
        else:
            print "cluster **LEADER** not elected\r"
        print '------------------------------------------------------------------\r'
        if self.leader_local():
            print 'Cluster DMC Version %s\r'%self.dmc_config_version_get()
            #print '------------------------------------------------------------------\r'
            for dps_node in self.Node_Hash.values():
                dps_node.statistics.show()
            print '------------------------------------------------------------------\r'
        for domain in self.Domain_Replication_Hash.keys():
            num_nodes, replication = self.domain_get_replication_status(domain)
            print 'Domain %s, Replication Factor %s, Nodes %s\r'%(domain, replication, num_nodes)
        print '------------------------------------------------------------------\r'
        return
