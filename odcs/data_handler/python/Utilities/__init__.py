'''
Created on June 18, 2012

@author: amitabha.biswas
'''
import base64
from logging import getLogger
log = getLogger(__name__)

class Encoding(object):
    '''
    Return a base64 encoding of the input string.
    '''
    def base64_encode(self,input_string):
        return base64.b64encode(input_string)