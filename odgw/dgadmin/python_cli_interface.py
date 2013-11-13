'''
Created on Feb 21, 2012

@author: abiswas
'''
import logging
from logging import getLogger
log = getLogger(__name__)

import threading
import time
import os
import threading
import sys
from threading import Thread
import cli_class_main
from cli_class_main import cli_class_main

class python_cli_interface(object):

    #The singleton instance - Save in the class
    #TODO: Not elegant - Change
    instance = None

    def __init__(self, exit_on_ctrl_c):
        '''
        Initializes an object of Python Interface
        '''
        if exit_on_ctrl_c:
            f_ctrl_c = True
        else:
            f_ctrl_c = False
        self.dps_server_cli_instance = cli_class_main(f_ctrl_c)
        python_cli_interface.instance = self

    def cli_start1(self):
        '''
        This function starts the DPS SERVER CLI
        @param exit_on_ctrl_c: 'True' or 'False'
        @type exit_on_ctrl_c: String
        '''
        self.dps_server_cli_instance.handle_user_input()
        return 0

    def cli_start(self):
        '''
        This function starts the DPS SERVER CLI
        @param exit_on_ctrl_c: 'True' or 'False'
        @type exit_on_ctrl_c: String
        '''
        self.thread = self.cli_thread(self.dps_server_cli_instance)
        self.thread.start()
        return 0

    class cli_thread(Thread):
        """
        """
        def __init__(self, dps_server_cli_instance):
            """
            """
            Thread.__init__(self)
            self.dps_server_cli_instance = dps_server_cli_instance
            self.setDaemon(True)

        def run(self):
            '''
            '''
            self.dps_server_cli_instance.handle_user_input()

