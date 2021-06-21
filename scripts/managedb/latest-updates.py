#!/usr/bin/env python2
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

from pymongo import MongoClient, DESCENDING
from datetime import datetime 
from sys import argv

# This script can be easily adapted to used creation date instead of modification date
# just changing the following variable to 'creDate'
refDate = 'modDate'

def entityString(entity):
    s = entity['id']
    if (entity.has_key('type')):
        s += ' (' + entity['type'] + ')'
    if (entity.has_key('servicePath')):
        s += ' (sp: ' + entity['servicePath'] + ')'
    else:
        s += ' (sp: <null>)'
    return s

def printAttrs(attrHash, max):
    # Given that each entity can have N attributes where N can be greater than 1, we need to add a second level
    # of limit control (beyong the ".limit(max)" in the mongo query)
    n = 0
    for d in sorted(attrHash.keys(), reverse=True):
        for attr in attrHash[d]:
            printableDate = datetime.fromtimestamp(d).strftime('%Y-%m-%d %H:%M:%S')
            print '-- ' + printableDate + ': '+ attr
            n += 1
            if n == max:
                return

if 4 <= len(argv) <= 6:
    type = argv[1]
    db = argv[2]
    max = int(argv[3])
else:
    print 'Wrong number of arguments'
    print '   Usage:   ./latest-updates.py <entities|attributes> <db> <limit> [entity_filter] [sp]'
    print '   Example  ./latest-updates.py entities orion 10'
    print '   Example  ./latest-updates.py entities orion 10 TEST_SENSOR'
    print '   Example  ./latest-updates.py entities orion 10 TEST_SENSOR /myServicePath'
    exit(1)

# Optional argument: filter
query = {}
if len(argv) == 5:
    query['_id.id'] = {'$regex': argv[4]}

if len(argv) == 6:
    query['_id.servicePath'] = argv[5]

client = MongoClient('localhost', 27017)
col = client[db]['entities']

if type == 'entities':
    query[refDate] = {'$exists': True}
    for doc in col.find(query).sort(refDate, direction=DESCENDING).limit(max):
        date = int(doc[refDate])
        dateString = datetime.fromtimestamp(date).strftime('%Y-%m-%d %H:%M:%S')
        print '-- ' + dateString + ': ' + entityString(doc['_id'])
elif type == 'attributes':
    # Attributes are stored in a hash. The key of the hash is the modification date, so it is actually a 
    # hash of lists (due to several attributes could have the same modification date)
    attrHash = { } 
    query['attrs.' + refDate] = {'$exists': True}
    for doc in col.find(query).sort(refDate, direction=DESCENDING).limit(max):
        for attr in doc['attrs']:
           if attr.has_key(refDate):
              date = int(attr[refDate])
              attrString = attr['name'] + ' - ' + entityString(doc['_id'])
              if attrHash.has_key(date):
                  attrHash[date].append(attrString)
              else:
                  attrHash[date] = [attrString]
    printAttrs(attrHash, max)
else:
    print 'Unsuported type: <' + type + '>'
    exit (1)
