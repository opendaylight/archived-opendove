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

import sys
import time
import struct, os
import socket
import string
import time
import common_cli
import gateway_cli
from common_cli.terminal import MyTerminal
from common_cli import cli_base
from gateway_cli.main_menu import login_context
from gateway_cli.main_menu import cli_login
#Import CLI for CLI Base
from gateway_cli.main_menu import cli_main_menu
#Import CLI for Service
from gateway_cli.service import cli_service
##Import CLI for CLI Client Server Protocol
#from gateway_cli.client_server_protocol import cli_client_server

class cli_class_main:
    '''
    Represents a DOVE GATEWAY Class
    '''

    def __init__(self, exit_on_ctrl_c):
        '''
        Initializes 
        '''
        self.session = login_context()
        self.exit_on_ctrl_c = exit_on_ctrl_c
        self.terminal = MyTerminal(exit_on_ctrl_c)

    def execute_user_input(self, cli_contexts, current_context, user_command):
        '''
        Executes command entered by the user
        @param cli_contexts: The stack of cli_contexts
        @param cli_contexts: List
        @param current_context: The Current CLI Context
        @type current_context: Python Class
        @param user_command: The entire user command
        @param user_command: String
        @rtype: The new CLI context if any, otherwise the same context
        '''
        return_context = current_context
        while 1:
            user_commands = user_command.split()
            if user_commands[0] == '':
                #print cli_base.user_help_output
                break
            command_id = user_commands[0].lower()
            #Don't allow EXIT OR HELP from login CLI
            if current_context != cli_login:
                #Now Process
                current_context.command_history.add_string(user_command)
                if command_id in current_context.help_strings:
                    current_context.help(self.session.god_mode)
                    break
                elif command_id in cli_base.cli_generic_exit.exit_strings:
                    #Exiting current context
                    try:
                        current_context.cli_exit(self.session)
                    finally:
                        try:
                            return_context = cli_contexts.pop()
                        except Exception:
                            self.terminal.release()
                            print 'ALERT!!! CLI Error: PLEASE REPORT TO DOVE-DEV Exiting...'
                            os._exit(1)
                        break
            #Parse the Command
            cli_context, cli_object = current_context.parse(self.session.god_mode,
                                                            self.terminal,
                                                            user_command)
            if ((cli_context is not None) and (cli_context != current_context)):
                #CLI context changed
                cli_contexts.append(current_context)
                return_context = cli_context
                break
            if cli_context == current_context:
                #Nothing to execute
                break
            if type(cli_object) != cli_login and type(cli_object) == current_context:
                #Other than the Login Context - If the type of the cli_object is
                #the same as the current context it means there is nothing to execute
                cli_object = None
            if not cli_object:
                print "Invalid Command! Please try again..."
                break
            self.terminal.release()
            try:
                new_context = cli_object.execute(self.session)
                if new_context is not None:
                    cli_contexts.append(current_context)
                    return_context = new_context
            except Exception, ex:
                print 'ERROR EXECUTE: %s'%ex
            self.terminal.acquire()
            break
        return return_context

    def get_input_from_terminal(self, output_command, command_history,
                                command_chars, help_chars, fshow_chars):
        '''
        This routine gets the user input from the terminal. The user input is
        in the form of a List of Characters.
        @param output_command: The String to show on the terminal
        @type output_command: String
        @param command_history: A history of commands to use for this command
        @type command_history: List of commands
        @param command_chars: Existing command characters
        @param command_chars: List
        @param help_chars: A list of characters that represents user needs help
                           The command is not considered complete at this point
        @type help_chars: List
        @param show_chars: Whether to show the characters on the screen
        @type show_chars: Boolean
        '''
        sys.stdout.write(output_command)
        terminal_chars, finished = self.terminal.get_user_input(command_history,
                                                                command_chars,
                                                                help_chars,
                                                                fshow_chars)
        return (terminal_chars, finished)

    def handle_user_input(self):
        '''
        Handles commands entered by user
        '''
        try:
            command_chars = []
            cli_contexts = []
            cli_context_current = cli_login
            while(True):
                try:
                    input_string = '%s'%(cli_context_current.user_input_string_raw)
                    self.terminal.acquire()
                    terminal_chars, finished = self.get_input_from_terminal(input_string, 
                                                                            cli_context_current.command_history,
                                                                            command_chars, 
                                                                            cli_context_current.help_chars,
                                                                            cli_context_current.input_char_show)
                    self.terminal.release()
                    user_command = ''.join(terminal_chars)
                    print ''
                    if finished:
                        if user_command == '':
                            command_chars = []
                            continue
                        #User Finished
                        self.terminal.acquire()
                        cli_context_current = self.execute_user_input(cli_contexts,
                                                                      cli_context_current,
                                                                      user_command)
                        self.terminal.release()
                        command_chars = []
                    else:
                        #User entered TAB i.e. wants to know next commands
                        command_chars = cli_context_current.complete_user_input(self.session.god_mode,
                                                                                user_command)
                except Exception, ex:
                    #log.warning('ERROR: Exception in handle_user_input [%s]\n'%ex)
                    continue
        except Exception, ex:
            log.warning('handle_user_input: Exception [%s]', ex)
        finally:
            log.warning('handle_user_input: Exiting...')
            self.terminal.release()
