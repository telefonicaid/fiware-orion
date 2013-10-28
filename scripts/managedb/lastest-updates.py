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

from pymongo import MongoClient, DESCENDING
from datetime import datetime 
from sys import argv

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

if 4 <= len(argv) <= 5:
    type = argv[1]
    db = argv[2]
    max = int(argv[3])
else:
    print 'Wrong number of arguments'
    print '   Usage:   ./lastest-updates.py <entities|attributes> <db> <limit> [entity_filter] '
    print '   Example  ./lastest-updates.py entities orion 10'
    print '   Example  ./lastest-updates.py entities orion 10 TEST_SENSOR'
    exit(1)

# Optional argument: filter
query = {}
if len(argv) == 5:
    query['_id.id'] = {'$regex': argv[4]}

client = MongoClient('localhost', 27017)
col = client[db]['entities']

if type == 'entities':
    query['modDate'] = {'$exists': True}
    for doc in col.find(query).sort('modDate', direction=DESCENDING).limit(max):
        modDate = int(doc['modDate'])
        dateString = datetime.fromtimestamp(modDate).strftime('%Y-%m-%d %H:%M:%S')
        entityString = doc['_id']['id'] + ' (' + doc['_id']['type'] + ')'
        print '-- ' + dateString + ': ' + entityString
elif type == 'attributes':
    # Attributes are stored in a hash. The key of the hash is the modification date, so it is actually a 
    # hash of lists (due to several attributes could have the same modification date)
    attrHash = { } 
    query['attrs.modDate'] = {'$exists': True}
    for doc in col.find(query).sort('modDate', direction=DESCENDING).limit(max):
        entityString = doc['_id']['id'] + ' (' + doc['_id']['type'] + ')'
        for attr in doc['attrs']:
           if attr.has_key('modDate'):
              modDate = int(attr['modDate'])
              attrString = attr['name'] + ' - ' + entityString
              if attrHash.has_key(modDate):
                  attrHash[modDate].append(attrString)
              else:
                  attrHash[modDate] = [attrString]
    printAttrs(attrHash, max)
else:
    print 'Unsuported type: <' + type + '>'
    exit (1)
