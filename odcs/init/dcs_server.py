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
        print "Usage: ./dps_server.py <udp port> <REST port> <CLI?>\r"
        print "udp port range: Default 12345\r"
        print "REST service port: Default 1888\r"
        print "CLI (Y/N): Default Y \r"
        print "Exit on Ctrl+C (Y/N): Default Y\r"
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
        #Start CLI?
        try:
            DebugCli = sys.argv[3]
        except Exception:
            DebugCli = 'Y'
        if str(DebugCli).lower() in ['yes', 'y']:
            fDebugCli = 1
        else:
            fDebugCli = 0
        #Exit on CTRL+C?
        try:
            ExitOnCtrlC = sys.argv[4]
        except Exception:
            ExitOnCtrlC = 'Y'
        if str(ExitOnCtrlC).lower() in ['no', 'n']:
            fExitOnCtrlC = 0
            signal.signal(signal.SIGINT, handler_no_quit)
            signal.signal(signal.SIGABRT, handler_no_quit)
            signal.signal(signal.SIGTSTP, handler_no_quit)
            #signal.signal(signal.SIG_DFL, handler_no_quit)
            signal.signal(signal.SIG_IGN, handler_no_quit)
            signal.signal(signal.SIGFPE, handler_no_quit)
            signal.signal(signal.SIGILL, handler_no_quit)
            signal.signal(signal.SIGQUIT, handler_no_quit)
            #signal.signal(signal.SIGSEGV, handler_no_quit)
            signal.signal(signal.SIGTERM, handler_no_quit)
        else:
            fExitOnCtrlC = 1
        if len(sys.argv) == 6:
            dcslib.initialize(udp_port, rest_port, fDebugCli, fExitOnCtrlC, sys.argv[5])
        else:
            dcslib.initialize(udp_port, rest_port, fDebugCli, fExitOnCtrlC)
    except Exception, ex:
        log.warning('Exception %s', ex)
        print "Usage: ./dps_server.py <udp port> <REST port> <CLI (Y/N)> <exit on ctrl+c (Y/N)>\r"
        sys.exit()
