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

import Queue #renamed to queue in Python 3.0
import threading
import struct
from threading import Thread
from threading import Lock

class DOVEGatewayTypes:
    '''
    This class contains the various types of gateway 
    '''
    #typedef enum {
    #    GATEWAY_TYPE_EXTERNAL = 1,
    #    GATEWAY_TYPE_VLAN = 2,
    #    GATEWAY_TYPE_IMPLICIT = 3,
    #} data_handler_gateway_types;
    GATEWAY_TYPE_EXTERNAL = 1
    GATEWAY_TYPE_VLAN = 2
    GATEWAY_TYPE_IMPLICIT = 3

class policy_update_worker_thread(Thread):
    """
    This is the thread that can be run in the background to the send
    new policy updates to DPS Clients in that Domain.
    """
    def __init__(self, queue):
        """
        @param queue: The queue that will contain all the items to be
                      worked on.
        @type queue: Queue
        """
        Thread.__init__(self)
        self.queue = queue
        self.setDaemon(True)

    def run(self):
        """
        This is the worker thread that sends policy updates to all DPS Clients
        in the Domain.
        """
        while True:
            dvg, traffic_type = self.queue.get()
            try:
                dvg.send_policy_update(traffic_type)
            except (Exception, MemoryError):
                pass
            self.queue.task_done()
        log.warning('policy_update_thread: Exiting...!\r')

class vm_migration_worker_thread(Thread):
    """
    This is the thread that can be run in the background to the send
    vm migration updates to DPS Clients in that Domain.
    """
    def __init__(self, queue):
        """
        @param queue: The queue that will contain all the items to be
                      worked on.
        @type queue: Queue
        """
        Thread.__init__(self)
        self.queue = queue
        self.setDaemon(True)

    def run(self):
        """
        This is the worker thread that sends vm migration updates to all DPS Clients
        in the Domain.
        """
        while True:
            endpoint_obj, domain_obj, vnid = self.queue.get()
            try:
                domain_obj.send_vm_migration_update(endpoint_obj,vnid)
            except (Exception, MemoryError):
                pass
            self.queue.task_done()
        log.warning('vm_migration_worker_thread: Exiting...!\r')

class gateway_update_worker_thread(Thread):
    """
    This is the thread that can be run in the background to the send
    new gateway updates to DPS Clients in that Domain.
    """
    def __init__(self, queue):
        """
        @param queue: The queue that will contain all the items to be
                      worked on.
        @type queue: Queue
        """
        Thread.__init__(self)
        self.queue = queue
        self.setDaemon(True)

    def run(self):
        """
        This is the worker thread that sends policy updates to all DPS Clients
        in the Domain.
        """
        while True:
            dvg, gateway_type = self.queue.get()
            try:
                dvg.gateway_list_modified(gateway_type)
            except (Exception, MemoryError):
                pass
            self.queue.task_done()
        log.warning('gateway_update_worker_thread: Exiting...!\r')

class address_resolution_worker_thread(Thread):
    """
    This is the thread that can be run in the background to the send
    new gateway updates to DPS Clients in that Domain.
    """

    def __init__(self, queue):
        """
        @param queue: The queue that will contain all the items to be
                      worked on.
        @type queue: Queue
        """
        Thread.__init__(self)
        self.queue = queue
        self.setDaemon(True)

    def run(self):
        """
        This is the worker thread that sends performs address resolution
        operations.
        """
        while True:
            function, param = self.queue.get()
            try:
                function(param)
            except (Exception, MemoryError):
                pass
            self.queue.task_done()
        log.warning('address_resolution_worker_thread: Exiting...!\r')

class cluster_heartbeat_request_worker_thread(Thread):
    """
    This is the thread that can be run in the background to the send
    new DPS Cluster Heartbeat Request messages to non-leader nodes
    """

    def __init__(self, queue):
        """
        @param queue: The queue that will contain all the items to be
                      worked on.
        @type queue: Queue
        """
        Thread.__init__(self)
        self.queue = queue
        self.setDaemon(True)

    def run(self):
        """
        This is the worker thread that sends performs address resolution
        operations.
        """
        while True:
            cluster = self.queue.get()
            try:
                cluster.leader_send_hearbeat_request()
            except (Exception, MemoryError):
                pass
            self.queue.task_done()
        log.warning('cluster_heartbeat_request_worker_thread: Exiting...!\r')

def mac_bytes(mac_string):
    '''
    Returns the byte array representing a MAC String.
    This routine expects the MAC to be of the format A:B:C:D:E:F
    @raise exception: If MAC address not of right format
    '''
    try:
        macs = mac_string.split(':')
#        if len(macs) != 6:
#            raise Exception('MAC Address should be of the form aa:bb:cc:dd:ee:ff')
        for i in range(len(macs)):
            macs[i] = int(macs[i], 16)
            if macs[i] < 0 or macs[i] > 255:
                raise Exception('MAC Address Bytes not well formed')
    except Exception:
        raise Exception('MAC Address should be of the form aa:bb:cc:dd:ee:ff')
    return ''.join(chr(macs[i]) for i in range(len(macs)))

def mac_show(mac):
    '''
    This returns the MAC Address in string format A:B:C:D:E:F
    '''
    return '%x:%x:%x:%x:%x:%x'%(ord(mac[0]), ord(mac[1]), ord(mac[2]),
                                ord(mac[3]), ord(mac[4]), ord(mac[5]))

class DpsCollection(object):
    '''
    This class contains all the collection
    '''
    Shared_VNID = 0
    Shared_DomainID = 0
    #Internal Gateway MAC
    IGateway_MAC_String = '00:18:b1:aa:aa:00'
    IGateway_MAC_Bytes = mac_bytes(IGateway_MAC_String)
    #Internal Gateway MAC
    EGateway_MAC_String = '00:18:b1:aa:aa:01'
    EGateway_MAC_Bytes = mac_bytes(EGateway_MAC_String)
    #All Multi MAC
    Multicast_All_MAC = '01:00:5E:00:00:00'
    Multicast_All_MAC_Bytes = mac_bytes(Multicast_All_MAC)
    #Invalid MAC
    Invalid_MAC_String = '00:00:00:00:00:00'
    Invalid_MAC_Bytes = mac_bytes(Invalid_MAC_String)
    #Dummy IP used for Gateway Tunnel 0.0.0.0
    Gateway_Tunnel_IP_Packed = struct.pack('I', 0)
    #Dummy IP used for other purposes 0.0.0.0
    Invalid_IP_Packed = struct.pack('I', 0)
    #The Controller IP
    Controller_Location = None
    #Collection of all Domains handled by this node
    Domain_Hash = {}
    #Collection of all DVG_IDS/VNIDs in the DOVE Environment
    VNID_Hash = {}
    VNID_Hash[Shared_VNID] = Shared_DomainID
    #Collection of all Domain in the DOVE Environment. 
    #Each domain will have a set of vnids it handles (ids only)
    Domain_Hash_Global = {}
    domain_shared_mapping = {}
    domain_shared_mapping[Shared_VNID] = Shared_VNID
    Domain_Hash_Global[Shared_DomainID] = domain_shared_mapping
    #Collection of all Domains in the Cluster
    Cluster_Domains_Hash = {}
    #Collection of all VLAN Gateways in DOVE
    VLAN_Gateways ={}
    #Collection of Replication Requests
    Domain_Replication_Requests = {}
    #Count of Endpoints Registered
    Endpoints_Count = 0
    Endpoints_Count_Max = 400000
    Endpoints_vIP_Max = 8
    policy_all_max_per_datagram = 1000
    #A queue for sending Policy Updates in a Domain
    policy_update_queue = Queue.Queue()
    policy_update_thread = policy_update_worker_thread(policy_update_queue)
    policy_update_thread.start()
    #A queue to send Implicit Gateway Updates in a Domain
    gateway_update_queue = Queue.Queue()
    gateway_update_thread = gateway_update_worker_thread(gateway_update_queue)
    gateway_update_thread.start()
    #A queue for Address Resolution
    address_resolution_queue = Queue.Queue()
    address_resolution_thread = address_resolution_worker_thread(address_resolution_queue)
    address_resolution_thread.start()
    #A queue for Heartbeat Requests
    heartbeat_request_queue = Queue.Queue()
    heartbeat_request_thread = cluster_heartbeat_request_worker_thread(heartbeat_request_queue)
    heartbeat_request_thread.start()
    #A queue for sending VM Migration Updates in a Domain
    vm_migration_update_queue = Queue.Queue()
    vm_migration_update_thread = vm_migration_worker_thread(vm_migration_update_queue)
    vm_migration_update_thread.start()
    #Collection of Broadcast Update Request. This contains the list of DVGs
    #for which updates must be sent to every DOVE Switch in that DVGs.
    VNID_Broadcast_Updates = {}
    #This contains Broadcast Sent to Specific DPS Client. Should be used in case of
    #failure in Broadcast Updates,Collection of (DVG,DPSClient) Tuples. 
    #Indexed by "DVG_ID,DPSClient.IPvalue"
    VNID_Broadcast_Updates_To = {}
    #Collection of (DVG,DPSClient) Tuples. Indexed by "DVG_ID,DPSClient.IPvalue"
    Policy_Updates_To = {}
    #Collection of (DVG,DPSClient) Tuples. Indexed by "DVG_ID,DPSClient.IPvalue"
    Gateway_Updates_To = {}
    #Collection of (sender dvgs) which need to told about new Receiver List
    VNID_Multicast_Updates = {}
    #This contains Multicast Sent to Specific DPS Client. Should be used in case of
    #failure in Broadcast Updates,Collection of (DVG,DPSClient) Tuples. 
    #Indexed by "DVG_ID,DPSClient.IPvalue"
    VNID_Multicast_Updates_To = {}
    #Collection of Address Resolution requests waiting to be resolved.
    Address_Resolution_Requests = {}
    #Collection of Address Resolution which need to be retransmitted
    Address_Resolution_Requests_To = {}
    #Collection of Conflict Resolution requests waiting to be resolved
    Conflict_Detection_Requests = {}
    #The Maximum number of Pending Messages per failure
    Max_Pending_Queue_Size = 256
    #Endpoint Expiration Timer List. Endpoint which are "deleted" from a tunnel 
    #will be put on this timer list for a max of 2 iterations in the protocol 
    #timer to see if endpoint has vmotioned to another host. This will help us preserve
    #the vIPs of the Endpoint from the previous incarnation.
    Endpoint_Expiration_Timer_Queue = {}
    #Maximum number of endpoints in limbo at a given time
    Max_Endpoint_Timer_Queue_Length = 8192
    #The Mapping of Mass Transfer Query IDs to DPS Mass Transfer Objects
    #Key = Query ID, Value = DPSMassTransfer Object
    MassTransfer_QueryID_Mapping = {}
    #Number of Active Mass Transfers
    MassTransfer_Active = 0
    #Maximum of 10 Domain Mass Transfers per host
    MassTransfer_Max = 100
    #Mass transfer lock
    mass_transfer_lock = threading.Lock()
    #Mass transfer get ready list. Collection of Domains which are expecting
    #to be transferred over. The value represent the time when the Get Ready
    #was issued
    MassTransfer_Get_Ready = {}
    #A Global Counter and Lock for the runtime database
    QueryID = 1
    MaxQueryID = 2147483647
    global_lock = threading.Lock()
    #Collection of Query IDs and corresponding messages
    Query_ID_Hash = {}

    @classmethod
    def generate_query_id(cls):
        '''
        This routine generates a query id that the DPS Server can use
        @return: 
        '''
        ret_val = cls.QueryID
        cls.QueryID += 1
        if cls.QueryID >= cls.MaxQueryID:
            cls.QueryID = 1
        return ret_val

class DOVEStatus:
    '''
    This class contains the DOVE Status Error Codes
    '''
    DOVE_STATUS_OK = 0
    DOVE_STATUS_INVALID_FD = 1
    DOVE_STATUS_INVALID_PATH = 2
    DOVE_STATUS_NO_MEMORY = 3
    DOVE_STATUS_NO_RESOURCES = 4
    DOVE_STATUS_EMPTY = 5
    DOVE_STATUS_RETRY = 6
    DOVE_STATUS_INVALID_PARAMETER = 7
    DOVE_STATUS_BAD_ADDRESS = 8
    DOVE_STATUS_EXCEEDS_CAP = 9
    DOVE_STATUS_EXISTS = 10
    DOVE_STATUS_NOT_FOUND = 11
    DOVE_STATUS_NOT_SUPPORTED = 12
    DOVE_STATUS_INTERRUPT = 13
    DOVE_STATUS_BIND_FAILED = 14
    DOVE_STATUS_INVALID_DOMAIN = 15
    DOVE_STATUS_INVALID_DVG = 16
    DOVE_STATUS_INVALID_POLICY = 17
    DOVE_STATUS_THREAD_FAILED = 18
    DOVE_STATUS_OSIX_INIT_FAILED = 19
    DOVE_STATUS_INVALID_SERVICE = 20
    DOVE_STATUS_RESTC_INIT_FAILED = 21
    DOVE_STATUS_LOCAL_DOMAIN = 22
    DOVE_SERVICE_ADD_STATUS_FAIL = 23
    DOVE_SERVICE_TYPE_STATUS_INVALID = 24
    DOVE_STATUS_ERROR = 25
    DOVE_STATUS_BUSY = 26
    DOVE_STATUS_INACTIVE = 27
    DOVE_STATUS_UNKNOWN = 28

    ErrorToString = {DOVE_STATUS_OK: 'Success',
                     DOVE_STATUS_INVALID_FD: 'Invalid File Descriptor',
                     DOVE_STATUS_INVALID_PATH: 'Invalid Path',
                     DOVE_STATUS_NO_MEMORY: 'No Memory',
                     DOVE_STATUS_NO_RESOURCES: 'No Resources',
                     DOVE_STATUS_EMPTY: 'Nothing To Report',
                     DOVE_STATUS_RETRY: 'Retry',
                     DOVE_STATUS_INVALID_PARAMETER: 'Invalid Parameter',
                     DOVE_STATUS_BAD_ADDRESS: 'Bad Address',
                     DOVE_STATUS_EXCEEDS_CAP: 'Exceeds Current Capability',
                     DOVE_STATUS_EXISTS: 'Already Exists',
                     DOVE_STATUS_NOT_FOUND: 'Does not exist!',
                     DOVE_STATUS_NOT_SUPPORTED: 'Not Supported',
                     DOVE_STATUS_INTERRUPT: 'Interrupt',
                     DOVE_STATUS_BIND_FAILED: 'Socket Bind Failed',
                     DOVE_STATUS_INVALID_DOMAIN: 'Invalid Domain',
                     DOVE_STATUS_INVALID_DVG: 'Invalid DVG',
                     DOVE_STATUS_INVALID_POLICY: 'Invalid Policy',
                     DOVE_STATUS_THREAD_FAILED: 'Thread Creation Failed',
                     DOVE_STATUS_OSIX_INIT_FAILED: 'OSIX initialization Failed',
                     DOVE_STATUS_INVALID_SERVICE: 'Invalid Service',
                     DOVE_STATUS_RESTC_INIT_FAILED: 'REST Client Initialization Failed',
                     DOVE_STATUS_LOCAL_DOMAIN: 'Domain Handled by Local Node',
                     DOVE_SERVICE_ADD_STATUS_FAIL: 'Service Add Failed',
                     DOVE_SERVICE_TYPE_STATUS_INVALID: 'Service Type Status Invalid',
                     DOVE_STATUS_ERROR: 'Status Error',
                     DOVE_STATUS_BUSY: 'Busy',
                     DOVE_STATUS_INACTIVE: 'Inactive',
                     DOVE_STATUS_UNKNOWN: 'Status Unknown'
                     }

class IPSUBNETListOpcode:
    '''
    This class contains the IP subnet list operate codes
    '''
    #IP_SUBNET_LIST_OPCODE_CREATE = 1
    IP_SUBNET_LIST_OPCODE_DESTROY = 2
    IP_SUBNET_LIST_OPCODE_ADD_SUBNET = 3
    IP_SUBNET_LIST_OPCODE_DEL_SUBNET = 4
    IP_SUBNET_LIST_OPCODE_LOOKUP = 5
    IP_SUBNET_LIST_OPCODE_GET = 6
    IP_SUBNET_LIST_OPCODE_LIST = 7
    IP_SUBNET_LIST_OPCODE_FLUSH = 8
    IP_SUBNET_LIST_OPCODE_GETALLIDS = 9

class IPSUBNETMode:
    '''
    This class contains the IP subnet mode codes
    '''
    IP_SUBNET_MODE_DEDICATED = 0
    IP_SUBNET_MODE_SHARED = 1

class AssociatedType:
    '''
    This class contains the IP subnet associated type codes
    '''
    ASSOCIATED_TYPE_DOMAIN = 1
    ASSOCIATED_TYPE_VNID = 2

class IPSUBNETAssociatedType:
    '''
    This class contains the IP subnet associated type codes
    '''
    IP_SUBNET_ASSOCIATED_TYPE_DOMAIN = 1
    IP_SUBNET_ASSOCIATED_TYPE_VNID = 2

class DpsClientType:
    '''
    This class contains the DPS IDs of all components that participate in the
    DPS Client Server Protocol
    '''
    #typedef enum {
    #    DPS_POLICY_SERVER_ID = 1,
    #    DPS_SWITCH_AGENT_ID = 2,
    #    DOVE_VLAN_GATEWAY_AGENT_ID = 3,
    #    DOVE_EXTERNAL_GATEWAY_AGENT_ID = 4,
    #} dps_client_id;
    dps_server = 1
    dove_switch = 2
    vlan_gateway = 3
    external_gateway = 4
    controller = 5
    types = {dps_server: 'DPS Server', 
             dove_switch: 'DOVE Switch', 
             vlan_gateway: 'VLAN Gateway', 
             external_gateway: 'External Gateway',
             controller: 'DMC'}

class DpsTransactionType:
    '''
    This class contains the DPS Transaction Types.
    '''
    #typedef enum {
    #    DPS_TRANSACTION_NORMAL = 1,
    #    DPS_TRANSACTION_REPLICATION = 2,
    #    DPS_TRANSACTION_MASS_COPY = 3,
    #} dps_transaction_type;
    normal = 1
    replication = 2
    mass_transfer = 3
    types = {normal: 'Transaction Normal',
             replication: 'Transaction Replication',
             mass_transfer: 'Transaction Mass Copy'}

class DpsLogLevels:
    '''
    This class contains the DPS Log Levels
    '''
    EMERGENCY = 0
    ALERT = 1
    CRITICAL = 2
    ERROR = 3
    WARNING = 4
    NOTICE = 5
    INFO = 6
    VERBOSE = 7
