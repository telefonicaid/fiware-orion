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
# fermin at tid dot es

# This script extract XML fragments of a given file (passed as first arguments),
# each fragment put in a separate file in a given directory (passed as second argument).
# This is mainly aimed at processing test harness files (.test files) to pass the
# xmlChecker.sh on then afterwards. Note that the heuristic used by the script is not
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

def ngsi_token(s):
    if s == 'registerContextRequest':
        return 'ngsi9'
    elif s == 'registerContextResponse':
        return 'ngsi9'
    elif s == 'discoverContextAvailabilityRequest':
        return 'ngsi9'
    elif s == 'discoverContextAvailabilityResponse':
        return 'ngsi9'
    elif s == 'subscribeContextAvailabilityRequest':
        return 'ngsi9'
    elif s == 'subscribeContextAvailabilityResponse':
        return 'ngsi9'
    elif s == 'updateContextAvailabilitySubscriptionRequest':
        return 'ngsi9'
    elif s == 'updateContextAvailabilitySubscriptionResponse':
        return 'ngsi9'
    elif s == 'unsubscribeContextAvailabilityRequest':
        return 'ngsi9'
    elif s == 'unsubscribeContextAvailabilityResponse':
        return 'ngsi9'
    elif s == 'notifyContextAvailabilityRequest':
        return 'ngsi9'
    elif s == 'notifyContextAvailabilityResponse':
        return 'ngsi9'
    elif s == 'queryContextRequest':
        return 'ngsi10'
    elif s == 'queryContextResponse':
        return 'ngsi10'
    elif s == 'updateContextRequest':
        return 'ngsi10'
    elif s == 'updateContextResponse':
        return 'ngsi10'
    elif s == 'subscribeContextRequest':
        return 'ngsi10'
    elif s == 'subscribeContextResponse':
        return 'ngsi10'
    elif s == 'updateContextSubscriptionRequest':
        return 'ngsi10'
    elif s == 'updateContextSubscriptionResponse':
        return 'ngsi10'
    elif s == 'unsubscribeContextRequest':
        return 'ngsi10'
    elif s == 'unsubscribeContextResponse':
        return 'ngsi10'
    elif s == 'notifyContextRequest':
        return 'ngsi10'
    elif s == 'notifyContextResponse':
        return 'ngsi10'
    else:
        return 'unknown'

if len (argv) != 4:
    print 'Wrong number of arguments'
    print argv[0] + ' <file_to_parse> <directory_for_fragments> <prefix>'
    exit(1)

file = str(argv[1])
dir = str(argv[2])
prefix = str(argv[3])

buffer = []
xml_headers_counter = 0
xml_fragments_counter = 0
search_mode = True
buffer = []

with open (file, 'r') as f:
    for l in f:
        line = l

        if search_mode:
            # Search mode is on: looking for a root element
            if re.search('<\?xml', line):
                xml_headers_counter += 1
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
            buffer.append(line)

            if re.search('<\/'+ root_element + '>', line):
                # We add a 'ngsi9' or 'ngsi10' token in the filename to help xmlChecker.sh script
                filename = dir + '/' + ngsi_token(root_element) + '.' + prefix + '.' + str(xml_fragments_counter)
                write_fragment(buffer, filename)
                buffer = []
                search_mode = True

if xml_headers_counter != xml_fragments_counter:
    print 'Warning: XML headers (' + str(xml_headers_counter) + ') and ' \
          'generated fragments (' + str(xml_fragments_counter) + ') differ'
