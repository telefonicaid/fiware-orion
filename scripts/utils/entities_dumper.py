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

# This script gets a pipe separated list of entities passed as argument and print
# it in a line. It is though to test no changes are in the API after a
# DB migration process on entities.
#
# Eg:
#
# PYTHONIOENCODING=utf8 ./entities_dumper.py 'hi_10|hi_20|hi_30'

from __future__ import print_function

import sys
import requests
requests.packages.urllib3.disable_warnings()

__author__ = 'fermin'

##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

cb_endpoint = 'http://localhost:1026'

headers = {
    'fiware-service': 'service',
    'fiware-servicepath': '/subservice',
    'x-auth-token': 'token'
}

#url_op = '/v1/contextEntities/'
url_op = '/v2/entities/'

# END of the configuration part (don't touch below this line ;)
##############################################################

def dump_entity(id):
    """
    Get an entity and print it
    :param id:
    :return:
    """

    res = requests.get(cb_endpoint + url_op + id, headers=headers, verify=False)
    if res.status_code != 200:
        print()
        print('Error getting entity %s (%d): %s' % (id, res.status_code, res.json()))
        return False

    print(res.text, end='')
    return True

### Main program starts here ###

if len(sys.argv) != 2:
    print("invalid number of arguments, please provide a pipe separated list of entities")
    sys.exit()

ids = sys.argv[1].split('|')

print('[',end='')

for id in ids:
    if not dump_entity(id):
        break
    if ids.index(id) != len(ids) - 1:
        print(',', end='')

print (']',end='')