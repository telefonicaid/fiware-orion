#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

__author__ = 'fermin'

from pymongo import MongoClient
import sys

# The way in which Python manage dictionaries doesn't make easy to be sure
# of field ordering, which is importanf for MongoDB in the case of using an
# embedded document for _id. The flatten() function helps
def flatten(_id):
    r = {'_id.id': _id['id']}

    if 'type' in _id:
       r['_id.type'] = _id['type']
    else:
       r['_id.type'] = {'$exists': False}

    if 'servicePath' in _id:
       r['_id.servicePath'] = _id['servicePath']
    else:
       r['_id.servicePath'] = {'$exists': False}

    return r

def old_coordinates_format(doc):
    c = doc['location']['coords']
    # Is c a list and has 2 element?
    if type(c) is list:
        return (len(c) == 2)
    else:
        return False

if len(sys.argv) != 2:
    print "missing db name"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

client = MongoClient('localhost', 27017)
db = client[DB]

n = 0
changed = 0
error = 0

# Note the 'location.coords.type': {$exists: False} part of the query is a way of ensuring that modified
# document are not "reinyected" in the cursor (some weird behaviour has been observed if that part of the
# query is not used)
for doc in db[COL].find({'location.coords': {'$exists': True}, 'location.coords.type': {'$exists': False}}):
    n += 1

    # Is uses an array of coordinates(i.e. pre-0.21.0 format)?
    if (old_coordinates_format(doc)):
        coordX = doc['location']['coords'][0]
        coordY = doc['location']['coords'][1]
        new_coords = {
            'type': 'Point',
            'coordinates': [ coordX, coordY ]
        }
        db[COL].update(flatten(doc['_id']), {'$set': {'location.coords': new_coords}})
        # Check update was ok
        check_doc = db[COL].find_one(flatten(doc['_id']))
        if (old_coordinates_format(check_doc)):
            print "ERROR: document <%s> change attemp failed!" % str(check_doc['_id'])
            error += 1
        else:
            changed += 1

print '---- Documents using old coordinates format: %d' % n
print '---- Changed documents:                      %d' % changed
print '---- Errors changing documents:              %d' % error

