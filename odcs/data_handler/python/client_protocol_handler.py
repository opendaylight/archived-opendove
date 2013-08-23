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

import struct

from dcs_objects.Dvg import DVG
from dcs_objects.Policy import Policy
from dcs_objects.TunnelEndpoint import DPSClient
from dcs_objects.TunnelEndpoint import TunnelEndpoint
from dcs_objects.Endpoint import Endpoint
from dcs_objects.IPAddressLocation import IPAddressLocation
from dcs_objects.Multicast import Multicast
from dcs_objects.DPSClientHost import DPSClientHost
from cluster_database import ClusterDatabase

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
        #Reference to Cluster Database
        self.cluster_db = ClusterDatabase()
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

    def Endpoint_Not_Found(self, domain, dvg, vIP_type, vIP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        This routines handles the case when a Endpoint is not found in a 
        domain and dvg when a lookup is done on it.
        @param domain: The DVG Object
        @type domain: DVG
        @param dvg: The DVG Object
        @type dvg: DVG
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_val: IPv6 or IPv4 Address
        @type vIP_val: String or Integer
        @return: Status, pIPv4 List, pIPv6 List, vMAC, vIP Packed
        @return: Status - dps_error_none, dps_error_no_routine, dps_error_invalid_dst_ip
        @rtype: Integer, [], [], Bytes, Bytes
        '''
        status = self.dps_error_none
        fResolveAddress = False
        vip_packed = struct.pack(IPAddressLocation.fmts[vIP_type],
                                 vIP_val)
        if vIP_type == socket.AF_INET and domain.IP_Subnet_List.count > 0:
            ret_val, subnet_ip, subnet_mask, subnet_mode, subnet_gateway = domain.ip_subnet_lookup(vIP_val)
            if ret_val == DOVEStatus.DOVE_STATUS_OK:
                fResolveAddress = True
                if subnet_mode == IPSUBNETMode.IP_SUBNET_MODE_DEDICATED:
                    #print 'Endpoint_Not_Found: dps_error_invalid_dst_ip\r'
                    return (self.dps_error_invalid_dst_ip, fResolveAddress, 
                            [], [], DpsCollection.IGateway_MAC_Bytes, vip_packed)
        #TODO: Determine if we should do resolution if there is no subnets
        #elif domain.IP_Subnet_List.count <= 0:
        #    fResolveAddress = True
        #Return External Gateway List
        if (dvg.ExternalGatewayIPListv4.count() + dvg.ExternalGatewayIPListv6.count()) <= 0:
            status = self.dps_error_no_route
        ipv4_list = dvg.ExternalGatewayIPListv4.ip_list[:]
        ipv6_list = dvg.ExternalGatewayIPListv6.ip_list[:]
        #print 'Endpoint_Not_Found: status %s\r'%(status)
        return (status, fResolveAddress, ipv4_list, ipv6_list, DpsCollection.EGateway_MAC_Bytes, vip_packed)

    def Endpoint_Find_and_Detect_Conflict(self, domain, dvg, tunnel_endpoint, vMac, 
                                          vIP_type, vIP_val, transaction_type):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
        This routines detects if there is Endpoint Conflict based on the
        Endpoint Update Sent by the DPS Client.
        If no Conflict is detected the corresponding Endpoint Object (if
        it exists) is returned.
        @param domain: The Domain Object
        @type domain: Domain
        @param dvg: The DVG/VNID claiming this endpoint
        @type dvg: DVG
        @param tunnel_endpoint: The Tunnel Endpoint based on the Endpoint Update
        @type tunnel_endpoint: TunnelEndpoint
        @param vMac: The Virtual Mac Address of the Endpoint
        @type vMac: String 6 chars
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_val: The IP address of the Endpoint
        @type vIP_val: Integer or String
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        @return: Existing Endpoint Object if any
        @rtype: None or Endpoint
        '''
        fvIPConflict = False
        #Check if Endpoint exists in domain using MAC
        try:
            endpoint_mac = domain.Endpoint_Hash_MAC[vMac]
        except Exception:
            endpoint_mac = None
        #Check if Endpoint exists in domain using vIP
        try:
            if vIP_type == socket.AF_INET:
                endpoint_ip = domain.Endpoint_Hash_IPv4[vIP_val]
            else:
                endpoint_ip = domain.Endpoint_Hash_IPv6[vIP_val]
        except Exception:
            endpoint_ip = None
        if endpoint_ip is not None and (endpoint_mac != endpoint_ip):
            #CONFLICT DETECTED: Add endpoint_ip to Conflict claim.
            #Note that the endpoint_mac will be registered later in Endpoint Update
            fvIPConflict = True
            #log.warning('Conflict Detected: Invoking Resolutions\r')
            domain.ConflictDetection.endpoint_claim_vIP(endpoint_ip, vIP_type, vIP_val, transaction_type)
        return (endpoint_mac, fvIPConflict)

    def Endpoint_Update(self, domain_id, dvg_id, vnid, client_type, transaction_type,
                        dps_client_IP_type, dps_client_IP_packed, dps_client_port,
                        pIP_type, pIP_packed, 
                        vMac, vIP_type, vIP_packed,
                        operation, version):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        Handles Generic Endpoint Update:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param dvg_id: The DVG
        @type dvg_id: Integer
        @param vnid: The VNID (Maybe different from dvg_id for Domain/DVG 0)
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @param dps_client_port: The DPS Client Port (for UDP communication)
        @type dps_client_port: Integer
        @param pIP_type: socket.AF_INET6 or socket.AF_INET
        @type pIP_type: Integer
        @param pIP_packed: The IP address of the DPS Client - DOVE Switch or
                        Gateway hosting the Endpoint
        @type pIP_packed: Packed Data
        @param vMac: The Virtual Mac Address of the Endpoint
        @type vMac: String 6 chars
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_packed: The IP address of the Endpoint
        @type vIP_packed: Packed Data
        @param operation: The type of endpoint update operation
        @type operation: Integer
        @param version: The Object Version Number
        @type version: Integer
        @return: dps_resp_status_t
        @rtype: Integer
        '''
        #print'Endpoint_Update: Enter domain_id %s, dvg %s, vMac %s\r'%(domain_id, dvg_id, mac_show(vMac))
        ret_val = self.dps_error_none
        fnew_endpoint = False
        fnew_dps_client = False
        fnew_tunnel = False
        vIP_Addresses = []
        #IP Mode Integer (0 = Dedicated, 1 = Shared)
        ip_mode = IPSUBNETMode.IP_SUBNET_MODE_DEDICATED
        self.lock.acquire()
        try:
            while True:
                #Make sure vMAC is not a gateway MAC
                if vMac == DpsCollection.EGateway_MAC_Bytes or vMac == DpsCollection.IGateway_MAC_Bytes:
                    ret_val = self.dps_error_invalid_euid
                    break
                #Get the IP address as values
                try:
                    dps_client_IP_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_IP_packed)
                except Exception:
                    ret_val = self.dps_error_invalid_src_ip
                    break
                try:
                    DPSClientHost.Host_Touch(dps_client_IP_val, dps_client_port)
                except Exception:
                    pass
                try:
                    pIP_val = self.ip_get_val_from_packed[pIP_type](pIP_packed)
                except Exception:
                    ret_val = self.dps_error_invalid_src_ip
                    break
                try:
                    vIP_val = self.ip_get_val_from_packed[vIP_type](vIP_packed)
                except Exception:
                    vIP_val = 0
                #log.info('Endpoint_Update: dps_client_IP_val %s, pIP_val %s, vIP_val %s',
                #         dps_client_IP_val, pIP_val, vIP_val)
                #Check if vIP is valid
                if vIP_val == 0 or vIP_val == '':
                    vIP_valid = False
                else:
                    vIP_valid = True
                vIP_register = vIP_valid
                #Get Domain Object
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                if not domain_obj.valid:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                #Update Endpoint Update Count
                domain_obj.Endpoint_Update_Count += 1
                #log.info('Endpoint_Update: Got Domain %s', domain_obj.unique_id)
                #Get DVG Object
                try:
                    dvg_obj = domain_obj.DVG_Hash[dvg_id]
                except Exception:
                    ret_val = self.dps_error_invalid_src_dvg
                    break
                if not dvg_obj.valid:
                    ret_val = self.dps_error_invalid_src_dvg
                    break
                #vIP MUST not be a gateway IP
                if dvg_obj.ImplicitGatewayIPListv4.search(vIP_val):
                    ret_val = self.dps_error_invalid_src_ip
                    break
                #vIP MUST fall in one of SUBNET ranges for the DVG
                if vIP_valid and vIP_type == socket.AF_INET and dvg_obj.IP_Subnet_List.count > 0:
                    if operation == Endpoint.op_update_add or operation == Endpoint.op_update_vip_add:
                        try:
                            ip_mode = dvg_obj.IP_Subnet_List.valid_ip(vIP_val)
                        except Exception:
                            ret_val = self.dps_error_invalid_src_ip
                            break
                #TODO Handle IPv6 SUBNET ranges
                #log.info('Endpoint_Update: Got DVG %s', dvg_obj.unique_id)
                #Get DPS Client i.e. (UDP Address and Port)
                try:
                    if dps_client_IP_type == socket.AF_INET:
                        dps_client_obj = domain_obj.DPSClients_Hash_IPv4[dps_client_IP_val]
                    else:
                        dps_client_obj = domain_obj.DPSClients_Hash_IPv6[dps_client_IP_val]
                    #Update port
                    dps_client_obj.location.port = dps_client_port
                except Exception:
                    if domain_id == DpsCollection.Shared_DomainID:
                        #For Shared Domain, add DPS Client Implicitly
                        try:
                            dps_client_obj = DPSClient(domain_obj, dps_client_IP_type, 
                                                       dps_client_IP_val, dps_client_port)
                            fnew_dps_client = True
                        except Exception:
                            ret_val = self.dps_error_no_memory
                            break
                    else:
                        message = 'Endpoint_Update, Cannot find DCS Client'
                        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                        ret_val = self.dps_error_invalid_tunnel
                        break
                #log.info('Endpoint_Update: Got dps_client %s', dps_client_obj.location.show())
                #Get Tunnel Endpoint Object i.e. DOVE Switch or VLAN Gateway
                try:
                    if pIP_type == socket.AF_INET:
                        tunnel_obj = dvg_obj.Tunnel_Endpoints_Hash_IPv4[client_type][pIP_val]
                    else:
                        tunnel_obj = dvg_obj.Tunnel_Endpoints_Hash_IPv6[client_type][pIP_val]
                except Exception:
                    if domain_id == DpsCollection.Shared_DomainID:
                        #For Domain 0, add Tunnel Implicitly
                        try:
                            #log.info('Creating TunnelEndpoint')
                            TunnelEndpoint.register(domain_obj, dvg_obj, client_type, transaction_type, dps_client_obj, [(pIP_type, pIP_val)])
                            if pIP_type == socket.AF_INET:
                                tunnel_obj = dvg_obj.Tunnel_Endpoints_Hash_IPv4[client_type][pIP_val]
                            else:
                                tunnel_obj = dvg_obj.Tunnel_Endpoints_Hash_IPv6[client_type][pIP_val]
                            #log.info('Created TunnelEndpoint')
                            fnew_tunnel = True
                        except Exception:
                            #log.warning('Cannot create TunnelEndpoint\r')
                            ret_val = self.dps_error_no_memory
                            break
                    else:
                        message = 'Endpoint_Update, Cannot find Tunnel Endpoint'
                        dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                        ret_val = self.dps_error_invalid_tunnel
                        break
                #Check if tunnel has been registered with the right dps client
                if tunnel_obj.dps_client != dps_client_obj:
                    message = 'Endpoint_Update, Existing Tunnel DCS Client Mismatch'
                    dcslib.dps_cluster_write_log(DpsLogLevels.INFO, message)
                    ret_val = self.dps_error_invalid_tunnel
                    break
                #log.info('Endpoint_Update: Got Tunnel Endpoint %s', tunnel_obj.primary_ip().show())
                ###################################################################
                #Find Existing Endpoint and check for conflicts
                ###################################################################
                endpoint_obj, fvIPConflict = self.Endpoint_Find_and_Detect_Conflict(domain_obj, dvg_obj, 
                                                                                    tunnel_obj,
                                                                                    vMac, vIP_type, vIP_val,
                                                                                    transaction_type)
                if fvIPConflict:
                    #Don't register the vIP at this point
                    vIP_register = False
                #log.info('Endpoint_Update: No conflicts endpoint %s', endpoint_obj)
                ###################################################################
                #Check if Endpoint has Migrated without DPS knowledge
                ###################################################################
                if endpoint_obj is not None and (endpoint_obj.tunnel_endpoint != tunnel_obj):
                    #Migrate Endpoint Object to New Values
                    if ((operation == Endpoint.op_update_delete) or 
                        (operation == Endpoint.op_update_vip_del) or 
                        (operation == Endpoint.op_update_migrate_out)):
                        #Ignore (delete) updates from Tunnel that doesn't have the endpoint
                        ret_val = self.dps_error_none
                        break
                    try:
                        message ='Endpoint_Update: Endpoint %s, Migrated from %s to %s without informing DPS\r'%(mac_show(endpoint_obj.vMac), 
                                                                                                                 endpoint_obj.tunnel_endpoint.primary_ip().show(),
                                                                                                                 tunnel_obj.primary_ip().show())
                        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                        endpoint_obj.vmotion(dvg_obj, vnid, client_type, tunnel_obj, transaction_type)
                    except Exception:
                        pass
                if ((endpoint_obj is None) and 
                    (operation == Endpoint.op_update_delete) and 
                    (operation == Endpoint.op_update_migrate_out)
                    (transaction_type != DpsTransactionType.mass_transfer)):
                    #Non existent Endpoint being deleted?
                    ret_val = self.dps_bad_operation
                    break
                #########################################################
                #Create vIP Object
                #########################################################
                if vIP_register:
                    try:
                        vIP_obj = IPAddressLocation(vIP_type, vIP_val, 0)
                        vIP_obj.mode = ip_mode
                    except Exception:
                        ret_val = self.dps_error_no_memory
                        break
                else:
                    vIP_obj = None
                if endpoint_obj is None:
                    if ((operation == Endpoint.op_update_add) or 
                        (operation == Endpoint.op_update_vip_add) or
                        (operation == Endpoint.op_update_migrate_in) or
                        (transaction_type == DpsTransactionType.mass_transfer)):
                        ###############################################################
                        #Brand new Endpoint reported
                        ###############################################################
                        try:
                            endpoint_obj = Endpoint(domain_obj, dvg_obj, vnid, 
                                                    client_type, transaction_type,
                                                    tunnel_obj, vMac, vIP_obj, version)
                            fnew_endpoint = True
                        except Exception:
                            ret_val = self.dps_error_no_memory
                            break
                    else:
                        #Ask client to retry, probably out of order request
                        ret_val = self.dps_error_retry
                else:
                    if operation == Endpoint.op_update_delete or operation == Endpoint.op_update_migrate_out:
                        ip_mode = endpoint_obj.vIP_shared_mode()
                    #Existing Endpoint: Update the object
                    try:
                        endpoint_obj.update(dvg_obj, vnid, client_type, transaction_type, version,
                                            operation, vIP_obj)
                    except Exception, ex:
                        #Ask client to retry, probably out of order request
                        ret_val = self.dps_error_retry
                        break
                version = endpoint_obj.version
                if operation == Endpoint.op_update_vip_add or operation == Endpoint.op_update_vip_del:
                    vIP_Addresses = [vIP_val]
                else:
                    vIP_Addresses = endpoint_obj.vIP_set_show.keys()
                self.Address_Resolution_Resolve(vIP_packed)
                if ((vIP_valid) and 
                    ((operation == Endpoint.op_update_add) or 
                     (operation == Endpoint.op_update_vip_add))):
                    domain_obj.AddressResolution.process_endpoint_update(endpoint_obj, vIP_val)
                    domain_obj.ConflictDetection.endpoint_register_vIP(endpoint_obj, vIP_type, vIP_val)
                if operation == Endpoint.op_update_migrate_in:
                    DpsCollection.vm_migration_update_queue.put((endpoint_obj,domain_obj,vnid))
                #Mass Transfer Register Delta
                if domain_obj.mass_transfer is not None:
                    mass_transfer_ret_val = domain_obj.mass_transfer.register_endpoint(dvg_id,
                                                                                       endpoint_obj,
                                                                                       operation,
                                                                                       vIP_type,
                                                                                       vIP_val
                                                                                       )
                    if mass_transfer_ret_val != DOVEStatus.DOVE_STATUS_OK:
                        ret_val = self.dps_error_retry
                break
            if ret_val != self.dps_error_none and ret_val != self.dps_error_retry:
                if fnew_endpoint:
                    endpoint_obj.delete()
                if fnew_tunnel:
                    tunnel_obj.delete()
                if fnew_dps_client:
                    dps_client_obj.delete()
        except Exception, ex:
            message = 'Endpoint_Update, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (ret_val, ip_mode, version, vIP_Addresses)

    def Endpoint_Location_vIP(self, domain_id, client_type, dps_client_IP_type, dps_client_IP_packed,
                              src_dvg_id, vIP_type, vIP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine Handles the Endpoint Location Request based on Virtual
        IP Address
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param client_type: The Type of Client: should be in DpsClientType
        @type client_type: Integer 
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @param src_dvg_id: The Source DVG
        @type src_dvg_id: Integer
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_val: IPv6 or IPv4 Address
        @type vIP_val: String or Integer
        @return: status, dvg_id, version, pIP_packed, vMac, vIP_packed
        @rtype: Integer, Integer, Integer, ByteArray, 6 char string, ByteArray
        '''
        #log.info('Endpoint_Location_vIP: Enter domain %s, vIP_type %s, vIP_val %s',
        #         domain_id, vIP_type, vIP_val)
        status = self.dps_error_none
        fGateway = 0
        self.lock.acquire()
        try:
            while True:
                try:
                    dps_client_IP_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_IP_packed)
                    DPSClientHost.Host_Touch(dps_client_IP_val, 0)
                except Exception:
                    status = self.dps_error_invalid_tunnel
                    break
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                if not domain.active:
                    status = self.dps_error_retry
                    break
                #Update Endpoint Lookup Count
                domain.Endpoint_Lookup_Count = domain.Endpoint_Lookup_Count + 1
                vip_packed = struct.pack(IPAddressLocation.fmts[vIP_type],vIP_val)
                try:
                    if vIP_type == socket.AF_INET:
                        endpoint = domain.Endpoint_Hash_IPv4[vIP_val]
                    else:
                        endpoint = domain.Endpoint_Hash_IPv6[vIP_val]
                    #log.info('Endpoint_Location_vIP: Found Endpoint %s', endpoint)
                    pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
                    pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
                    self.lock.release()
                    return (status, endpoint.dvg.unique_id, endpoint.version,
                            pipv4_list, pipv6_list, endpoint.vMac, vip_packed, fGateway)
                except Exception:
                    pass
                #Check in source DVG's Implicit Gateway List
                try:
                    src_dvg = domain.DVG_Hash[src_dvg_id]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                if vIP_type == socket.AF_INET:
                    gateway_list = src_dvg.ImplicitGatewayIPListv4
                else:
                    gateway_list = src_dvg.ImplicitGatewayIPListv6
                if gateway_list.search(vIP_val) == True:
                    self.lock.release()
                    fGateway = 1
                    return (status, src_dvg_id, 1, 
                            [0], [],
                            DpsCollection.IGateway_MAC_Bytes,
                            vip_packed, fGateway)
                #Endpoint Not Found
                status, fResolveAddress, pipv4_list, pipv6_list, vmac, vip_packed_return = self.Endpoint_Not_Found(domain, src_dvg, vIP_type, vIP_val)
                if fResolveAddress:
                    #Perform Address Resolution in background
                    #Get the IP address as values
                    try:
                        domain.process_lookup_not_found(dps_client_IP_type, dps_client_IP_val, src_dvg, 
                                                        vIP_type, vIP_val, vip_packed)
                    except Exception:
                        pass
                self.lock.release()
                return (status, src_dvg_id, 1, pipv4_list, pipv6_list, vmac, vip_packed_return, fGateway)
            #Failure
        except Exception, ex:
            message = 'Endpoint_Location_vIP, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, 0, 0, [], [], '', '', fGateway)

    def Endpoint_Location_vMac(self, domain_id, src_dvg_id, vMac):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine Handles the Endpoint Location Request based on vMac
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param src_dvg_id: The Source DVG
        @type src_dvg_id: Integer
        @param vMac: The Virtual MAC address
        @type vMac: String/Bytearray
        @return: status, dvg_id, version, pIP_packed, vMac, vIP_packed
        @rtype: Integer, Integer, Integer, ByteArray, ByteArray
        '''
        status = self.dps_error_none
        fGateway = 0
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                if not domain.active:
                    status = self.dps_error_retry
                    break
                #Update Endpoint Lookup Count
                domain.Endpoint_Lookup_Count = domain.Endpoint_Lookup_Count + 1
                if vMac == DpsCollection.IGateway_MAC_Bytes:
                    self.lock.release()
                    fGateway = 1
                    return (status, src_dvg_id, 1, 
                            [0], [],
                            DpsCollection.IGateway_MAC_Bytes,
                            DpsCollection.Invalid_IP_Packed, fGateway)        
                try:
                    endpoint = domain.Endpoint_Hash_MAC[vMac]
                except Exception:
                    status = self.dps_error_invalid_euid
                    break
                pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
                pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
                vip_addresses = endpoint.vIP_set.values()
                if len(vip_addresses) > 0:
                    vip_packed = vip_addresses[0].ip_value_packed
                else:
                    vip_packed = DpsCollection.Invalid_IP_Packed
                self.lock.release()
                return (status, endpoint.dvg.unique_id, endpoint.version, 
                        pipv4_list, pipv6_list, endpoint.vMac, vip_packed, fGateway)
            #Failure
        except Exception, ex:
            message = 'Endpoint_Location_vMac, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, 0, 0, [], [], '', '', fGateway)

    def Policy_Resolution_vMac(self, domain_id, src_dvg_id, vMac):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine Handles the Policy Resolution Request based on vMac
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param src_dvg_id: The Source DVG
        @type src_dvg_id: Integer
        @param vMac: The Virtual MAC address
        @type vMac: String/Bytearray
        @return: status, endpoint_dvg_id, endpoint_version, 
                 endpoint_pIP_packed, endpoint_vMac, endpoint_vIP_packed,
                 policy_version, policy_ttl, policy_type, policy_action
        @rtype: Integer, Integer, Integer, ByteArray, ByteArray, ByteArray
                Integer, Integer, Integer, Integer, ByteArray
        '''
        status = self.dps_error_none
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                if not domain.active:
                    status = self.dps_error_retry
                    break
                #Update Policy Lookup Count
                domain.Policy_Lookup_Count += 1
                try:
                    endpoint = domain.Endpoint_Hash_MAC[vMac]
                except Exception:
                    status = self.dps_error_invalid_euid
                    break
                try:
                    src_dvg = domain.DVG_Hash[src_dvg_id]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                dst_dvg_id = endpoint.dvg.unique_id
                pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
                pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
                #Get primary VIP
                vip_addresses = endpoint.vIP_set.values()
                if len(vip_addresses) > 0:
                    vip_packed = vip_addresses[0].ip_value_packed
                else:
                    vip_packed = DpsCollection.Invalid_IP_Packed
                #Get Policy Object
                try:
                    key = Policy.key_fmt%(src_dvg_id, dst_dvg_id)
                    policy = domain.Policy_Hash_DVG[0][key]
                except Exception:
                    ###policy = domain_obj.Default_Policy?
                    #Return DENY Policy
                    self.lock.release()
                    return (self.dps_error_none,
                            endpoint.dvg.unique_id, endpoint.version,
                            pipv4_list,
                            pipv6_list,
                            endpoint.vMac, vip_packed, 
                            0, 0,
                            Policy.type_connectivity, Policy.action_drop_packed)
                #Return the Policy
                self.lock.release()
                return (status, endpoint.dvg.unique_id, endpoint.version, 
                        pipv4_list,
                        pipv6_list,
                        endpoint.vMac, vip_packed, 
                        policy.version, policy.ttl,
                        policy.type, policy.action)
            #Failure
        except Exception, ex:
            message = 'Policy_Resolution_vMac, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, 0, 0, [], [], '', '', 0, 0, 0, 0, '')

    def Policy_Resolution_vIP(self, domain_id, client_type, dps_client_IP_type, dps_client_IP_packed,
                              src_dvg_id, vIP_type, vIP_val):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine Handles the Policy Resolution Request based on vMac
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param client_type: The Type of Client: should be in DpsClientType
        @type client_type: Integer 
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @param src_dvg_id: The Source DVG
        @type src_dvg_id: Integer
        @param vIP_type: socket.AF_INET or socket.AF_INET6
        @type vIP_type: Integer
        @param vIP_val: IPv6 or IPv4 Address
        @type vIP_val: String or Integer
        @return: status, endpoint_dvg_id, endpoint_version, 
                 endpoint_pIP_packed, endpoint_vMac, endpoint_vIP_packed,
                 policy_version, policy_ttl, policy_type, policy_action
        @rtype: Integer, Integer, Integer, ByteArray, ByteArray, ByteArray
                Integer, Integer, Integer, Integer, ByteArray
        '''
        status = self.dps_error_none
        self.lock.acquire()
        try:
            while True:
                try:
                    dps_client_IP_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_IP_packed)
                    DPSClientHost.Host_Touch(dps_client_IP_val, 0)
                except Exception:
                    status = self.dps_error_invalid_tunnel
                    break
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                if not domain.active:
                    status = self.dps_error_retry
                    break
                #Update Policy Lookup Count
                domain.Policy_Lookup_Count += 1
                try:
                    src_dvg = domain.DVG_Hash[src_dvg_id]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                vip_packed = struct.pack(IPAddressLocation.fmts[vIP_type],vIP_val)
                try:
                    if vIP_type == socket.AF_INET:
                        endpoint = domain.Endpoint_Hash_IPv4[vIP_val]
                    else:
                        endpoint = domain.Endpoint_Hash_IPv6[vIP_val]
                    #Endpoint Found
                    dst_dvg_id = endpoint.dvg.unique_id
                    pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
                    pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
                    #Get Policy Object
                    try:
                        key = Policy.key_fmt%(src_dvg_id, dst_dvg_id)
                        policy = domain.Policy_Hash_DVG[0][key]
                    except Exception:
                        #Return DENY Policy
                        self.lock.release()
                        return (self.dps_error_none, 
                                endpoint.dvg.unique_id, endpoint.version, 
                                pipv4_list,
                                pipv6_list,
                                endpoint.vMac, vip_packed, 
                                0, 0,
                                Policy.type_connectivity, Policy.action_drop_packed)
                    #Return the Policy
                    self.lock.release()
                    return (status, endpoint.dvg.unique_id, endpoint.version, 
                            pipv4_list,
                            pipv6_list,
                            endpoint.vMac, vip_packed, 
                            policy.version, policy.ttl,
                            policy.type, policy.action)
                except Exception:
                    pass
                #Endpoint not found in Domain collection
                #Check in source DVG's Implicit Gateway List
                if vIP_type == socket.AF_INET:
                    gateway_list = src_dvg.ImplicitGatewayIPListv4
                else:
                    gateway_list = src_dvg.ImplicitGatewayIPListv6
                if gateway_list.search(vIP_val) == True:
                    self.lock.release()
                    return (status, src_dvg_id, 1, 
                            [0],
                            [],
                            DpsCollection.IGateway_MAC_Bytes, vip_packed,
                            0, 0,
                            Policy.type_connectivity, Policy.action_forward_packed)
                #Endpoint Not Found in Domain Collection or Gateway List
                status, fResolveAddress, pipv4_list, pipv6_list, vmac, vip_packed_return = self.Endpoint_Not_Found(domain, src_dvg, vIP_type, vIP_val)
                if fResolveAddress:
                    #Perform Address Resolution in background
                    #Get the IP address as values
                    try:
                        domain.process_lookup_not_found(dps_client_IP_type, dps_client_IP_val, src_dvg, 
                                                        vIP_type, vIP_val, vip_packed)
                    except Exception:
                        pass
                if status == self.dps_error_none:
                    policy_action = Policy.action_forward_packed
                else:
                    policy_action = Policy.action_drop_packed
                self.lock.release()
                return(status, src_dvg_id, 1, 
                       pipv4_list, pipv6_list, 
                       vmac, vip_packed_return,
                       0, 0, 
                       Policy.type_connectivity, policy_action)
            #Failure
        except Exception, ex:
            message = 'Policy_Resolution_vIP, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, 0, 0, [], [], '', '', 0, 0, 0, '')

    def Implicit_Gateway_List(self, domain_id, vnid):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine returns the List of Implicit Gateways
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID ID
        @type vnid: Integer
        @return 2 Lists: 1st IPv4 Gateways, 2nd IPv6 Gateways
        '''
        status = self.dps_error_none
        ListIPv4 = None
        ListIPv6 = None
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                try:
                    dvg = domain.DVG_Hash[vnid]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                if ((dvg.ImplicitGatewayIPListv4.count() > 0) or 
                    (dvg.ImplicitGatewayIPListv6.count() > 0)):
                    ListIPv4 = dvg.ImplicitGatewayIPListv4.ip_list[:]
                    ListIPv6 = dvg.ImplicitGatewayIPListv6.ip_list[:]
                else:
                    ListIPv4 = domain.ImplicitGatewayIPListv4.ip_list[:]
                    ListIPv6 = domain.ImplicitGatewayIPListv6.ip_list[:]
                break
        except Exception, ex:
            message = 'Implicit_Gateway_List, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, ListIPv4, ListIPv6)

    def Broadcast_List(self, domain_id, vnid):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine returns the List of DOVE Switches in a Domain
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID ID
        @type vnid: Integer
        @return 2 Lists: 1st IPv4 DOVE Switches, 2nd IPv6 DOVE Switches
        '''
        status = self.dps_error_none
        ListIPv4 = []
        ListIPv6 = []
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                try:
                    dvg = domain.DVG_Hash[vnid]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                for client_type in dvg.Tunnel_Endpoints_Hash_IPv4.keys():
                    ListIPv4 += dvg.Tunnel_Endpoints_Hash_IPv4[client_type].keys()
                for client_type in dvg.Tunnel_Endpoints_Hash_IPv6.keys():
                    ListIPv6 += dvg.Tunnel_Endpoints_Hash_IPv6[client_type].keys()
                break
        except Exception, ex:
            message = 'Broadcast_List, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, ListIPv4, ListIPv6)

    def Gateway_List(self, domain_id, vnid, gateway_type):
        '''
        This routine returns the list of gateways for to whom a VNID
        can send traffic to.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID ID
        @type vnid: Integer
        @param gateway: The Type of Gateway
        @type gateway: DOVEGatewayTypes (GATEWAY_TYPE_IMPLICIT not supported)
                       Use Implicit_Gateway_List for GATEWAY_TYPE_IMPLICIT
        @return: (v4 gateway List, v6 Gateway List)
        @rtype: ([],[]) where each element of the list is (vnid, ip_value)
        '''
        status = self.dps_error_none
        ListIPv4 = []
        ListIPv6 = []
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    status = self.dps_error_invalid_domain_id
                    break
                try:
                    dvg = domain.DVG_Hash[vnid]
                except Exception:
                    status = self.dps_error_invalid_src_dvg
                    break
                ListIPv4, ListIPv6 = dvg.communication_gateway_list(True, gateway_type)
                break
        except Exception, ex:
            message = 'Gateway_List, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, ListIPv4, ListIPv6)

    def Is_VNID_Handled_Locally(self, vnid, transaction_type):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine determines if the VNID is handled by the local node
        @param vnid: The VNID ID
        @type vnid: Integer
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        @return: (Status {OK, NOT_FOUND}, fLocalDomain {0 = False, 1 = True}, domain_id)
        @rtype: (Integer, Integer, Integer)
        '''
        ret_val = 0
        domain_id = 0
        status = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        try:
            while True:
                try:
                    domain_id = self.VNID_Hash[vnid]
                except Exception:
                    self.vnid_query_controller[vnid] = vnid
                    status = DOVEStatus.DOVE_STATUS_NOT_FOUND
                    break
                try:
                    domain = self.Domain_Hash[domain_id]
                    if domain.active or transaction_type == DpsTransactionType.mass_transfer:
                        ret_val = 1
                except Exception:
                    break
                break
        except Exception, ex:
            message = 'Is_VNID_Handled_Locally, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (status, ret_val, domain_id)

    def pIP_Get_DPS_Client(self, domain_id, pIP_type, pIP_packed):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine determines the DPS Client (IP/Port) for a DOVE Tunnel
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param pIP_type: socket.AF_INET6 or socket.AF_INET
        @type pIP_type: Integer
        @param pIP_packed: The IP address of the DPS Client - DOVE Switch or
                           Gateway hosting the Endpoint
        @type pIP_packed: Packed Data
        @return: status, dps_client_ip_packed, dps_client_port
        @rtype: Integer, ByteArray, Integer
        '''
        self.lock.acquire()
        try:
            while True:
                try:
                    pIP_val = self.ip_get_val_from_packed[pIP_type](pIP_packed)
                except Exception:
                    ret_val = self.dps_error_invalid_src_ip
                    break
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                try:
                    if pIP_type == socket.AF_INET:
                        tunnel_obj = domain_obj.Tunnel_Endpoints_Hash_IPv4[pIP_val]
                    else:
                        tunnel_obj = domain_obj.Tunnel_Endpoints_Hash_IPv6[pIP_val]
                except Exception:
                    ret_val = self.dps_error_invalid_dst_ip
                    break
                dps_client_obj = tunnel_obj.dps_client
                ip_packed = dps_client_obj.location.ip_value_packed
                port = dps_client_obj.location.port
                self.lock.release()
                return (self.dps_error_none, ip_packed, port)
        except Exception, ex:
            message = 'pIP_Get_DPS_Client, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (ret_val, '', 0)

    def vIP_Get_DVG(self, domain_id, vIP_type, vIP_packed):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        This routine determines the DVG of a VIP
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_packed: The vIP address of the Endpoint
        @type vIP_packed: Packed Data
        @return: status, DVG
        @rtype: Integer, Integer
        '''
        ret_val = self.dps_error_invalid_src_ip
        self.lock.acquire()
        try:
            while True:
                try:
                    vIP_val = self.ip_get_val_from_packed[vIP_type](vIP_packed)
                except Exception:
                    break
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                try:
                    if vIP_type == socket.AF_INET:
                        endpoint = domain_obj.Endpoint_Hash_IPv4[vIP_val]
                    else:
                        endpoint = domain_obj.Endpoint_Hash_IPv6[vIP_val]
                except Exception:
                    break
                self.lock.release()
                return (self.dps_error_none, endpoint.dvg.unique_id)
        except Exception, ex:
            message = 'vIP_Get_DVG, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return (ret_val, 0)

    def Broadcast_Updates_Send(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        This routine checks which domains need to send broadcast
        table updates to their switches and send it to them.
        '''
        #log.warning('VNID_Broadcast_Updates size %s\r', len(self.VNID_Broadcast_Updates))
        try:
            #First try and send items remaining from previous time
            dvg_dps_tuple_list = []
            tuple_keys = self.VNID_Broadcast_Updates_To.keys()
            for tuple_key in tuple_keys:
                try:
                    dvg_dps_tuple_list.append(self.VNID_Broadcast_Updates_To[tuple_key])
                    del self.VNID_Broadcast_Updates_To[tuple_key]
                except Exception:
                    pass
            for dvg_tuple in dvg_dps_tuple_list:
                try:
                    dvg = dvg_tuple[0]
                    dps_client = dvg_tuple[1]
                    #print 'Resending Broadcast Table To VNID %s DPS Client %s\r'%(dvg.unique_id, dps_client.location.show())
                    dvg.send_broadcast_table_to(dps_client)
                except Exception:
                    pass
            #Send new updates
            dvg_list = []
            dvg_keys = self.VNID_Broadcast_Updates.keys()
            for dvg_id in dvg_keys:
                try:
                    dvg_list.append(self.VNID_Broadcast_Updates[dvg_id])
                    del self.VNID_Broadcast_Updates[dvg_id]
                except Exception:
                    pass
            for dvg in dvg_list:
                try:
                    dvg.send_broadcast_table_update()
                except Exception:
                    pass
        except Exception, ex:
            message = 'Broadcast_Updates_Send, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def Policy_Updates_Send(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        This routine checks sends Policy updates to DPS Clients
        '''
        #log.warning('Policy_Updates_To size %s\r', len(self.Policy_Updates_To))
        try:
            dps_client_list = []
            for key in self.Policy_Updates_To.keys():
                try:
                    dps_client_list.append(self.Policy_Updates_To[key])
                    del self.Policy_Updates_To[key]
                except Exception:
                    pass
            for dps_tuple in dps_client_list:
                try:
                    dvg = dps_tuple[0]
                    dps_client = dps_tuple[1]
                    #log.warning('Policy_Updates_To_Send: Dvg %s Enter\r', dvg.unique_id)
                    dvg.send_policies_to(dps_client, 0, 0)
                    #gwy_v4, gwy_v6 = dvg.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL)
                    #dvg.send_gateway_update(DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL, gwy_v4, gwy_v6)
                    #gwy_v4, gwy_v6 = dvg.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_VLAN)
                    #dvg.send_gateway_update(DOVEGatewayTypes.GATEWAY_TYPE_VLAN, gwy_v4, gwy_v6)
                    #log.warning('DPSClient_New: Dvg %s Exit\r', dvg.unique_id)
                except Exception, ex:
                    message = 'Policy_Updates_Send: Exception in sending to DPS Client %s'%ex
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                    pass
        except Exception, ex:
            message = 'Policy_Updates_Send, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def Policy_Updates_Send_To(self, vnid_id, query_id, dps_client_IP_type, dps_client_ip_packed):
        '''
        This routine send Policy Updates to a specific DPS Client
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param vnid_id: The VNID/DVG ID
        @type vnid_id: Integer
        @param query_id: The Query ID sent by the DPS Client
        @type query_id: Integer
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @return None
        '''
        self.lock.acquire()
        try:
            while True:
                try:
                    domain_id = self.VNID_Hash[vnid_id]
                except Exception:
                    break
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    break
                try:
                    dvg_obj = domain_obj.DVG_Hash[vnid_id]
                except Exception:
                    break
                try:
                    ip_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_ip_packed)
                except Exception:
                    break
                if dps_client_IP_type == socket.AF_INET:
                    ip_hash = domain_obj.DPSClients_Hash_IPv4
                else:
                    ip_hash = domain_obj.DPSClients_Hash_IPv6
                try:
                    dps_client = ip_hash[ip_val]
                except Exception:
                    break
                dps_client.failure_count = DPSClient.failure_count_max
                dvg_obj.send_policies_to(dps_client, 1, query_id)
                break
        except Exception, ex:
            message = 'Policy_Updates_Send_To, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return

    def Gateway_Updates_Send(self):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE
        This routine checks sends Gateway updates to DPS Clients
        '''
        #log.warning('Gateway_Updates_Send size %s\r', len(self.Gateway_Updates_To))
        try:
            dps_client_list = []
            for key in self.Gateway_Updates_To.keys():
                try:
                    dps_client_list.append(self.Policy_Updates_To[key])
                    del self.Gateway_Updates_To[key]
                except Exception:
                    pass
            for dps_tuple in dps_client_list:
                try:
                    dvg = dps_tuple[0]
                    dps_client = dps_tuple[1]
                    gwy_type = dps_tuple[2]
                    gwy_v4, gwy_v6 = dvg.communication_gateway_list(True, gwy_type)
                    dvg.send_gateways_to(dps_client, gwy_type, gwy_v4, gwy_v6)
                except Exception, ex:
                    message = 'Gateway_Updates_Send, Exception in sending to DPS Client %s'%ex
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                    pass
        except Exception, ex:
            message = 'Gateway_Updates_Send, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def Handle_Unsolicited_Message_Reply(self, message_type, 
                                         response_error, vnid_id,
                                         dps_client_IP_type,
                                         dps_client_ip_packed):
        '''
        This routine handles the general ACKs from a DPS Client. The following
        original message types are handled:
            DPS_UNSOLICITED_VNID_POLICY_LIST
            DPS_UNSOLICITED_BCAST_LIST_REPLY
            DPS_UNSOLICITED_EXTERNAL_GW_LIST
            DPS_UNSOLICITED_VLAN_GW_LIST
            DPS_CTRL_PLANE_HB
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param message_type: The original message type
        @type message_type: Integer
        @param response_error: The Response Error Code sent by the DPS Client
        @type response_error: Integer
        @param vnid_id: The VNID/DVG ID
        @type vnid_id: Integer
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @return None
        '''
        self.lock.acquire()
        try:
            while True:
                try:
                    domain_id = self.VNID_Hash[vnid_id]
                except Exception:
                    break
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    break
                try:
                    dvg_obj = domain_obj.DVG_Hash[vnid_id]
                except Exception:
                    break
                try:
                    ip_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_ip_packed)
                except Exception:
                    break
                if dps_client_IP_type == socket.AF_INET:
                    ip_hash = domain_obj.DPSClients_Hash_IPv4
                else:
                    ip_hash = domain_obj.DPSClients_Hash_IPv6
                try:
                    dps_client = ip_hash[ip_val]
                except Exception:
                    break
                #If error is not RETRY it means the DPS Client is up and and running
                if response_error != self.dps_no_response:
                    dps_client.failure_count = DPSClient.failure_count_max
                    DPSClientHost.Host_Touch(ip_val, 0)
                    break
                #Check if MAX Failure have been reached for non local DPS Client
                if dps_client.failure_count <= 0:
                    if dps_client.location.ip_value != self.cluster_db.Node_Local_Get_IPAddress():
                        message = 'ALERT!!! dps_client %s unreachable'%dps_client.location.show_ip()
                        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                        #dps_client.delete()
                    break
                #Decrement the failure count
                dps_client.failure_count -= 1
                if message_type == self.DPS_UNSOLICITED_VNID_POLICY_LIST:
                    dvg_obj.send_policies_to(dps_client, 0, 0)
                elif message_type == self.DPS_UNSOLICITED_BCAST_LIST_REPLY:
                    dvg_obj.send_broadcast_table_to(dps_client)
                elif message_type == self.DPS_UNSOLICITED_EXTERNAL_GW_LIST:
                    gwy_v4, gwy_v6 = dvg_obj.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL)
                    dvg_obj.send_gateway_update(DOVEGatewayTypes.GATEWAY_TYPE_EXTERNAL, gwy_v4, gwy_v6)
                elif message_type == self.DPS_UNSOLICITED_VLAN_GW_LIST:
                    gwy_v4, gwy_v6 = dvg_obj.communication_gateway_list(True, DOVEGatewayTypes.GATEWAY_TYPE_VLAN)
                    dvg_obj.send_gateway_update(DOVEGatewayTypes.GATEWAY_TYPE_VLAN, gwy_v4, gwy_v6)
                elif message_type == self.DPS_UNSOLICITED_VNID_DEL_REQ:
                    dvg_obj.send_vnid_deletion_to(dps_client)
                break
        except Exception, ex:
            message = 'Handle_Unsolicited_Message_Reply, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return

    def Handle_Resolution_Reply(self, message_type, 
                                response_error, vnid_id,
                                dps_client_IP_type,
                                dps_client_ip_packed,
                                vIP_type,
                                vIP_packed):
        '''
        This routine handles the general ACKs from a DPS Client. The following
        original message types are handled:
            DPS_ADDR_RESOLVE
            DPS_ENDPOINT_LOC_REPLY_UNSOLICITED
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param message_type: The original message type
        @type message_type: Integer
        @param response_error: The Response Error Code sent by the DPS Client
        @type response_error: Integer
        @param vnid_id: The VNID/DVG ID
        @type vnid_id: Integer
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @param vIP_type: socket.AF_INET6 or socket.AF_INET
        @type vIP_type: Integer
        @param vIP_packed: The Virtual IP as packed data
        @type vIP_packed: Packed Data
        @return None
        '''
        self.lock.acquire()
        try:
            while True:
                try:
                    domain_id = self.VNID_Hash[vnid_id]
                except Exception:
                    break
                try:
                    domain_obj = self.Domain_Hash[domain_id]
                except Exception:
                    break
                try:
                    dvg_obj = domain_obj.DVG_Hash[vnid_id]
                except Exception:
                    break
                try:
                    ip_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_ip_packed)
                except Exception:
                    break
                if dps_client_IP_type == socket.AF_INET:
                    ip_hash = domain_obj.DPSClients_Hash_IPv4
                else:
                    ip_hash = domain_obj.DPSClients_Hash_IPv6
                try:
                    dps_client = ip_hash[ip_val]
                except Exception:
                    break
                #If error is not RETRY it means the DPS Client is up and
                #and running
                if response_error != self.dps_no_response:
                    dps_client.failure_count = DPSClient.failure_count_max
                    DPSClientHost.Host_Touch(ip_val, 0)
                    break
                #Check if MAX Failure have been reached
                if dps_client.failure_count <= 0:
                    if dps_client.location.ip_value != self.cluster_db.Node_Local_Get_IPAddress():
                        message = 'ALERT!!! dps_client %s unreachable'%dps_client.location.show_ip()
                        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
                        #dps_client.delete()
                    break
                #Decrement the failure count
                dps_client.failure_count -= 1
                ip_packed = dps_client.location.ip_value_packed
                port = dps_client.location.port
                if message_type == self.DPS_ADDR_RESOLVE:
                    dvg_obj.send_address_resolution_to(dps_client, vIP_packed)
                elif message_type == self.DPS_UNSOLICITED_ENDPOINT_LOC_REPLY:
                    try:
                        vIP_val = self.ip_get_val_from_packed[vIP_type](vIP_packed)
                    except Exception:
                        break
                    try:
                        if vIP_type == socket.AF_INET:
                            endpoint = domain_obj.Endpoint_Hash_IPv4[vIP_val]
                        else:
                            endpoint = domain_obj.Endpoint_Hash_IPv6[vIP_val]
                        #log.info('Endpoint_Location_vIP: Found Endpoint %s', endpoint)
                        pipv4_list = endpoint.tunnel_endpoint.ip_listv4.ip_list[:]
                        pipv6_list = endpoint.tunnel_endpoint.ip_listv6.ip_list[:]
                        dvg_obj.send_endpoint_location_reply_to(ip_packed,
                                                                port,
                                                                endpoint.dvg.unique_id,
                                                                endpoint.version,
                                                                pipv4_list,
                                                                pipv6_list,
                                                                endpoint.vMac,
                                                                vIP_packed)
                    except Exception:
                        break
                break
        except Exception, ex:
            message = 'Handle_Resolution_Reply, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        self.lock.release()
        return

    def Tunnel_Register(self, domain_id, vnid, client_type, transaction_type,
                        dps_client_IP_type, dps_client_IP_packed, dps_client_port,
                        pip_tuple_list):
        '''
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param dps_client_IP_type: socket.AF_INET6 or socket.AF_INET
        @type dps_client_IP_type: Integer
        @param dps_client_IP_packed: The DPS Client IP as packed data
        @type dps_client_IP_packed: Packed Data
        @param dps_client_port: The DPS Client Port (for UDP communication)
        @type dps_client_port: Integer
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        @return: dps_resp_status_t
        @rtype: Integer
        '''
        ret_val = self.dps_error_none
        self.lock.acquire()
        try:
            while True:
                #Verify valid number
                if len(pip_tuple_list) == 0:
                    ret_val = self.dps_error_invalid_src_ip
                    break
                #Get the IP address as values
                try:
                    dps_client_IP_val = self.ip_get_val_from_packed[dps_client_IP_type](dps_client_IP_packed)
                except Exception:
                    ret_val = self.dps_error_invalid_src_ip
                    break
                try:
                    DPSClientHost.Host_Touch(dps_client_IP_val, dps_client_port)
                except Exception:
                    pass
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                if not domain.valid:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                try:
                    dvg = domain.DVG_Hash[vnid]
                except Exception:
                    ret_val = self.dps_error_invalid_src_dvg
                    break
                if not dvg.valid:
                    ret_val = self.dps_error_invalid_src_dvg
                    break
                #Get DPS Client i.e. (UDP Address and Port)
                try:
                    if dps_client_IP_type == socket.AF_INET:
                        dps_client = domain.DPSClients_Hash_IPv4[dps_client_IP_val]
                    else:
                        dps_client = domain.DPSClients_Hash_IPv6[dps_client_IP_val]
                    #Update port
                    dps_client.location.port = dps_client_port
                except Exception:
                    try:
                        dps_client = DPSClient(domain, dps_client_IP_type, 
                                                dps_client_IP_val, dps_client_port)
                    except Exception, ex:
                        message = 'Tunnel_Register: Cannot create DPS Client %s'%ex
                        dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                        ret_val = self.dps_error_no_memory
                        break
                #Update the TunnelEndpoint
                TunnelEndpoint.register(domain, dvg, client_type, transaction_type, dps_client, pip_tuple_list)
                ##Doing mass transfer in DVG now
                # if domain.mass_transfer is not None:
                #     ret_val = domain.mass_transfer.register_tunnel(vnid, client_type, True, dps_client.location, pip_tuple_list)
                break
        except Exception, ex:
            message = 'Tunnel_Register, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_retry
        self.lock.release()
        return ret_val

    def Tunnel_Unregister(self, domain_id, vnid, client_type, pip_tuple_list):
        '''
        This routine unregisters a set of TunnelEndpoints from the Domain.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param pip_tuple_list: A list of tuples of Physical IP Addresses, each 
                                tuple is of the form (inet_type, ip_value).
                                The inet_type = socket type i.e. AF_INET or AF_INET6
                                The ip_value = IPv4 in integer or IPv6 in string
        @type pip_tuple_list: List of tuples
        @return: dps_resp_status_t
        @rtype: Integer
        '''
        ret_val = self.dps_error_none
        self.lock.acquire()
        try:
            while True:
                try:
                    domain = self.Domain_Hash[domain_id]
                except Exception:
                    ret_val = self.dps_error_invalid_domain_id
                    break
                try:
                    dvg = domain.DVG_Hash[vnid]
                except Exception:
                    ret_val = self.dps_error_invalid_src_dvg
                    break
                #Unregister the TunnelEndpoint
                TunnelEndpoint.unregister(domain, dvg, client_type, pip_tuple_list)
                if domain.mass_transfer is not None:
                    host_location = IPAddressLocation(socket.AF_INET, 0, 0)
                    ret_val = domain.mass_transfer.register_tunnel(vnid, client_type, False, host_location, pip_tuple_list)
                break
        except Exception, ex:
            message = 'Tunnel_Unregister, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_retry
        self.lock.release()
        return ret_val

    def Multicast_Get_Objects(self, domain_id, vnid, client_type, transaction_type,
                              all_multi, multicast_mac, 
                              multicast_ip_family, multicast_ip_packed,
                              tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine returns the relevant PYTHON objects from the given parameters
        @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
        @attention: This routine must be called with the global lock held
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param all_multi: If this is related to all multicast group
        @type all_multi: Boolean
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_family: 0, AF_INET, AF_INET6
        @type tunnel_ip_family: Integer
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        @return: (status, Multicast Object, DVG, Tunnel)
        '''
        ret_val = self.dps_error_none
        while True:
            if all_multi:
                multicast_ip_family = 0
                multicast_IP_val = 0
            else:
                #Get Multicast IP address as values
                try:
                    multicast_IP_val = self.ip_get_val_from_packed[multicast_ip_family](multicast_ip_packed)
                    try:
                        multicast_mac = Multicast.MAC_from_IP(multicast_ip_family, multicast_IP_val)
                    except Exception:
                        ret_val = self.dps_error_invalid_src_ip
                        break
                except Exception:
                    multicast_ip_family = 0
                    multicast_IP_val = 0
                #Validate Multicast MAC
                if not Multicast.MAC_validate(multicast_mac):
                    ret_val = self.dps_error_invalid_euid
                    break
            #Get Tunnel IP as value
            try:
                tunnel_IP_val = self.ip_get_val_from_packed[tunnel_ip_family](tunnel_ip_packed)
            except Exception:
                ret_val = self.dps_error_invalid_dst_ip
                break
            #Get Domain Object
            try:
                domain = self.Domain_Hash[domain_id]
            except Exception:
                ret_val = self.dps_error_invalid_domain_id
                break
            #Get DVG Object
            try:
                dvg = domain.DVG_Hash[vnid]
            except Exception:
                ret_val = self.dps_error_invalid_src_dvg
                break
            #Get Tunnel Object from value
            try:
                if tunnel_ip_family == socket.AF_INET:
                    tunnel = domain.Tunnel_Endpoints_Hash_IPv4[tunnel_IP_val]
                else:
                    tunnel = domain.Tunnel_Endpoints_Hash_IPv6[tunnel_IP_val]
                #Add Tunnel to DVG
                DVG.tunnel_endpoint_add(dvg, False, tunnel, client_type, transaction_type)
            except Exception:
                #Tunnel must have been registered previously in some dvg
                #in the Domain
                ret_val = self.dps_error_invalid_dst_ip
                break
            return (ret_val, domain.Multicast, dvg, multicast_mac, multicast_ip_family, multicast_IP_val, tunnel)
        return (ret_val, None, None, '', 0, 0, None)

    def Multicast_Sender_Register(self, domain_id, vnid, client_type, transaction_type,
                                  multicast_mac, multicast_ip_family, multicast_ip_packed,
                                  tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine registers a Multicast Sender with the following parameters
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_family: 0, AF_INET, AF_INET6
        @type tunnel_ip_family: Integer
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        self.lock.acquire()
        try:
            while True:
                ret_val, multicast, dvg, mac, inet_type, ip_value, tunnel = self.Multicast_Get_Objects(
                                                                                domain_id, vnid, client_type, 
                                                                                transaction_type,
                                                                                False, multicast_mac, 
                                                                                multicast_ip_family, 
                                                                                multicast_ip_packed, 
                                                                                tunnel_ip_family, tunnel_ip_packed)
                if ret_val != self.dps_error_none:
                    break
                try:
                    multicast.sender_add(dvg, mac, inet_type, ip_value, tunnel)
                    domain = dvg.domain
                    if domain.mass_transfer is not None:
                        ret_val = domain.mass_transfer.register_multicast(domain_id,
                                                                          vnid,
                                                                          True,
                                                                          True,
                                                                          client_type,
                                                                          False,
                                                                          multicast_mac, multicast_ip_family, multicast_ip_packed,
                                                                          tunnel_ip_family, tunnel_ip_packed
                                                                         )
                except Exception:
                    ret_val = self.dps_error_retry
                break
        except Exception, ex:
            message = 'Multicast_Sender_Register, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_no_memory
        self.lock.release()
        return ret_val

    def Multicast_Sender_Unregister(self, domain_id, vnid, client_type, transaction_type,
                                    multicast_mac, multicast_ip_family, multicast_ip_packed,
                                    tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine un-registers a Multicast Sender with the following parameters
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_family: 0, AF_INET, AF_INET6
        @type tunnel_ip_family: Integer
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        self.lock.acquire()
        try:
            while True:
                ret_val, multicast, dvg, mac, inet_type, ip_value, tunnel = self.Multicast_Get_Objects(
                                                                                domain_id, vnid, client_type, 
                                                                                transaction_type,
                                                                                False, multicast_mac, 
                                                                                multicast_ip_family, 
                                                                                multicast_ip_packed, 
                                                                                tunnel_ip_family, tunnel_ip_packed)
                if ret_val != self.dps_error_none:
                    break
                try:
                    multicast.sender_delete(dvg, mac, inet_type, ip_value, tunnel)
                    domain = dvg.domain
                    if domain.mass_transfer is not None:
                        ret_val = domain.mass_transfer.register_multicast(domain_id,
                                                                          vnid,
                                                                          True,
                                                                          False,
                                                                          client_type,
                                                                          False,
                                                                          multicast_mac, multicast_ip_family, multicast_ip_packed,
                                                                          tunnel_ip_family, tunnel_ip_packed
                                                                         )
                except Exception:
                    ret_val = self.dps_error_no_memory
                break
        except Exception, ex:
            message = 'Multicast_Sender_Unregister, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_no_memory
        self.lock.release()
        return ret_val

    def Multicast_Receiver_Register(self, domain_id, vnid, client_type, transaction_type, global_scope,
                                    multicast_mac, multicast_ip_family, multicast_ip_packed,
                                    tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine registers a Multicast Receiver with the following parameters
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Integer
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_family: 0, AF_INET, AF_INET6
        @type tunnel_ip_family: Integer
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        self.lock.acquire()
        try:
            while True:
                if global_scope == 1:
                    fglobal_scope = True
                    multicast_mac = DpsCollection.Multicast_All_MAC_Bytes
                else:
                    fglobal_scope = False
                if multicast_mac == DpsCollection.Multicast_All_MAC_Bytes:
                    all_multi = True
                else:
                    all_multi = False
                ret_val, multicast, dvg, mac, inet_type, ip_value, tunnel = self.Multicast_Get_Objects(
                                                                                domain_id, vnid, client_type, 
                                                                                transaction_type,
                                                                                all_multi, multicast_mac, 
                                                                                multicast_ip_family, multicast_ip_packed, 
                                                                                tunnel_ip_family, tunnel_ip_packed)
                if ret_val != self.dps_error_none:
                    break
                try:
                    multicast.receiver_add(dvg, fglobal_scope, all_multi, mac, inet_type, ip_value, tunnel)
                    domain = dvg.domain
                    if domain.mass_transfer is not None:
                        ret_val = domain.mass_transfer.register_multicast(domain_id,
                                                                          vnid,
                                                                          False,
                                                                          True,
                                                                          client_type,
                                                                          fglobal_scope,
                                                                          multicast_mac, multicast_ip_family, multicast_ip_packed,
                                                                          tunnel_ip_family, tunnel_ip_packed
                                                                         )
                except Exception:
                    ret_val = self.dps_error_no_memory
                break
        except Exception, ex:
            message = 'Multicast_Receiver_Register, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_no_memory
        self.lock.release()
        return ret_val

    def Multicast_Receiver_Unregister(self, domain_id, vnid, client_type, transaction_type, global_scope,
                                      multicast_mac, multicast_ip_family, multicast_ip_packed,
                                      tunnel_ip_family, tunnel_ip_packed):
        '''
        This routine un-registers a Multicast Receiver with the following parameters
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param vnid: The VNID/DVG
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in Domain.transaction_types
        @type transaction_type: Integer
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Integer
        @param multicast_mac: The MAC address of the Multicast Group
        @type multicast_mac: ByteArray
        @param multicast_ip_family: 0, AF_INET, AF_INET6
        @type multicast_ip_family: Integer
        @param multicast_ip_packed: Multicast IP in packed format
        @param tunnel_ip_family: 0, AF_INET, AF_INET6
        @type tunnel_ip_family: Integer
        @param tunnel_ip_packed: Tunnel IP in packed format
        @param tunnel_ip_packed: ByteArray
        '''
        self.lock.acquire()
        try:
            while True:
                if global_scope == 1:
                    fglobal_scope = True
                    multicast_mac = DpsCollection.Multicast_All_MAC_Bytes
                else:
                    fglobal_scope = False
                if multicast_mac == DpsCollection.Multicast_All_MAC_Bytes:
                    all_multi = True
                else:
                    all_multi = False
                ret_val, multicast, dvg, mac, inet_type, ip_value, tunnel = self.Multicast_Get_Objects(
                                                                                domain_id, vnid, client_type, 
                                                                                transaction_type,
                                                                                all_multi, multicast_mac, 
                                                                                multicast_ip_family, multicast_ip_packed, 
                                                                                tunnel_ip_family, tunnel_ip_packed)
                if ret_val != self.dps_error_none:
                    break
                try:
                    multicast.receiver_delete(dvg, fglobal_scope, all_multi, mac, inet_type, ip_value, tunnel)
                    domain = dvg.domain
                    if domain.mass_transfer is not None:
                        ret_val = domain.mass_transfer.register_multicast(domain_id,
                                                                          vnid,
                                                                          False,
                                                                          False,
                                                                          client_type,
                                                                          fglobal_scope,
                                                                          multicast_mac, multicast_ip_family, multicast_ip_packed,
                                                                          tunnel_ip_family, tunnel_ip_packed
                                                                         )
                except Exception:
                    ret_val = self.dps_error_no_memory
                break
        except Exception, ex:
            message = 'Multicast_Receiver_Unregister, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            ret_val = self.dps_error_no_memory
        self.lock.release()
        return ret_val

    def Multicast_Global_Scope_Get(self, domain_id):
        '''
        This routine returns the global scope tunnel location for a specific domain id
        @param domain_id: The domain id
        @type domain_id: Integer
        @return: status, vnid, ip_packed
        @rtype: Integer, Integer, ByteArray
        '''
        #print 'Multicast_Global_Scope_Get: Enter\r'
        tunnel_ip_packed = DpsCollection.Invalid_IP_Packed
        tunnel_vnid = 0
        ret_val = self.dps_error_no_route
        self.lock.acquire()
        try:
            while True:
                domain = self.Domain_Hash[domain_id]
                multicast = domain.Multicast
                gscope = multicast.Global_Scope
                vnid_groups = gscope.VNID_Hash.values()
                try:
                    vnid_group = vnid_groups[0]
                except Exception:
                    break
                if not vnid_group.global_scope:
                    break
                if len(vnid_group.receiver_all) == 0:
                    break
                for tunnel in vnid_group.receiver_all.keys():
                    tunnel_vnid = vnid_group.dvg.unique_id
                    tunnel_ip_packed = tunnel.primary_ip().ip_value_packed
                    ret_val = self.dps_error_none
                    break
                break
        except Exception:
            ret_val = self.dps_error_invalid_domain_id
        self.lock.release()
        #print 'Multicast_Global_Scope_Get: Exit\r'
        return (ret_val, tunnel_vnid, tunnel_ip_packed)

    def Multicast_Updates_Send(self):
        '''
        This routine should be called periodically to send updated receiver
        multicast list to the senders.
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        '''
        #log.warning('VNID_Multicast_Updates size %s\r', len(self.VNID_Multicast_Updates))
        try:
            #First process failed lists from before
            tuple_list = []
            for key in self.VNID_Multicast_Updates_To.keys():
                try:
                    vnid_tuple = self.VNID_Multicast_Updates_To[key]
                    del self.VNID_Multicast_Updates_To[key]
                except Exception:
                    continue
                tuple_list.append(vnid_tuple)
            for vnid_tuple in tuple_list:
                sender_dvg = vnid_tuple[0]
                dps_client = vnid_tuple[1]
                mac = vnid_tuple[2]
                inet_type = vnid_tuple[3]
                ip_packed = vnid_tuple[4]
                global_scope = vnid_tuple[5]
                try:
                    ip_value = self.ip_get_val_from_packed[inet_type](ip_packed)
                except Exception:
                    continue
                try:
                    sender_dvg.domain.Multicast.sender_update_vnid_to(sender_dvg, 
                                                                      mac, inet_type, ip_value,
                                                                      dps_client, global_scope)
                except Exception:
                    message = 'Multicast_Updates_Send[0]: Problems sending to VNID %s'%sender_dvg.unique_id
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
            #Nexy process new lists
            tuple_list = []
            for key in self.VNID_Multicast_Updates.keys():
                try:
                    vnid_tuple = self.VNID_Multicast_Updates[key]
                    del self.VNID_Multicast_Updates[key]
                except Exception:
                    continue
                tuple_list.append(vnid_tuple)
            for vnid_tuple in tuple_list:
                sender_dvg = vnid_tuple[0]
                mac = vnid_tuple[1]
                inet_type = vnid_tuple[2]
                ip_value = vnid_tuple[3]
                global_scope = vnid_tuple[4]
                try:
                    sender_dvg.domain.Multicast.sender_update_vnid(sender_dvg, mac, inet_type, ip_value, global_scope)
                except Exception:
                    message = 'Multicast_Updates_Send[1]: Problems sending to VNID %s'%sender_dvg.unique_id
                    dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        except Exception, ex:
            message = 'Multicast_Updates_Send, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def Address_Resolution_Resolve(self, vIP_packed):
        '''
        This routine removes all resolved entries from Address Resolution Send To
        @param vIP_packed: The IP address of the Endpoint
        @type vIP_packed: Packed Data
        '''
        for key in self.Address_Resolution_Requests_To.keys():
            try:
                tuple_ar = self.Address_Resolution_Requests_To[key]
                if vIP_packed == tuple_ar[2]:
                    del self.Address_Resolution_Requests_To[key]
            except Exception:
                pass
        return

    def Address_Resolution_Timeout(self):
        '''
        This routine should be called on a periodic basis to handle timeouts 
        for pending address resolution queries. There are 2 types of pending
        requests.
        1. Address Resolution Requests to specific DPS Client which could
           not be sent out previously (due to resources constraints in the 
           protocol handler).
        2. Address Resolution (for a particular vIP) that hasn't been answered. 
        '''
        #Process Pending Address Resolutions
        pending_ars = []
        for key in self.Address_Resolution_Requests_To.keys():
            try:
                tuple_ar = self.Address_Resolution_Requests_To[key]
                pending_ars.append(tuple_ar)
                del self.Address_Resolution_Requests_To[key]
            except Exception:
                pass
        for tuple_ar in pending_ars:
            dvg = tuple_ar[0]
            dps_client = tuple_ar[1]
            vip_packed = tuple_ar[2]
            dvg.send_address_resolution_to(dps_client, vip_packed)
        #Process Timeouts
        try:
            domain_list = []
            for domain_id in self.Address_Resolution_Requests.keys():
                try:
                    domain = self.Address_Resolution_Requests[domain_id]
                    domain_list.append(domain)
                    del self.Address_Resolution_Requests[domain_id]
                except Exception:
                    pass
            for domain in domain_list:
                if not domain.valid:
                    continue
                domain.AddressResolution.process_timeout()
                if domain.AddressResolution.total > 0:
                    self.Address_Resolution_Requests[domain.unique_id] = domain
        except Exception, ex:
            message = 'Address_Resolution_Timeout, exception %s'%ex
            dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
        return

    def Conflict_Detection_Timeout(self):
        '''
        This routine should be called on a periodic basis to handle timeouts 
        for pending conflict detection queries.
        '''
        #Process Pending Conflict Detectin
        domain_ids = self.Conflict_Detection_Requests.keys()
        #log.warning('Conflict_Detection_Timeout: domain ids %s\r', domain_ids)
        for domain_id in domain_ids:
            try:
                conflict = self.Conflict_Detection_Requests[domain_id]
                del self.Conflict_Detection_Requests[domain_id]
                conflict.process_timeout()
            except Exception:
                pass
        return

    def Endpoint_Expiration_Timeout(self):
        '''
        This routine handle those Endpoint which have been deleted from a
        tunnel but have not been claimed by any other tunnel as yet.
        Typically this will happen in a VM migration scenario when one tunnel
        deletes a VM but another one claims it soon after. We like to
        preserve the vIP addresses on the endpoint from the 1st tunnel.
        This routine will delete those endpoints that haven't been claimed
        by a new tunnel.
        '''
        for endpoint in self.Endpoint_Expiration_Timer_Queue.keys():
            try:
                timeout = self.Endpoint_Expiration_Timer_Queue[endpoint]
                if timeout == 0:
                    del self.Endpoint_Expiration_Timer_Queue[endpoint]
                    endpoint.delete()
                else:
                    self.Endpoint_Expiration_Timer_Queue[endpoint] = timeout - 1
            except Exception:
                pass

    def VNID_Query_Timeout(self):
        '''
        This routine sends a message to query for VNIDs which
        the local node is not aware of.
        @attention: DO NOT hold the global lock when calling this routine
        '''
        self.lock.acquire()
        vnid_list = self.vnid_query_controller.keys()
        self.vnid_query_controller.clear()
        self.lock.release()
        for vnid in vnid_list:
            dcslib.vnid_query_send_to_controller(vnid)
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
        self.lock.acquire()
        #print 'Protocol_Timer_Routine: Enter\r'
        self.Broadcast_Updates_Send()
        self.Policy_Updates_Send()
        self.lock.release()
        self.lock.acquire()
        self.Gateway_Updates_Send()
        self.Multicast_Updates_Send()
        self.lock.release()
        self.lock.acquire()
        self.Address_Resolution_Timeout()
        self.Conflict_Detection_Timeout()
        self.lock.release()
        self.lock.acquire()
        self.Endpoint_Expiration_Timeout()
        #print 'Protocol_Timer_Routine: Exit\r'
        self.lock.release()
        self.lock.acquire()
        DPSClientHost.Send_Heartbeat()
        self.lock.release()
        self.VNID_Query_Timeout()
        if self.started:
            self.timer = Timer(self.Timer_Frequency, self.Protocol_Timer_Routine)
            self.timer.start()
        return
