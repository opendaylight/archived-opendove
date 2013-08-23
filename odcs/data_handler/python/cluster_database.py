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

import struct
import socket
import random
import threading

from object_collection import DOVEStatus
from dcs_objects.Cluster import Cluster
from dcs_objects.Cluster import DPSNodePartitionState
from dcs_objects.Cluster import DPSNodeClusterState
from dcs_objects.Cluster import DPSNodeState
from dcs_objects.Cluster import DPSNodeCollectionClass
from object_collection import DpsCollection
from object_collection import DpsLogLevels
#from Utilities.ArrayInteger import ArrayInteger
from dcs_objects.DPSNodeStatistics import DPSNodeStatValues

import dcslib

class ClusterDatabase(object):
    '''
    This class handles the Cluster Database. In other words this database
    stores the attributes of every node in the Cluster.
    This class has routines which can be called from the C code. Routines
    that are not supposed to called from the C code MUST be marked with the
    following:
    @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
    '''
    #cluster_lock = threading.Lock()

    def __init__(self):
        '''
        Constructor:
        '''
        self.cluster = Cluster()
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

#    def ipv4_pack(self, IPAddress):
#        '''
#        This routine packs an IPv4 structure based on ip_addr_t and returns
#        an IPAddressLocation Structure
#        '''
#        return struct.pack(self.fmt_ipv4_ipunion_addr,
#                           IPAddress.inet_type,
#                           IPAddress.port,
#                           0, 0,
#                           IPAddress.ip_value,
#                           '')
#
#    def ipv6_pack(self, IPAddress):
#        '''
#        This routine packs an IPv6 structure based on ip_addr_t and returns
#        an IPAddressLocation Structure
#        '''
#        return struct.pack(self.fmt_ipv6_addr,
#                           IPAddress.inet_type,
#                           IPAddress.port,
#                           0, 0,
#                           IPAddress.ip_value)

    def Node_Add(self, ip_type, ip_packed, port):
        '''
        This routine adds a dps node to the collection. Note that
        this routine will create a new dps node if the dps node with
        the corresponding ip doesn't exist.
        @attention: To update Local Node's IP call Node_Local_Update
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            ret_val = DOVEStatus.DOVE_STATUS_OK
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                dps_node.location.port = port
                break
            except Exception:
                pass
            try:
                dps_node = self.cluster.node_add(ip_type, ip_val, port)
                #Leader election must start now if not started yet
                self.cluster.leader_election_trigger_set(True)
            except (Exception, MemoryError):
                ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
            break
        self.lock.release()
        return ret_val

    def Node_Get_Port(self, ip_type, ip_packed):
        '''
        This routine determines the client server protocol port of a node from the cluster
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        port = 0
        self.lock.acquire()
        while True:
            ret_val = DOVEStatus.DOVE_STATUS_OK
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                port = dps_node.location.port
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_FOUND
            break
        self.lock.release()
        return (ret_val, port)

    def Node_Delete(self, ip_type, ip_packed):
        '''
        This routine deletes a node from the cluster
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
                self.cluster.node_delete(ip_val)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            break
        self.lock.release()
        return ret_val

    def Node_Local_Update(self, ip_type, ip_packed, port):
        '''
        This routine should be called to update the local Node's IP and port 
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        '''
        self.lock.acquire()
        while True:
            ret_val = DOVEStatus.DOVE_STATUS_OK
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                self.cluster.node_update_local(ip_type, ip_val, port)
            except (Exception, MemoryError):
                ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
            break
        self.lock.release()
        return ret_val

    def Node_Local_Get_IPAddress(self):
        '''
        This routine gets the local node IP Address
        @attention: This routine must be called with the global lock held
        @return: The Local Node's IP Address
        @rtype: Integer or String
        '''
        ip_value = 0
        while True:
            try:
                local = self.cluster.Local.get()
                ip_value = local.location.ip_value
            except Exception:
                break
            break
        return ip_value

    def Node_Local_Activate(self, factive):
        '''
        This routine should be called to change the state of local node 
        Active (Y/N). This indicates if the local node is part of DCS Cluster
        @param factive: Whether to activate the local node or not
        @type factive: Integer
        @return: If the Local Node can be put in the desired state 1: Allow, 0: Deny
        @rtype: Integer
        '''
        status = DOVEStatus.DOVE_STATUS_OK
        fsendheartbeat = False
        self.lock.acquire()
        while True:
            local = self.cluster.Local.get()
            leader = self.cluster.Leader.get()
            if leader is not None and leader != local:
                fsendheartbeat = True
            if local is not None:
                if factive:
                    self.cluster.Start_Threads()
                    local.state = DPSNodeState.up
                    message = 'DCS: Local Node Active'
                    dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                else:
                    fallow = self.cluster.node_inactivate_allow(local)
                    if fallow:
                        local.state = DPSNodeState.inactive
                        leader = self.cluster.Leader.get()
                        self.cluster.Stop_Threads()
                        message = 'DCS: Local Node Inactive'
                        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                    else:
                        status = DOVEStatus.DOVE_STATUS_BUSY
                        message = 'DCS: Local Node Busy, cannot be reset!'
                        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                        fsendheartbeat = False
            if fsendheartbeat and local is not None and leader is not None:
                dcslib.dps_cluster_send_heartbeat_to(factive,
                                                     local.dmc_config_version,
                                                     leader.location.ip_value_packed)
            break
        self.lock.release()
        return status

    def Node_Add_Domain_PYTHON(self, ip_type, ip_packed, port,
                               domain_id, replication_factor):
        '''
        @attention: This routine must be called with the global lock held
        @attention: IMPORT THIS FUNCTION ONLY FROM PYTHON CODE
        @param fLocal: If this is the local Node
        @type fLocal: Boolean
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        @param domain_id: Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @type replication_factor: Integer
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
            except Exception:
                #try:
                #    #Local node always should be found, so this must be a remote node
                #    dps_node = self.cluster.node_add(ip_type, ip_val, port)
                #except (Exception, MemoryError):
                #    ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
                #    break
                ret_val = DOVEStatus.DOVE_STATUS_NOT_FOUND
                break
            try:
                self.cluster.node_add_domain(dps_node, domain_id, replication_factor)
            except (Exception, MemoryError):
                ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
                break
            break
        return ret_val

    def Node_Add_Domain(self, ip_type, ip_packed, port, domain_id, replication_factor):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param port: The DPS Node Port (for Client Server Communication)
        @type port: Integer
        @param domain_id: Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @type replication_factor: Integer
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        ret_val = self.Node_Add_Domain_PYTHON(ip_type, ip_packed, port, 
                                              domain_id, replication_factor)
        self.lock.release()
        return ret_val

    def Node_Delete_Domain_PYTHON(self, ip_type, ip_packed, domain_id):
        '''
        @attention: This routine must be called with the global lock held
        @attention: IMPORT THIS FUNCTION ONLY FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param domain_id: Domain ID
        @type domain_id: Integer
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                self.cluster.node_delete_domain(dps_node, domain_id)
            except Exception:
                pass
            break
        return ret_val

    def Node_Delete_Domain(self, ip_type, ip_packed, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param domain_id: Domain ID
        @type domain_id: Integer
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        ret_val = self.Node_Delete_Domain_PYTHON(ip_type, ip_packed, domain_id)
        self.lock.release()
        return ret_val

    def Node_Delete_All_Domains_PYTHON(self, ip_type, ip_packed):
        '''
        @attention: This routine must be called with the global lock held
        @attention: IMPORT THIS FUNCTION ONLY FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                self.cluster.node_delete_all_domains(dps_node)
            except Exception:
                pass
            break
        return ret_val

    def Node_Delete_All_Domains(self, ip_type, ip_packed):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        ret_val = self.Node_Delete_All_Domains_PYTHON(ip_type, ip_packed)
        self.lock.release()
        return ret_val

    def Nodes_Reconcile_PYTHON(self, node_list):
        '''
        This routine adds new nodes to the cluster node set based on the
        node_list provided and deleted nodes from the cluster which are
        no longer part of the node_list
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        @attention: This routine assumes that the calling routine has the global
                    lock held
        @param node_list: List consisting of (ip_type, ip_packed, port) tuples
        @type node_list: [] of (integer, bytearray, port)
        @return new_nodes: List of DPS Nodes which were newly added
        @rtype: [DPSNode]
        '''
        #Get a temporary_list of all nodes in the new list
        updated_nodes = {}
        new_nodes = []
        for node_tuple in node_list:
            ip_type = node_tuple[0]
            ip_packed = node_tuple[1]
            port = node_tuple[2]
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                continue
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                updated_nodes[dps_node] = True
                dps_node.location.port = port
            except Exception:
                try:
                    dps_node = self.cluster.node_add(ip_type, ip_val, port)
                    updated_nodes[dps_node] = True
                    message = 'Added new DCS Node %s to Cluster'%dps_node.location.show_ip()
                    dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                    new_nodes.append(dps_node)
                except (Exception, MemoryError):
                    pass
        #Remove all nodes (other than local node and new nodes from list)
        for dps_node in self.cluster.Node_Hash.values():
            try:
                updated_node = updated_nodes[dps_node]
                continue
            except Exception:
                if dps_node.flocal:
                    continue
                try:
                    ip_val = dps_node.location.ip_value
                    self.cluster.node_delete(ip_val)
                    message = 'Deleted DCS Node %s from Cluster'%dps_node.location.show_ip()
                    dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                except Exception:
                    pass
        #Leader election must start now if not started yet
        self.cluster.leader_election_trigger_set(True)
        return new_nodes

    def Nodes_Update(self, node_list):
        '''
        This routine should be called when the controller sends in an
        updated list of dcs nodes from the controller.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param node_list: List consisting of (ip_type, ip_packed, port) tuples
        @type node_list: [] of (integer, bytearray, port)
        '''
        #Get a temporary_list of all nodes in the new list
        while True:
            local = self.cluster.Local.get()
            if local is None:
                break
            try:
                message = 'Got Updated List of Cluster Nodes from DMC, Local Node State %s\r'%(DPSNodeState.strings[local.state])
                dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
            except Exception:
                message = 'Got Updated List of Cluster Nodes from DMC, Local Node State Unknown\r'%(DPSNodeState.strings[local.state])
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            if local.is_inactive():
                break
            self.lock.acquire()
            try:
                new_nodes = self.Nodes_Reconcile_PYTHON(node_list)
            except Exception, ex:
                message = 'Nodes_Reconcile: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                new_nodes = []
            self.lock.release()
            #Exchange Domain Mapping with the new nodes
            self.cluster.domain_mapping_exchange(new_nodes)
            break
        return

    def Node_Touch(self, ip_type, ip_packed):
        '''
        This routine should be called whenever a REST message is received from
        a remote node.
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        '''
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
                self.cluster.node_touch(ip_val)
            except Exception:
                break
            break
        self.lock.release()
        return

    def Node_Validate_Remote_DCS(self, ip_type, ip_packed):
        '''
        This routine checks if the ip is a valid DCS node or not
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        '''
        valid = 0
        while True:
            local = self.cluster.Local.get()
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                if dps_node != local:
                    valid = 1
            except Exception:
                pass
            break
        return valid

    def Node_Heartbeat(self, ip_type, ip_packed, dmc_config_version, factive):
        '''
        This routine should be called when a heartbeat is received. A receipt
        of a heartbeat by the leader should 
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param dmc_config_version: The DMC Config version as seen by the node
        @type dmc_config_version: Long Integer
        @param factive: Whether the node is active (1 active, 0 inactive)
        @type factive: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            self.lock.acquire()
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                dps_node.dmc_config_version = dmc_config_version
                if factive:
                    dps_node.state = DPSNodeState.up
                else:
                    dps_node.state = DPSNodeState.inactive
                #Update the Cluster's DMC version
                if self.cluster.dmc_config_version_get() < dmc_config_version:
                    self.cluster.dmc_config_version_set(dmc_config_version)
            except Exception, ex:
                self.lock.release()
                message = 'Node_Heartbeat: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                break
            self.lock.release()
            try:
                #The Leader election has already run (via Node Touch)
                local = self.cluster.Local.get()
                if self.cluster.leader_local():
                    #If I am the leader send back list of nodes
                    self.cluster.leader_send_node_list_status_to(dps_node)
                elif dps_node == self.cluster.Leader.get():
                    #Why is the leader sending a heartbeat. It probably
                    #ran leader election and decided I am the leader.
                    self.cluster.leader_election_run()
                elif local != dps_node:
                    #print 'Got heartbeat from non-leader %s, Initiating domain exchange\r'%dps_node.location.show_ip()
                    self.cluster.domain_mapping_exchange([dps_node])
            except Exception, ex:
                message = 'Node_Heartbeat: Exception in Sending [%s]'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                pass
            break
        return ret_val

    def Node_Heartbeat_Request(self, ip_type, ip_packed):
        '''
        This routine should be called when a heartbeat request is received
        @param ip_type: socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            self.lock.acquire()
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                #print 'Got heartbeat request from %s\r'%dps_node.location.show_ip()
                local = self.cluster.Local.get()
                if dps_node.partition_state == DPSNodePartitionState.same:
                    #Got heartbeat request from a DCS Node in same partition. 
                    #Make this node the leader
                    self.cluster.Leader.set(dps_node)
                    leader = dps_node
                    if local == dps_node:
                        dcslib.dps_cluster_set_leader(1,
                                                      dps_node.location.ip_value_packed,
                                                      dps_node.location.port)
                    else:
                        dcslib.dps_cluster_set_leader(0,
                                                      dps_node.location.ip_value_packed,
                                                      dps_node.location.port)
                else:
                    #Different partition, leader election was run in node_touch
                    leader = self.cluster.Leader.get()
            except Exception, ex:
                self.lock.release()
                message = 'Node_Heartbeat_Request, Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                break
            self.lock.release()
            try:
                if dps_node == leader:
                    #Only send to heartbeat to who I think is the leader
                    if local.is_inactive():
                        factive = 0
                    else:
                        factive = 1
                    dcslib.dps_cluster_send_heartbeat_to(factive,
                                                         local.dmc_config_version,
                                                         leader.location.ip_value_packed)
                elif self.cluster.Local.get() != dps_node:
                    message = 'Got heartbeat request from Leader %s in Different Partition\r'%dps_node.location.show_ip()
                    dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                    self.cluster.domain_mapping_exchange([dps_node])
            except Exception, ex:
                message = 'Node_Heartbeat_Request, Exception in Sending [%s]'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                pass
            break
        return ret_val

    def Node_Heartbeat_Process(self, fProcess):
        '''
        This routine determines if the cluster should be processing heartbeats
        @param fProcess: Whether to Process heartbeats
        @type fProcess: Integer
        '''
        if fProcess == 0:
            self.cluster.node_status_change_allow_set(False)
            message = 'Not Processing heartbeats\r'
            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
        else:
            self.cluster.node_status_change_allow_set(True)
            message = 'Processing heartbeats\r'
            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
        return

    def Nodes_Status(self, leader_ip_type, leader_ip_packed, config_version, dps_nodes):
        '''
        This routine should be called when status of all nodes is received from
        the Leader
        @param leader_ip_type: socket.AF_INET6 or socket.AF_INET
        @type leader_ip_type: Integer
        @param leader_ip_packed: Packed IP Address
        @type leader_ip_packed: ByteArray
        @param config_version: The Configuration Version as seen by the Cluster
        @type config_version: Long Integer
        @param dps_nodes: A Collection of [Node IP Type, Node IP Packed, Up/Down (1/0)]
        @type dps_nodes: [] of (Integer, ByteArray, Integer)
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                leader_ip_val = self.ip_get_val_from_packed[leader_ip_type](leader_ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            self.lock.acquire()
            while True:
                try:
                    dps_node = self.cluster.Node_Hash[leader_ip_val]
                except Exception:
                    break
                try:
                    if dps_node.partition_state == DPSNodePartitionState.same:
                        #Got heartbeat request from a DCS Node in same partition. 
                        #Make this node the leader
                        self.cluster.Leader.set(dps_node)
                        leader = dps_node
                        dcslib.dps_cluster_set_leader(0,
                                                      dps_node.location.ip_value_packed,
                                                      dps_node.location.port)
                    else:
                        #Different partition, leader election was run in node_touch
                        leader = self.cluster.Leader.get()
                    if dps_node != leader:
                        message = 'Got Node List from Leader in different partition %s, my leader is %s\r'%(dps_node.location.show_ip(), leader.location.show_ip())
                        dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                        break
                    #Update the Cluster's DMC version
                    if self.cluster.dmc_config_version_get() < config_version:
                        self.cluster.dmc_config_version_set(config_version)
                    for node_tuple in dps_nodes:
                        ip_type = node_tuple[0]
                        ip_packed = node_tuple[1]
                        state_up = node_tuple[2]
                        try:
                            ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
                        except Exception:
                            continue
                        if state_up == DPSNodeClusterState.inactive:
                            #Inactive
                            self.cluster.node_inactivate(ip_val)
                            self.cluster.node_touch(ip_val)
                        elif state_up == DPSNodeClusterState.active:
                            #Up
                            self.cluster.node_touch(ip_val)
                        else:
                            #Down or hint down
                            self.cluster.node_hint_down(ip_val)
                except Exception, ex:
                    message = 'Nodes_Status: Exception [%s]'%ex
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                break
            self.lock.release()
            break
        return ret_val

    def Domain_Mapping_Exchange(self):
        '''
        This routine exchanges the Local Domain mapping with all nodes in the 
        cluster
        '''
        DPSNodeCollectionClass.Exchange_Domain_Mapping = True
        return

    def Domain_Activate_Mapping_Exchange_Synchronous(self,
                                                     ip_source_type, 
                                                     ip_source_packed):
        '''
        This routine should be used when a Domain Activate is called AND when
        the new mapping needs to exchanged synchronously and in a particular order.
        In case the mappings are exchange synchronously then the Source Node
        (for the Domain Transfer) can be notified at the end. That would
        guarantee that all other nodes were notified before the Source Node.
        @param ip_source_type: socket.AF_INET6 or socket.AF_INET
        @type ip_source_type: Integer
        @param ip_source_packed: Packed IP Address
        @type ip_source_packed: ByteArray
        '''
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_source_type](ip_source_packed)
            except Exception:
                print 'Domain_Activate: Cannot get source ip\r'
                break
            nodes = []
            self.lock.acquire()
            try:
                src_node = self.cluster.Node_Hash[ip_val]
            except Exception:
                src_node = None
            try:
                for node in self.cluster.Node_Hash.values():
                    if node == src_node:
                        #Don't add src node till the end
                        continue
                    nodes.append(node)
                if src_node is not None:
                    nodes.append(src_node)
            except Exception, ex:
                message = 'Domain_Activate_Mapping: Exception [%s]'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            self.lock.release()
            self.cluster.domain_mapping_exchange(nodes)
            break
        return

    def Show(self):
        '''
        This routine shows the cluster details
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        try:
            self.cluster.show()
            ret_val = DOVEStatus.DOVE_STATUS_OK
        except Exception, ex:
            log.info('Show Exception %s', ex)
            ret_val = DOVEStatus.DOVE_STATUS_EMPTY
        self.lock.release()
        return ret_val

    def Domain_Get_Nodes_PYTHON(self, domain_id):
        '''
        @attention: This routine must be called with the global lock held
        @attention: IMPORT THIS FUNCTION ONLY FROM PYTHON CODE
        This routine returns the list of dps nodes that handle a given domain.
        Each element in the list is a tuple of the form:
        (ip_type, ip_packed, port)
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        list_nodes = []
        try:
            #Append local node first
            local = self.cluster.Local.get()
            try:
                local_domains = local.domains[domain_id]
                list_nodes.append((local.location.inet_type,
                                   local.location.ip_value_packed,
                                   local.location.port))
            except Exception:
                pass
            for node in self.cluster.Domain_Hash[domain_id].values():
                if node == local:
                    continue
                if node.is_up():
                    list_nodes.append((node.location.inet_type, node.location.ip_value_packed, node.location.port))
        except Exception:
            pass
        return list_nodes

    def Domain_Get_Nodes(self, domain_id):
        '''
        @attention: This routine must NOT be called with the global lock held
        This routine returns the list of dps nodes that handle a given domain.
        Each element in the list is a tuple of the form:
        (ip_type, ip_packed, port)
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        self.lock.acquire()
        list_nodes = self.Domain_Get_Nodes_PYTHON(domain_id)
        self.lock.release()
        return list_nodes

    def Domain_Get_Replication_Factor(self, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine returns the number of DPS Nodes handling a domain
        (ip_type, ip_packed, port)
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        self.lock.acquire()
        try:
            replication_factor = len(self.cluster.Domain_Hash[domain_id].values())
        except Exception:
            replication_factor = 0
        self.lock.release()
        return replication_factor

    def Domain_Delete(self, domain_id):
        '''
        This routine deletes domain from the cluster
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        self.lock.acquire()
        self.cluster.domain_delete(domain_id)
        self.lock.release()
        return

    def Domain_Delete_Send_To_All(self, domain_id):
        '''
        This routine sends the Domain Delete to all nodes in the
        cluster
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        self.lock.acquire()
        try:
            dps_nodes = self.cluster.Node_Hash.values()
        except Exception:
            dps_nodes = []
        self.lock.release()
        for dps_node in dps_nodes:
            if dps_node.flocal:
                continue
            dcslib.dps_rest_domain_delete_send_to_dps_node(dps_node.location.ip_value_packed,
                                                           domain_id)
        return

    def Domain_Exists(self, domain_id):
        '''
        This routine checks the Domain Existence in the cluster
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        fExists = 0
        self.lock.acquire()
        try:
            domain = self.cluster.Domain_Hash[domain_id]
            fExists = 1
        except Exception:
            pass
        self.lock.release()
        return fExists

    def VNID_Existence_Send_To_All(self, domain_id, vnid, fAdd):
        '''
        This routine sends the Domain Delete to all nodes in the
        cluster
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID
        @type vnid: Integer
        @param fAdd: Whether Add(1) or Delete(0)
        @type fAdd: Integer
        '''
        self.lock.acquire()
        try:
            dps_nodes = self.cluster.Node_Hash.values()
        except Exception:
            dps_nodes = []
        self.lock.release()
        for dps_node in dps_nodes:
            if dps_node.flocal:
                continue
            if fAdd == 0:
                dcslib.dps_rest_vnid_delete_send_to_dps_node(dps_node.location.ip_value_packed,
                                                             domain_id, vnid)
            else:
                dcslib.dps_rest_vnid_add_send_to_dps_node(dps_node.location.ip_value_packed,
                                                          domain_id, vnid)
        return

    def Cluster_Get_All_Nodes(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine will return all the dps nodes in the cluster.
        Each element of the list will be a tuple of the form.
        (ip_type,ip_packed)
        '''
        cluster_nodes = []
        self.lock.acquire()
        try:
            dps_nodes = self.cluster.Node_Hash.values()
            for dps_node in dps_nodes:
                if not dps_node.is_up():
                    continue
                cluster_nodes.append((dps_node.location.inet_type, 
                                      dps_node.location.ip_value_packed))
        except Exception:
            pass
        self.lock.release()
        return cluster_nodes

    def Node_Get_Domains(self, ip_type, ip_packed):
        '''
        This routine returns the domains handled by a node.
        The domains are returned as a string.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: DPS Node socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: DPS Node Packed IP Address
        @type ip_packed: ByteArray
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        domain_str = ''
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                domain_str = str(dps_node.domains)
                #print 'Node_Get_Domains: %s\r'%domain_str
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_FOUND
                break
            break
        self.lock.release()
        return (ret_val,domain_str)

    def Node_Print_Domain_Mapping(self, dps_node, domain_set):
        '''
        This routine logs the domain mapping sent by the dps node
        @param dps_node: The DPS Node
        @param dps_node: DpsNode
        @param domain_set: The set of domains. Key = domain id, Value = Replication Factor
        @type domain_set: {}
        '''
        items = domain_set.keys()
        if len(items) <= 0:
            return
        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, 
                                     '-------------------------------------------------------------------------------')
        start_index = 0
        end_index = len(items)
        while start_index < end_index:
            print_items = items[start_index:start_index+10]
            message = 'node %s: Domains %s'%(dps_node.location.show_ip(), print_items)
            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
            start_index += 10
        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, 
                                     '-------------------------------------------------------------------------------')
        return

    def Node_Set_Domain_Mapping(self, ip_type, ip_packed, port, version, domain_str):
        '''
        This routine sets the domain list for a node. The domain_str has to be
        converted to a 'PYTHON' list before setting.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param ip_type: DPS Node socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: DPS Node Packed IP Address
        @type ip_packed: ByteArray
        @param port: DPS Node RAW Protocol Port
        @type port: Integer
        @param version: The domain mapping version number seen by the DPS Node
        @type version: Integer
        @param domain_str: The List of Domains handled by the node in string form
        @type domain_str: String
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                dps_node = self.cluster.Node_Hash[ip_val]
                #print 'Node_Set_Domain_Mapping: node %s, domain_str %s\r'%(dps_node.location.show_ip(), domain_str)
                #new_domain_list = ArrayInteger.GenerateToList(domain_str, True)
                new_domain_set = eval(domain_str)
                #print 'Node_Set_Domain_Mapping %s\r'%new_domain_set
                #self.Node_Print_Domain_Mapping(dps_node, new_domain_set)
                #Check if any de-active domains in the dps_node need to be deleted
                domains_deactive = dps_node.domains_get_deactivated()
                if len(domains_deactive) > 0:
                    message = 'DPS Node %s, Deactive Domain %s'%(dps_node.location.show_ip(), domains_deactive)
                    dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                for domain_deactive in domains_deactive:
                    try:
                        domain_present = new_domain_set[domain_deactive]
                        #It's present in the Nodes Mapping so remove from deactivate
                        dps_node.domain_activate(domain_deactive)
                        message = "Activated (Deactive) Domain %s in DPS Node %s"%(domain_deactive,
                                                                                   dps_node.location.show_ip())
                        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                    except Exception:
                        #Not present in new mapping either. Delete
                        try:
                            self.cluster.node_delete_domain(dps_node, domain_deactive)
                            message = "Deleted Domain %s from DPS Node %s"%(domain_deactive,
                                                                            dps_node.location.show_ip())
                            dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                        except Exception:
                            pass
                #Check if any domains previously held by this dps_node needs to be deactivated
                domains_active = dps_node.domains_get()
                for domain_active in domains_active:
                    try:
                        domain_present = new_domain_set[domain_active]
                    except Exception:
                        #Not present in new mapping. Deactivate
                        dps_node.domain_deactivate(domain_active)
                        message = "Deactivated Domain %s in DPS Node %s"%(domain_active,
                                                                          dps_node.location.show_ip())
                        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                #Add all the new ones
                for domain, replication in new_domain_set.items():
                    if dps_node.domain_supported(domain, replication):
                        try:
                            self.cluster.Domain_Replication_Hash[domain] = replication
                            continue
                        except Exception:
                            pass #Fall through
                    ret_val = self.Node_Add_Domain_PYTHON(ip_type, ip_packed, port, domain, replication)
                    if ret_val != DOVEStatus.DOVE_STATUS_OK:
                        message = "Error %d in Node_Add_Domain_PYTHON for domain %d, DPS Node %s\r" %(ret_val,
                                                                                                      domain,
                                                                                                      dps_node.location.show_ip())
                        dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                #self.cluster.node_set_domain_mapping(ip_val, version, new_domain_set.keys())
            except Exception, ex:
                #print 'Node_Set_Domain_Mapping Exception: %s\r'%ex
                ret_val = DOVEStatus.DOVE_STATUS_NOT_FOUND
                #os._exit(1)
            break
        self.lock.release()
        return ret_val

    def Domain_Get_Random_Remote_Node(self, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine returns a random DPS node that handle a given domain.
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: Tuple of the form (status, ip_type, ip_packed, port)
        '''
        self.lock.acquire()
        try:
            list_nodes = self.cluster.Domain_Hash[domain_id].values()
            #Remove the local node from the list
            list_nodes_remote = []
            for list_node in list_nodes:
                if not list_node.flocal and list_node.is_up():
                    list_nodes_remote.append(list_node)
            if len(list_nodes_remote) > 0:
                node = list_nodes_remote[random.randint(0,(len(list_nodes_remote)-1))]
                self.lock.release()
                return (DOVEStatus.DOVE_STATUS_OK,
                        node.location.inet_type, 
                        node.location.ip_value_packed, 
                        node.location.port)
            else:
                self.lock.release()
                return (DOVEStatus.DOVE_STATUS_EMPTY, 0, '', 0)
        except Exception:
            pass
        self.lock.release()
        return (DOVEStatus.DOVE_STATUS_INVALID_DOMAIN, 0, '', 0)

    def Node_Update_Domain_Statistics(self, ip_type, ip_packed, domain_id, 
                                      endpoints, tunnels, 
                                      endpoint_updates, endpoint_lookups, policy_lookups):
        '''
        This routine updates the Domain Statistics in a Node
        @param ip_type: DPS Node socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: DPS Node Packed IP Address
        @type ip_packed: ByteArray
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
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                break
            try:
                self.cluster.node_update_domain_stats(ip_val, domain_id, 
                                                      endpoints, tunnels, 
                                                      endpoint_updates, endpoint_lookups, policy_lookups)
            except Exception, ex:
                pass
            break
        self.lock.release()
        return 0

    def Node_Show_Load(self, ip_type, ip_packed):
        '''
        This routine shows the load on a particular DPS Node
        @param ip_type: DPS Node socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: DPS Node Packed IP Address
        @type ip_packed: ByteArray
        '''
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                break
            try:
                self.cluster.node_show_load(ip_val)
            except Exception:
                pass
            break
        self.lock.release()
        return

    def Get_Lowest_Loaded_Available_Nodes(self, desired_number):
        '''
        This routine returns the list of lowest loaded available nodes in the cluster
        @param desired_number: The number of nodes desired
        @type desired_number: Integer
        @return: An array of Node IPs
        @rtype:[ByteArray]
        '''
        self.lock.acquire()
        try:
            nodes = self.cluster.lowest_loaded_nodes(desired_number)
        except Exception, ex:
            nodes = []
            message = "Get Lowest Loaded Available Nodes: Exception [%s]"%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return nodes

    def Get_Lowest_Loaded_Available_Nodes_Not_Hosting(self, desired_number, domain_id):
        '''
        This routine returns the list of lowest loaded available nodes in the cluster
        that are not hosting a particular domain.
        @param desired_number: The number of nodes desired
        @type desired_number: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: An array of Node IPs
        @rtype:[ByteArray]
        '''
        self.lock.acquire()
        try:
            nodes = self.cluster.lowest_loaded_nodes_not_hosting(desired_number, domain_id)
        except Exception, ex:
            nodes = []
            message = "Get Lowest Loaded Available Nodes Not Hosting domain %d: Exception [%s]"%(domain_id, ex)
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return nodes

    def Get_Highest_Loaded_Nodes_Hosting(self, desired_number, domain_id):
        '''
        This routine returns the list of highest loaded available nodes in the cluster
        that are hosting a particular domain.
        @param desired_number: The number of nodes desired
        @type desired_number: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: An array of Node IPs
        @rtype:[ByteArray]
        '''
        self.lock.acquire()
        try:
            nodes = self.cluster.highest_loaded_nodes_hosting(desired_number, domain_id)
        except Exception, ex:
            nodes = []
            message = "Get Highest Loaded Available Nodes Hosting domain %d: Exception [%s]"%(domain_id, ex)
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return nodes

    def Set_Heavy_Load_Value(self, heavy_load_value):
        '''
        This routine sets the heavy load threshold for a node
        @param heavy_load_value: The % load that indicates heavy load
        @type heavy_load_value: Integer
        '''
        if heavy_load_value < 1 or heavy_load_value > 100:
            DPSNodeStatValues.Heavy_Load = DPSNodeStatValues.Heavy_Load_Default
        else:
            DPSNodeStatValues.Heavy_Load = heavy_load_value
        return

    def Get_DomainID_From_VNID(self, vn_id):
        '''
        This routine returns a domain ID which the virtual network (VNID) is associated with.
        @param vn_id: The Virtual Network ID
        @type vn_id: Integer
        @return: Tuple of the form (status, domain_id)
        '''
        #TODO: Remove this routine and use the controller handler
        try:
            domain_id = DpsCollection.VNID_Hash[vn_id]
            return (DOVEStatus.DOVE_STATUS_OK, domain_id)
        except Exception:
            return (DOVEStatus.DOVE_STATUS_INVALID_DVG, 0)
