'''
 @copyright (c) 2010-2013 IBM Corporation
 All rights reserved.
 
 This program and the accompanying materials are made available under the
 terms of the Eclipse Public License v1.0 which accompanies this
 distribution, and is available at http://www.eclipse.org/legal/epl-v10.html

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
