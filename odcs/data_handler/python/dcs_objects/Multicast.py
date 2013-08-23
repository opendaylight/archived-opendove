'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas
'''
import logging
from logging import getLogger
log = getLogger(__name__)

import struct
import socket

from dcs_objects.IPAddressLocation import IPAddressLocation
from object_collection import mac_bytes
from object_collection import DpsCollection
from object_collection import mac_show

class Multicast_Global_Scope:
    '''
    Represents a Global Scope address
    '''
    #Global Scope
    v4_Low = '224.0.1.0'
    v4_High = '238.255.255.255'
    v4_Low_Int = socket.ntohl(int(struct.unpack('I', socket.inet_pton(socket.AF_INET, v4_Low))[0]))
    v4_High_Int = socket.ntohl(int(struct.unpack('I', socket.inet_pton(socket.AF_INET, v4_High))[0]))

    fmts = {socket.AF_INET6: '16s', socket.AF_INET: 'I'}

    @staticmethod
    def IPAddress_Is(ip_value):
        '''
        This routine determines if an IP address is global scope
        '''
        if (type(ip_value) == type(1)):
            inet_type = socket.AF_INET
        else:
            inet_type = socket.AF_INET6
        try:
            #ip_value_packed = struct.pack(Multicast_Global_Scope.fmts[inet_type], ip_value)
            #ip_string = socket.inet_ntop(inet_type, ip_value_packed)
            if ((inet_type == socket.AF_INET) and 
                (socket.ntohl(ip_value) >= Multicast_Global_Scope.v4_Low_Int) and
                (socket.ntohl(ip_value) <= Multicast_Global_Scope.v4_High_Int)):
                #print 'IP Address %s is Global Scope\r'%ip_string
                return True
            else:
                #print 'IP Address %s is not Global Scope\r'%ip_string
                return False
        except Exception:
            return False

class Multicast_VNID_Group:
    '''
    Represents a MULTICAST Group Address per VNID (per Domain). Each
    Multicast DVG Group contains a set of tunnels that belongs to
    that Multicast Group in that DVG
    '''

    def __init__(self, dvg, global_scope, mac):
        '''
        @param dvg: The DVG ID
        @type dvg: DVG
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        '''
        self.dvg = dvg
        self.global_scope = global_scope
        self.mac = mac
        #List of Receiver Tunnels that don't have a Multicast IP - Hash Key is Tunnel
        self.receiver_all = {}
        #List of Tunnels that are receivers in a particular Multicast IP. Each
        #element in this set is a {} of Tunnels for that Multicast IP
        self.receiver_iphash = {}
        #List of Sender Tunnels that don't have a Multicast IP - Hash Key is Tunnel
        self.sender_all = {}
        #List of Tunnels that are sender in a particular Multicast IP. Each
        #element in this set is a {} of Tunnels for that Multicast IP
        self.sender_iphash = {}

    def receiver_add(self, inet_type, ip_value, tunnel):
        '''
        This routine adds a tunnel to the list of receivers in this VNID
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            try:
                iphash = self.receiver_iphash[ip_value]
            except Exception:
                self.receiver_iphash[ip_value] = {}
                iphash = self.receiver_iphash[ip_value]
            iphash[tunnel] = True
        else:
            self.receiver_all[tunnel] = True

    def receiver_delete(self, inet_type, ip_value, tunnel):
        '''
        This routine deletes a tunnel from the list of receivers in this VNID
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            try:
                iphash = self.receiver_iphash[ip_value]
                del iphash[tunnel]
                if len(iphash) == 0:
                    del self.receiver_iphash[ip_value]
            except Exception:
                pass
        else:
            try:
                del self.receiver_all[tunnel]
            except Exception:
                pass

    def sender_add(self, inet_type, ip_value, tunnel):
        '''
        This routine adds a tunnel to the list of receivers in this VNID
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            try:
                iphash = self.sender_iphash[ip_value]
            except Exception:
                self.sender_iphash[ip_value] = {}
                iphash = self.sender_iphash[ip_value]
            iphash[tunnel] = True
        else:
            self.sender_all[tunnel] = True

    def sender_delete(self, inet_type, ip_value, tunnel):
        '''
        This routine deletes a tunnel from the list of receivers in this VNID
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            try:
                iphash = self.sender_iphash[ip_value]
                del iphash[tunnel]
                if len(iphash) == 0:
                    del self.sender_iphash[ip_value]
            except Exception:
                pass
        else:
            try:
                del self.sender_all[tunnel]
            except Exception:
                pass

    def tunnel_delete(self, tunnel):
        '''
        This routine deletes a tunnel from it's collection. This routine
        should be called whenever a tunnel is deleted from the DVG.
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        for key in self.receiver_iphash.keys():
            try:
                iphash = self.receiver_iphash[key]
                del iphash[tunnel]
                if len(iphash) == 0:
                    del self.receiver_iphash[key]
            except Exception:
                pass
        try:
            del self.receiver_all[tunnel]
        except Exception:
            pass
        for key in self.sender_iphash.keys():
            try:
                iphash = self.sender_iphash[key]
                del iphash[tunnel]
                if len(iphash) == 0:
                    del self.sender_iphash[key]
            except Exception:
                pass
        try:
            del self.sender_all[tunnel]
        except Exception:
            pass

    def senders_get(self, inet_type, ip_val):
        '''
        This routine gets a list of sender tunnels corresponding in this VNID that
        match the IP Address. If the inet_type is 0, it signifies (*,G) i.e. all
        senders in all Multicast Addresses.
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @return: Dictionary of sender. Key is tunnel object
        '''
        sender_list = {}
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            #Add senders from the ip hash
            try:
                tunnels = self.sender_iphash[ip_val].keys()
                for tunnel in tunnels:
                    sender_list[tunnel] = True
            except Exception:
                pass
        else:
            #Includes sender from all ips
            for ip in self.sender_iphash.keys():
                try:
                    iphash = self.sender_iphash[ip]
                    tunnels = iphash.keys()
                    for tunnel in tunnels:
                        sender_list[tunnel] = True
                except Exception:
                    pass
        #Include the *G
        tunnels = self.sender_all.keys()
        for tunnel in tunnels:
            sender_list[tunnel] = True
        return sender_list

    def senders_global_scope_get(self):
        '''
        This routine gets list of all sender tunnels in this VNID that have sender
        whose IP falls into Global Scope
        '''
        sender_list = {}
        for ip in self.sender_iphash.keys():
            if not Multicast_Global_Scope.IPAddress_Is(ip):
                continue
            #ip is global scope
            try:
                iphash = self.sender_iphash[ip]
                tunnels = iphash.keys()
                for tunnel in tunnels:
                    sender_list[tunnel] = True
            except Exception:
                pass
        #Include the *G
        tunnels = self.sender_all.keys()
        for tunnel in tunnels:
            sender_list[tunnel] = True
        return sender_list

    def receivers_get(self, inet_type, ip_val):
        '''
        This routine gets a list of receiver tunnels corresponding in this VNID that
        match the IP Address. If the inet_type is 0, it signifies (*,G) i.e. all
        receivers in all Multicast Addresses.
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @return: dictionary
        '''
        receiver_list = {}
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            #Add senders from the ip hash
            try:
                tunnels = self.receiver_iphash[ip_val].keys()
                for tunnel in tunnels:
                    receiver_list[tunnel] = True
            except Exception:
                pass
        else:
            #Includes receiver from all ips
            for ip in self.receiver_iphash.keys():
                try:
                    iphash = self.sender_iphash[ip]
                    tunnels = iphash.keys()
                    for tunnel in tunnels:
                        receiver_list[tunnel] = True
                except Exception:
                    pass
        #Include the *G
        tunnels = self.receiver_all.keys()
        for tunnel in tunnels:
            receiver_list[tunnel] = True
        return receiver_list

    def ip_get(self):
        '''
        This routine returns all (inet_type, ip_values) in this VNID group
        including (0, 0)
        '''
        ip_list = [(0,0)]
        for ip_val in self.receiver_iphash.keys():
            if type(ip_val) == type(1): #Integer
                inet_type = socket.AF_INET
            else:
                inet_type = socket.AF_INET6
            ip_list.append((inet_type, ip_val))
        return ip_list

    def empty(self):
        '''
        Check if all lists are empty
        @return: True if empty
        @rtype: Boolean
        '''
        if len(self.receiver_all) + len(self.receiver_iphash) + len(self.sender_all) + len(self.sender_iphash) == 0:
            return True
        else:
            return False

    def delete(self):
        '''
        Delete self
        '''
        self.receiver_all.clear()
        for iphash in self.receiver_iphash.values():
            iphash.clear()
        self.receiver_iphash.clear()
        self.sender_all.clear()
        for iphash in self.sender_iphash.values():
            iphash.clear()
        self.sender_iphash.clear()

    def show(self):
        '''
        '''
        print '----------------------------------------------\r'
        if self.global_scope:
            print 'GLOBAL SCOPE, VNID %s\r'%self.dvg.unique_id
        else:
            print 'MULTICAST MAC %s, VNID %s\r'%(mac_show(self.mac), self.dvg.unique_id)
        if len(self.receiver_all) > 0:
            print 'MULTICAST Receivers (NO IP Address)\r'
            count = 1
            for tunnel in self.receiver_all.keys():
                print '    [%d] Tunnel %s\r'%(count, tunnel.primary_ip().show())
                count += 1
        if len(self.receiver_iphash) > 0:
            for iphash_key in self.receiver_iphash.keys():
                try:
                    if type(iphash_key) == type(1):
                        ip_address = IPAddressLocation(socket.AF_INET, iphash_key, 0)
                    else:
                        ip_address = IPAddressLocation(socket.AF_INET6, iphash_key, 0)
                except Exception:
                    continue
                print 'MULTICAST Receivers IP Address %s\r'%ip_address.show()
                iphash = self.receiver_iphash[iphash_key]
                count = 1;
                for tunnel in iphash.keys():
                    print '    [%d] Tunnel %s\r'%(count, tunnel.primary_ip().show())
                    count += 1
        if len(self.sender_all) > 0:
            print 'MULTICAST Senders (NO IP Address)\r'
            count = 1
            for tunnel in self.sender_all.keys():
                print '    [%d] Tunnel %s\r'%(count, tunnel.primary_ip().show())
                count += 1
        if len(self.sender_iphash) > 0:
            for iphash_key in self.sender_iphash.keys():
                try:
                    if type(iphash_key) == type(1):
                        ip_address = IPAddressLocation(socket.AF_INET, iphash_key, 0)
                    else:
                        ip_address = IPAddressLocation(socket.AF_INET6, iphash_key, 0)
                except Exception:
                    continue
                print 'MULTICAST Senders IP Address %s\r'%ip_address.show()
                iphash = self.sender_iphash[iphash_key]
                count = 1
                for tunnel in iphash.keys():
                    print '    [%d] Tunnel %s\r'%(count, tunnel.primary_ip().show())
                    count += 1
        print '----------------------------------------------\r'
        return

    def size(self):
        count = len(self.receiver_all.keys())
        for iphash_key in self.receiver_iphash.keys():
            try:
                iphash = self.receiver_iphash[iphash_key]
            except Exception:
                continue
            count += len(iphash.keys())
        count += len(self.sender_all.keys())
        for iphash_key in self.sender_iphash.keys():
            try:
                iphash = self.sender_iphash[iphash_key]
            except Exception:
                continue
            count += len(iphash.keys())
        return count

class Multicast_Group:
    '''
    Represents a MULTICAST group address. A Multicast Group is represented by
    a MAC Address.
    '''

    #The Key format to store the Sender's Identity
    sender_key_fmt = '%s:%s'

    def __init__(self, MAC):
        '''
        @param MAC: The MAC Address of the Multicast Group
        @type MAC: Byte Array
        '''
        self.mac = MAC
        #A Collection of VNIDs which have either Receivers or Senders in this
        #Multicast Group: Multicast_VNID_Group
        self.VNID_Hash = {}
        return

    def receiver_add(self, dvg, global_scope, inet_type, ip_value, tunnel):
        '''
        This routine adds a tunnel to the list of receivers in this Multicast Group
        @param dvg: The DVG Object
        @param dvg: DVG
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
        except Exception:
            self.VNID_Hash[dvg.unique_id] = Multicast_VNID_Group(dvg, global_scope, self.mac)
            vnid_group = self.VNID_Hash[dvg.unique_id]
        vnid_group.receiver_add(inet_type, ip_value, tunnel)
        return

    def receiver_delete(self, dvg, inet_type, ip_value, tunnel):
        '''
        This routine deletes a tunnel from the list of receivers in this Multicast Group
        @param dvg: The DVG Object
        @param dvg: DVG
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            vnid_group.receiver_delete(inet_type, ip_value, tunnel)
            if vnid_group.empty():
                del self.VNID_Hash[dvg.unique_id]
        except Exception:
            pass
        return

    def sender_add(self, dvg, inet_type, ip_value, tunnel):
        '''
        This routine adds a tunnel to the list of senders in this Multicast Group
        @param dvg: The DVG Object
        @param dvg: DVG
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
        except Exception:
            self.VNID_Hash[dvg.unique_id] = Multicast_VNID_Group(dvg, False, self.mac)
            vnid_group = self.VNID_Hash[dvg.unique_id]
        vnid_group.sender_add(inet_type, ip_value, tunnel)
        return

    def sender_delete(self, dvg, inet_type, ip_value, tunnel):
        '''
        This routine deletes a tunnel from the list of senders in this Multicast Group
        @param dvg: The DVG Object
        @param dvg: DVG
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            vnid_group.sender_delete(inet_type, ip_value, tunnel)
            if vnid_group.empty():
                del self.VNID_Hash[dvg.unique_id]
        except Exception:
            pass
        return

    def tunnel_delete(self, dvg, tunnel):
        '''
        This routine deletes a tunnel from it's collection. This routine
        should be called whenever a tunnel is deleted from the DVG.
        @param dvg: The DVG Object
        @param dvg: DVG
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            vnid_group.tunnel_delete(tunnel)
            if vnid_group.empty():
                del self.VNID_Hash[dvg.unique_id]
        except Exception:
            pass
        return

    def senders_get(self, dvg, inet_type, ip_val):
        '''
        This routine gets a list of sender tunnels corresponding in this VNID that
        match the IP Address. If the inet_type is 0, it signifies (*,G) i.e. all
        senders in all Multicast Adddresses.
        @param dvg: The DVG Object
        @type dvg: DVG
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @return: Dictionary of sender. Key is tunnel object
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            sender_set = vnid_group.senders_get(inet_type, ip_val)
        except Exception:
            sender_set = {}
        return sender_set

    def senders_global_scope_get(self, dvg):
        '''
        This routine gets a list of sender tunnels corresponding in this VNID that
        match the IP Address. If the inet_type is 0, it signifies (*,G) i.e. all
        senders in all Multicast Adddresses.
        @param dvg: The DVG Object
        @type dvg: DVG
        @return: Dictionary of sender. Key is tunnel object
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            sender_set = vnid_group.senders_global_scope_get()
        except Exception:
            sender_set = {}
        return sender_set

    def receivers_get(self, dvg, inet_type, ip_val):
        '''
        This routine gets a list of receiver tunnels corresponding in this VNID that
        match the IP Address. If the inet_type is 0, it signifies (*,G) i.e. all
        senders in all Multicast Adddresses.
        @param dvg: The DVG Object
        @type dvg: DVG
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @return: Dictionary of sender. Key is tunnel object
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            receiver_set = vnid_group.receivers_get(inet_type, ip_val)
        except Exception:
            receiver_set = {}
        return receiver_set

    def ip_get(self, dvg):
        '''
        This routine gets a list of [(inet_type, ip_value)] tuples for a DVG
        '''
        try:
            vnid_group = self.VNID_Hash[dvg.unique_id]
            ip_list = vnid_group.ip_get()
        except Exception:
            ip_list = []
        return ip_list

    def empty(self):
        '''
        Check if all lists are empty
        @return: True if empty
        @rtype: Boolean
        '''
        if len(self.VNID_Hash) == 0:
            return True
        else:
            return False

    def delete(self):
        '''
        Delete self
        '''
        for vnid_group in self.VNID_Hash.values():
            vnid_group.delete()
        self.VNID_Hash.clear()
        return

    def show(self):
        '''
        '''
        for vnid_group in self.VNID_Hash.values():
            vnid_group.show()
        return

    def size(self):
        '''
        '''
        count = 0
        for vnid_group in self.VNID_Hash.values():
            count += vnid_group.size()
        return count

    def show_vnid(self, vnid):
        '''
        Shows only Multicast Receivers and Senders in a particular VNID
        @param vnid: The VNID
        @type vnid: Integer
        '''
        for vnid_group in self.VNID_Hash.values():
            if vnid_group.dvg.unique_id == vnid:
                vnid_group.show()
        return

class Multicast:
    '''
    Represents the MULTICAST Handling for a Domain. This class contains
    the necessary routines and structure needed to handle MULTICAST
    RX and TX receiver lists for a Domain in the DOVE environment.
    '''

    #Represents the format for the IPv4 and IPv6 addresses
    #IPv6 is represented as a character string of 16 bytes
    #IPv4 is represented as an Integer
    fmts = {socket.AF_INET6: '16s',
            socket.AF_INET: 'I'}
    ipv4_mac_prefix_string = '01:00:5E'
    ipv4_mac_prefix = mac_bytes(ipv4_mac_prefix_string)
    ipv6_mac_prefix_string = '33:33'
    ipv6_mac_prefix = mac_bytes(ipv6_mac_prefix_string)

    def __init__(self, domain, domain_id):
        '''
        Constructor:
        @param domain_id: The Domain for this Multicast Structure
        @type domain_id: Domain
        @param domain_id: The Domain ID for this Multicast Structure
        @type domain_id: Integer
        '''
        self.domain = domain
        self.domain_id = domain_id
        #Global Scope contains receivers for Global Scope Multicast
        self.Global_Scope = Multicast_Group(DpsCollection.Multicast_All_MAC_Bytes)
        #All contains receivers for Multicast group in the domain.
        self.All = Multicast_Group(DpsCollection.Multicast_All_MAC_Bytes)
        #Set of Multicast Group MAC
        self.MAC_Group = {}

    @staticmethod
    def MAC_from_IP(inet_type, ip_value):
        '''
        This routine returns the Multicast MAC corresponding to the 
        Multicast IP.
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @raise exception: IP Value is not a Multicast address
        @return: ByteArray representing the MAC
        @rtype: ByteArray
        '''
        if inet_type == socket.AF_INET:
            mac_start = Multicast.ipv4_mac_prefix_string
            #Convert to host order
            hip = socket.ntohl(ip_value)
            #Validate upper 4 bits
            ip_multi3 = (hip >> 28) & 0xf
            if ip_multi3 != 14:
                #log.warning('Invalid IPv4 Address HostOrder 0x%x\r', hip)
                raise Exception('Invalid IPv4 Address')
            #Get lower 3 bytes
            ip_multi2 = (hip >> 16) & 0x7f
            ip_multi1 = (hip >> 8) & 0xff
            ip_multi0 = hip & 0xff
            #Convert to string
            ip_multi_string = ':%x:%x:%x'%(ip_multi2, ip_multi1, ip_multi0)
            mac = mac_start + ip_multi_string
            return mac_bytes(mac)
        elif inet_type == socket.AF_INET6:
            mac_start = Multicast.ipv6_mac_prefix_string
            mac = mac_start + ':%x:%x:%x:%x'%(ip_value[3],ip_value[2],ip_value[1],ip_value[0])
            return mac_bytes(mac)
        raise Exception('Not supported')

    @staticmethod
    def MAC_validate(mac):
        '''
        This routine validates the MAC address
        @param mac: The MAC Address
        @type mac: ByteArray
        @return: True if valid
        @rtype: Boolean
        '''
        if mac.startswith(Multicast.ipv4_mac_prefix):
            return True
        if mac.startswith(Multicast.ipv6_mac_prefix):
            return True
        return False

    @staticmethod
    def Validate(mac, inet_type, ip_value):
        '''
        This routine validates all the values and returns the correct
        values. In case the parameters cannot be validated this returns
        an exception
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        '''
        if inet_type == socket.AF_INET or inet_type == socket.AF_INET6:
            ret_mac = Multicast.MAC_from_IP(inet_type, ip_value)
        else:
            if not Multicast.MAC_validate(mac):
                raise Exception('Invalid MAC')
        return (ret_mac, inet_type, ip_value)

    def receiver_add(self, dvg, global_scope, all_multi, mac, inet_type, ip_value, tunnel):
        '''
        This routine adds a Tunnel to set of receivers in a VNID. This
        routine assumes that error checking has already been performed
        on the parameters
        @param dvg: The DVG Object
        @param dvg: DVG
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @param all_multi: If this is related to all multicast group
        @type all_multi: Boolean
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @raise exception: If parameters are invalid
        '''
        #log.warning('receiver_add: Enter mac %s\r', mac_show(mac))
        while True:
            if global_scope:
                self.Global_Scope.delete()
                self.Global_Scope.receiver_add(dvg, True, inet_type, ip_value, tunnel)
            elif all_multi:
                self.All.receiver_add(dvg, False, inet_type, ip_value, tunnel)
            else:
                try:
                    mac_group = self.MAC_Group[mac]
                except Exception:
                    mac_group = Multicast_Group(mac)
                    self.MAC_Group[mac] = mac_group
                mac_group.receiver_add(dvg, False, inet_type, ip_value, tunnel)
            if not self.domain.active:
                break
            if global_scope:
                #All VNIDs allowed
                try:
                    domain = dvg.domain
                    sender_vnids = domain.DVG_Hash.keys()
                except Exception:
                    sender_vnids = []
            else:
                #Determine all vnids which can send data to this vnid
                sender_vnids = dvg.policy_allowed_vnids(False, 1)
            for sender_vnid in sender_vnids:
                try:
                    dvg_send = dvg.domain.DVG_Hash[sender_vnid]
                except Exception:
                    continue
                #Insert to VNID_Multicast_Updates
                if global_scope or all_multi:
                    #Need to get all Senders MACs
                    for mac_group in self.MAC_Group.values():
                        key = '%s:%s:%s:%s'%(dvg_send.unique_id, mac_group.mac, ip_value, global_scope)
                        DpsCollection.VNID_Multicast_Updates[key] = (dvg_send, mac_group.mac, inet_type, ip_value, global_scope)
                else:
                    key = '%s:%s:%s:%s'%(dvg_send.unique_id,mac, ip_value, global_scope)
                    DpsCollection.VNID_Multicast_Updates[key] = (dvg_send, mac, inet_type, ip_value, global_scope)
            break
        return

    def receiver_delete(self, dvg, global_scope, all_multi, mac, inet_type, ip_value, tunnel):
        '''
        This routine deletes a Tunnel from set of receivers in a VNID. This
        routine assumes that error checking has already been performed
        on the parameters
        @param dvg: The DVG Object
        @param dvg: DVG
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Integer
        @param all_multi: If this is related to all multicast group
        @type all_multi: Boolean
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        @raise exception: If parameters are invalid
        '''
        #log.warning('receiver_delete: Enter mac %s\r', mac_show(mac))
        fdeleted = False
        while True:
            if all_multi:
                self.All.receiver_delete(dvg, inet_type, ip_value, tunnel)
                fdeleted = True
            else:
                try:
                    mac_group = self.MAC_Group[mac]
                    mac_group.receiver_delete(dvg, inet_type, ip_value, tunnel)
                    fdeleted = True
                    if mac_group.empty():
                        del self.MAC_Group[mac]
                except Exception:
                    pass
            if not fdeleted:
                break
            if not self.domain.active:
                break
            #Determine all vnids which can send data to this vnid
            sender_vnids = dvg.policy_allowed_vnids(False, 1)
            for sender_vnid in sender_vnids:
                try:
                    dvg_send = dvg.domain.DVG_Hash[sender_vnid]
                except Exception:
                    continue
                #Insert to VNID_Multicast_Updates
                if all_multi:
                    #Need to get all Senders MACs
                    for mac_group in self.MAC_Group.values():
                        key = '%s:%s:%s:%s'%(dvg_send.unique_id, mac_group.mac, ip_value, global_scope)
                        DpsCollection.VNID_Multicast_Updates[key] = (dvg_send, mac_group.mac, inet_type, ip_value, global_scope)
                else:
                    key = '%s:%s:%s:%s'%(dvg_send.unique_id,mac,ip_value, global_scope)
                    DpsCollection.VNID_Multicast_Updates[key] = (dvg_send, mac, inet_type, ip_value, global_scope)
            break
        #log.warning('receiver_delete: Leave\r')
        return

    def sender_add(self, dvg, mac, inet_type, ip_value, tunnel):
        '''
        This routine adds a Tunnel to set of senders in a VNID. This
        routine assumes that error checking has already been performed
        on the parameters
        @param dvg: The DVG Object
        @param dvg: DVG
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        @raise exception: If parameters are invalid
        '''
        try:
            mac_group = self.MAC_Group[mac]
        except Exception:
            mac_group = Multicast_Group(mac)
            self.MAC_Group[mac] = mac_group
        mac_group.sender_add(dvg, inet_type, ip_value, tunnel)
        if Multicast_Global_Scope.IPAddress_Is(ip_value):
            global_scope = True
        else:
            global_scope = False
        #Send list of receivers to this tunnel
        receiver_tunnel_set = self.get_receiver_list(dvg, mac_group, inet_type, ip_value, global_scope)
        dps_client = tunnel.dps_client
        try:
            ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],ip_value)
        except Exception:
            ip_packed = ''
        dvg.send_multicast_update_to(dps_client,
                                     receiver_tunnel_set, 
                                     mac, inet_type, ip_packed, global_scope)
        return

    def sender_delete(self, dvg, mac, inet_type, ip_value, tunnel):
        '''
        This routine adds a Tunnel to set of receivers in a VNID. This
        routine assumes that error checking has already been performed
        on the parameters
        @param dvg: The DVG Object
        @param dvg: DVG
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        @raise exception: If parameters are invalid
        '''
        #log.warning('sender_delete: Enter mac %s\r', mac_show(mac))
        try:
            mac_group = self.MAC_Group[mac]
            mac_group.sender_delete(dvg, inet_type, ip_value, tunnel)
            if mac_group.empty():
                del self.MAC_Group[mac]
        except Exception:
            pass
        #log.warning('sender_delete: Exit\r')
        return

    def tunnel_delete(self, dvg, tunnel):
        '''
        This routine MUST be called whenever a tunnel is removed from a dvg
        @param dvg: The DVG Object
        @param dvg: DVG
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        '''
        self.All.tunnel_delete(dvg, tunnel)
        for mac_group in self.MAC_Group.values():
            mac_group.tunnel_delete(dvg, tunnel)

    def get_receiver_list(self, dvg, mac_group, inet_type, ip_value, global_scope):
        '''
        This routine gets the list of receivers which has receive multicast traffic
        from a dvg a location (mac,ip)
        @attention: This routine must be called with global lock held.
        @param dvg: The DVG Object
        @param dvg: DVG
        @param mac_group: The Multicast MAC Group
        @type mac_group: Multicast_Group
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @return: Dictionary of Receiver Tunnels keyed by receiver vnid
        @rtype: {}
        '''
        #log.warning('get_receiver_list: Enter\r')
        receiver_tunnel_set = {}
        if global_scope:
            try:
                receiver_vnids = dvg.domain.DVG_Hash.keys()
            except Exception:
                receiver_vnids = []
        else:
            receiver_vnids = dvg.policy_allowed_vnids(True, 1)
        #log.warning('get_receiver_list: receiver_vnids %s\r', receiver_vnids)
        for receiver_vnid in receiver_vnids:
            try:
                dvg_recv = dvg.domain.DVG_Hash[receiver_vnid]
            except Exception:
                continue
            #Get list of receivers in All Multi
            tunnels_all = self.All.receivers_get(dvg_recv, inet_type, ip_value)
            if global_scope:
                tunnels_global_scope = self.Global_Scope.receivers_get(dvg_recv, 0, 0)
            else:
                tunnels_global_scope = {}
            #log.warning('get_receiver_list: VNID %s, tunnels_all %s\r', receiver_vnid, tunnels_all)
            #Get list of receivers in Specific MAC Group
            tunnels_specific = mac_group.receivers_get(dvg_recv, inet_type, ip_value)
            #log.warning('get_receiver_list: VNID %s, tunnels_specific %s\r', receiver_vnid, tunnels_specific)
            #Get list of receiver in mac group
            #Find the union of sets
            tunnelsv4 = {}
            tunnelsv6 = {}
            for tunnel in tunnels_global_scope.keys():
                primary_ip = tunnel.primary_ip()
                if primary_ip.inet_type == socket.AF_INET:
                    tunnelsv4[primary_ip.ip_value] = True
                else:
                    tunnelsv6[primary_ip.ip_value] = True
            for tunnel in tunnels_all.keys():
                primary_ip = tunnel.primary_ip()
                if primary_ip.inet_type == socket.AF_INET:
                    tunnelsv4[primary_ip.ip_value] = True
                else:
                    tunnelsv6[primary_ip.ip_value] = True
            for tunnel in tunnels_specific.keys():
                primary_ip = tunnel.primary_ip()
                if primary_ip.inet_type == socket.AF_INET:
                    tunnelsv4[primary_ip.ip_value] = True
                else:
                    tunnelsv6[primary_ip.ip_value] = True
            #Store in receiver tunnel set
            receiver_tunnel_set[receiver_vnid] = (tunnelsv4.keys(), tunnelsv6.keys())
            #log.warning('get_receiver_list: vnid %s, receiver_tunnel_set %s\r',
            #            receiver_vnid, receiver_tunnel_set)
        #log.warning('get_receiver_list: Exit vnids %s\r', receiver_tunnel_set.keys())
        return receiver_tunnel_set

    def sender_update_vnid(self, dvg, mac, inet_type, ip_value, global_scope):
        '''
        This routine sends updates containing list of receivers to every sender
        matching the parameters
        @attention: This routine must be called with global lock held.
        @param dvg: The DVG Object
        @param dvg: DVG
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @raise exception: If parameters are invalid
        '''
        #Determine the List of VNIDs who can receive data from the sender dvg
        try:
            mac_group = self.MAC_Group[mac]
        except Exception:
            return
        receiver_tunnel_set = self.get_receiver_list(dvg, mac_group, inet_type, ip_value, global_scope)
        #Determine the List of senders corresponding to these parameter
        if global_scope:
            tunnels = mac_group.senders_global_scope_get(dvg)
        else:
            tunnels = mac_group.senders_get(dvg, inet_type, ip_value)
        #log.warning('sender_update_vnid: Sender Tunnels tunnels %s\r', tunnels)
        sender_dps_clients = {}
        for tunnel in tunnels.keys():
            sender_dps_clients[tunnel.dps_client] = True
        dps_client_list = sender_dps_clients.keys()
        try:
            ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],ip_value)
        except Exception:
            ip_packed = struct.pack(IPAddressLocation.fmts[socket.AF_INET],0)
        dvg.send_multicast_update(dps_client_list, receiver_tunnel_set, mac, inet_type, ip_packed, global_scope)
        return

    def sender_update_vnid_to(self, dvg, mac, inet_type, ip_value, dps_client, global_scope):
        '''
        This routine sends multicast update to a particular dps_client sender
        @attention: This routine must be called with global lock held.
        @param dvg: The DVG Object
        @param dvg: DVG
        @param mac: The MAC Address
        @type mac: ByteArray
        @param inet_type: 0, AF_INET, AF_INET6. 0 denotes unknown
        @type inet_type: Integer
        @param ip_value: IP Address
        @type ip_value: Integer or String
        @param tunnel: TunnelEndpoint
        @type tunnel: TunnelEndpoint
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        @raise exception: If parameters are invalid
        '''
        #Determine the List of VNIDs who can receive data from the sender dvg
        try:
            mac_group = self.MAC_Group[mac]
        except Exception:
            return
        receiver_tunnel_set = self.get_receiver_list(dvg, mac_group, inet_type, ip_value, global_scope)
        try:
            ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],ip_value)
        except Exception:
            ip_packed = struct.pack(IPAddressLocation.fmts[socket.AF_INET],0)
        dvg.send_multicast_update_to(dps_client, receiver_tunnel_set, mac, inet_type, ip_packed, global_scope)
        return

    def policy_update_mac(self, mac_group, src_dvg, global_scope):
        '''
        This routine handles sending updates to senders on in a specific
        MAC Group when a policy change occurs on a Source DVG
        @param mac_group: The MAC Group
        @type mac_group: Multicast_Group
        @param src_dvg: The Source DVG
        @type src_dvg: DVG
        @param global_scope: Whether the Receiver is registering as Global Scope
        @type global_scope: Boolean
        '''
        #Get all (inet, ip) pairs in mac_group for the Source DVG
        ip_list = mac_group.ip_get(src_dvg)
        for ip_tuple in ip_list:
            inet_type = ip_tuple[0]
            ip_value = ip_tuple[1]
            if global_scope:
                sender_set = mac_group.senders_global_scope_get(src_dvg)
            else:
                sender_set = mac_group.senders_get(src_dvg, inet_type, ip_value)
            #Send list of receivers to this tunnel
            receiver_tunnel_set = self.get_receiver_list(src_dvg, mac_group, inet_type, ip_value, global_scope)
            for tunnel in sender_set.keys():
                dps_client = tunnel.dps_client
                try:
                    ip_packed = struct.pack(IPAddressLocation.fmts[inet_type],ip_value)
                except Exception:
                    ip_packed = ''
                src_dvg.send_multicast_update_to(dps_client,
                                                 receiver_tunnel_set, 
                                                 mac_group.mac, inet_type, ip_packed, global_scope)
        return

    def policy_update(self, src_dvg):
        '''
        This routine handles sending updates to senders on in a specific
        MAC Group when a policy change occurs on a Source DVG
        @param src_dvg: The Source DVG
        @type src_dvg: DVG
        '''
        self.policy_update_mac(self.Global_Scope, src_dvg, True)
        self.policy_update_mac(self.All, src_dvg, False)
        for mac_group in self.MAC_Group.values():
            self.policy_update_mac(mac_group, src_dvg, False)
        return

    def delete(self):
        '''
        '''
        self.All.delete()
        self.All = None
        for mac_group in self.MAC_Group.values():
            mac_group.delete()
        self.MAC_Group.clear()
        return

    def size(self):
        '''
        Determines the size of 
        '''
        count = 0
        count += self.Global_Scope.size()
        count += self.All.size()
        for mac_group in self.MAC_Group.values():
            count += mac_group.size()
        return count

    def show(self):
        '''
        '''
        print '------- MULTICAST DETAILS for Domain %s -------\r'%(self.domain_id)
        self.Global_Scope.show()
        self.All.show()
        for mac_group in self.MAC_Group.values():
            mac_group.show()
        #############################################################
        #Show Policies.
        #############################################################
        domain = self.domain
        count = len(domain.Policy_Hash_DVG[0])
        if count > 0:
            print '----------- MULTICAST Policies -------------------\r'
            for policy in domain.Policy_Hash_DVG[1].values():
                if policy.src_dvg == policy.dst_dvg and policy.action_connectivity == policy.action_forward:
                    continue
                print '%s\r'%policy.show()
            #print '-----------------------------------------------------------\r'
        else:
            print 'Policy Count %s\r'%count
        return

    def show_vnid(self, vnid):
        '''
        Shows only Multicast Receivers and Senders in a particular VNID
        @param vnid: The VNID
        @type vnid: Integer
        '''
        print '------- MULTICAST DETAILS for VNID %s -------\r'%(vnid)
        self.All.show_vnid(vnid)
        for mac_group in self.MAC_Group.values():
            mac_group.show_vnid(vnid)
        #############################################################
        #Show Policies.
        #############################################################
        domain = self.domain
        count = len(domain.Policy_Hash_DVG[0])
        if count > 0:
            print '----------- MULTICAST Policies -------------------\r'
            for policy in domain.Policy_Hash_DVG[1].values():
                if policy.src_dvg.unique_id != vnid and policy.dst_dvg.unique_id != vnid:
                    continue
                if policy.src_dvg == policy.dst_dvg and policy.action_connectivity == policy.action_forward:
                    continue
                print '%s\r'%policy.show()
            #print '-----------------------------------------------------------\r'
        else:
            print 'Policy Count %s\r'%count
        return
