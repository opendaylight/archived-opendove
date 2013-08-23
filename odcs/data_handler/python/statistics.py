'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: jinghe
'''
import logging
import time
from logging import getLogger
log = getLogger(__name__)

from object_collection import DpsCollection
from object_collection import DOVEStatus

class DpsStatisticsHandler(object):
    '''
    This class handles requests from the DOVE Controller or Leader of cluster. 
    This class has routines which can be called from the C code. Routines
    that are not supposed to called from the C code MUST be marked with the
    following:
    @attention: DO NOT IMPORT THIS FUNCTION FROM C CODE.
    '''
    #Default array size = 24h*60m*2sample/min = 2880
    array_maxsize = 2880

    def __init__(self):
        '''
        Constructor:
        '''
        #Collection of Domains
        self.Domain_Hash = DpsCollection.Domain_Hash
        #Global Lock
        self.lock = DpsCollection.global_lock

    def Domain_List_Get(self):
        '''
        This routine return all Domain IDs in the Node
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @return: The string of domain IDs separate by ','
        @rtype: String
        '''
        domain_list_str = ''
        self.lock.acquire()
        try:
            domain_list = self.Domain_Hash.keys()
            domain_list.sort()
            domain_list_str = ','.join(str(i) for i in domain_list)
        except Exception:
            pass
        self.lock.release()
        return domain_list_str

    def Statistics_Load_Balancing_Update(self, domain_id):
        '''
        This routine updates the statistics of load balancing of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: status
        @rtype: Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                index = len(domain_obj.Stats_Array) - 1
                domain_obj.Endpoint_Update_Count_Delta = (domain_obj.Endpoint_Update_Count - 
                                                          domain_obj.Stats_Array[index][1])
                domain_obj.Endpoint_Lookup_Count_Delta = (domain_obj.Endpoint_Lookup_Count -
                                                          domain_obj.Stats_Array[index][2])
                domain_obj.Policy_Lookup_Count_Delta = (domain_obj.Policy_Lookup_Count -
                                                        domain_obj.Stats_Array[index][3])
                curr_time = time.time()
                domain_obj.Stats_Array.append([curr_time, 
                                               domain_obj.Endpoint_Update_Count, 
                                               domain_obj.Endpoint_Lookup_Count, 
                                               domain_obj.Policy_Lookup_Count])
                if len(domain_obj.Stats_Array) > self.array_maxsize:
                    domain_obj.Stats_Array.pop(0)
                #print '%f:%d,%d,%d\r'%(curr_time, domain_obj.Endpoint_Update_Count, domain_obj.Endpoint_Lookup_Count, domain_obj.Policy_Lookup_Count)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            break
        self.lock.release()
        return ret_val

    def Statistics_Load_Balancing_Get(self, domain_id):
        '''
        This routine gets the statistics of load balancing of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: status, Endpoint_Update_Count, Endpoint_Lookup_Count, 
                 Policy_Lookup_Count, Total number of endpoints
        @rtype: Integer, Integer, Integer, Integer, Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        eu_count = 0
        el_count = 0
        pl_count = 0
        en_count = 0
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                eu_count = domain_obj.Endpoint_Update_Count_Delta
                el_count = domain_obj.Endpoint_Lookup_Count_Delta
                pl_count = domain_obj.Policy_Lookup_Count_Delta
                en_count = len(domain_obj.Endpoint_Hash_MAC) + len(domain_obj.Endpoint_Hash_IPv4) + len(domain_obj.Endpoint_Hash_IPv6)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            break
        self.lock.release()
        return (ret_val, eu_count, el_count, pl_count, en_count)

    def Statistics_General_Statistics_Get(self, domain_id):
        '''
        This routine gets the general statistics of the domain specified by domain_id
        @attention: DO NOT IMPORT THIS FUNCTION FROM PYTHON CODE
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: status, Endpoint_Update_Count, Endpoint_Lookup_Count, 
                 Policy_Lookup_Count, Multicast_Lookup_Count, Internal_Gateway_Lookup_Count
        @rtype: Integer, Integer, Integer, Integer, Integer, Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        eu_count = 0
        el_count = 0
        pl_count = 0
        ml_count = 0
        ig_count = 0
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                eu_count = domain_obj.Endpoint_Update_Count
                el_count = domain_obj.Endpoint_Lookup_Count
                pl_count = domain_obj.Policy_Lookup_Count
                ml_count = domain_obj.Multicast_Lookup_Count
                ig_count = domain_obj.Internal_Gateway_Lookup_Count
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
            break
        self.lock.release()
        return (ret_val, eu_count, el_count, pl_count, ml_count, ig_count)

    def Statistics_Domain_Endpoints(self, domain_id):
        '''
        This routine returns the number of Endpoints and Tunnels in a domain
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @return: status, Endpoint_Count, Tunnel_Count
        @rtype: Integer, Integer, Integer
        '''
        ret_val = DOVEStatus.DOVE_STATUS_OK
        en_count = 0
        tn_count = 0
        self.lock.acquire()
        while True:
            try:
                domain_obj = self.Domain_Hash[domain_id]
                en_count = len(domain_obj.Endpoint_Hash_MAC)
                tn_count = len(domain_obj.Tunnel_Endpoints_Hash_IPv4) + len(domain_obj.Tunnel_Endpoints_Hash_IPv6)
            except Exception:
                ret_val = DOVEStatus.DOVE_STATUS_INVALID_DOMAIN
                break
        self.lock.release()
        return (ret_val, en_count, tn_count)
