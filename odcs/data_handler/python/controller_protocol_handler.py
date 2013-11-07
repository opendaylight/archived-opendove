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
from mass_transfer_handler import DPSDomainMassTransfer
from cluster_database import ClusterDatabase
from object_collection import DpsLogLevels

import struct
import time

from dcs_objects.Domain import Domain
from dcs_objects.Dvg import DVG
from dcs_objects.Policy import Policy
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.DPSClientHost import DPSClientHost

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
        #Reference to the cluster database
        self.cluster_db = ClusterDatabase()
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

    def Domain_Add_Local(self, domain_id, replication_factor):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine adds a domain/tenant to the collection of Domains handled
        by this DPS Node
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @param replication_factor: The Replication Factor of the Domain
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            self.lock.acquire()
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                #Domain doesn't exist. This routine will add
                #the domain object to the collection
                try:
                    domain = Domain(domain_id, True)
                except (Exception, MemoryError):
                    ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
                    self.lock.release()
                    break
            domain.replication_factor = replication_factor
            #Derive the DVGs in this domain
            try:
                domain_global = self.Domain_Hash_Global[domain_id]
            except Exception:
                #No DVGs to add for this domain
                self.lock.release()
                break
            domain_vnids = domain_global.keys()
            #Now we add the DVGs in this domain
            self.lock.release()
            for vnid in domain_vnids:
                ret_val = self.Dvg_Add(domain_id, vnid)
                if ret_val != DOVEStatus.DOVE_STATUS_OK:
                    self.Domain_Delete(domain_id)
                    break
            #Remove from Get Ready
            try:
                del self.MassTransfer_Get_Ready[domain_id]
            except Exception:
                pass
            break
        return ret_val

    def Domain_Delete(self, domain_id):
        '''
        This routine deletes a domain from the collection of Domains handled
        by this DPS Node
        @attention: This function must called be without the lock held
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        #Remove from Get Ready
        try:
            del self.MassTransfer_Get_Ready[domain_id]
        except Exception:
            pass
        #Remove from Local Mapping
        try:
            domain_obj = self.Domain_Hash[domain_id]
            Domain.delete(domain_obj, True)
        except Exception:
            pass
        #Remove from Global Mapping
        try:
            domain_global = self.Domain_Hash_Global[domain_id]
            for vnid in domain_global.keys():
                try:
                    del self.VNID_Hash[vnid]
                except Exception:
                    pass
                try:
                    del domain_global[vnid]
                except Exception:
                    pass
                #Remove from Reregistration
                try:
                    del self.VNID_Reregistration[vnid]
                except Exception:
                    pass
            del self.Domain_Hash_Global[domain_id]
        except Exception:
            pass
        self.lock.release()
        return DOVEStatus.DOVE_STATUS_OK

    def Domain_Get_Ready_For_Transfer(self, domain_id):
        '''
        This routine is called during Mass Transfer of Domain Data Process.
        This is Step 1 of Mass Migration. It is an indication from the Leader
        that this DPS Node should get ready to receive mass transfer messages
        from another DPS Node
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            self.lock.acquire()
            try:
                domain = self.Domain_Hash[domain_id]
                self.lock.release()
                ret_val = DOVEStatus.DOVE_STATUS_EXISTS
                break
            except Exception:
                pass
            try:
                domain = Domain(domain_id, False)
            except (Exception, MemoryError):
                self.lock.release()
                ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
                break
            #Derive the DVGs in this domain
            try:
                domain_global = self.Domain_Hash_Global[domain_id]
            except Exception:
                #No DVGs to add for this domain
                self.lock.release()
                break
            domain_vnids = domain_global.keys()
            #Now we add the DVGs in this domain
            self.lock.release()
            for vnid in domain_vnids:
                ret_val = self.Dvg_Add(domain_id, vnid)
                if ret_val != DOVEStatus.DOVE_STATUS_OK:
                    self.Domain_Delete(domain_id)
                    break
            #Add to mass transfer get ready set
            self.MassTransfer_Get_Ready[domain_id] = time.time()
            break
        return ret_val

    def Domain_Start_Transfer_To(self, domain_id, ip_type, ip_packed, port):
        '''
        This routine is called during Mass Transfer of Domain Data Process.
        This is Step 2 of Mass Migration. It is an indication from the Leader
        that this DPS Node should transfer the domain data to another DPS Node
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param ip_type: The remote DCS Server socket.AF_INET6 or socket.AF_INET
        @type ip_type: Integer
        @param ip_packed: The remote DCS Server Packed IP Address
        @type ip_packed: ByteArray
        @param port: The remote DCS Server port
        @type port: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        fmass_transfer_already_active = False
        self.lock.acquire()
        while True:
            try:
                ip_val = self.ip_get_val_from_packed[ip_type](ip_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                if domain.mass_transfer is not None:
                    #Check if it's same remote node
                    if domain.mass_transfer.remote_location.ip_value == ip_val:
                        fmass_transfer_already_active = True
                    ret_val = DOVEStatus.DOVE_STATUS_RETRY
                    break
                DpsCollection.mass_transfer_lock.acquire()
                try:
                    #Check the weight of domain required for Mass Transfer
                    mass_transfer_weight_left = DpsCollection.MassTransfer_Max - DpsCollection.MassTransfer_Active
                    if mass_transfer_weight_left > 0:
                        mass_transfer_weight_needed = DPSDomainMassTransfer.domain_mass_transfer_weight(domain)
                        #Determine how much this domain will consume from the Active Mass Transfer
                        if mass_transfer_weight_needed > mass_transfer_weight_left:
                            mass_transfer_weight_consumed = mass_transfer_weight_left
                        else:
                            mass_transfer_weight_consumed = mass_transfer_weight_needed
                        message = 'Domain [%d] Mass Transfer: Weights Left %d, Needed %d, Consumed %d\r'%(domain_id,
                                                                                                          mass_transfer_weight_left,
                                                                                                          mass_transfer_weight_needed,
                                                                                                          mass_transfer_weight_consumed)
                        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                        domain.mass_transfer = DPSDomainMassTransfer(domain,
                                                                     ip_type,
                                                                     ip_val,
                                                                     port,
                                                                     domain.mass_transfer_finish,
                                                                     mass_transfer_weight_consumed)
                        DpsCollection.MassTransfer_Active += mass_transfer_weight_consumed
                    else:
                        ret_val = DOVEStatus.DOVE_STATUS_EXCEEDS_CAP
                except Exception:
                    ret_val = DOVEStatus.DOVE_STATUS_EXCEEDS_CAP
                DpsCollection.mass_transfer_lock.release()
                if ret_val != DOVEStatus.DOVE_STATUS_OK:
                    break
                #Start the transfer
                domain.mass_transfer.transfer_start()
            except Exception, ex:
                ret_val = DOVEStatus.DOVE_STATUS_EXCEEDS_CAP
                message = 'Start_Transfer_To: Exception %s'%ex
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                break
            break
        self.lock.release()
        if ret_val != DOVEStatus.DOVE_STATUS_OK and not fmass_transfer_already_active:
            try:
                remote_location = IPAddressLocation(ip_type, ip_val, port)
                message = 'Domain %s: Mass Transfer not initiated, sending Deactivate message to remote Node %s'%(domain_id, 
                                                                                                                  remote_location.show_ip())
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                dcslib.dps_domain_deactivate_on_node(remote_location.ip_value_packed, 
                                                     self.unique_id)
            except Exception:
                pass
        return ret_val

    def Domain_Update(self, domain_id, replication_factor):
        '''
        This routine is called to update the domain properties
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @param replication_factor: The Replication Factor of the Domain
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                domain = self.Domain_Hash[domain_id]
                domain.replication_factor = replication_factor
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            break
        self.lock.release()
        return ret_val

    def Domain_Activate(self, domain_id, replication_factor):
        '''
        This routine is called during Mass Transfer of Domain Data Process.
        This is Step 5 of Mass Migration. It is an indication from DPS Node
        transferring data to this node is done with its transfer.
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @param replication_factor: The Replication Factor of the Domain
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            #Remove from Get Ready
            try:
                del self.MassTransfer_Get_Ready[domain_id]
            except Exception:
                pass
            try:
                domain = self.Domain_Hash[domain_id]
                domain.active = True
                domain.replication_factor = replication_factor
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            break
        self.lock.release()
        return ret_val

    def Domain_Deactivate(self, domain_id):
        '''
        This routine deactivates a domain from this node. It means
        this node no longer hosts the Domain but the Domain is
        still present in the DOVE Environment
        @attention: This routine must not be called with lock held
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            #Remove from Get Ready
            try:
                del self.MassTransfer_Get_Ready[domain_id]
            except Exception:
                pass
            #Remove from Local Mapping
            try:
                domain_obj = self.Domain_Hash[domain_id]
                if domain_obj.mass_transfer_processing():
                    message = 'ALERT! Cannot Deactivate domain %s'%domain_id
                    dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
                    domain_obj.show_mass_transfer()
                    ret_val = DOVEStatus.DOVE_STATUS_RETRY
                for vnid in domain_obj.DVG_Hash.keys():
                    #Remove from Reregistration
                    try:
                        del self.VNID_Reregistration[vnid]
                    except Exception:
                        pass
                message = 'Domain [%s]: Deactivating'%domain_id
                dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                Domain.delete(domain_obj, False)
            except Exception:
                break
            break
        self.lock.release()
        return ret_val

    def Domain_Delete_All_Local(self):
        '''
        This routine deletes all domains from the local database and
        informs all nodes in the cluster about its new local mapping
        @attention: Do not call this routine with global lock held
        '''
        domains = []
        self.lock.acquire()
        try:
            #Clear all mass transfers
            self.MassTransfer_Get_Ready.clear()
            #Deactivate all domains
            domains = self.Domain_Hash.keys()
        except Exception:
            pass
        self.lock.release()
        for domain in domains:
            self.Domain_Deactivate(domain)
        #Remove from Local Node Mapping
        local_dps_node = self.cluster_db.cluster.Local.get()
        if local_dps_node is not None:
            self.cluster_db.Node_Delete_All_Domains(local_dps_node.location.inet_type, 
                                                    local_dps_node.location.ip_value_packed)
        #Send Domain deletion to all nodes
        try:
            for domain in domains:
                self.cluster_db.Domain_Delete(domain)
        except Exception:
            pass
        return

    def Domain_Show_Local(self, domain_id, fdetails):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine shows the domain details for a domain handled
        by Local Node
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param fdetails: Whether to show detailed information
        @type fdetails: Integer (0 = False, 1 = True)
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        try:
            domain_obj = self.Domain_Hash[domain_id]
            domain_obj.show(fdetails)
            ret_val = DOVEStatus.DOVE_STATUS_OK
        except Exception, ex:
            ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
        self.lock.release()
        return ret_val

    def Domain_Validate_Local(self, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine validate the Domain ID handled by this DPS Node
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        try:
            domain_obj = self.Domain_Hash[domain_id]
            ret_val = DOVEStatus.DOVE_STATUS_OK
        except Exception:
            ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
        self.lock.release()
        return ret_val

    def Domain_GetAllIds_Local(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine return all domain IDs in this DPS
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The string of domain IDs separate by ','
        @rtype: String
        '''
        domain_list_str = ''
        self.lock.acquire()
        try:
            domain_list = self.Domain_Hash.keys()
            domain_list.sort(key=int)
            domain_list_str = ','.join(str(i) for i in domain_list)
        except Exception:
            pass
        self.lock.release()
        return domain_list_str

    def Domain_Recovery_Routine(self, domain_id, replication_factor):
        '''
        This routine should be invoked from a thread dedicated to this
        work. This routine will get all the configuration related to
        the domain from the DMC.
        1. Get ready for Domain.
        2. Get Policies (for src and dst vnid combination)
        3. Get Subnets (for vnids)
        4. Activate the Domain
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @param replication_factor: The Replication Factor of the Domain
        '''
        success = True
        while True:
            # try:
            #     replication_factor = self.Domain_Recovery[domain_id]
            # except Exception:
            #     break
            #Get the replication factor from the DMC
            status, replication_factor = dcslib.domain_query_from_controller(domain_id)
            message = 'Domain Recovery %s: Query Controller returns status %s, rep factor %s'%(domain_id, status, replication_factor)
            dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
            if status != DOVEStatus.DOVE_STATUS_OK:
                #Inform all nodes of the cluster that this domain is not valid
                self.cluster_db.Domain_Delete_Send_To_All(domain_id)
                break
            #Set up the Domain (Inactive on the local node)
            status = self.Domain_Get_Ready_For_Transfer(domain_id)
            if status != DOVEStatus.DOVE_STATUS_OK and status != DOVEStatus.DOVE_STATUS_EXISTS:
                message = 'Domain Recovery %s: Get Ready returns %s'%(domain_id, status)
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                break
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception:
                message = 'Domain Recovery %s: No domain_obj'%(domain_id)
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                break
            if not self.started:
                message = 'Domain Recovery %s: Not self started'%(domain_id)
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                break
            self.lock.acquire()
            try:
                vnids = domain_obj.DVG_Hash.keys()
            except Exception:
                vnids = []
            self.lock.release()
            #Get policies from DMC
            for vnid_src in vnids:
                for vnid_dst in vnids:
                    dcslib.domain_query_policy_from_controller(domain_id, vnid_src, vnid_dst, 0)
                    dcslib.domain_query_policy_from_controller(domain_id, vnid_src, vnid_dst, 1)
                    if not self.started:
                        break
                if not self.started:
                    break
            #Get subnets from DMC
            for vnid in vnids:
                #print 'Get subnets for Domain %s, VNID %s\r'%(domain_id, vnid)
                dcslib.vnid_query_subnets_from_controller(vnid)
                if not self.started:
                    return
            if not self.started:
                break
            #Activate the Domain
            status = self.Domain_Activate(domain_id, replication_factor)
            if status != DOVEStatus.DOVE_STATUS_OK:
                message = 'Domain Recovery %s: Activating returns %s\r'%(domain_id,status)
                dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                success = False
                break
            #Add Domain to Cluster Table
            dps_local = self.cluster_db.cluster.Local.get()
            self.cluster_db.Node_Add_Domain(dps_local.location.inet_type,
                                            dps_local.location.ip_value_packed,
                                            dps_local.location.port,
                                            domain_id,
                                            replication_factor)
            self.cluster_db.Domain_Mapping_Exchange()
            #Send trigger to DMC to ask Dove Switches to re-register
            message ='Domain Recovery %s: Ask DMC to request re-registration from all DOVE Switches on'%domain_id
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            ret = dcslib.dps_cluster_reregister_endpoints(domain_id, 0)
            if ret != 0:
                self.VNID_Reregistration[0] = (domain_id, 0)
            break
        if not success:
            #Delete the domain if we were not successful.
            self.Domain_Delete(domain_id)
        #Remove from Domain_Recovery Table
        try:
            del self.Domain_Recovery[domain_id]
        except Exception:
            pass
        return

    def Domain_Recovery_Thread_Start(self, domain_id, replication_factor):
        '''
        This routine starts the thread for Domain Recovery.
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param replication_factor: The Replication Factor of the Domain
        @param replication_factor: The Replication Factor of the Domain
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                if len(self.Domain_Recovery) > self.Domain_Recovery_Max:
                    ret_val = DOVEStatus.DOVE_STATUS_NO_RESOURCES
                    break
                self.Domain_Recovery[domain_id] = replication_factor
                timer = Timer(5, self.Domain_Recovery_Routine, (domain_id, replication_factor, ))
                timer.start()
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_NO_RESOURCES
            break
        self.lock.release()
        return ret_val

    def Domain_Show_Global_Mapping(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine shows the global mapping domain --> vnids present
        on this local node. This will show the mapping for the entire
        DOVE environment and check for inconsistencies.
        '''
        self.lock.acquire()
        try:
            print '-----------------------------------------------\r'
            for domain_id in self.Domain_Hash_Global.keys():
                try:
                    domain = self.Domain_Hash_Global[domain_id]
                except Exception:
                    continue
                try:
                    domain_local = self.Domain_Hash[domain_id]
                    vnid_keys_local = domain_local.DVG_Hash.keys()
                    vnid_keys_local.sort()
                    vnid_keys = domain.keys()
                    vnid_keys.sort()
                    print '[Local]  Domain %s: VNIDs %s\r'%(domain_id, vnid_keys_local)
                    if vnid_keys_local != vnid_keys:
                        print 'ALERT! Domain %s\r'%domain_id
                        print 'ALERT! Local  VNIDs %s\r'%vnid_keys_local
                        print 'ALERT! Global VNIDs %s\r'%vnid_keys
                except Exception:
                    print '[Remote] Domain %s: VNIDs %s\r'%(domain_id, domain.keys())
                #Verify correctness
                for vnid in domain.keys():
                    try:
                        if domain_id != self.VNID_Hash[vnid]:
                            print 'ALERT! VNID %s claimed by Domain %s and %s\r'%(vnid, domain_id, self.VNID_Hash[vnid])
                    except Exception:
                        pass
            print '-----------------------------------------------\r'
        except Exception:
            pass
        self.lock.release()
        return DOVEStatus.DOVE_STATUS_OK

    def Multicast_Show(self, associated_type, associated_id):
        '''
        This routine show the Multicast details for a domain
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                if associated_type == AssociatedType.ASSOCIATED_TYPE_DOMAIN:
                    domain_obj = self.Domain_Hash[associated_id]
                    domain_obj.Multicast.show()
                elif associated_type == AssociatedType.ASSOCIATED_TYPE_VNID:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    domain_obj.Multicast.show_vnid(associated_id)
                else:
                    ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
            break
        self.lock.release()
        return ret_val

    def Domain_Show_Address_Resolution(self, domain_id):
        '''
        This routine show the Address Resolution details for a domain
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        try:
            domain_obj = self.Domain_Hash[domain_id]
            domain_obj.AddressResolution.show()
            ret_val = DOVEStatus.DOVE_STATUS_OK
        except Exception:
            ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
        self.lock.release()
        return ret_val

    def Dvg_Add(self, domain_id, dvg_id):
        '''
        This routine adds a VNID to the collection. This routine
        should be called for every VNID in DOVE even if the Domain
        is not part of the list of Domains Handled by this DPS Node.
        @attention: This routine MUST not be called with the Global Lock held
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param dvg_id: The DVG ID
        @type dvg_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_id_old = self.VNID_Hash[dvg_id]
                if domain_id_old != domain_id:
                    ret_val = DOVEStatus.DOVE_STATUS_EXISTS
                    break
            except Exception:
                pass
            #Add to Global Collection
            self.VNID_Hash[dvg_id] = domain_id
            try:
                domain_global = self.Domain_Hash_Global[domain_id]
            except Exception:
                self.Domain_Hash_Global[domain_id] = {}
                domain_global = self.Domain_Hash_Global[domain_id]
            domain_global[dvg_id] = dvg_id
            #Add to Local Collection if domain is handled locally
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                #Not part of the Domain handled by this node, 
                #but we have added it to the VNID Collection
                ret_val = DOVEStatus.DOVE_STATUS_OK
                break
            try:
                dvg = domain.DVG_Hash[dvg_id]
                ret_val = DOVEStatus.DOVE_STATUS_OK
                break
            except Exception:
                pass
            try:
                dvg = DVG(domain, dvg_id)
                if domain.mass_transfer is not None:
                    domain.mass_transfer.register_vnid(dvg_id, True)
                #Create the default (allow) policy for this DVG
                policy = Policy(domain, 0,
                                Policy.type_connectivity, 
                                dvg, dvg, 67108864, 
                                struct.pack(Policy.fmt_action_struct_hdr,
                                            0, 0, Policy.action_forward))
                policy = Policy(domain, 1,
                                Policy.type_connectivity, 
                                dvg, dvg, 67108864, 
                                struct.pack(Policy.fmt_action_struct_hdr,
                                            0, 0, Policy.action_forward))
                ret_val = DOVEStatus.DOVE_STATUS_OK
            except (Exception, MemoryError):
                ret_val = DOVEStatus.DOVE_STATUS_NO_MEMORY
            break
        self.lock.release()
        return ret_val

    def Dvg_Add_Query(self, domain_id, dvg_id):
        '''
        This routine should be called when a VNID is discovered after
        calling the DMC i.e. /api/dove/dps/vnid/*/info. This is a
        different case than when the VNID was configured on the DCS.
        In this case this node must go and fetch policies and subnets
        related to this VNID if this node manages the domain.
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param dvg_id: The DVG ID
        @type dvg_id: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            #Check if domain is handled locally
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception:
                nodes = self.cluster_db.Domain_Get_Nodes(domain_id)
                #print 'Nodes Handling Domain %s = %s\r'%(domain_id, nodes)
                if len(nodes) == 0:
                    #No other node holds this domain. The domain itself
                    #needs to be recovered
                    self.Domain_Recovery_Thread_Start(domain_id, 2)
                else:
                    #Remote VNID, just store the value and done!
                    ret_val = self.Dvg_Add(domain_id, dvg_id)
                break
            #Check if VNID already existed
            try:
                dvg_obj = domain_obj.DVG_Hash[dvg_id]
            except Exception:
                #Need to recover the VNID properties
                self.VNID_Recovery[dvg_id] = (domain_id, dvg_id)
            break
        return ret_val

    def Dvg_Delete(self, vnid):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine deletes a dvg from the collection
        @param vnid: The VNID
        @type vnid: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            #Remove from Local Collection
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                break
            try:
                dvg = domain.DVG_Hash[vnid]
                dvg.delete()
                if domain.mass_transfer is not None:
                    domain.mass_transfer.register_vnid(vnid, True)
            except Exception:
                pass
            break
        #Remove from Global Collection
        try:
            del self.VNID_Hash[vnid]
        except Exception:
            pass
        try:
            del self.Domain_Hash_Global[domain_id][vnid]
            if len(self.Domain_Hash_Global[domain_id]) == 0:
                del self.Domain_Hash_Global[domain_id]
        except Exception:
            pass
        #Remove from Re-registration
        try:
            del self.VNID_Reregistration[vnid]
        except Exception:
            pass
        self.lock.release()
        return DOVEStatus.DOVE_STATUS_OK

    def Dvg_Validate(self, domain_id, dvg_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine validate the DVG ID in specified domain
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param dvg_id: The DVG ID
        @type dvg_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception, ex:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg_obj = domain_obj.DVG_Hash[dvg_id]
                ret_val = DOVEStatus.DOVE_STATUS_OK
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
            break;
        self.lock.release()
        return ret_val

    def Dvg_GetAllIds(self, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine return all DVG IDs of the domain specified by domain_id
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The string of DVG IDs separate by ','
        @rtype: String
        '''
        self.lock.acquire()
        dvg_list_str=''
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                dvg_list = domain_obj.DVG_Hash.keys()
                dvg_list.sort(key=int)
                dvg_list_str = ','.join(str(i) for i in dvg_list)
            except Exception:
                break
            break
        self.lock.release()
        return dvg_list_str

    def DVG_Recovery_Routine(self, domain_id, vnid):
        '''
        This routine retrieves the configuration related to a VNID from the DMC
        @attention: This routine MUST not be called with the Global Lock held
        @param domain_id: Domain ID
        @type domain_id: Integer
        @param vnid: VNID
        @type vnid: Integer
        '''
        #Get policies from DMC
        while True:
            self.lock.acquire()
            try:
                domain_obj = self.Domain_Hash[domain_id]
                vnids = domain_obj.DVG_Hash.keys()
            except Exception:
                self.lock.release()
                break
            self.lock.release()
            #Add the VNID
            self.Dvg_Add(domain_id, vnid)
            #print 'Get unicast policy for Domain %s, <%s:%s>\r'%(domain_id, vnid, vnid)
            dcslib.domain_query_policy_from_controller(domain_id, vnid, vnid, 0)
            #print 'Get multicast policy for Domain %s, <%s:%s>\r'%(domain_id, vnid, vnid)
            dcslib.domain_query_policy_from_controller(domain_id, vnid, vnid, 1)
            for vnid_other in vnids:
                if vnid_other == vnid:
                    continue
                #print 'Get unicast policy for Domain %s, <%s:%s>\r'%(domain_id, vnid, vnid_other)
                dcslib.domain_query_policy_from_controller(domain_id, vnid, vnid_other, 0)
                #print 'Get multicast policy for Domain %s, <%s:%s>\r'%(domain_id, vnid, vnid_other)
                dcslib.domain_query_policy_from_controller(domain_id, vnid, vnid_other, 1)
                #dcslib...
                #print 'Get unicast policy for Domain %s, <%s:%s>\r'%(domain_id, vnid_other, vnid)
                dcslib.domain_query_policy_from_controller(domain_id, vnid_other, vnid, 0)
                #print 'Get multicast for Domain %s, <%s:%s>\r'%(domain_id, vnid_other, vnid)
                dcslib.domain_query_policy_from_controller(domain_id, vnid_other, vnid, 1)
                if not self.started:
                    break
            if not self.started:
                break
            print 'Get subnets for Domain %s, VNID %s\r'%(domain_id, vnid)
            dcslib.vnid_query_subnets_from_controller(vnid)
            #Ask the DMC to request re-registration
            message ='Ask DMC to request re-registration from all DOVE Switches in VNID %s'%vnid
            dcslib.dps_cluster_write_log(DpsLogLevels.ALERT, message)
            ret = dcslib.dps_cluster_reregister_endpoints(domain_id, vnid)
            if ret != 0:
                self.lock.acquire()
                self.VNID_Reregistration[vnid] = (domain_id, vnid)
                self.lock.release()
            break
        return

    def Dvg_Show(self, dvg_id, fdetails):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine shows the DVG details
        @param dvg_id: The DVG ID
        @type dvg_id: Integer
        @param fdetails: Whether to show detailed information
        @type fdetails: Integer (0 = False, 1 = True)
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[dvg_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain_obj = self.Domain_Hash[domain_id]
                dvg_obj = domain_obj.DVG_Hash[dvg_id]
                dvg_obj.show(fdetails)
                ret_val = DOVEStatus.DOVE_STATUS_OK
            except Exception, ex:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
            break
        self.lock.release()
        return ret_val

    def VNID_Recovery_Timeout(self):
        '''
        This routine helps recover all the 
        @attention: This routine MUST not be called with the Global Lock held
        '''
        while True:
            if len(self.VNID_Recovery) == 0:
                break
            self.lock.acquire()
            vnids = self.VNID_Recovery.keys()
            self.lock.release()
            for vnid in vnids:
                try:
                    vnid_tuple = self.VNID_Recovery[vnid]
                    domain_id = vnid_tuple[0]
                except Exception:
                    pass
                self.DVG_Recovery_Routine(domain_id, vnid)
                self.lock.acquire()
                try:
                    del self.VNID_Recovery[vnid]
                except Exception:
                    pass
                self.lock.release()
            #Retry Registration
            self.lock.acquire()
            vnids = self.VNID_Reregistration.keys()
            self.lock.release()
            for vnid in vnids:
                try:
                    vnid_tuple = self.VNID_Reregistration[vnid]
                    domain_id = vnid_tuple[0]
                    vnid = vnid_tuple[1]
                except Exception:
                    continue
                ret = dcslib.dps_cluster_reregister_endpoints(domain_id, vnid)
                if ret == 0:
                    self.lock.acquire()
                    try:
                        del self.VNID_Reregistration[vnid]
                    except Exception:
                        pass
                    self.lock.release()
            break
        return

    def Mass_Transfer_Get_Ready_Timeout(self):
        '''
        This routine removes domains marked GET Ready if the mass transfer and
        the mass transfer hasn't yet completed
        '''
        domain_list = []
        time_current = time.time()
        self.lock.acquire()
        try:
            domain_get_ready = self.MassTransfer_Get_Ready.keys()
            for domain_id in domain_get_ready:
                try:
                    time_start = self.MassTransfer_Get_Ready[domain_id]
                except Exception:
                    continue
                if time_current - time_start > self.MASS_TRANSFER_COMPLETE_MAX:
                    message ='Domain %s: Mass Transfer not completed in %s seconds, de-activating from local node'%(domain_id, self.MASS_TRANSFER_COMPLETE_MAX)
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                    try:
                        del self.MassTransfer_Get_Ready[domain_id]
                    except Exception:
                        pass
                    domain_list.append(domain_id)
        except Exception, ex:
            message = 'Mass_Transfer_Get_Ready_Timeout: Exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        for domain_id in domain_list:
            self.Domain_Deactivate(domain_id)
        return

    def Policy_Add(self, traffic_type, domain_id, policy_type, src_dvg_id, dst_dvg_id, ttl, action):
        '''
        @attention: This routine MUST not be called with the Global Lock held
        This routine adds a dvg to the collection
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param policy_type: The Policy Type
        @type policy_type: Integer
        @param src_dvg_id: The Source DVG ID
        @type src_dvg_id: Integer
        @param dst_dvg_id: The Source DVG ID
        @type dst_dvg_id: Integer
        @param ttl: Time to Live
        @param ttl: Integer
        @param action: Action taken on the policy
        @param action: Integer or ByteArray
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                src_dvg_obj = domain_obj.DVG_Hash[src_dvg_id]
            except Exception:
                #Create DVG implicitly
                self.lock.release()
                ret_val = self.Dvg_Add(domain_id, src_dvg_id)
                self.lock.acquire()
                if ret_val != DOVEStatus.DOVE_STATUS_OK:
                    break
            try:
                dst_dvg_obj = domain_obj.DVG_Hash[dst_dvg_id]
            except Exception:
                #Create DVG implicitly
                self.lock.release()
                ret_val = self.Dvg_Add(domain_id, dst_dvg_id)
                self.lock.acquire()
                if ret_val != DOVEStatus.DOVE_STATUS_OK:
                    break
            if policy_type != Policy.type_connectivity:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
                break
            try:
                policy_key = Policy.key_fmt%(src_dvg_id, dst_dvg_id)
                policy_obj = domain_obj.Policy_Hash_DVG[traffic_type][policy_key]
                policy_obj.update(ttl, action)
            except Exception:
                try:
                    policy_obj = Policy(domain_obj, traffic_type, policy_type, 
                                        src_dvg_obj, dst_dvg_obj, ttl, action)
                except (Exception, MemoryError):
                    ret_val = DOVEStatus.DOVE_STATUS_NO_RESOURCES
                    break
            if domain_obj.mass_transfer is not None:
                ret_val = domain_obj.mass_transfer.register_policy(policy_obj, True)
            else:
                ret_val = DOVEStatus.DOVE_STATUS_OK
            break
        self.lock.release()
        return ret_val

    def Policy_Delete(self, traffic_type, domain_id, src_dvg_id, dst_dvg_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine deletes a Policy from the Collection
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param src_dvg_id: The Source DVG ID
        @type src_dvg_id: Integer
        @param dst_dvg_id: The Source DVG ID
        @type dst_dvg_id: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        status = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception:
                break
            try:
                policy_key = Policy.key_fmt%(src_dvg_id, dst_dvg_id)
                policy_obj = domain_obj.Policy_Hash_DVG[traffic_type][policy_key]
                if domain_obj.mass_transfer is not None:
                    status = domain_obj.mass_transfer.register_policy(policy_obj, False)
                policy_obj.delete()
            except Exception:
                pass
            break
        self.lock.release()
        return status

    def Controller_Location_Update(self, IP_type, IP_packed, Port):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine adds the Controller Location to the collection
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @param ip_packed: Packed IP Address
        @type ip_packed: ByteArray
        @param Port: The Port of the Controller
        @type Port: Integer (Host Byte Order)
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                IP_val = self.ip_get_val_from_packed[IP_type](IP_packed)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_BAD_ADDRESS
                break
            try:
                location = IPAddressLocation(IP_type, IP_val, Port)
                DpsCollection.Controller_Location = location
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_NO_RESOURCES
                break
            break
        self.lock.release()
        return ret_val

    def Controller_Location_Get(self):
        '''
        This routine gets the Controller Location
        '''
        controller_location = DpsCollection.Controller_Location
        if controller_location is None:
            return (DOVEStatus.DOVE_STATUS_EMPTY, struct.pack('I', 0), 0)
        else:
            return (DOVEStatus.DOVE_STATUS_OK, 
                    controller_location.ip_value_packed,
                    controller_location.port)

    def External_Gateway_Add(self, vnid, IP_type, IP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine adds an external gateway to the Domain.
        @param vnid: The VNID
        @type vnid: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @param IP_val: The IP address of the Gateway
        @type IP_val: Integer or String
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                if IP_type == socket.AF_INET:
                    IPList = dvg.ExternalGatewayIPListv4
                else:
                    IPList = dvg.ExternalGatewayIPListv6
                IPList.add(IP_type, IP_val)
                if domain.active:
                    DpsCollection.gateway_update_queue.put((dvg, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
            break
        self.lock.release()
        return ret_val

    def External_Gateway_Delete(self, vnid, IP_type, IP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine deletes an external gateway from the Domain.
        @param vnid: The VNID
        @type vnid: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @param IP_val: The IP address of the Gateway
        @type IP_val: Integer or String
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                if IP_type == socket.AF_INET:
                    IPList = dvg.ExternalGatewayIPListv4
                else:
                    IPList = dvg.ExternalGatewayIPListv6
                IPList.remove(IP_type, IP_val)
                if domain.active:
                    DpsCollection.gateway_update_queue.put((dvg, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
            break
        self.lock.release()
        return ret_val

    def External_Gateway_Clear(self, vnid):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine clears external gateways from the Domain.
        @param vnid: The VNID
        @type vnid: Integer
        '''
        self.lock.acquire()
        ret_val = DOVEStatus.DOVE_STATUS_OK
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                IPList = dvg.ExternalGatewayIPListv4
                IPList.clear()
                IPList = dvg.ExternalGatewayIPListv6
                IPList.clear()
                if domain.active:
                    DpsCollection.gateway_update_queue.put((dvg, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL))
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
            break
        self.lock.release()
        return ret_val

    def External_Gateway_Validate(self, vnid, IP_type, IP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine validates the Gateway ID in specified domain
        @param vnid: The VNID
        @type vnid: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer        
        @param IP_val: The Gateway IP
        @type IP_val: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            try:
                if IP_type == socket.AF_INET:
                    IPList = dvg.ExternalGatewayIPListv4
                else:
                    IPList = dvg.ExternalGatewayIPListv6
                if IPList.search(IP_val):
                    ret_val = DOVEStatus.DOVE_STATUS_OK;
                else :
                    ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER;
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_PARAMETER
            break;
        self.lock.release()
        return ret_val

    def External_Gateway_GetAllIds(self, vnid, IP_type):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine return all External Gateway IDs of the domain specified by domain_id
        @param vnid: The VNID
        @type vnid: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @return: The string of External Gateway IPs separate by ','
        @rtype: String
        '''
        IPList=''
        self.lock.acquire()
        while True:
            try:
                domain_id = self.VNID_Hash[vnid]
            except Exception:
                break
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                break
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                break
            if IP_type == socket.AF_INET:
                IPList += dvg.ExternalGatewayIPListv4.toString();
            else:
                IPList += dvg.ExternalGatewayIPListv6.toString();
            break
        self.lock.release()
        return IPList


    def Policy_Get(self, traffic_type, domain_id, src_dvg_id, dst_dvg_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine Get a policy content
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param src_dvg_id: The Source DVG ID
        @type src_dvg_id: Integer
        @param dst_dvg_id: The Source DVG ID
        @type dst_dvg_id: Integer
        @return: status, policy_type, policy_ttl, policy_action, policy_version
        @rtype: Integer, Integer, Integer, Integer, ByteArray, Integer
        '''
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
            except Exception:
                break
            try:
                policy_key = Policy.key_fmt%(src_dvg_id, dst_dvg_id)
                policy_obj = domain_obj.Policy_Hash_DVG[traffic_type][policy_key]
            except Exception:
                break
            self.lock.release()
            return (DOVEStatus.DOVE_STATUS_OK, policy_obj.type,
                    policy_obj.ttl, policy_obj.action, policy_obj.version)
        self.lock.release()
        return (DOVEStatus.DOVE_STATUS_INVALID_POLICY, 0, 0, '', 0)

    def Policy_GetAllIds(self, traffic_type, domain_id):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine return all Policy IDs of specified domain
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: The string of domain IDs separate by ','
        @rtype: String
        '''
        policy_list_str = ''
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                policy_list = domain_obj.Policy_Hash_DVG[traffic_type].keys()
                policy_list.sort()
                policy_list_str = ','.join(str(i) for i in policy_list)
            except Exception:
                break
            break
        self.lock.release()
        return policy_list_str

    def IP_Subnet_Add(self, associated_type, associated_id, IP_type, IP_value, mask_value, mode, gateway):
        '''
        This routine adds a IP subnet to list of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @param IP_value: IPv6 or IPv4 Address of Subnet
        @type IP_value: String or Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @param mode: Mode of Subnet
        @type mode: Integer (0 = Dedicated, 1 = Shared)
        @param gateway: Gateway of Subnet
        @type gateway: String or Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
                try:
                    associated_obj = self.Domain_Hash[associated_id]
                    domain_obj = associated_obj
                except Exception:
                    ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            elif associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_VNID:
                try:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    associated_obj = domain_obj.DVG_Hash[associated_id]
                except Exception:
                    ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                    break
            else:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
                break
            if IP_type == socket.AF_INET:
                ret_val = associated_obj.ip_subnet_add(IP_value, mask_value, gateway, mode)
                if ret_val == DOVEStatus.DOVE_STATUS_OK and domain_obj.mass_transfer is not None:
                    ret_val = domain_obj.mass_transfer.register_subnet(True, associated_type, IP_value, mask_value, gateway, mode)
            else:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
            break
        self.lock.release()
        return ret_val

    def IP_Subnet_Delete(self, associated_type, associated_id, IP_type, IP_value, mask_value):
        '''
        This routine deletes a IP subnet from list of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer
        @param IP_value: IPv6 or IPv4 Address of Subnet
        @type IP_value: String or Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        status = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
                    associated_obj = self.Domain_Hash[associated_id]
                    domain_obj = associated_obj
            except Exception:
                break
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_VNID:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    associated_obj = domain_obj.DVG_Hash[associated_id]
            except Exception:
                break
            if IP_type == socket.AF_INET:
                associated_obj.ip_subnet_delete(IP_value, mask_value)
                if domain_obj.mass_transfer is not None:
                    status = domain_obj.mass_transfer.register_subnet(False, associated_type, IP_value, mask_value, 0, 0)
            break
        self.lock.release()
        return status

    def IP_Subnet_Get(self, associated_type, associated_id, IP_type, IP_value, mask_value):
        '''
        This routine gets a IP subnet from list of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer        
        @param IP_value: IPv6 or IPv4 Address of Subnet
        @type IP_value: String or Integer
        @param mask_value: Mask of Subnet
        @type mask_value: Integer                
        @return: status, subnet_ip_packed, subnet_mask, subnet_mode, subnet_gateway_packed
        @rtype: Integer, ByteArray, Integer, Integer, ByteArray
        '''
        self.lock.acquire()
        while True:
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
                    associated_obj = self.Domain_Hash[associated_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_VNID:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    associated_obj = domain_obj.DVG_Hash[associated_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            if IP_type == socket.AF_INET:
                ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = associated_obj.ip_subnet_get(IP_value, mask_value)
                subnet_ip_packed = struct.pack(IPAddressLocation.fmts[IP_type], subnet_ip)
                subnet_gateway_packed = struct.pack(IPAddressLocation.fmts[IP_type], subnet_gateway)
                self.lock.release()
                return (ret_val, subnet_ip_packed, subnet_mask, subnet_mode, subnet_gateway_packed)
            else:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
                self.lock.release()
                return (ret_val, '', 0, 0, '')
            break
        self.lock.release()
        return (ret_val, '', 0, 0, '')

    def IP_Subnet_List(self, associated_type, associated_id, IP_type):
        '''
        This routine lists all IP subnets of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer        
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
                    associated_obj = self.Domain_Hash[associated_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_VNID:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    associated_obj = domain_obj.DVG_Hash[associated_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            if IP_type == socket.AF_INET:
                ret_val = associated_obj.ip_subnet_show()
            else:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
            break
        self.lock.release()
        return ret_val

    def IP_Subnet_Flush(self, associated_type, associated_id, IP_type):
        '''
        This routine flushes all IP subnets of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param associated_type: The associated which a IP subnet belongs to (Domain or VNID)
        @type associated_type: Integer
        @param associated_id: The associated ID (Domain ID or VNID)
        @type associated_id: Integer
        @param IP_type: socket.AF_INET or socket.AF_INET6
        @type IP_type: Integer        
        @return: The status of the operation
        @rtype: dove_status (defined in include/status.h) Integer
        '''
        self.lock.acquire()
        while True:
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_DOMAIN:
                    associated_obj = self.Domain_Hash[associated_id]
                    domain_obj = associated_obj
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            try:
                if associated_type == IPSUBNETAssociatedType.IP_SUBNET_ASSOCIATED_TYPE_VNID:
                    domain_id = self.VNID_Hash[associated_id]
                    domain_obj = self.Domain_Hash[domain_id]
                    associated_obj = domain_obj.DVG_Hash[associated_id]
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DVG
                break
            if IP_type == socket.AF_INET:
                if domain_obj.mass_transfer is not None:
                    PySubnetList = associated_obj.IP_Subnet_List.PyList
                    for subnet in PySubnetList.values():
                        domain_obj.mass_transfer.register_subnet(False, associated_type, 
                                                                 subnet.ip_value, 
                                                                 subnet.mask_value, 
                                                                 subnet.ip_gateway,
                                                                 subnet.mode)
                ret_val = associated_obj.ip_subnet_flush()
            else:
                ret_val = DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
            break
        self.lock.release()
        return ret_val

    def DPSClientsShow(self):
        DPSClientHost.show()

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
            self.cluster_db = ClusterDatabase()
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

