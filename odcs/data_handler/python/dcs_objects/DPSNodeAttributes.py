'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

 @author: Amitabha Biswas
'''
import logging
#from client_protocol_handler import DpsClientHandler

from Utilities.ArrayInteger import ArrayInteger
from logging import getLogger
log = getLogger(__name__)

class DPSNodeAttributes:
    '''
    This class represents attributes of a DPS node.
    '''

    def __init__(self):
        '''
        Table-1
        ---------------------------------------------------------
        | Node | Version | List of Domains Handled by that node |
        --------------------------------------------------------
        nodeId --> [version,[domainIds]]
        eg:
        {
            1 : [101, [1,2,3]],
            2 : [103, [2,4,5]],
            3 : [104, [5,6]],
            ...
        }
        '''
        self.node_domain_table = {}
        
        '''
        Table-2
        ------------------------------------------------
        | Node | List of versions of other nodes known |
        -----------------------------------------------
        eg:
        {
            1 : {1:101, 2:200, 3:301},
            2 : {},
            3 : {},
            ...
        }
        '''
        self.node_version_table = {}
        '''
        Domain-to-Node mapping table
        ----------------------------------------------
        | Domain | List of Nodes handling the Domain |
        ---------------------------------------------
        eg:
        {
            11223344 : [1.2.3.4, 5.6.7.8, 11.13.11.13]
            12345678 : [5.6.7.8, 11.13.11.13, 14.17.14.17]
        }
        '''

    def clear(self):
        '''
        '''
        self.node_domain_table.clear()
        self.node_version_table.clear()

    def add_node_to_node_domain_table(self,nodeIP):
        try:
            row = self.node_domain_table[nodeIP]
        except Exception:
            self.node_domain_table[nodeIP] = []
            self.node_domain_table[nodeIP].insert(0,0)
            self.node_domain_table[nodeIP].insert(1,[])

    def add_domain_to_node(self,nodeIP,domain, version=1):
        '''
        This function adds an entry / updates existing entry
        in Table-1 which is indexed on nodeIP.
        '''
        try:
            domain_list = self.node_domain_table[nodeIP]
        except Exception:
            '''
                nodeIP not found in dictionary. i.e a new node
                is being added to the dictionary.
            '''
            self.node_domain_table[nodeIP] = []
            self.node_domain_table[nodeIP].insert(0,version)
            self.node_domain_table[nodeIP].insert(1,[])
            domain_list = self.node_domain_table[nodeIP]
            domain_list[1].append(domain)
            return
            # return DpsClientHandler.dps_error_none
        
        domain_list[0] = version
        domain_list[1].append(domain)
        #return DpsClientHandler.dps_error_none

    def add_domain_ver_list_to_node(self,nodeIP,dom_ver_list):
        '''
        This function adds a domain version list for a node (nodeIP)
        to table-1. List is of the form
        [ver, [dom-1, dom-2, dom-3]]
        '''
        try:
            domain_list = self.node_domain_table[nodeIP]
            # compare local version with remote version
            # replace if locally known version is lower
            if domain_list[0] < dom_ver_list[0]:
                self.node_domain_table[nodeIP] = dom_ver_list
        except Exception:
            pass

    def del_domain_from_node(self,nodeIP,domain,version=1):
        '''
        This function removes a domain from a node and updates
        table-1.
        '''
        try:
            self.node_domain_table[nodeIP][0] = version
            domain_list = self.node_domain_table[nodeIP][1]
            domain_list.remove(domain)
        except: # nodeIP not found in dictionary
            pass
        #return DpsClientHandler.dps_error_none
    
    def del_all_domains_from_node(self,nodeIP,version=1):
        '''
        This function removes all domains handled by a node
        and updates table-1.
        '''
        try:
            self.node_domain_table[nodeIP][0] = version
            self.node_domain_table[nodeIP][1] = []
        except: # nodeIP not found in dictionary
            pass
        return

    def del_node_from_node_domain_table(self,nodeIP):
        try:
            del self.node_domain_table[nodeIP]
        except Exception:
            pass
        return

    def print_node_domain_table(self):
        print '%s\r'%self.node_domain_table
        return

    def print_node_version_table(self):
        print '%s\r'%self.node_version_table
        return

    def add_to_local_node(self,rowIP,columnIP,version):
        try:
            row = self.node_version_table[rowIP]
        except Exception: # rowIP not found in the table
            self.node_version_table[rowIP] = {}
            self.node_version_table[rowIP][columnIP] = version
            return
        # TODO: should we check if locally known version is higher 
        # before replacing it.
        row[columnIP] = version
        return

    def delete_node_from_node_version_table(self,nodeIP):
        try:
            del self.node_version_table[nodeIP]
            for key in self.node_version_table:
                row = self.node_version_table[key]
                del row[nodeIP]
        except Exception:
            pass

    def get_table2_row(self,nodeIP):
        try:
            row = self.node_version_table[nodeIP]
        except Exception:
            return
        return row

    def get_domains_for_node(self,nodeIP):
        try:
            row = self.node_domain_table[nodeIP]
        except Exception:
            return []
        return row[1]

    def set_domain_list_for_node(self,nodeIP,ver,domain_list):
        '''
        Sets the domain list (python list) for a node
        '''
        try:
            row = self.node_domain_table[nodeIP]
        except Exception:
            self.add_node_to_node_domain_table(nodeIP)
            row = self.node_domain_table[nodeIP]
        row[0] = ver
        row[1] = domain_list
        return

def main2():
    attrib = DPSNodeAttributes()
    print "printing the node version table...\r"
    attrib.add_to_local_node(1, 1, 100)
    attrib.add_to_local_node(1, 2, 200)
    attrib.add_to_local_node(1, 3, 300)
#    attrib.print_node_version_table()
    attrib.add_to_local_node(2, 1, 100)
    attrib.add_to_local_node(2, 2, 200)
    attrib.add_to_local_node(2, 3, 300)
    
    attrib.add_to_local_node(3, 1, 100)
    attrib.add_to_local_node(3, 2, 200)
    attrib.add_to_local_node(3, 3, 300)
    attrib.print_node_version_table()
    attrib.delete_node_from_node_version_table(2)
    attrib.print_node_version_table()
    
    print attrib.get_table2_row(3)

def main():
    attrib = DPSNodeAttributes()
    attrib.add_domain_to_node(1, 100, 11)
#    attrib.print_node_domain_table()
    attrib.add_domain_to_node(2,100,13)
    #attrib.print_node_domain_table()
    attrib.add_domain_to_node(1,101,14)
    attrib.add_domain_to_node(1,102,16)
    attrib.print_node_domain_table()
    attrib.del_domain_from_node(1, 103, 14)
    print "get domains for node\r"
    domain_str = ArrayInteger.GenerateFromList(attrib.get_domains_for_node(4), True)
    print '%s\r'%domain_str
    attrib.print_node_domain_table()

if __name__ == "__main__":
    main()
