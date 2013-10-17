'''
Created on 28 DEC, 2012

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
        self.node_uuid = uuid.uuid1()

    def get_uuid(self):
        '''
        Get UUID
        '''
        node_uuid1 = ''
        node_uuid1 = uuid.uuid1()
        return '%s'%(self.node_uuid)

    def get_uuid2(self):
        '''
        Get UUID2
        '''
        return '%s'%(self.node_uuid)
