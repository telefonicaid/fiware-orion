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


# This script is inspired by old attrsvector2attrsobject.py

__author__ = 'fermin'

from pymongo import MongoClient
import json
import sys
from time import sleep

CONDITIONS = "conditions"

def update_ok(doc, n_conditions):
    """
    Check that csub document was updated correctly at DB.

    :param doc: the doc to check
    :param n_conditions: the expected number of conditions in the document
    :return: True if the check is ok, False otherwise
    """

    if (not CONDITIONS in doc):
        print "debug1"
        return False

    ck_conditions = len(doc[CONDITIONS])

    if (n_conditions != ck_conditions):
        print "debug 2: %d %d" % (n_conditions, ck_conditions)
        return False

    return True

if len(sys.argv) != 2:
    print "invalid number of arguments, please check https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-3-0/index.html"
    sys.exit()

DB = sys.argv[1]
COL = 'csubs'

# Warn user
print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
print "do a backup of your database before using it as described in https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html#backup. Use this script at your own risk."
print "If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

client = MongoClient('localhost', 27017)
db = client[DB]

need_fix = False

skipped_conditions_not_found      = 0
skipped_conditions_not_vector     = 0
skipped_conditions_empty          = 0
skipped_conditions_without_object = 0
skipped_conditions_without_value  = 0
skipped_conditions_without_type   = 0
skipped_value_not_vector          = 0
skipped_invalid_type              = 0

changed     = 0
error       = 0
processed   = 0

total = db[COL].count()

print "- processing csubs collection (%d csubs) merging all condValues into a single conditions vector... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id', 1)]).batch_size(100):

    processed += 1

    sys.stdout.write('- processing csub: %d/%d   \r' % (processed, total) )
    sys.stdout.flush()

    n_sum_md_names = 0
    n_sum_mds      = 0
    n_attrs        = 0

    # Does conditions field exist?
    if not CONDITIONS in doc:
        print '- %d: csub without conditions field: %s. Skipping' % (processed, doc['_id'])
        skipped_conditions_not_found += 1
        continue   # csubs loop

    # Is it a vector?
    if not isinstance(doc[CONDITIONS], list):
        print '- %d: csub conditions field is not a vector: %s. Skipping' % (processed, doc['_id'])
        skipped_conditions_not_vector += 1
        continue   # csubs loop

    # Is empty?
    if len(doc[CONDITIONS]) == 0:
        print '- %d: csub conditions is empty: %s. Skipping' % (processed, doc['_id'])
        skipped_conditions_empty += 1
        continue   # csubs loop

    to_skip = False
    new_conditions = [ ]
    for cond in doc[CONDITIONS]:
        
        # Is it a object?
        if not isinstance(cond, dict):
            print '- %d: csub has condition that is not an object: %s. Skipping' % (processed, doc['_id'])
            skipped_conditions_without_object += 1
            to_skip = True
            break   # conds loop

        # Has type?
        if not 'type' in cond:
            print '- %d: csub has condition without type: %s. Skipping' % (processed, doc['_id'])
            skipped_conditions_without_type += 1
            to_skip = True
            break   # conds loop
        
        # Is ONCHANGE type?
        if cond['type'] != "ONCHANGE":
            print '- %d: csub has invalid type "%s": %s. Skipping' % (processed, cond['type'], doc['_id'])
            skipped_invalid_type += 1
            to_skip = True
            break   # conds loop

        # Has value?
        if not 'value' in cond:
            print '- %d: csub has condition without value: %s. Skipping' % (processed, doc['_id'])
            skipped_conditions_without_value += 1
            to_skip = True
            break   # conds loop

        # Is value a vector?
        if not isinstance(cond['value'], list):
            print '- %d: csub has condition which value is not a vector: %s. Skipping' % (processed, doc['_id'])
            skipped_value_not_vector += 1
            to_skip = True
            break   # conds loop

        for value in cond['value']:
            new_conditions.append(value)

    # Need to skip to next csub?
    if to_skip:
        continue    # csub loop 

    # Update document with the new condtions field
    db[COL].update({'_id': doc['_id']}, {'$set': {CONDITIONS: new_conditions}})

    # Check update was ok (this is not an exhaustive checking that is better than nothing :)
    check_doc = db[COL].find_one(doc['_id'])

    if update_ok(check_doc, len(new_conditions)):
        changed += 1
    else:
        print "- %d: ERROR: document <%s> change attempt failed!" % (processed, json.dumps(check_doc['_id']))
        need_fix = True
        error += 1

skipped = skipped_conditions_not_found + skipped_conditions_not_vector + skipped_conditions_empty + skipped_conditions_without_object + skipped_conditions_without_value + skipped_conditions_without_type + skipped_invalid_type + skipped_value_not_vector

print '- processing entity: %d/%d' % (processed, total)
print '- documents processed:   %d' % processed
print '  * changed:             %d' % changed
print '  * skipped:             %d' % skipped
print '    - conditions not found     %d' % skipped_conditions_not_found 
print '    - conditions not a vector  %d' % skipped_conditions_not_vector
print '    - empty conditions:        %d' % skipped_conditions_empty
print '    - condition not object     %d' % skipped_conditions_without_object
print '    - condition w/o value      %d' % skipped_conditions_without_value
print '    - condition w/o type       %d' % skipped_conditions_without_type 
print '    - invalid type             %d' % skipped_invalid_type
print '    - value not a vector       %d' % skipped_value_not_vector
print '  * error:               %d' % error

if skipped > 0:
    print "------------------------------------------------------"
    print "WARNING: some csub were skipped. Please check the documentation at https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-3-0/index.html"
