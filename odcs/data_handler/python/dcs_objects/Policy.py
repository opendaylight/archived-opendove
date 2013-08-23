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
from object_collection import DOVEGatewayTypes
from object_collection import DOVEStatus

from dcs_objects import dcs_object
from dcs_objects.Domain import Domain
from dcs_objects.Dvg import DVG

import struct

class Policy(dcs_object):
    '''
    This represents an Policy Object in the DOVE Environment
    '''

    #typedef enum {
    #    DPS_POLICY_TYPE_CONNECTIVITY = 1,
    #    DPS_POLICY_TYPE_SOURCE_ROUTING,
    #} dps_policy_type;
    #List of action type
    type_connectivity = 1
    type_ip_list = 2
    type_bad = 3

    action_drop = 0
    action_forward = 1

    key_fmt = '%d:%d'

    #TODO: Confirm this structure remains the same
    #Definition of action provided below
    #typedef struct dps_object_policy_action_s{
    #uint8_t ver;
    #uint8_t pad;
    #union
    #{
    #    uint16_t connectivity;
    #    struct
    #    {
    #        uint16_t address_count;
    #        uint8_t ip_list[0];
    #    } source_routing;
    #};
    #} dps_object_policy_action_t;
    fmt_action_struct_hdr = 'BBH'
    fmt_action_struct_hdr_size = 4
    action_drop_packed = struct.pack(fmt_action_struct_hdr, 0, 0, action_drop)
    action_forward_packed = struct.pack(fmt_action_struct_hdr, 0, 0, action_forward)

    def __init__(self, domain, traffic_type, policy_type, src_dvg, dst_dvg, ttl, action):
        '''
        Constructor:
        @param domain: The Domain Object
        @type domain: Domain
        @param traffic_type: The Traffic Type (0: Unicast, 1:Multicast)
        @type traffic_type: Integer
        @param policy_type: The Policy Type
        @type policy_type: Integer
        @param src_dvg: The Source DVG Object
        @type src_dvg: DVG
        @param dst_dvg: The Destination DVG Object
        @type dst_dvg: DVG
        @param ttl: Time-To-Live cached in DPS client
        @type ttl: Integer
        @param action: Action taken to traffic from Source DVG to destination DVG
        @type action: IPAddressList or Integer depends on action_type
        @raise DOVE_STATUS_INVALID_POLICY: Bad policy type
        '''
        #Based on Object Model Chapter 5: Policy Object
        if policy_type <= 0 or policy_type >= self.type_bad:
            raise DOVEStatus.DOVE_STATUS_INVALID_POLICY
        if policy_type == self.type_ip_list:
            raise DOVEStatus.DOVE_STATUS_NOT_SUPPORTED
        self.domain = domain
        self.traffic_type = traffic_type
        self.type = policy_type
        self.src_dvg = src_dvg
        self.dst_dvg = dst_dvg
        self.ttl = ttl
        self.action = action
        action_struct  = struct.unpack(self.fmt_action_struct_hdr,
                                       self.action[:self.fmt_action_struct_hdr_size])
        self.action_connectivity = self.action_drop
        if self.type == self.type_connectivity:
            self.action_connectivity = action_struct[2] #3rd parameter
        self.version = action_struct[0]
        self.key = self.key_fmt%(src_dvg.unique_id, dst_dvg.unique_id)
        #TODO: Store the Policy result in a packed data format that
        #      can be sent to the C Code when needed.
        #
        #############################################################
        dcs_object.__init__(self, self.key)
        #############################################################
        #Add the policy into a Policy_Hash table in a domain,
        #source DVG and destination DVG
        #############################################################
        Domain.policy_add(self.domain, self)
        DVG.policy_add_src(src_dvg, dst_dvg.unique_id, self)
        DVG.policy_add_dst(dst_dvg, src_dvg.unique_id, self)

    def update(self, ttl, action):
        '''
        This routine updates a policy. The routine assumes that the 
        Source DVG and Destination DVG of the policy remains the same
        @param ttl: Time-To-Live cached in DPS client
        @type ttl: Integer
        @param action: Action taken to traffic from Source DVG to destination DVG
        @type action: IPAddressList or Integer depends on action_type
        '''
        #log.warning('Policy.Update\r')
        self.ttl = ttl
        self.action = action
        action_struct  = struct.unpack(self.fmt_action_struct_hdr,
                                       self.action[:self.fmt_action_struct_hdr_size])
        self.action_connectivity = self.action_drop
        if self.type == self.type_connectivity:
            self.action_connectivity = action_struct[2] #3rd parameter
        #log.info('action_connectivity %s', self.action_connectivity)
        self.version += 1
        #Send update to source DVG
        if self.domain.active:
            DpsCollection.policy_update_queue.put((self.src_dvg, self.traffic_type))

    def delete(self):
        '''
        Destructor: Remove itself from collection
        '''
        if not self.valid:
            return
        self.valid = False
        if self.src_dvg == self.dst_dvg:
            #Restore to default allow policy and send update to VNID
            self.update(0, 67108864, struct.pack(Policy.fmt_action_struct_hdr,
                                                 0, 0, Policy.action_forward))
            if self.domain.active:
                DpsCollection.policy_update_queue.put((self.src_dvg, self.traffic_type))
        else:
            #Remove self from Domain, SRC and DST VNID/DVG
            DVG.policy_del_dst(self.dst_dvg, self.src_dvg.unique_id, self.traffic_type)
            DVG.policy_del_src(self.src_dvg, self.dst_dvg.unique_id, self.traffic_type)
            Domain.policy_del(self.domain, self)

    def show(self):
        '''
        Display Contents of a Policy
        '''
        #############################################################
        #To be finished
        return 'VNIDs <%s> Connectivity Action %s'%(self.key, self.action_connectivity)

Policy.add_class()
