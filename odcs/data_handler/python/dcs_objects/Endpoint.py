'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas and jinghe
'''
import socket

import logging
from logging import getLogger
log = getLogger(__name__)

from object_collection import DpsCollection
from object_collection import DpsClientType
from object_collection import DpsTransactionType
from object_collection import mac_show
from dcs_objects.Domain import Domain
from dcs_objects.Dvg import DVG
from dcs_objects.TunnelEndpoint import TunnelEndpoint
from dcs_objects.IPAddressLocation import IPAddressLocation
from object_collection import IPSUBNETMode

class Endpoint:
    '''
    This represents an Endpoint Object in the DOVE Environment
    '''

    #######################################################################
    #Endpoint Update Codes based on the following structure:
    #         dps_resp_status_t
    #Described in dps_client_common.h
    #######################################################################
    #typedef enum {
    #    DPS_ENDPOINT_UPDATE_ADD = 1,
    #    DPS_ENDPOINT_UPDATE_DELETE = 2,
    #    DPS_ENDPOINT_UPDATE_VIP_ADD = 3,
    #    DPS_ENDPOINT_UPDATE_VIP_DEL = 4,
    #    DPS_ENDPOINT_UPDATE_MIGRATE_IN = 5,
    #    DPS_ENDPOINT_UPDATE_MIGRATE_OUT = 6,
    #    DPS_ENDPOINT_UPDATE_MAX = 7
    #} dps_endpoint_update_subtype;
    op_update_add = 1
    op_update_delete = 2
    op_update_vip_add = 3
    op_update_vip_del = 4
    op_update_migrate_in = 5
    op_update_migrate_out = 6

    version_start = 0

    def __init__(self, domain, dvg, vnid, client_type, transaction_type, 
                 tunnel_endpoint, vMac, vIP, version):
        '''
        Constructor:
        @param domain: The Domain Object
        @type domain: Domain
        @param dvg: The DVG Object
        @type dvg: DVG
        @param vnid: The VNID (Maybe different from dvg.unique_id for Domain/DVG 0)
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        @param tunnel_endpoint: The DOVE Switch on which the Endpoint is hosted
                                Maybe(None) - Then dove_gateway must be valid
        @type tunnel_endpoint: The Tunnel Endpoint
        @param vMAC: The Virtual MAC Address
        @type vMAC: ByteArray/String
        @param vIP: The IP Address of the Endpoint
        @type vIP: IPAddressLocation
        @param version: The version to start with
        @type version: integer
        '''
        if DpsCollection.Endpoints_Count >= DpsCollection.Endpoints_Count_Max:
            raise MemoryError('Out of Memory')
        self.domain = domain
        self.dvg = dvg
        self.vnid = vnid
        self.client_type = client_type
        self.vMac = vMac
        self.tunnel_endpoint = tunnel_endpoint
        self.vIP_set = {}
        self.vIP_set_show = {}
        if vIP is not None:
            self.vIP_set[vIP.ip_value] = vIP
            self.vIP_set_show[vIP.ip_value] = vIP
        #self.version = self.version_start
        self.version = version
        self.valid = True
        self.in_migration = False
        #############################################################
        #TODO:To be finished
        #Add to the appropriate DPS Client List
        #log.info('__init__: Adding to TunnelEndpoint\r')
        TunnelEndpoint.endpoint_add(tunnel_endpoint, self, transaction_type)
        #log.info('__init__: Added to TunnelEndpoint\r')
        #Add to the appropriate Domain Lists
        Domain.endpoint_add(domain, self)
        #log.info('__init__: Added to Domain\r')
        #Add to the appropriate DVG/VNID Lists
        DVG.endpoint_add(dvg, self)
        #log.info('__init__: Added to DVG\r')
        #self.show(None)
        DpsCollection.Endpoints_Count += 1

    def vIP_add(self, vIP):
        '''
        Add a virtual IP address
        @param vIP: The IP Address of the Endpoint
        @type vIP: IPAddressLocation
        '''
        if vIP is not None:
            self.vIP_set[vIP.ip_value] = vIP
            Domain.endpoint_vIP_add(self.domain, self, vIP)
            DVG.endpoint_vIP_add(self.dvg, self, vIP)
            if len(self.vIP_set) < DpsCollection.Endpoints_vIP_Max:
                self.vIP_set_show[vIP.ip_value] = vIP

    def vIP_del(self, vIP):
        '''
        Delete a virtual IP address
        @param vIP: The IP Address of the Endpoint
        @type vIP: IPAddressLocation
        '''
        if vIP is not None:
            try:
                del self.vIP_set[vIP.ip_value]
                Domain.endpoint_vIP_del(self.domain, vIP)
                DVG.endpoint_vIP_del(self.dvg, vIP)
                del self.vIP_set_show[vIP.ip_value]
            except Exception:
                pass

    def vIP_delete_all(self):
        '''
        Delete all virtual IP Addresses in this endpoint
        '''
        for vIP in self.vIP_set.values():
            try:
                self.vIP_del(vIP)
            except Exception:
                pass

    def vIP_shared_mode(self):
        '''
        This routine determines if there is a shared vIP in the Endpoint
        '''
        mode = IPSUBNETMode.IP_SUBNET_MODE_DEDICATED
        for vIP in self.vIP_set.values():
            if vIP.mode == IPSUBNETMode.IP_SUBNET_MODE_SHARED:
                mode = IPSUBNETMode.IP_SUBNET_MODE_SHARED
                break
        return mode

    def vmotion(self, dvg, vnid, client_type, tunnel_endpoint, transaction_type):
        '''
        This routine handles the scenario where the Endpoint has vmotioned
        (without being explicitly told by the migration source).
        This routine will unlink the Endpoint from the existing DpsClient
        and remove the vIPs
        @param dvg: The DVG Object
        @type dvg: DVG
        @param vnid: The VNID (Maybe different from dvg.unique_id for Domain/DVG 0)
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param tunnel_endpoint: The New Tunnel Endpoint on which the endpoint exists
        @type tunnel_endpoint: TunnelEndpoint
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        '''
        #######################################################################
        #Remove from old values
        #######################################################################
        if self.tunnel_endpoint is not None:
            TunnelEndpoint.endpoint_del(self.tunnel_endpoint, self)
            self.tunnel_endpoint = None
        if self.dvg is not None:
            DVG.endpoint_del(self.dvg, self)
            if self.dvg != dvg:
                #Change in VNIDs. Clear all IPs
                DVG.endpoint_del(self.dvg, self)
                self.vIP_delete_all()
                self.vIP_set_show.clear()
        #######################################################################
        #Add to new values
        #######################################################################
        self.dvg = dvg
        self.vnid = vnid
        DVG.endpoint_add(dvg, self)
        self.tunnel_endpoint = tunnel_endpoint
        self.client_type = client_type
        self.in_migration = False
        TunnelEndpoint.endpoint_add(self.tunnel_endpoint, self, transaction_type)
        return

    def delete(self):
        '''
        Destructor:
        '''
        if not self.valid:
            return
        self.valid = False
        #Remove from Conflict Detection List
        self.domain.ConflictDetection.endpoint_delete(self)
        #Remove from DPS Client List
        TunnelEndpoint.endpoint_del(self.tunnel_endpoint, self)
        #Remove from Domain Collection
        Domain.endpoint_del(self.domain, self)
        #Remove from VNID/DVG collection
        DVG.endpoint_del(self.dvg, self)
        #Remove vIPs
        self.vIP_set.clear()
        self.vIP_set_show.clear()
        DpsCollection.Endpoints_Count -= 1
        return

    def update_del(self, vIP):
        '''
        This handles the Endpoint Update with the DPS_ENDPOINT_UPDATE_DELETE
        option
        @param vIP: The IP Address of the Endpoint
        @type vIP: IPAddressLocation
        '''
        #Add vIP: Again a needed HACK for mass transfer case
        self.vIP_add(vIP)
        #Mark self type as In Migration State
        self.in_migration = True
        if len(DpsCollection.Endpoint_Expiration_Timer_Queue) < DpsCollection.Max_Endpoint_Timer_Queue_Length:
            #Put on the global expiration timer list
            #Remove from Tunnel since it could be a followed by migration. 
            #But keep a reference to the old tunnel till a new tunnel claims it.
            #Even if the old tunnel is removed this reference will keep the tunnel 
            #object around.
            #THIS IS A HACK that keeps endpoint.tunnel_endpoint a valid entity.
            #Otherwise we would have to have the following checks every
            #if endpoint.tunnel_endpoint != None: <---- TOO MANY PLACES!!!
            TunnelEndpoint.endpoint_del(self.tunnel_endpoint, self)
            DpsCollection.Endpoint_Expiration_Timer_Queue[self] = 3
        else:
            self.delete()
        return

    update_function_table = {op_update_add: vIP_add,
                             op_update_delete: update_del,
                             op_update_vip_add: vIP_add,
                             op_update_vip_del: vIP_del,
                             op_update_migrate_in: vIP_add,
                             op_update_migrate_out: update_del}

    def update(self, dvg, vnid, client_type, transaction_type, version, operation, vIP):
        '''
        This handle Endpoint Update on an Endpoint. The domain and 
        DPS Client (on which the endpoint resides) should have been
        resolved before this step i.e. Migration and New Endpoint
        Scenario handled before this routine is called.
        @param dvg: The DVG based on Endpoint Update Parameters
        @type dvg: DVG
        @param vnid: The VNID (Maybe different from dvg.unique_id for Domain/DVG 0)
        @type vnid: Integer
        @param client_type: The Type of Client: should be in DpsClientType.types
        @type client_type: Integer 
        @param transaction_type: The Type of Transaction: should be in DpsTransactionType
        @type transaction_type: Integer
        @param version: The Object Version Number
        @type version: Integer
        @param operation: Should be a dps_endpoint_update_option
        @type operation: Integer
        @param vIP: The IP Address of the Endpoint
        @type vIP: IPAddressLocation
        '''
        self.in_migration = False #The Delete operation can set it to True.
        #Update client type
        self.client_type = client_type
        self.vnid = vnid
        #Since we got an update on this endpoint from an entity, 
        #we can remove from Endpoint Expiration Timer List
        try:
            del DpsCollection.Endpoint_Expiration_Timer_Queue[self]
        except Exception:
            pass
        if self.dvg != dvg:
            #Remove self from old DVG
            DVG.endpoint_del(self.dvg, self)
            #Remove all old vIPs
            self.vIP_delete_all()
            self.vIP_set_show.clear()
            self.dvg = dvg
            DVG.endpoint_add(self.dvg, self)
        if transaction_type == DpsTransactionType.mass_transfer:
            #Don't verify version for mass transfer. Just stay at the highest rev
            if self.version < version:
                self.version = version
        else:
            if version == Endpoint.version_start:
                self.version = Endpoint.version_start
            elif version == self.version:
                self.version = version
            else:
                if version != self.version + 1:
                    raise Exception('Invalid Version: Expecting %s, got %s!!!'%(self.version + 1, version))
                self.version = version
        try:
            self.update_function_table[operation](self, vIP)
        except Exception:
            #log.warning('update: operation %s not supported\r', operation)
            pass
        return

    def show(self):
        '''
        Display Contents of a Endpoint
        '''
        #############################################################
        #To be finished
        #############################################################
        vIP_string = ''
        for vIP in self.vIP_set_show.values():
            vIP_string += vIP.show_ip() + ', '
        vIP_string = vIP_string.rstrip()
        vIP_string = vIP_string.rstrip(',')
        string = 'VNID %s, MAC %s, version %s, vIPs [%s], pIPs[%s]'%(self.vnid,
                                                                     mac_show(self.vMac),
                                                                     self.version,
                                                                     vIP_string,
                                                                     self.tunnel_endpoint.ip_list_get())
        print '%s\r'%string
        return

    def get_string(self):
        '''
        Get a string representing the endpoint
        '''
        vIP_string = ''
        for vIP in self.vIP_set.values():
            vIP_string += vIP.show_ip()
            break
        endpoint_string = 'VNID %s, MAC %s, vIP %s'%(self.vnid, mac_show(self.vMac), vIP_string)
        return endpoint_string
