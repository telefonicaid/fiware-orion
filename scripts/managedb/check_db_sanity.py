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

# This script does several checkings on a Orion DB, looking for some "anomaly" patterns
#
# The list of checks (to be extended):
#
# 1. Check that there is no more than one attribute with the same name in a given entity document
# 2. Check that there is no duplication in _id fields (for mongo {_id: {x: 1, y: 2}} and {_id: {y:2, x:1}} are different documents)

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

def warn(s):
    print 'WARNING: %s' % s
    global need_fix
    need_fix = True

def duplicated_attr_name(id, type, sp, list):
    names = {}
    at_least_one_dup = False
    for attr in list:
        if (attr['name'] in names):
            warn('<%s, %s, %s> attribute %s is duplicated' % (id, type, sp, attr['name']))
            at_least_one_dup = True 
        else:
            names[attr['name']] = 1
    return at_least_one_dup

def value_or_empty(d, k):
    if k in d:
       return d[k]
    else:
       return ''

def check_dup_attrs():

    for doc in db[COL].find():
        id = doc['_id']['id']
        type = value_or_empty(doc['_id'], 'type')
        sp = value_or_empty(doc['_id'], 'servicePath')

        duplicated_attr_name(id, type, sp, doc['attrs'])

def check_dup_id_subfields():

    for doc in db[COL].find():
        query = {'_id.id': doc['_id']['id']}

        if 'type' in doc['_id']:
            query['_id.type'] = doc['_id']['type']
        else:
            query['_id.type'] = {'$exists': False}
        
        if 'servicePath' in doc['_id']:
            query['_id.servicePath'] = doc['_id']['servicePath']
        else:
            query['_id.servicePath'] = {'$exists': False}

        c = db[COL].find(query).count()
        if c > 1:
            warn('<%s> has duplicated entities with same _id subfields: %d' % (str(doc['_id']), c))

check_dup_attrs()
check_dup_id_subfields()
