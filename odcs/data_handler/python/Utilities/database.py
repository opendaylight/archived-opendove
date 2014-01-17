'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: amitabha.biswas
'''

import sqlite3

import threading

from logging import getLogger
log = getLogger(__name__)

class Database(object):
    '''
    Represents the base class for a 
    '''

    def __init__(self, params):
        '''
        Constructor
        '''
        
