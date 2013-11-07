'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

@author: amitabha.biswas
'''

import logging
from logging import getLogger
log = getLogger(__name__)

import sys
import tty
import termios
import os
import string
import fcntl

class CommandHistory(object):
    '''
    Represents a List of Commands
    '''
    def __init__(self):
        self.list = []
        self.head = 0 #Points to the nth element in the list, there is no 0th element i.e it starts from 1
        self.max = 32
        self.unexecuted_command = None

    def add(self, command):
        '''
        Adds a command to the list. This should be called after the command
        has been executed
        @param command: The new command to be added to the list
        @type command: An element in the list
        @return: None
        '''
        fAdd = True
        self.head = len(self.list)
        if self.head > 0:
            last_command = self.list[self.head-1]
            last_sentence = ''.join(last_command)
            curr_sentence = ''.join(command)
            if last_sentence == curr_sentence:
                fAdd = False
        if fAdd:
            self.list.append(command)
            if len(self.list) > self.max:
                self.list = self.list[1:]
            self.head = len(self.list)
        self.unexecuted_command = None
        return

    def add_string(self, command_string):
        '''
        Adds a command string to the list. This routine will break up
        the command string into a list of characters
        @param command_string: User command
        @param command_string: String
        '''
        command = []
        for i in range(len(command_string)):
            command.append(command_string[i])
        self.add(command)
        return

    def previous(self, command):
        '''
        Returns the previous command from the current history of commands
        @param command: The current command
        @type command: An element in the list
        @return: The previous command
        @rtype: An element in the list
        '''
        if self.head == 0:
            sys.stdout.write(chr(7)) #BELL
            return command
        if self.unexecuted_command is None:
            self.unexecuted_command = command
        try:
            ret_command = self.list[self.head - 1][:]
            self.head = self.head - 1
            if self.head < 0:
                log.error('CommandHistory.previous: Error in index %d', self.index)
        except Exception:
            ret_command = command
        return ret_command

    def next(self, command):
        '''
        Returns the next command from the current history of commands
        @param command: The current command
        @type command: An element in the list
        '''
        if self.head == len(self.list):
            sys.stdout.write(chr(7)) #BELL
            return command
        try:
            self.head = self.head + 1
            if self.head == len(self.list):
                ret_command = self.unexecuted_command
                self.unexecuted_command = None
            else:
                ret_command = self.list[self.head][:]
        except Exception:
            ret_command = command
        if ret_command is None:
            ret_command = command
        return ret_command

    def show(self):
        '''
        Display command history
        '''
        print '---------- History ----------\r'
        for i in range(len(self.list)):
            print '[%3d] %s\r'%(i, "".join(self.list[i]))
        print '-----------------------------\r'

    def subcommand_history(self, param_number):
        '''
        This routine creates a new history of command based on the
        parameter number in the current history
        @param param_number: The number N(th) parameter in the command
                             Starts from 0
        @type param_number: Integer
        '''
        subcommand_history = CommandHistory()
        for i in range(len(self.list)):
            command = ''.join(self.list[i])
            params = command.split()
            try:
                param = params[param_number]
            except Exception:
                param = None
            if param:
                subcommand_history.add_string(param)
        return subcommand_history

class MyTerminal(object):
    '''
    Represents my own terminal class
    '''

    @staticmethod
    def clear_screen():
        '''
        Clears the screen
        '''
        os.system("clear")
        # cache output
        clear = os.popen("clear").read()
        #print clear
        # or to avoid print's newline
        sys.stdout.write(clear)

    def __init__(self, exit_on_ctrl_c):
        '''
        Initializes the Terminal Class. When this routine is called,
        the terminal class takes charge of the terminal. The terminal
        class will not be released till the "release" function is called
        explicitly
        @param exit_on_ctrl_c: Whether to exit when user presses CTRL+C
        @type exit_on_ctrl_c: Boolean
        '''
        self.exit_on_ctrl_c = exit_on_ctrl_c
        self.ord_27_91_process = {49: self.ord_27_91_49, #Special Case
                                  51: self.ord_27_91_delete, #Delete
                                  52: self.ord_27_91_end, #End
                                  65: self.ord_27_91_up_arrow, #Up Arrow
                                  66: self.ord_27_91_down_arrow, #Down Arrow
                                  67: self.ord_27_91_right_arrow, #Right Arrow
                                  68: self.ord_27_91_left_arrow, #Left Arrow
                                  90: self.ord_27_91_shift_tab, #SHIFT + TAB
                                  }
        #self.fd = sys.stdin.fileno()
        #self.old_settings = termios.tcgetattr(self.fd)
        #log.warning('__init__ fd %s\r', self.fd)
        #log.warning('__init__ old_settings %s\r', self.old_settings)
        self.fd = None
        self.old_settings = None
        self.oldflags = None

    def ctrl_c_settings(self, exit_on_ctrl_c):
        '''
        @param exit_on_ctrl_c: Whether to exit when user presses CTRL+C
        @type exit_on_ctrl_c: Boolean
        '''
        self.exit_on_ctrl_c = exit_on_ctrl_c

    def acquire(self):
        '''
        This acquires the terminal
        '''
        if self.fd is not None:
            return
        try:
            self.fd = sys.stdin.fileno()
            self.old_settings = termios.tcgetattr(self.fd)
            #self.oldflags = fcntl.fcntl(self.fd, fcntl.F_GETFL)
            #fcntl.fcntl(self.fd, fcntl.F_SETFL, self.oldflags | os.O_NONBLOCK)
        except Exception, ex:
            #log.warning('acquire: Exception [%s]: Please inform Development Team\r', ex)
            pass


    def release(self):
        '''
        This release the terminal class from ownership of the terminal.
        This routine MUST be called for the terminal class to give up
        it's ownership of the terminal
        '''
        try:
            if self.fd is not None:
                #if self.oldflags is not None:
                #    fcntl.fcntl(self.fd, fcntl.F_SETFL, self.oldflags)
                #self.oldflags = None
                termios.tcsetattr(self.fd, termios.TCSADRAIN, self.old_settings)
            self.fd = None
            self.old_settings = None
        except Exception, ex:
            #log.warning('release: Exception [%s]: Please inform Development Team\r', ex)
            pass

    def get_single_user_input_key(self):
        '''
        This routine gets a single key entered by the user.
        Note: Sometimes entering a single key on the keyboard,
              actually results in multiple keys. For e.g.
              the "Delete" key results in the sequence: [27, 91, 51, ~]
              This routine returns only char in that sequence
        '''
        while 1:
            ch = None
            try:
                tty.setraw(sys.stdin.fileno())
                ch = sys.stdin.read(1)
                break
            except Exception:
                continue
            if ch is None:
                continue
            break
        return ch

    def delete(self):
        '''
        Implements delete action. A Delete action deletes the current
        character on the screen.
        '''
        if self.command_index == len(self.command):
            return
        try:
            list_start = self.command[:self.command_index+1]
            list_start.pop()
        except Exception:
            list_start = []
        try:
            list_end = self.command[self.command_index+1:]
            list_end.append(' ')
        except Exception:
            list_end = []
        list_start.extend(list_end)
        if self.fshow_chars:
            sys.stdout.write(''.join(list_end))
            sys.stdout.write('\b'*len(list_end))
        self.command = list_start
        return

    def back_space(self):
        '''
        Implements backspace. A back space action deletes the previous
        character on the screen.
        '''
        if self.command_index == 0:
            return
        try:
            list_start = self.command[:self.command_index]
            list_start.pop()
        except Exception:
            list_start = []
        try:
            list_end = self.command[self.command_index:]
            list_end.append(' ')
        except Exception:
            list_end = []
        list_start.extend(list_end)
        if self.fshow_chars:
            sys.stdout.write('\b')
            sys.stdout.write(''.join(list_end))
            sys.stdout.write('\b'*len(list_end))
        self.command = list_start
        self.command_index = self.command_index - 1
        return

    def delete_curr_command_line(self):
        '''
        Deletes the entire current command.
        '''
        if len(self.command) > 0 and self.fshow_chars:
            sys.stdout.write('\b'*self.command_index)
            sys.stdout.write(' '*len(self.command))
            sys.stdout.write('\b'*len(self.command))
        self.command_index = 0
        return

    def ord_consume_till_end(self, ch):
        '''
        This routine consumes escape sequence till it ends with ~
        @param ch: The current character
        @type ch: Character
        '''
        while ch != '~' and ord(ch) != 13:
            #print 'ord_consume_till_end %d,%s\r'%(ord(ch),ch)
            if ord(ch) == 3 and self.exit_on_ctrl_c: #CTRL+C
                self.release()
                print '\r'
                os._exit(1)
            ch = self.get_single_user_input_key()
        return

    def ord_27_91_up_arrow(self):
        '''
        Implements up arrow 
        '''
        self.delete_curr_command_line()
        self.command = self.history.previous(self.command)
        self.command_index = len(self.command)
        sys.stdout.write(''.join(self.command))
        return

    def ord_27_91_down_arrow(self):
        '''
        Implements down arrow 
        '''
        self.delete_curr_command_line()
        self.command = self.history.next(self.command)
        self.command_index = len(self.command)
        sys.stdout.write(''.join(self.command))
        return

    def ord_27_91_right_arrow(self):
        '''
        Implements right arrow 
        '''
        if self.command_index < len(self.command):
            if self.fshow_chars:
                sys.stdout.write(self.command[self.command_index])
            self.command_index = self.command_index + 1
        return

    def ord_27_91_left_arrow(self):
        '''
        Implements left arrow 
        '''
        if self.command_index > 0:
            if self.fshow_chars:
                sys.stdout.write('\b')
            self.command_index = self.command_index - 1
        return

    def ord_27_91_delete(self):
        '''
        Implements 'Delete Key'
        '''
        ch = self.get_single_user_input_key()
        self.ord_consume_till_end(ch)
        self.delete()
        return

    def ord_27_91_shift_tab(self):
        '''
        Implements shift+tab = [27,91,90]
        '''
        sys.stdout.write(chr(7)) #BELL
        return

    def ord_27_91_home(self):
        '''
        Implements "Home" key being pressed. This will take user to beginning of line
        '''
        if self.command_index > 0:
            if self.fshow_chars:
                sys.stdout.write('\b'*self.command_index)
        self.command_index = 0
        return

    def ord_27_91_end(self):
        '''
        Implements "End" key being pressed. This will take user to the end of line
        '''
        ch = self.get_single_user_input_key()
        self.ord_consume_till_end(ch)
        self.delete_curr_command_line()
        self.command_index = len(self.command)
        sys.stdout.write(''.join(self.command))
        return

    def ord_27_91_49(self):
        '''
        Implements the sequence 27_91_49. If the next is '~', then the "Home" key
        is pressed else consume till end.
        '''
        ch = self.get_single_user_input_key()
        if ch == '~':
            self.ord_27_91_home()
        else:
            self.ord_consume_till_end(ch)
        return

    def ord_27_91(self):
        '''
        ORD 27 and ORD 91. This is where most escape sequences end
        up in. The processing is done based on the next character
        in the sequence
        '''
        ch = self.get_single_user_input_key()
        try:
            self.ord_27_91_process[ord(ch)]()
        except:
            ##Un-comment next 2 lines to check the key board input
            #ch1 = self.get_single_user_input_key()
            #print 'ERROR: Got handled ASCII key sequence [27, 91, %d, %d]\r'%(ord(ch), ord(ch1))
            self.ord_consume_till_end(ch)
        return

    def escape_ord_27(self):
        '''
        This is the ESCAPE sequence i.e. special character. This handles
        1. Arrows
        2. F1-F12
        3. Page Up/Down, Home, Insert etc.
        '''
        ch = self.get_single_user_input_key()
        if ord(ch) == 91:
            self.ord_27_91()
        else:
            #print 'escape_ord_27: Got handled ASCII key sequence [27, %d]\r'%(ord(ch))
            self.ord_consume_till_end(ch)
        return

    def add_char(self, ch):
        '''
        Append a character to the command based on the location of the arrow
        @param ch: The 'readable' character entered by user
        @type ch: Character
        '''
        if self.command_index == len(self.command):
            #Insert at the end (EASY!!!)
            self.command.append(ch)
            if self.fshow_chars:
                sys.stdout.write(ch)
                sys.stdout.flush()
        else:
            #Need to insert in the middle
            try:
                list_start = self.command[:self.command_index]
            except Exception:
                list_start = []
            try:
                list_end = self.command[self.command_index:]
            except Exception:
                list_end = []
            list_start.append(ch)
            if self.fshow_chars:
                #Write the new char
                sys.stdout.write(ch)
                #Write the rest of remaining chars at the end
                sys.stdout.write(''.join(list_end))
                #Move the pointer back to the current char
                sys.stdout.write('\b'*len(list_end))
                sys.stdout.flush()
            list_start.extend(list_end)
            self.command = list_start
            self.command[self.command_index] = ch
        self.command_index = self.command_index + 1
        return

    def command_clean(self):
        '''
        This routine removes unnecessary whitespace from commands
        '''
        user_input = ''.join(self.command)
        self.command = []
        user_params = user_input.split()
        user_input_clean = ''.join((user_params[i]+' ') for i in range(len(user_params)))
        user_input_clean = user_input_clean.rstrip()
        for i in range(len(user_input_clean)):
            self.command.append(user_input_clean[i])
        self.command_index = len(self.command)
        return

    def get_user_input(self, history, existing_command_chars, help_chars, fshow_chars):
        '''
        This create a My Terminal Object that returns the command that
        was entered by user.
        @param history: A history of commands to use for this command
        @type history: List of commands
        @param existing_command_chars: Existing command characters
        @param existing_command_chars: List
        @param help_chars: A list of characters that represents user needs help
                           The command is not considered complete at this point
        @type help_chars: List
        @param show_chars: Whether to show the characters on the screen
        @type show_chars: Boolean
        @return: Whether the user finished entering or incomplete
        @rtype: Boolean
        '''

        self.fshow_chars = fshow_chars
        if existing_command_chars:
            self.command = existing_command_chars
            self.command_index = len(existing_command_chars)
            sys.stdout.write(''.join(existing_command_chars))
            sys.stdout.flush()
        else:
            self.command = []
            self.command_index = 0
        if history:
            self.history = history
        else:
            self.history = []
        #Indicates if user finished entering
        user_input_finished = False
        while 1:
            ch = self.get_single_user_input_key()
            if ord(ch) == 3: #CTRL+C
                if self.exit_on_ctrl_c:
                    self.release()
                    print '\r'
                    os._exit(1)
                else:
                    continue
            if ord(ch) == 13: #Enter
                self.command_clean()
                user_input_finished = True
                break
            elif ch in help_chars: #Tab
                self.command_clean()
                break
            elif ch in string.digits + string.ascii_letters + string.punctuation + string.whitespace:
                self.add_char(ch)
            elif ord(ch) == 127 or ord(ch) == 8: #Backspace
                self.back_space()
            elif ord(ch) == 27: #All ESCAPE characters
                self.escape_ord_27()
            elif ord(ch) == 26 or ord(ch) == 22: #Break and CTRL+V
                continue
            else:
                #error_str = 'ERROR: Got handled ASCII key [%d]'%ord(ch)
                #sys.stdout.write(error_str)
                sys.stdout.flush()
        sys.stdout.flush()
        self.command_index = 0
        self.history = None
        return (self.command, user_input_finished)

