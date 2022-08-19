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

# Delete a list of entities provided as a file, one entity id per line

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

batch_size = 500

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

    if len(data['contextResponses']) == 0:
        return False

    if not 'statusCode' in data['contextResponses'][0]:
        return False

    if not 'code' in data['contextResponses'][0]['statusCode']:
        return False

    if data['contextResponses'][0]['statusCode']['code'] != '200':
        return False

    return True


def remove_batch(entities):
    """
    Removes a batch of entities
    :return: True if removal was ok, False otherwise
    """

    # We use NGSIv1 as this script has been used to remove entities that were created with NGSIv1
    # using chars in the name that are forbidden in NGSIv2

    body = {
        'contextElements': entities,
        'updateAction': 'DELETE'
    }

    res = requests.post(cb_endpoint + '/v1/updateContext', json=body, headers=headers, verify=False)
    if res.status_code != 200 or not response_is_ok(res.json()):
        print '*** Error deleting entities (%d): %s' % (res.status_code, res.json())
        return False

    return True


def remove_all(filename):
    """
    Remove all entities, batch after page
    """
    i = 0
    n = 0
    entities = []

    with open(filename, 'r') as file:
        for line in file:
            id = line.strip()
            entities.append({'id': id })
            i = i + 1
            if i == batch_size:
                n = n + 1
                i = 0

                print '- Deleting batch %d (%d entities)' % (n, batch_size)
                if not remove_batch(entities):
                    return False

                entities = []

        # Last batch with remaining entities
        if i > 0:
            print '- Deleting batch %d (%d entities)' % (n + 1, i)
            remove_batch(entities)


### Main program starts here ###

if len(sys.argv) != 2:
    print("invalid number of arguments, please provide the file name of entities")
    sys.exit()

filename = sys.argv[1]

# Warn user
print "WARNING!!!! This script will delete all the entities in the file '%s'" % filename
print "This action cannot be undone. If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

if remove_all(filename):
    print 'We are done!'
