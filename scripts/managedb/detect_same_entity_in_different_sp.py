#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

# This script analyze all the entities in Orion DB, clasifying them by SP,
# in order to detect same entity (i.e. same id+type) in differents SP. This
# may correspond to a wrong situation. 
#

__author__ = 'fermin'

from pymongo import MongoClient
import sys

if len(sys.argv) != 2:
    print "missing db name"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

client = MongoClient('localhost', 27017)
db = client[DB]

entities = {}

def value_or_empty(d, k):
    if k in d:
       return d[k]
    else:
       return ''

def value_or_slash(d, k):
    if k in d:
       return d[k]
    else:
       return '/'


n = 0
for doc in db[COL].find():
    n += 1

    id = doc['_id']['id']
    type = value_or_empty(doc['_id'], 'type')
    sp = value_or_slash(doc['_id'], 'servicePath')

    if type not in entities:
        entities[type] = {}
    entities_type = entities[type]

    if id not in entities_type:
        entities_type[id] = [sp]
    else:
        entities_type[id].append(sp)

    if (n % 1000 == 0):
        print "+ Processed entities: " + str(n)

print "+ Total entities: " +  str(n)

for type in entities.keys():
    for id in entities[type].keys():
        if len(entities[type][id]) > 1:
            print "- Entity <%s, %s> appears in several sps: %s" % (id, type, str(entities[type][id]))
                
