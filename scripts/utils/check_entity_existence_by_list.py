#!/usr/bin/env python
# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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

# This script check that a list of entities (contained in the file passed as
# argument, one id per line) exists. It prints the following line for each existing
# entity:
#
# <id>|<type>|OK|200
#
# or the following line for each not found entity
#
# <id>|-|<FAIL>|<status_code>
#
# at the end it prints a summary of OKs and FAILs

import sys
import json
import requests

__author__ = 'fermin'

##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

cb_endpoint = 'http://localhost:1026'

headers = {
    'fiware-service': 'service',
    'fiware-servicepath': '/subservice',
    'x-auth-token': 'token'
}

# END of the configuration part (don't touch below this line ;)
##############################################################

def check_entity(id):
    """
    Check an entity and print result line
    :param id:
    """
 
    global n_ok
    global n_fail

    res = requests.get(cb_endpoint + '/v2/entities/' + id, headers=headers, verify=False)

    if res.status_code != 200:
        # res.json() is not used but it can be if we want
        print '%s|-|FAIL|%d' % (id, res.status_code)
        n_fail += 1
        return

    type = res.json()['type']
    print '%s|%s|OK|200' % (id, type)
    n_ok += 1
    return


def check_entities(filename):
    """
    Check entities in filename
    :param filename:
    :return:
    """

    with open(filename, 'r') as file:
        for line in file:
            id = line.strip()
            check_entity(id)

### Main program starts here ###

if len(sys.argv) != 2:
    print "invalid number of arguments, please provide the file name of entities"
    sys.exit()

filename = sys.argv[1]

n_ok = 0
n_fail = 0
check_entities(filename)

print '----------------'
print 'OK:    %d' % n_ok
print 'FAIL:  %d' % n_fail
print 'TOTAL: %d' % (n_ok + n_fail)
