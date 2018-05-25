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
import json
import sys
from time import sleep

ATTRNAMES = 'attrNames'
ATTRS     = 'attrs'

# The way in which Python manage dictionaries doesn't make easy to be sure
# of field ordering, which is important for MongoDB in the case of using an
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

def update_ok(doc, n_attr_names, n_attrs):

    if (not ATTRNAMES in doc or not ATTRS in doc):
        print "debug1"
        return False

    ck_attr_names = len(doc[ATTRNAMES])
    ck_attrs = len(doc[ATTRS].keys())

    if (n_attr_names != ck_attr_names or n_attrs != ck_attrs):
        print "debug 2: %d %d %d %d" % (n_attr_names, ck_attr_names, ck_attrs, n_attrs)
        return False

    return True

if len(sys.argv) != 2:
    print "missing db name"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

# Warn user
print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
print "do a backup of your database before using it as described in https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Backup. Use this script at your own risk."
print "If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

client = MongoClient('localhost', 27017)
db = client[DB]

need_fix = False

skipped     = 0
changed     = 0
error       = 0
processed   = 0

total = db[COL].count()

print "- processing entities collection (%d entities) changing attrs vectors to objects, this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    processed += 1

    sys.stdout.write('- processing entity: %d/%d   \r' % (processed, total) )
    sys.stdout.flush()

    # Check that attribute field exists and is a vector (otherwise this entity is skipped)
    old_attrs = doc[ATTRS]
    if not isinstance(old_attrs, list):
        #print '- %d: not vector attribute for entity %s. Skipping' % (processed, json.dumps(doc['_id']))
        skipped += 1
        continue

    # Process attribute by attribute, storing them in a temporal hashmap and list
    attr_names = []
    attrs = {}


    to_skip = False
    for attr in old_attrs:
        name = attr.pop('name')
        # Does the attribute have an ID field?
        if 'id' in attr:
            id = attr.pop('id')
            attr_key = name + "__" + id
        else:
            attr_key = name

        if not name in attr_names:
            attr_names.append(name)

        if attr_key in attrs:
            print '- %d: dupplicate attribute detected in entity %s: <%s>. Skipping' % (processed, json.dumps(doc['_id']), attr_key)
            need_fix = True
            to_skip = True
            break

        # All '.' are transformed into '=' given that MongoDB doesn't allow '.' in key names
        attr_key = attr_key.replace('.', '=')

        attrs[attr_key] = attr

    if to_skip:
        skipped += 1
        continue

    n_attr_names = len(attr_names)
    n_attrs = len(attrs.keys())

    # Update document with the new attribute fields
    db[COL].update(flatten(doc['_id']), {'$set': {ATTRNAMES: attr_names, ATTRS: attrs}})

    # Check update was ok (this is not an exhaustive checking that is better than nothing :)
    check_doc = db[COL].find_one(flatten(doc['_id']))

    if update_ok(check_doc, n_attr_names, n_attrs):
        changed += 1
    else:
        print "- %d: ERROR: document <%s> change attempt failed!" % (processed, json.dumps(check_doc['_id']))
        need_fix = True
        error += 1

print '- processing entity: %d/%d' % (processed, total)
print '- documents processed:   %d' % processed
print '  * changed:             %d' % changed
print '  * skipped:             %d' % skipped
print '  * error:               %d' % error

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please, check the documentation at https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Upgrading_to_0.21.0_and_beyond_from_any_pre-0.21.0_version for solving it"
