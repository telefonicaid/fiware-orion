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

import requests
import sys
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

page_size = 500

attr_to_delete = 'a2'

filter = '&q=a2'

# END of the configuration part (don't touch below this line ;)
##############################################################


def testing_populate(n, entity_type):
    """
    Populate a set of entities, just for testing

    :param n: number of entities to populate (with ids from e1 to e<n>)
    :param entity_type: entity type used for the populated entities (all with the same type)
    """
    print 'Populating %d entities of type %s' % (n, entity_type)

    entities = []
    for i in range(0, n):
        entity = {
            'id': 'e' + str(i),
            'type': entity_type,
            'a1': {
                'value': 'foo',
                'type': 'Text'
            },
            'a2': {
                'value': 42,
                'type': 'Number'
            }
        }
        entities.append(entity)

    body = {
        'entities': entities,
        'actionType': 'append'
    }

    res = requests.post(cb_endpoint + '/v2/op/update', json=body, headers=headers, verify=False)
    if res.status_code != 204:
        print 'Error populating entities (%d): %s' % (res.status_code, res.json())


def get_entities_count():
    """
    Return the total number of entities

    :return: number of entities (or -1 if there was some error)
    """

    # Trick: asking for a non existing attribute (attrs=noexist) will make response very small (only entity id/type)
    res = requests.get(cb_endpoint + '/v2/entities?limit=1&options=count&attrs=noexist' + filter, headers=headers,
                       verify=False)
    if res.status_code != 200:
        print 'Error getting entities count (%d): %s' % (res.status_code, res.json())
        return -1
    else:
        return int(res.headers['fiware-total-count'])


def initial_statistics():
    """
    Print some initial data statistics

    :return: the total number of entities
    """
    total = get_entities_count()
    print 'There are %d entities' % total
    pages = total / page_size
    rest = total - (pages * page_size)
    print 'There are %d pages of %d entities (and a final page of %d entities)' % (pages, page_size, rest)

    return total


def send_batch(entities):
    """
    Send a POST /v2/op/update batch

    :param entities: the entities to be included in the batch (up to page_size, but construction)
    :return: True if removal was ok, False otherwise
    """

    body = {
        'entities': entities,
        'actionType': 'delete'
    }

    res = requests.post(cb_endpoint + '/v2/op/update?options=keyValues', json=body, headers=headers, verify=False)
    if res.status_code != 204:
        print 'Error in batch operation (%d): %s' % (res.status_code, res.json())
        return False

    return True


### Main program starts here ###

# Warn user
print "WARNING!!!! This script will delete the atrribute '%s' in all the entities matching the filter '%s'" % (attr_to_delete, filter)
print "This action cannot be undone. If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

#testing_populate(1876, 'device')

count = initial_statistics()

if count == 0:
    print 'Nothing to do'
    sys.exit(0)

entities = []

# Number of batches
pages = count / page_size
for i in range(0, pages + 1):
    print '- Getting entities page %d' % (i + 1)
    offset = i * page_size
    res = requests.get('%s/v2/entities?offset=%s&limit=%s%s' % (cb_endpoint, str(offset), str(page_size), filter),
                       headers=headers, verify=False)

    if res.status_code != 200:
        print 'Error getting entities (%d): %s' % (res.status_code, res.json())
        sys.exit(1)

    for entity in res.json():
        entities.append({'id': entity['id'], 'type': entity['type']})

accum = 0
batch = []
for entity in entities:

    batch.append({'id': entity['id'], 'type': entity['type'], attr_to_delete: 0})
    accum += 1

    if (accum == page_size):
        print '- Update batch of %d entities' % len(batch)
        send_batch(batch)
        accum = 0
        batch = []

# Last batch
if len(batch) > 0:
    print '- Update batch of %d entities' % len(batch)
    send_batch(batch)

print 'We are done!'
