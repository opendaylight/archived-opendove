'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: trevor tao
'''

import struct
import uuid

class UUIDv0:
    '''
    Represents the UUID (Version 0) of a DPS node in the DOVE environment
    '''
    def __init__(self):
        '''
        Constructor:
        @param domain_id: The Domain ID
        @type domain_id: Integer
        @param mac: The MAC Address
        @type mac: Byte Array
        '''
#        self.domain = domain_id
#        self.mac = mac
 
        self.node_uuid = uuid.uuid1()
#        print "Current DPS node uuid is %s" %(self.node_uuid)


        #Represents the Key that can be used to insert into a hash table
#        self.key = struct.pack('I6s', domain_id, mac)

    def get_uuid(self):
        '''
        Get UUID
        '''
        node_uuid1 = ''
        node_uuid1 = uuid.uuid1()
#        print node_uuid1
#        return self.node_uuid
#        return node_uuid1
#        return '%s'%(node_uuid1)
        return '%s'%(self.node_uuid)


    def get_uuid2(self):
        '''
        Get UUID2
        '''
        return '%s'%(self.node_uuid)
