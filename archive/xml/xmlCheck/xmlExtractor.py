#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

# This script extract XML fragments of a given file (passed as first arguments),
# each fragment put in a separate file in a given directory (passed as second argument).
# This is mainly aimed at processing test harness files (.test files) to pass the
# xmlCheck.sh on then afterwards. Note that the heuristic used by the script is not
# bullet-proof: it assumes that the starting and ending tag of the root element is
# in its own line, without other child starting or ending tags (but it should suffice
# for .test files)

__author__ = 'fermin'

from sys import argv
import re

def write_fragment(lines, filename):
    f = open(filename, 'w')
    f.writelines(lines)
    f.close()

tokens_map = {
    # NGSI9 standard operations
    'registerContextRequest':                        ['ngsi9', 'valid'],
    'registerContextResponse':                       ['ngsi9', 'valid'],
    'discoverContextAvailabilityRequest':            ['ngsi9', 'valid'],
    'discoverContextAvailabilityResponse':           ['ngsi9', 'valid'],
    'subscribeContextAvailabilityRequest':           ['ngsi9', 'valid'],
    'subscribeContextAvailabilityResponse':          ['ngsi9', 'valid'],
    'updateContextAvailabilitySubscriptionRequest':  ['ngsi9', 'valid'],
    'updateContextAvailabilitySubscriptionResponse': ['ngsi9', 'valid'],
    'unsubscribeContextAvailabilityRequest':         ['ngsi9', 'valid'],
    'unsubscribeContextAvailabilityResponse':        ['ngsi9', 'valid'],
    'notifyContextAvailabilityRequest':              ['ngsi9', 'valid'],
    'notifyContextAvailabilityResponse':             ['ngsi9', 'valid'],
    # NGSI10 standard operatoins
    'queryContextRequest':                ['ngsi10', 'valid'],
    'queryContextResponse':               ['ngsi10', 'valid'],
    'updateContextRequest':               ['ngsi10', 'valid'],
    'updateContextResponse':              ['ngsi10', 'valid'],
    'subscribeContextRequest':            ['ngsi10', 'valid'],
    'subscribeContextResponse':           ['ngsi10', 'valid'],
    'updateContextSubscriptionRequest':   ['ngsi10', 'valid'],
    'updateContextSubscriptionResponse':  ['ngsi10', 'valid'],
    'unsubscribeContextRequest':          ['ngsi10', 'valid'],
    'unsubscribeContextResponse':         ['ngsi10', 'valid'],
    'notifyContextRequest':               ['ngsi10', 'valid'],
    'notifyContextResponse':              ['ngsi10', 'valid'],
    # NGSI convenience operations exclusive types
    'registerProviderRequest':          ['ngsi9',  'postponed'],
    'updateContextElementRequest':      ['ngsi10', 'postponed'],
    'updateContextElementResponse':     ['ngsi10', 'postponed'],
    'appendContextElementRequest':      ['ngsi10', 'postponed'],
    'appendContextElementResponse':     ['ngsi10', 'postponed'],
    'updateContextAttributeRequest':    ['ngsi10', 'postponed'],
    'contextElementResponse':           ['ngsi10', 'postponed'],
    'contextAttributeResponse':         ['ngsi10', 'postponed'],
    'statusCode':                       ['ngsi', 'valid'],
    # New operations
    'entityTypeAttributesResponse':     ['ngsi10', 'postponed'],
    'entityTypesResponse':              ['ngsi10', 'postponed'],
    # Orion own types
    'orion':      ['orion', 'invalid'],
    'orionError': ['orion', 'invalid']
}

if len (argv) != 4:
    print 'Wrong number of arguments'
    print argv[0] + ' <file_to_parse> <directory_for_fragments> <base_name>'
    exit(1)

file = str(argv[1])
dir = str(argv[2])
base_name = str(argv[3])

buffer = []
xml_headers_counter = 0
xml_headers_correction = 0
xml_fragments_counter = 0
search_mode = True
next_xml_invalid = False
buffer = []

with open (file, 'r') as f:
    for l in f:
        line = l

        if re.search('#SORT_START', line) or re.search('#SORT_END', line):
            # Just skip these marks (they must not appear in the XML string)
            continue

        if search_mode:
            # Search mode is on: looking for a root element or for an 'invalid'
            if re.search('<\?xml', line):
                xml_headers_counter += 1
            elif re.search('User-Agent:', line):
                # Heuristic: each time a User-Agent is detected, then a nofityContextRequest or notifyContextAvailabiltiyRequest
                # if found in the .test file. These are fragments of code that comes from accumulator-script.py dumps, outside
                # the usual "xmllint decoration" that adds the XML preamble in that case. Thus, we have to count then and apply as
                # a correction factor in the warning generation
                xml_headers_correction += 1
            elif re.search('#INVALID_XML', line):
                next_xml_invalid = True
            else:
                m = re.match('\s*<(.*)>', line)
                if m != None:
                    xml_fragments_counter += 1
                    root_element = m.group(1)

                    # Add XML header and root element as first elements of the buffer
                    buffer.append('<?xml version="1.0"?>\n')
                    buffer.append(line)

                    search_mode = False

        else:
            # Search mode is off: accumulate each line until the ending tag is found
            # Note we remove any heading or trailing ' or ", that may occur in the 
            # case of using variables to store XMLs, e.g.: 
            # 
            # payload = '<?xml .... =?>
            #            <root>
            #              <foo>'"
            #                <bar>...</bar>
            #               "'</foo>
            #            ...
            #            </root>'
            #
            line = line.rstrip("'\"\n")
            line = line.lstrip(" '\"")

            # We have found that strings like "http://localhost:'$CP1_PORT'/v1" that breaks validation,
            # as they are not anyURI. In order to fix, we replace the "'$...'" part with a number
            line = re.sub("'\$.*PORT.*'", "9999", line)           

            # We have found that we cannot use things like '<duration>REGEX((PT5S|PT7S))</duration>', given that
            # duration uses type xs:duration, which has a predefined sytanx incompatible with REGEX(). Thus, we
            # change these cases on the fly
            if re.match('\s*<duration>', line):
                line = re.sub("REGEX\(.*\)", "PT1M", line)

            # Similar case with providingApplication
            if re.match('\s*<providingApplication>', line):
                line = re.sub("REGEX\(.*\)", "9997", line)

            buffer.append(line + "\n")

            if re.search('<\/'+ root_element + '>', line):              
                # We use some tokens in the filename to help xmlCheck.sh script
                if tokens_map.has_key(root_element):
                    family = tokens_map[root_element][0]
                    vality = tokens_map[root_element][1]
                else:                    
                    family = 'unknown'
                    vality = 'invalid'

                # No matter the result of the map, if the invalid mark was used in the .test, then
                # file is alawys marked as 'invalid'
                if next_xml_invalid:
                    vality = 'invalid'
 
                filename = dir + '/' + family + '.' + base_name + '.part_' + str(xml_fragments_counter) + '.' + vality + '.xml'

                write_fragment(buffer, filename)
                buffer = []
                search_mode = True
                next_xml_invalid = False

if xml_headers_counter != xml_fragments_counter - xml_headers_correction:
    print 'Warning in ' + base_name + ': XML headers (' + str(xml_headers_counter) + ', correction: ' + str(xml_headers_correction) + ') and ' \
          'generated fragments (' + str(xml_fragments_counter) + ') differ'
