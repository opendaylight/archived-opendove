'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: Amitabha Biswas
'''

import sys
import time
import socket

import struct
from struct import *
import socket

class dcs_object(object):
    '''
    This is the base class for DPS Objects. All DPS Objects must use this
    Class as it's base class.
    '''

    #Every DPS Object Class MUST have an unique name
    type_name = 'DPSObject'
    #Set of all the DPS object types, index by Class Name
    type_set = {}

    @classmethod
    def add_class(cls):
        """
        Adds a (sub)class of dcs_object. This 
        """
        cls.type_set[cls.type_name] = cls

    def __init__(self, unique_id):
        '''
        Constructor:
        Initializes the base part of a DPS Object. This routine must
        be called for every object that is initialized.
        @param unique_id: The Unique Identifier of the Object.
        @type unique_id: String or Integer
        '''
        #Uniquely identifies the Object in the DOVE environment.
        #The individual Object Class describes what the Unique ID actually 
        #represents
        self.unique_id = unique_id
        #The Number of times an object has been looked up by
        #the DPS infrastructure - Useful for debugging and statistics
        self.version = 0
        #Valid. Mark as invalid before deletion
        self.valid = True
