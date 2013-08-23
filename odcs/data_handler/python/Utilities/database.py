'''
Created on Jun 18, 2012

@author: abiswas
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
        