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

logging.basicConfig(level=logging.INFO)

import sys
import signal
import os

#This is the DOVE GATEWAY LIBRARY NAME
import dgadmin

def handler_no_quit(signum, frame):
    '''
    Generic signal handler
    '''
    print "Interrupt by user. Not Exiting..."
    return

if __name__ == '__main__':

    if len(sys.argv) > 3 or len(sys.argv) < 1:
        print "Usage: ./dove_gateway.py <exit on Ctrl+C (Y/N)> <Debug CLI (Y/N)"
        sys.exit()
    try:
        try:
            ExitOnCtrlC = sys.argv[1]
        except Exception:
            ExitOnCtrlC = 'Yes'
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
        try:
            DebugCli = sys.argv[2]
        except Exception:
            DebugCli = 'Y'
        if str(DebugCli).lower() in ['yes', 'y']:
            fDebugCli = 1
        else:
            fDebugCli = 0
        dgadmin.initialize(fExitOnCtrlC, fDebugCli)
    except Exception, ex:
        log.warning('Exception %s', ex)
        print "Usage: ./dove_gateway.py <exit on ctrl+c (True/False)>"
        sys.exit()

