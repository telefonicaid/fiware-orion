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
# iot_support at tid dot es

# This script takes two argument as parameters: collection name and field name. It does
# a type analysis for the chosen field and print it. It is a read-only process.

from pymongo import MongoClient
from sys import argv

# Types map (from http://docs.mongodb.org/manual/reference/operator/query/type/). Note
# that Min key and Max key are disabled, given that it seems to break the find() based on them
types = {
    1: 'Double',
    2: 'String',
    3: 'Object',
    4: 'Array',
    5: 'Binary data',
    6: 'Undefined (deprectaed)',
    7: 'Object id',
    8: 'Boolean',
    9: 'Data',
    10: 'Null',
    11: 'Regula Expression',
    13: 'JavaScript',
    14: 'Symbol',
    15: 'JavaScript (with scope)',
    16: '32-bit integer',
    17: 'Timestamp',
    18: '64-bit integer',
    #255: 'Min key',
    #127: 'Max key',
}

# Grab arguments
if len(argv) == 3:
    col = argv[1]
    field = argv[2]
else:
    print 'Wrong number of arguments'
    print '   Usage:   ./type-analizer.py.py <colection> <field>'
    exit(1)

client = MongoClient('localhost', 27017)
db = client['orion']

# Get data
total    = db[col].count()                                          # All documents in collection
no_field = db[col].find({field: {'$exists': False}}).count()        # Documents without field
types_n = {}
for type in types.keys():
    types_n[type] = db[col].find({field: {'$type': type}}).count()  # Per-type count

# Print data
print 'Total documents in collection "' + col + '": ' + str(total)
print '-- Documents without field "' + field + '": ' + str(no_field)
classified = 0;
for type in types_n.keys():
    n = types_n[type]
    if n > 0:
        print '-- Type ' + types[type] + ': ' + str(n)
        classified += n

not_classified = total - no_field - classified
print '-- Unknown type: ' + str(not_classified)
