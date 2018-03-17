#!/usr/bin/env python
# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
requests.packages.urllib3.disable_warnings()

__author__ = 'fermin'

##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

cb_endpoint = 'https://localhost:1026'

headers = {
    'fiware-service': 'service',
    'fiware-servicepath': '/subservice',
    'x-auth-token': 'token'
}

page_size = 500

# WARNING! use this filter or you will remove *all* entities
filter = '&type=device'

# END of the configuration part (don't touch below this line ;)
##############################################################


def testing_populate(n, entity_type):
    """
    Populate a set of entities, just for testing

    :param n: number of entitis to populate (with ids from e1 to e<n>)
    :param entity_type: entity type used for the populated entities (all with the same type)
    """
    print 'Populating %d entities of type %s' % (n, entity_type)

    entities = []
    for i in range(1, 110):
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
        'actionType': 'APPEND'
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
    """
    total = get_entities_count()
    print 'There are %d entities' % total
    pages = total / page_size
    rest = total - (pages * page_size)
    print 'There are %d pages of %d entities (and a final page of %d entities)' % (pages, page_size, rest)


def remove_page():
    """
    Removes the first page of entities
    :return: True if removal was ok, False otherwise
    """

    # Trick: asking for a non existing attribute (attrs=noexist) will make response very small (only entity id/type)
    res = requests.get(cb_endpoint + '/v2/entities?attrs=noexist&limit=' + str(page_size) + filter, headers=headers,
                       verify=False)
    if res.status_code != 200:
        return False

    entities = []
    for entity in res.json():
        entities.append({'id': entity['id'], 'type': entity['type']})

    body = {
        'entities': entities,
        'actionType': 'DELETE'
    }

    res = requests.post(cb_endpoint + '/v2/op/update', json=body, headers=headers, verify=False)
    if res.status_code != 204:
        print 'Error deleting entities (%d): %s' % (res.status_code, res.json())
        return False

    return True


def remove_all():
    """
    Remove all entities, page after page
    """
    i = 1
    while get_entities_count() > 0:
        print '- Remove page %d' % i
        remove_page()
        i += 1


### Main program starts here ###

# Warn user
print "WARNING!!!! This script will delete all the entities matching the filter '%s'" % filter
print "This action cannot be undone. If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()


#testing_populate(109, 'testing')
initial_statistics()
remove_all()
print 'We are done!'
