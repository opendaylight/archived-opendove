'''
Created on Dec 20, 2010

@author: amitabha.biswas
'''

from logging import getLogger
log = getLogger(__name__)

class ArrayInteger(object):
    """
    This class deals with storing and parsing arrays of integers.
    Let's say an integer array contains the following elements:
    1,2,3,4,99,100,101,102,103,104,105,350,351,352,400,500,501,502
    This above array will be represented as:
    "1-4,99-105,350-352,400,500-502"
    This way in case the array consists of consecutive integers it can be
    represented as N-M
    """
    consecutive_char = '-'
    seperator_char = ','
    
    @staticmethod
    def GenerateFromBitmap(bitmap, starting_offset):
        """
        Given a bitmap of integers [1-N] - where the Xth bit represented if
        the number X is set in the array, this routine will generate a string
        representing the Array.
        @param bitmap: A bitmap of ports
        @type bitmap: Bitmap
        @param starting_offset: Any offset that should be added to the numbers
                                in the bitmap
        @type starting_offset: Integer
        """
        previous = False
        start = None
        array_string = ''
        for i in range(bitmap.size):
            j = i + 1 #The actual value in the Bitmap
            if bitmap.BitGet(j):
                if not previous:
                    #Start of a new consecutive list
                    start = j
                previous = True
            else:
                if previous:
                    #End of a consecutive list (start to j-1)
                    if len(array_string) > 0:
                        array_string += ArrayInteger.seperator_char
                    if start == j - 1:
                        array_string += '%s'%(start + starting_offset)
                    else:
                        array_string += '%s%s%s'%(start+starting_offset, 
                                                  ArrayInteger.consecutive_char, 
                                                  j-1+starting_offset)
                previous = False
        #End of Loop
        if previous:
            #End of a consecutive list (start to j-1)
            if len(array_string) > 0:
                array_string += ArrayInteger.seperator_char
            if start == j:
                array_string += '%s'%(start+starting_offset)
            else:
                array_string += '%s%s%s'%(start+starting_offset,
                                          ArrayInteger.consecutive_char,
                                          j+starting_offset)
        return array_string
    
    @staticmethod
    def GenerateFromList(list, fsort):
        """
        Given a list of positive integers, this routine will generate a string 
        representing the Array.
        @param list: A list of numbers
        @type list: List []
        @param fsort: Sort the List before generating the list
        @type fsort: Boolean
        @return: The Array String representing the List
        @rtype: String
        """
        previous = -100 #A large negative number
        start = None
        array_string = ''
           
        if fsort:
            list.sort(key=None, reverse=False)
        for num in list:
            if previous != num - 1:
                #Beginning of a new consective set of numbers. Record the previous
                #series
                if start is not None:
                    #Legitimate Start
                    if len(array_string) > 0:
                        array_string += ArrayInteger.seperator_char
                    if start == previous:
                        array_string += '%s'%(start)
                    else:
                        array_string += '%s%s%s'%(start, ArrayInteger.consecutive_char, previous)
                #Update the start of new series
                start = num
            #Store the last element seen
            previous = num
        #Dump the final set
        if start is not None:
            #Legitimate Start
            if len(array_string) > 0:
                array_string += ArrayInteger.seperator_char
            if start == previous:
                array_string += '%s'%(start)
            else:
                array_string += '%s%s%s'%(start, ArrayInteger.consecutive_char, previous)
        return array_string

    @staticmethod
    def GenerateToList(array_string, fsort):
        """
        Given a string representing the array, this will create a list of numbers
        @param array_string: The string storing the array of numbers
        @type array_string: String
        @param fsort: Return a sorted list
        @type fsort: Boolean
        @return: The List of numbers
        @rtype: []
        """
        list = []
        if array_string is None or array_string == '':
            return list
        try:
            number_ranges = array_string.split(ArrayInteger.seperator_char)
            for number_range in number_ranges:
                numbers = number_range.split(ArrayInteger.consecutive_char)
                if len(numbers) == 1:
                    list.append(int(numbers[0]))
                else:
                    start = int(numbers[0])
                    end = int(numbers[1])
                    for i in range(start, end+1):
                        list.append(i)
            if fsort:
                list.sort(key=None, reverse=False)
        except Exception, ex:
            #log.warning('GenerateToList: array_string %s, Exception %s', array_string, ex)
            list = []
        return list
