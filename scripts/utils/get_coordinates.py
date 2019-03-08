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

# END of the configuration part (don't touch below this line ;)
##############################################################


def get_ngsiv1_location(entity):
    """
    Search for NGSIv1-style location, i.e. a "location" metadata in some of the attributes of the given entity.

    FIXME: currently metadata location doesn't appear in NGSIv2 results. This method could be pretty useless
    until https://github.com/telefonicaid/fiware-orion/issues/3122 gets solved, but we leave here for the future
    in any case.

    :param entity: the entity in which search location
    :return: the attribute which holds location, 'None' if no one was found
    """
    for field in entity:
        if field == 'id' or field == 'type':
            continue
        for m in entity[field]['metadata']:
            if m == 'location':
                return field
    return None


def get_ngsiv2_location(entity):
    """
    Search for NGSIv2-style location, i.e. a "geo:point" attribute in the given entity.

    :param entity: the entity in which search location
    :return: the attribute which holds location, 'None' if no one was found
    """
    for field in entity:
        if field == 'id' or field == 'type':
            continue
        if entity[field]['type'] == 'geo:point':
            return field
    return None


def process_page(entities):
    """
    Process a page of entities

    :param entities: page of entities (a list) to be processed
    """
    for entity in entities:
        loc_attr = get_ngsiv2_location(entity)
        if loc_attr is None:
            loc_attr = get_ngsiv1_location(entity)
        if loc_attr is not None:
            print '%s: %s' % (entity['id'], entity[loc_attr]['value'])


### Main program starts here ###

# Get first bunch of data
res = requests.get('%s/v2/entities?offset=0&limit=%s&options=count' % (cb_endpoint, str(page_size)),
                   headers=headers, verify=False)
if res.status_code != 200:
        print 'Error getting entities (%d): %s' % (res.status_code, res.json())
        sys.exit(1)

# Get count
count = int(res.headers['fiware-total-count'])

# Process the response
process_page(res.json())

# Number of batches
pages = count / page_size
for i in range(0, pages):
    offset = (i + 1) * page_size
    res = requests.get('%s/v2/entities?offset=%s&limit=%s' % (cb_endpoint, str(offset), str(page_size)),
                       headers=headers, verify=False)
    if res.status_code != 200:
        print 'Error getting entities (%d): %s' % (res.status_code, res.json())
        sys.exit(1)
    process_page(res.json())

print 'Hints:'
print '* use "awk -F \': \' \'{print $2}\'" filter if you want to grab just the coordinates'
print '* copy-paste output to https://www.darrinward.com/lat-long so see where you entities are on a map'
