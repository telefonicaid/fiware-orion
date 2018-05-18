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


__author__ = 'fermin'

from pymongo import MongoClient
import json
import sys
from time import sleep

ATTRS = 'attrs'
ATTRNAMES = 'attrNames'

def flatten(_id):
    """
    The way in which Python manage dictionaries doesn't make easy to be sure
    of field ordering, which is important for MongoDB in the case of using an
    embedded document for _id. This function helps.

    :param _id: JSON document containing id, type and servicePath
    :return: a "flatten" version of the _id
    """

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

def update_ok(doc, check_attrs):
    """
    Check that entity document was updated correctly at DB.

    :param doc: the doc to check
    :param check_attrs: list of attributes which existente is checked
    """

    if not ATTRS in doc:
        #print "debug1: no attrs"
        return False

    for attr in check_attrs:

        if attr not in doc[ATTRS]:
            #print "debug2: %s" % attr
            return False

    return True


##########################
# Main program starts here

autofix = False

if len(sys.argv) != 2:
    print "invalid number of arguments, please check https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-14-0/index.html"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

# Warn user
if autofix:
    print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
    print "do a backup of your database before using it as described in https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html#backup. Use this script at your own risk."
    print "If you are sure you want to continue type 'yes' and press Enter"

    confirm = raw_input()

    if (confirm != 'yes'):
        sys.exit()

client = MongoClient('localhost', 27017)
db = client[DB]

need_fix = False

untouched   = 0
changed     = 0
error       = 0
processed   = 0

total = db[COL].count()

print "- processing entities collection (%d entities) looking for attributes with ID metadata, this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    processed += 1

    sys.stdout.write('- processing entity: %d/%d   \r' % (processed, total) )
    sys.stdout.flush()

    # It may happen that entity doesn't have any attribute. We early detect that situation and skip in that case
    if len(doc[ATTRS].keys()) == 0:
        #print '- %d: entity without attributes %s. Skipping' % (processed, json.dumps(doc['_id']))
        untouched += 1
        continue   # entities loop

    # Processing attribute by attribute
    to_skip = True
    new_attrs = dict(doc[ATTRS])
    new_attrnames = list(doc[ATTRNAMES])
    check_attrs = []

    for attr in doc[ATTRS].keys():

        if attr.find("()") > 0:
            # Note that Orion <=1.13.0 doesn't allows attributes with same name with ID and not ID at the same time
            # in the same entity, so we don't need to take into account that situation.

            tokens = attr.split("()")
            name = tokens[0]
            id = tokens[1]
            print "- entity {}: found attr <{}>, metadata id <{}>, value <{}>".format(doc['_id'], name, id,
                                                                                      doc[ATTRS][attr]['value'])

            if autofix:
                # Prepare modification in new_attrs and new_attrnames

                # Composing new name, eg. temperature:ID1
                new_name = name + ':' + id

                # Remove all name from attrNames list (e.g. temperature)...
                if name in new_attrnames:
                    new_attrnames.remove(name)

                # ...and add the new one (temperature:ID1). Previous check of temperature:ID1 is not already
                # in the list: that will be an unsolvable situation
                if new_name in new_attrnames:
                    print "- ERROR: attribute <{}> already exist in entity {}. Cannot be automatically fixed".format(new_name, doc['_id'])
                    need_fix = True
                else:
                    new_attrnames.append(new_name)

                    # Changing temperature()ID1 for temperature:ID1 in the attrs key-map
                    new_attrs[new_name] = new_attrs.pop(attr)

                    check_attrs.append(new_name)

                    to_skip = False

    # Need to skip to next entity? Note that if autofix==False to_skip is always True
    if to_skip:
        if need_fix:
            error += 1
        else:
            untouched += 1
        continue    # entities loop 

    # Update document with the new attribute fields
    db[COL].update(flatten(doc['_id']), {'$set': {ATTRS: new_attrs, ATTRNAMES: new_attrnames}})

    # Check update was ok (this is not an exhaustive checking that is better than nothing :)
    check_doc = db[COL].find_one(flatten(doc['_id']))

    if update_ok(check_doc, check_attrs):
        changed += 1
    else:
        print "- %d: ERROR: document <%s> change attempt failed!" % (processed, json.dumps(check_doc['_id']))
        need_fix = True
        error += 1

print '- processing entity: %d/%d' % (processed, total)
print '- documents processed:   %d' % processed
print '  * changed:             %d' % changed
print '  * untouched:           %d' % untouched
print '  * error:               %d' % error

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please check the documentation at https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-14-0/index.html"
