#! /usr/bin/python
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

logging.basicConfig(level=logging.WARNING)

import sys
import threading
import signal
import time
import os
from threading import Lock
from threading import Thread

#This is the DPS Library Name
import dcslib

def handler_no_quit(signum, frame):
    '''
    Generic signal handler
    '''
    print "Interrupt by user. Not Exiting..."
    return

if __name__ == '__main__':
    if len(sys.argv) > 6 or len(sys.argv) < 2:
        print "Usage: ./dps_server.py <udp port> <REST port>\r"
        print "udp port: Range [12340-12349], Default 12345\r"
        print "REST service port: Default 1888\r"
        sys.exit()
    try:
        #UDP Port
        try:
            udp_port = int(sys.argv[1])
        except Exception:
            udp_port = 12345
        #REST Services Port
        try:
            rest_port = int(sys.argv[2])
        except Exception:
            rest_port = 1888
        if len(sys.argv) == 3:
            dcslib.initialize(udp_port, rest_port)
        else:
            raise Exception('Too many parameters')
    except Exception, ex:
        log.warning('Exception %s', ex)
        print "Usage: ./dps_server.py <udp port [range: 12340-12349]> <rest port>\r"
        sys.exit()

