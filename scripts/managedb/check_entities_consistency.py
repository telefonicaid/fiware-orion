#!/usr/bin/python
# -*- coding: utf-8 -*-
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

# Hint: use 'PYTHONIOENCODING=utf8 python check_metadata_id.py' if you are going to redirect the output of this
# script to a file

# Checks done:
#
# 1. heck that there is no duplication in _id fields (for mongo {_id: {x: 1, y: 2}} and {_id: {y:2, x:1}} are different documents)
# 2. All attribute in 'attrs' are in 'attrNames' and the other way around

__author__ = 'fermin'

from pymongo import MongoClient
import json
import sys
from datetime import datetime
from time import sleep

ATTRS = 'attrs'
ATTRNAMES = 'attrNames'

def date2string(time):
    """
    Convert date to string

    :param time: date time (as timestamp)
    :return: string representing the date time
    """

    return datetime.fromtimestamp(time).strftime("%Y-%m-%dT%H:%M:%SZ")


def base_name(attr):
    """
    Returns base name for attr, without take into account id (if any)
    :param attr:
    :return:
    """
    return attr.split('()')[0]

def check_entity_dup(entity_doc):
    """
    Check for entity duplicates in DB
    :param entity_doc: entity document
    :return: [], a list of errors otherwise
    """

    query = {'_id.id': entity_doc['_id']['id']}

    if 'type' in entity_doc['_id']:
        query['_id.type'] = entity_doc['_id']['type']
    else:
        query['_id.type'] = {'$exists': False}

    if 'servicePath' in entity_doc['_id']:
        query['_id.servicePath'] = entity_doc['_id']['servicePath']
    else:
        query['_id.servicePath'] = {'$exists': False}

    c = db[COL].find(query).count()
    if c > 1:
        return ['     * ERROR: duplicated entities number with same _id subfields: {0}'.format(c)]
    else:
        return []


def check_attrs(attrs, attr_names):
    """
    Check attrs

    :param attrs: key-value attrs
    :param attr_names: list with attr_names
    :return: [], a list of errors otherwise
    """

    r = []

    # We have found that some times attrs containts non-ASCII characters and printing can be problematic if
    # we don't use .encode('utf-8')

    # All in attrs is in attr_names
    for attr in attrs.keys():
        if base_name(attr).replace('=','.') not in attr_names:
            r.append('     * ERROR: base name of attr <{0}> in attrs keymap cannot be found in attrNames list'.format(attr.encode('utf-8')))

    # All in attr_names is in attrs
    for attr in attr_names:
        # map() used to get only the first token in attrs keys
        if attr not in map(lambda x: x.split('()')[0].replace('=','.'), attrs.keys()):
            r.append('     * ERROR: attr <{0}> in attrName list cannot be found in attrs keymap'.format(attr.encode('utf-8')))

    return r


##########################
# Main program starts here

if len(sys.argv) != 2:
    print "invalid number of arguments, please specifi db name (e.g. 'orion')"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

uri = 'mongodb://localhost:27017'
client = MongoClient(uri)
db = client[DB]

# At the end, n_processed = no_ok + n_not_ok
n_processed = 0
n_ok = 0
n_not_ok = 0

total = db[COL].count()

print "- checking entities collection (%d entities), this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    n_processed += 1

    sys.stdout.write('- processing entity: %d/%d   \r' % (n_processed, total) )
    sys.stdout.flush()


    errors = check_entity_dup(doc) + check_attrs(doc[ATTRS], doc[ATTRNAMES])

    if len(errors) == 0:
        n_ok += 1
    else:
        print '- {0}: entity {1} ({2}): metadata ID detected'.format(n_processed, json.dumps(doc['_id']), date2string(doc['modDate']))
        print '\n'.join(errors)
        n_not_ok += 1

print '- processing entity: %d/%d' % (n_processed, total)
print '- entities analyzed:                        %d' % n_processed
print '  * entities OK:                            %d' % n_ok
print '  * entities with consistency problems:     %d' % n_not_ok

"""
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

"""