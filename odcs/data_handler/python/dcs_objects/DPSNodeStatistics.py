'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: Amitabha Biswas
'''
import logging
import time
#from client_protocol_handler import DpsClientHandler

from object_collection import DpsLogLevels
from logging import getLogger
log = getLogger(__name__)

import dcslib

class DPSNodeStatValues:
    '''
    This represent the weight and values of each statistical category.
    Currently the MAJOR statistical categories are:
    1. Memory Consumed
    2. CPU utilization
    The statistical weight of a combination of F1, F2, F3
    where the weight of Fn = Wn is computed as
    (F1*W1 + F2*W2 + .... Fk*Wk)/(W1 + W2 + ... + Wk)
    The computation is in terms of 100%.
    Note that the Limit is not 100% i.e. it's possible for a Node to
    be above 100%. This is due to the facts that the maximum possible
    values are really plucked out of the air. Those max values can
    vary based on system capability.
    '''
    CPU = 'CPU'
    Memory = 'Memory'
    Domains = 'Domains'

    #Weights attributed to each entity for a Node
    Domain_Average_Per_Node = 500
    Node_Weight_Set = {CPU: 25, Memory: 50, Domains: 25}
    Node_Weight_Total = sum(Node_Weight_Set.values())

    #Weights attributed to each entity for a Domain in a Node
    Node_Domain_Weight_Set = {CPU: 34, Memory: 66}
    Node_Domain_Weight_Total = sum(Node_Domain_Weight_Set.values())

    #CPU Limits per Sec
    Cpu_Limit_Endpoint_Update_Per_Sec = 5000
    Cpu_Limit_Endpoint_Lookup_Per_Sec = 100000
    Cpu_Limit_Policy_Lookup_Per_Sec = 50000

    #Memory Limits Overall
    Memory_Limit_Endpoints = 50000
    Memory_Limit_Tunnels = 5000

    #Heavy Load (Percentage: 100%)
    Heavy_Load_Default = 60
    Heavy_Load = 60

class DPSNodeDomainStatistics:
    '''
    This represents the Statistics of a domain on a node
    '''
    # The CPU utilization values are stored in a circular array with 2 rows.
    # The current index of the row represents the latest values. In the example
    # below row 1 (with the latest) time is the current index.
    # The values in the row represents the number of operations between
    # Time Start and Time End in a row.
    # The reason we need 2 rows is because when a row moves forward e.g. from
    # array index 0 --> 1, the values in that new row are 0s, so for some time
    # we still need the old row 0 to compute the values.
    # --------------------------------------------------------------
    # | Index | Time Start |  Time End  | Update | Lookup | Policy |
    # --------------------------------------------------------------
    # |   0   |     T      |    T+1     | U(T+1) | L(T+1) | P(T+1) |
    # --------------------------------------------------------------
    # |   1   |    T+1     |   latest   |U(late.)|L(late.)|P(late.)|
    # --------------------------------------------------------------
    cpu_time_start_column = 0
    cpu_time_end_column = 1
    cpu_update_column = 2
    cpu_lookup_column = 3
    cpu_policy_column = 4

    cpu_max_time_diff_row = 120 #2 minutes between Time Start and End.
                                #after 2 minutes move to next row

    def __init__(self, domain_id):
        '''
        Constructor:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        self.domain_id = domain_id
        #(Memory)
        self.endpoints = 0
        self.tunnels = 0
        #(CPU) Array
        self.cpu_array = []
        curr_time = time.time()
        self.cpu_array.append([curr_time, curr_time, 0, 0, 0])
        self.cpu_array.append([curr_time, curr_time, 0, 0, 0])
        #[time.time(), time.time(), 0, 0, 0]]
        self.cpu_index = 0

    def update_statistics(self, endpoints, tunnels, update_delta, lookup_delta, policy_delta):
        '''
        This routine updates the domain statistics in a Node
        @param endpoints: The Number of Endpoints in that Domain
        @type endpoints: Integer
        @param tunnels: The Number of Tunnels in that Domain
        @type tunnels: Integer
        @param update_delta: The Number of Updates that were done since the last update
        @type update_delta: Integer
        @param lookup_delta: The Number of Lookups that were done since the last update
        @type lookup_delta: Integer
        @param policy_delta: The Number of Policy Lookups that were done since the last update
        @type policy_delta: Integer
        '''
        #print 'Domain [%s]: Update\r'%self.domain_id
        cpu_row = self.cpu_array[self.cpu_index]
        self.endpoints = endpoints
        self.tunnels = tunnels
        curr_time = time.time()
        curr_index_start_time = cpu_row[self.cpu_time_start_column]
        if curr_time - curr_index_start_time > self.cpu_max_time_diff_row:
            #Need to move to next index
            if self.cpu_index == 0:
                self.cpu_index = 1
            else:
                self.cpu_index = 0
            cpu_row = self.cpu_array[self.cpu_index]
            cpu_row[self.cpu_time_start_column] = curr_time
            cpu_row[self.cpu_update_column] = 0
            cpu_row[self.cpu_lookup_column] = 0
            cpu_row[self.cpu_policy_column] = 0
        #Update the current row
        #print 'Domain [%s]: cpu_index %s, row %s\r'%(self.domain_id, self.cpu_index, cpu_row)
        cpu_row[self.cpu_update_column] += update_delta
        cpu_row[self.cpu_lookup_column] += lookup_delta
        cpu_row[self.cpu_policy_column] += policy_delta
        cpu_row[self.cpu_time_end_column] = curr_time
        #print 'Domain [%s]: cpu_index %s, row %s\r'%(self.domain_id, self.cpu_index, cpu_row)
        #self.load_show()
        #print 'update_statistics: Exit self.domain_id %s\r'%self.domain_id
        return

    def load_cpu(self):
        '''
        This routine determines the cpu load in (numerical) value on a domain on a 
        node
        @return - The load
        @rtype - Float
        '''
        curr_index = self.cpu_index
        if curr_index == 0:
            prev_index = 1
        else:
            prev_index = 0
        curr_row = self.cpu_array[curr_index]
        prev_row = self.cpu_array[prev_index]
        #Get CPU load
        cpu_load = float(0)
        while True:
            time_diff = curr_row[self.cpu_time_end_column] - prev_row[self.cpu_time_start_column]
            if time_diff <= 0:
                break
            #Endpoint Update
            eu_load = float(curr_row[self.cpu_update_column] + prev_row[self.cpu_update_column])
            eu_load = (eu_load * 100)/float(DPSNodeStatValues.Cpu_Limit_Endpoint_Update_Per_Sec)
            eu_load = eu_load/time_diff
            #Endpoint Lookup
            el_load = float(curr_row[self.cpu_lookup_column] + prev_row[self.cpu_lookup_column])
            # print 'Domain [%s]: Endpoint Lookup curr_row %s [%s], prev_row %s [%s]\r'%(self.domain_id, 
            #                                                                            curr_index,
            #                                                                            curr_row[self.cpu_lookup_column],
            #                                                                            prev_index,
            #                                                                            prev_row[self.cpu_lookup_column])
            el_load = (el_load * 100)/float(DPSNodeStatValues.Cpu_Limit_Endpoint_Lookup_Per_Sec)
            el_load = el_load/time_diff
            #print 'Domain [%s]: Endpoint Lookup, Time Diff %s (secs), Load %s\r'%(self.domain_id, time_diff, el_load)
            #Policy Lookup
            pl_load = float(curr_row[self.cpu_policy_column] + prev_row[self.cpu_policy_column])
            pl_load = (pl_load * 100)/float(DPSNodeStatValues.Cpu_Limit_Policy_Lookup_Per_Sec)
            pl_load = pl_load/time_diff
            #Don't divide by 3 here since each is a max possible value on the system
            cpu_load = eu_load + el_load + pl_load
            #print 'Domain [%s]: CPU Load %s\r'%(self.domain_id, cpu_load)
            break
        return cpu_load

    def load_memory(self):
        '''
        This routine determines the memory load in (numerical) value on a domain on a 
        node
        @return - The load
        @rtype - Float
        '''
        #Endpoint
        em_load = float(self.endpoints*100)/float(DPSNodeStatValues.Memory_Limit_Endpoints)
        #Tunnels
        tm_load = float(self.tunnels*100)/float(DPSNodeStatValues.Memory_Limit_Tunnels)
        #Divide by 2 here since endpoint and tunnels are by themselves not max values
        memory_load = (em_load + tm_load)/2
        return memory_load

    def load(self):
        '''
        This routine determines the load in (numerical) value on a domain on a 
        node
        @return - The load
        @rtype - Float
        '''
        cpu_load = self.load_cpu()
        memory_load = self.load_memory()
        #Get Total Load
        load = ((cpu_load * DPSNodeStatValues.Node_Domain_Weight_Set[DPSNodeStatValues.CPU]) +
                (memory_load * DPSNodeStatValues.Node_Domain_Weight_Set[DPSNodeStatValues.Memory]))
        load = load/DPSNodeStatValues.Node_Domain_Weight_Total
        return load

    def load_show(self):
        '''
        This routine shows the load on the domain
        '''
        cpu_load = self.load_cpu()
        memory_load = self.load_memory()
        #Get Total Load
        load = ((cpu_load * DPSNodeStatValues.Node_Domain_Weight_Set[DPSNodeStatValues.CPU]) +
                (memory_load * DPSNodeStatValues.Node_Domain_Weight_Set[DPSNodeStatValues.Memory]))
        load = load/DPSNodeStatValues.Node_Domain_Weight_Total
        print 'Domain %s: CPU %.2f, Memory %.2f, Total(Amortized) %.2f\r'%(self.domain_id,
                                                                           cpu_load,
                                                                           memory_load,
                                                                           load)
        return

    @staticmethod
    def load_min(NodeDomain1, NodeDomain2):
        '''
        This method determines the lesser loaded of the 2 domains
        @param nodedomain1: The Domain1 on Node
        @type nodedomain1: DPSNodeDomainStatistics
        @param nodedomain2: The Domain2 on Node
        @type nodedomain2: DPSNodeDomainStatistics
        '''
        load1 = NodeDomain1.load()
        load2 = NodeDomain2.load()
        if load1 > load2:
            return NodeDomain2
        else:
            return NodeDomain1

    @staticmethod
    def load_max(NodeDomain1, NodeDomain2):
        '''
        This method determines the higher loaded of the 2 domains
        @param nodedomain1: The Domain1 on Node
        @type nodedomain1: DPSNodeDomainStatistics
        @param nodedomain2: The Domain2 on Node
        @type nodedomain2: DPSNodeDomainStatistics
        '''
        load1 = NodeDomain1.load()
        load2 = NodeDomain2.load()
        if load1 > load2:
            return NodeDomain1
        else:
            return NodeDomain2

    @staticmethod
    def load_min_array(NodeDomains):
        '''
        This gets the lowest loaded Domain in the Array
        '''
        lowest = None
        for nd in NodeDomains:
            if lowest is None:
                lowest = nd
                continue
            lowest = DPSNodeDomainStatistics.load_min(lowest, nd)
        return lowest

    @staticmethod
    def load_max_array(NodeDomains):
        '''
        This gets the lowest loaded Domain in the Array
        '''
        highest = None
        for nd in NodeDomains:
            if highest is None:
                highest = nd
                continue
            highest = DPSNodeDomainStatistics.load_max(highest, nd)
        return highest

    @staticmethod
    def load_array(NodeDomains):
        '''
        This method computes the cumulative load of all the domains in the node
        @param NodeDomains: List of DPSNodeDomainStatistics
        @type NodeDomains: [DPSNodeDomainStatistics]
        @return: The Load (CPU+Memory+Domains) on the List
        @rtype: float
        '''
        load_cpu = float(0)
        load_memory = float(0)
        for nd in NodeDomains:
            load_cpu += nd.load_cpu()
            load_memory += nd.load_memory()
        load_domain = (float(len(NodeDomains))*100)/float(DPSNodeStatValues.Domain_Average_Per_Node)
        #Get Total Load
        load = ((load_cpu * DPSNodeStatValues.Node_Weight_Set[DPSNodeStatValues.CPU]) +
                (load_memory * DPSNodeStatValues.Node_Weight_Set[DPSNodeStatValues.Memory]) +
                (load_domain * DPSNodeStatValues.Node_Weight_Set[DPSNodeStatValues.Domains]))/(DPSNodeStatValues.Node_Weight_Total)
        return load

class DPSNodeStatistics:
    '''
    This class represents the statistics and load of a DPS Node
    '''

    def __init__(self, location):
        '''
        Constructor
        @param location: The Location of the DPS Node
        @type location: IPAddressLocation
        '''
        self.location = location
        self.domain_statistics = {}

    def domain_add(self, domain_id):
        '''
        This routine adds a domain to the DPSNodeStatistics
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        try:
            domain_stats = self.domain_statistics[domain_id]
        except Exception:
            domain_stats = DPSNodeDomainStatistics(domain_id)
            self.domain_statistics[domain_id] = domain_stats
        return

    def domain_delete(self, domain_id):
        '''
        @param domain_id: The Domain ID
        @type domain_id: Integer
        '''
        try:
            del self.domain_statistics[domain_id]
        except Exception:
            pass
        return

    def domain_delete_all(self):
        '''
        Delete all the Domains
        '''
        self.domain_statistics.clear()
        return

    def domains_get(self):
        '''
        This returns the list of domain ids
        '''
        return self.domain_statistics.keys()

    def update_statistics(self, domain_id, endpoints, tunnels, update_delta, lookup_delta, policy_delta):
        '''
        This routine updates the domain statistics in a Node
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param endpoints: The Number of Endpoints in that Domain
        @type endpoints: Integer
        @param tunnels: The Number of Tunnels in that Domain
        @type tunnels: Integer
        @param update_delta: The Number of Updates that were done since the last update
        @type update_delta: Integer
        @param lookup_delta: The Number of Lookups that were done since the last update
        @type lookup_delta: Integer
        @param policy_delta: The Number of Policy Lookups that were done since the last update
        @type policy_delta: Integer
        '''
        try:
            domain_stats = self.domain_statistics[domain_id]
            domain_stats.update_statistics(endpoints, tunnels, update_delta, lookup_delta, policy_delta)
        except Exception:
            pass
        return

    def heavy_load(self):
        '''
        This routine returns if this node is heavily loaded.
        @return: True if heavy load, False otherwise
        @rtype: Boolean
        '''
        my_load = DPSNodeDomainStatistics.load_array(self.domain_statistics.values())
        message = 'Node %s, Load %s'%(self.location.show_ip(), my_load)
        dcslib.dps_cluster_write_log(DpsLogLevels.NOTICE, message)
        if my_load > DPSNodeStatValues.Heavy_Load:
            return (True, my_load)
        else:
            return (False, my_load)

    @staticmethod
    def load_min(node1, node2):
        '''
        This routine determine the lower loaded node of the 2 nodes
        @param node1: Node1
        @type node1: DPSNodeStatistics
        @param node2: Node2
        @type node2: DPSNodeStatistics
        @return: The lower loaded of the 2 nodes
        @rtype: DPSNodeStatistics
        '''
        load_node1 = DPSNodeDomainStatistics.load_array(node1.domain_statistics.values())
        load_node2 = DPSNodeDomainStatistics.load_array(node2.domain_statistics.values())
        if load_node1 < load_node2:
            return node1
        else:
            return node2

    @staticmethod
    def load_max(node1, node2):
        '''
        This routine determine the higher loaded node of the 2 nodes
        @param node1: Node1
        @type node1: DPSNodeStatistics
        @param node2: Node2
        @type node2: DPSNodeStatistics
        @return: The higher loaded of the 2 nodes
        @rtype: DPSNodeStatistics
        '''
        load_node1 = DPSNodeDomainStatistics.load_array(node1.domain_statistics.values())
        load_node2 = DPSNodeDomainStatistics.load_array(node2.domain_statistics.values())
        if load_node1 > load_node2:
            return node1
        else:
            return node2

    @staticmethod
    def load_min_array(nodes):
        '''
        This routine determines the lowest loaded node in the array of nodes
        @param nodes: Array of DPSNodeStatistics
        @type nodes:[DPSNodeStatistics]
        @return: The lowest loaded node in the array
        @rtype: DPSNodeStatistics
        '''
        lowest = None
        for node in nodes:
            if lowest is None:
                lowest = node
                continue
            lowest = DPSNodeStatistics.load_min(lowest, node)
        return lowest

    @staticmethod
    def load_max_array(nodes):
        '''
        This routine determines the highest loaded node in the array of nodes
        @param nodes: Array of DPSNodeStatistics
        @type nodes:[DPSNodeStatistics]
        @return: The highest loaded node in the array
        @rtype: DPSNodeStatistics
        '''
        highest = None
        for node in nodes:
            if highest is None:
                highest = node
                continue
            highest = DPSNodeStatistics.load_max(highest, node)
        return highest

    @staticmethod
    def load_min_max_array(nodes):
        '''
        This routine determines the highest loaded node in the array of nodes
        @param nodes: Array of DPSNodeStatistics
        @type nodes:[DPSNodeStatistics]
        @return: lowest and highest loaded node in the array
        @rtype: (DPSNodeStatistics, DPSNodeStatistics)
        '''
        highest = None
        lowest = None
        for node in nodes:
            if highest is None:
                highest = node
            else:
                highest = DPSNodeStatistics.load_max(highest, node)
            if lowest is None:
                lowest = node
            else:
                lowest = DPSNodeStatistics.load_min(lowest, node)
        return (lowest, highest)

    @staticmethod
    def load_available_nodes(nodes, num, fhigh):
        '''
        This routine determines the lowest or highest loaded nodes in the array
        @param nodes: Array of DPSNodeStatistics
        @type nodes:[DPSNodeStatistics]
        @param num: The number of nodes needed
        @type num: Integer
        @param fhigh: If this value is True, then this routine will return 
                      Highest Loaded nodes otherwise the Lowest Loaded Nodes
        @type fhigh: Boolean
        @return: List of loaded nodes - maximum number "num"
        @rtype:[ip_value1, ip_value2]...
        '''
        #The set of nodes index by Load
        Node_Array = []
        Load_Set = {}
        for node in nodes:
            load = DPSNodeDomainStatistics.load_array(node.domain_statistics.values())
            Load_Set[node.location.ip_value] = load
        #Sort the Dictionary based on Load i.e. value
        Load_Node_Array = sorted([(value,key) for (key,value) in Load_Set.items()],reverse=fhigh)
        for i in range(num):
            try:
                load_tuple = Load_Node_Array[i]
                Node_Array.append(load_tuple[1])
            except Exception:
                break
        return Node_Array

    @staticmethod
    def load_available_nodes_packed(nodes, num, fhigh):
        '''
        This routine determines the lowest or highest loaded nodes in the array
        @param nodes: Array of DPSNodeStatistics
        @type nodes:[DPSNodeStatistics]
        @param num: The number of nodes needed
        @type num: Integer
        @param fhigh: If this value is True, then this routine will return 
                      Highest Loaded nodes otherwise the Lowest Loaded Nodes
        @type fhigh: Boolean
        @return: List of loaded nodes - maximum number "num"
        @rtype:[ip_packed1, ip_packed2]...
        '''
        #The set of nodes index by Load
        Node_Array = []
        Load_Set = {}
        for node in nodes:
            load = DPSNodeDomainStatistics.load_array(node.domain_statistics.values())
            Load_Set[node.location.ip_value_packed] = load
        #Sort the Dictionary based on Load i.e. value
        Load_Node_Array = sorted([(value,key) for (key,value) in Load_Set.items()],reverse=fhigh)
        for i in range(num):
            try:
                load_tuple = Load_Node_Array[i]
                Node_Array.append(load_tuple[1])
            except Exception:
                break
        return Node_Array

    @staticmethod
    def load_balance_domain(node_high, node_low):
        '''
        This routine determines which domain can be moved from node1 to
        node2 or vice-versa that will minimize the load differential 
        between the two i.e. minimize |load(node_high) - load(node_low)|
        @param node_high: The Higher Loaded Node
        @type node_high: DPSNodeStatistics
        @param node_low: The Lower Loaded Node
        @type node_low: DPSNodeStatistics
        @return: domain_id
        @rtype: Integer
        @raise: Exception if node_high is already lower than node_low 
                or no domain is found
        '''
        #print 'load_balance_domain: Enter High %s, Low %s\r'%(node_high.location.show_ip(),
        #                                                      node_low.location.show_ip())
        load_high = DPSNodeDomainStatistics.load_array(node_high.domain_statistics.values())
        load_low = DPSNodeDomainStatistics.load_array(node_low.domain_statistics.values())
        #print 'load_balance_domain: load_high %s, load_low %s\r'%(load_high, load_low)
        if load_high <= load_low:
            raise Exception('Load on Node %s[%.2f], already lower than Node %s[%.2f]'%(node_high.location.show_ip(),
                                                                                       load_high,
                                                                                       node_low.location.show_ip(),
                                                                                       load_low))
        domain_id = None
        domain_ids = {}
        for node_domain in node_high.domain_statistics.values():
            #Check if this domain is present in the lower node
            domain_id = node_domain.domain_id
            try:
                node_domain_low = node_low.domain_statistics[domain_id]
                continue
            except Exception:
                pass
            #Determine the Load if this domain moved from node_high to node_low
            load_domain = node_domain.load()
            #print 'Loop: domain %s, load %s\r'%(node_domain.domain_id, load_domain)
            if load_domain > load_high:
                message = 'Coding Error? Load on domain %s[%.2f] on node %s higher the load on node %.2f'%(node_domain.domain_id,
                                                                                                           load_domain,
                                                                                                           node_high.location.show_ip(),
                                                                                                           load_high,
                                                                                                          )
                dcslib.dps_cluster_write_log(DpsLogLevels.WARNING, message)
                continue
            load_high_new = load_high - load_domain
            load_low_new = load_low + load_domain
            load_diff_new = abs(load_high_new - load_low_new)
            domain_ids[node_domain] = load_diff_new
        if domain_id is None:
            raise Exception('No suitable domain found')
        #Sort the List based on values
        domain_tuples = sorted([(value,key) for (key,value) in domain_ids.items()])
        #Only chose the 10 differential lowest domains i.e. the domains that cause max swing in load diff
        domain_tuples = domain_tuples[:10]
        print 'domain_tuples: %s\r'%domain_tuples
        domain_list = []
        load_high_new = load_high
        load_low_new = load_low
        for domain_tuple in domain_tuples:
            node_domain = domain_tuple[1]
            domain_id = node_domain.domain_id
            load_domain = node_domain.load()
            load_high_new -= load_domain
            load_low_new += load_domain
            if load_low_new > DPSNodeStatValues.Heavy_Load:
                break
            if load_high_new < load_low_new:
                break
            domain_list.append(domain_id)
        #print 'load_balance_domain: Exit domain_id %s\r'%domain_id
        if len(domain_list) == 0:
            raise Exception('No suitable domain found')
        print 'domain_list: %s\r'%domain_list
        return domain_list

    def show(self):
        '''
        Show
        '''
        print '------------------------------------------------------------------\r'
        print 'Load on DPS Node %s\r'%self.location.show_ip()
        load_cpu = float(0)
        load_memory = float(0)
        for domain in self.domain_statistics.keys():
            try:
                nd = self.domain_statistics[domain]
            except Exception:
                continue
            load_cpu += nd.load_cpu()
            load_memory += nd.load_memory()
            nd.load_show()
        total_load = DPSNodeDomainStatistics.load_array(self.domain_statistics.values())
        print 'Load: CPU %.3f, Memory %.3f, Overall(Amortized) %.3f\r'%(load_cpu, load_memory, total_load)
        print '------------------------------------------------------------------\r'
        return

