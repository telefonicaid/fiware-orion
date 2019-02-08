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
# Dump a list of entities provided as a file, one entity id per line. The output
# is also one entity in { .. } per line.
#
# Check: applying the following filter in the output
#
#    | awk -F '"id": "' '{print $2}' | awk -F '"' '{print $1}'
#
# should be equal to the input file

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


def response_is_ok(data):
    """
    Check response is ok
    :param data: response body as JSON dict
    :return:
    """

    if not 'contextResponses' in data:
        return False

    if len(data['contextResponses']) != 1:
        return False

    if not 'statusCode' in data['contextResponses'][0]:
        return False

    if not 'code' in data['contextResponses'][0]['statusCode']:
        return False

    if data['contextResponses'][0]['statusCode']['code'] != '200':
        return False

    return True


def dump_entity(id):
    """
    Get an entity and print it
    :param id:
    :return:
    """

    # We use NGSIv1 queryContext as this script has been used to djmp entities that were created with NGSIv1
    # using chars in the name id that are forbidden in NGSIv2 or problematic in URLS (such as / )

    body = {
        'entities': [ {'id': id} ]
    }

    res = requests.post(cb_endpoint + '/v1/queryContext', json=body, headers=headers, verify=False)

    if res.status_code != 200 or not response_is_ok(res.json()):
        print
        print '*** Error getting entity %s (%d): %s' % (id, res.status_code, res.json())
        return False

    entity_body = res.json()['contextResponses'][0]['contextElement']
    print json.dumps(entity_body)

    return True


def dump_entities(filename):
    """
    Dump entities in filename
    :param filename:
    :return:
    """

    with open(filename, 'r') as file:
        for line in file:
            id = line.strip()
            if not dump_entity(id):
                return False

### Main program starts here ###

if len(sys.argv) != 2:
    print "invalid number of arguments, please provide the file name of entities"
    sys.exit()

filename = sys.argv[1]

dump_entities(filename)