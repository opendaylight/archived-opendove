'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: abiswas and jinghe
'''

import struct

class EUIDv0:
    '''
    Represents the EUID (Version 0) of an Endpoint in the DOVE environment
    '''

    def __init__(self, domain_id, mac):
        '''
        Constructor:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param mac: The MAC Address
        @type mac: Byte Array
        '''
        self.domain = domain_id
        self.mac = mac
        #Represents the Key that can be used to insert into a hash table
        self.key = struct.pack('I6s', domain_id, mac)

    def show_mac(self):
        '''
        Shows the MAC
        '''
        return '%x:%x:%x:%x:%x:%x'%(ord(self.mac[0]), ord(self.mac[1]), ord(self.mac[2]),
                                    ord(self.mac[3]), ord(self.mac[4]), ord(self.mac[5]))

    def show(self):
        '''
        Returns a string of the form '(Domain, MAC)'
        '''
        return '(%s, %x:%x:%x:%x:%x:%x)'%(self.domain,
                                          ord(self.mac[0]), ord(self.mac[1]), ord(self.mac[2]),
                                          ord(self.mac[3]), ord(self.mac[4]), ord(self.mac[5]))
