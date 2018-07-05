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

ATTRS     = 'attrs'
MDS       = 'md'
MD_NAMES  = 'mdNames'

def flatten(_id):
    """
    The way in which Python manage dictionaries doesn't make easy to be sure
    of field ordering, which is important for MongoDB in the case of using an
    embedded document for _id. Thist function helps.

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

def update_ok(doc, n_attrs, n_sum_md_names, n_sum_mds):
    """
    Check that entity document was updated correctly at DB.

    :param doc: the doc to check
    :param n_attrs: the expected number of attributes in attrNames vector
    :param n_sum_md_names: the expected sum of elements in mdNames vectors
    :param n_sum_mds: the expected sum of elements in md key-maps
    :return: True if the check is ok, False otherwise
    """

    if (not ATTRS in doc):
        print "debug1"
        return False

    ck_attrs = len(doc[ATTRS].keys())

    ck_sum_md_names = 0
    ck_sum_mds      = 0 
    for attr in doc[ATTRS].keys():

        # By construction, mdNames is always there (sometimes empty), but mds may be or not
        md_names = doc[ATTRS][attr][MD_NAMES]
        ck_sum_md_names += len(md_names)

        if MDS in doc[ATTRS][attr].keys():        
            mds         = doc[ATTRS][attr][MDS]            
            ck_sum_mds  += len(mds.keys())


    if (n_attrs != ck_attrs or n_sum_md_names != ck_sum_md_names or n_sum_mds != ck_sum_mds):
        print "debug 2: %d %d %d %d %d %d" % (ck_attrs, n_attrs, n_sum_md_names, ck_sum_md_names, n_sum_mds, ck_sum_mds)
        return False

    return True

if len(sys.argv) != 2:
    print "invalid number of arguments, please check https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-3-0/index.html"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

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

skipped_noattr   = 0
skipped_mdnames  = 0
skipped_novector = 0
skipped_dupmd    = 0

changed     = 0
error       = 0
processed   = 0

total = db[COL].count()

print "- processing entities collection (%d entities) changing metadata vectors to objects, this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    processed += 1

    sys.stdout.write('- processing entity: %d/%d   \r' % (processed, total) )
    sys.stdout.flush()

    n_sum_md_names = 0
    n_sum_mds      = 0
    n_attrs        = 0

    # It may happen that entity doesn't have any attribute. We early detect that situation and skip in that case
    if len(doc[ATTRS].keys()) == 0:
        #print '- %d: entity without attributes %s. Skipping' % (processed, json.dumps(doc['_id']))
        skipped_noattr += 1
        continue   # entities loop

    # Processing attribute by attribute
    to_skip = False
    for attr in doc[ATTRS].keys():

       n_attrs += 1

       # If attribute has 'mdNames' field, then we can assume this script has already touched that entity, thus the entity is skipped
       if MD_NAMES in doc[ATTRS][attr].keys():
           #print '- %d: mdNames found for entity %s (attribute %s). Skipping' % (processed, json.dumps(doc['_id']), attr)
           to_skip = True
           skipped_mdnames += 1
           break    # attrs loop

       # If attribute doesn't have metadata, then fill with empty mdNames and, skip to the next attribute
       if not MDS in doc[ATTRS][attr].keys():           
           doc[ATTRS][attr][MD_NAMES] = []
           continue # attrs loop

       old_mds = doc[ATTRS][attr][MDS]
       if not isinstance(old_mds, list):
           #print '- %d: not vector md for entity %s (attribute %s). Skipping' % (processed, json.dumps(doc['_id']), attr)
           to_skip = True
           skipped_novector += 1
           break    # attrs loop
            
       # Process metadata by metadata, storing them in a temporal hashmap and list
       md_names = []
       mds = {}

       for md in old_mds:
           name = md.pop('name')

           if name in mds:
               print '- %d: duplicate metadata detected in entity %s (attribute %s): <%s>. Skipping' % (processed, json.dumps(doc['_id']), attr, name)
               need_fix = True
               to_skip = True
               skipped_dupmd += 1
               break   # mds loop
              
           # Append to mdNames field done *before* '.' replacement operation
           md_names.append(name)

           # All '.' are transformed into '=' given that MongoDB doesn't allow '.' in key names
           name = name.replace('.', '=')

           mds[name] = md

       if to_skip:
           break   # attrs loop

       n_sum_md_names += len(md_names)
       n_sum_mds      += len(mds.keys())

       doc[ATTRS][attr][MDS]      = mds
       doc[ATTRS][attr][MD_NAMES] = md_names;

    # Need to skip to next entity?
    if to_skip:
        continue    # entities loop 

    # Update document with the new attribute field
    db[COL].update(flatten(doc['_id']), {'$set': {ATTRS: doc[ATTRS]}})

    # Check update was ok (this is not an exhaustive checking that is better than nothing :)
    check_doc = db[COL].find_one(flatten(doc['_id']))

    if update_ok(check_doc, n_attrs, n_sum_md_names, n_sum_mds):
        changed += 1
    else:
        print "- %d: ERROR: document <%s> change attempt failed!" % (processed, json.dumps(check_doc['_id']))
        need_fix = True
        error += 1

skipped = skipped_noattr + skipped_mdnames + skipped_novector + skipped_dupmd


print '- processing entity: %d/%d' % (processed, total)
print '- documents processed:   %d' % processed
print '  * changed:             %d' % changed
print '  * skipped:             %d' % skipped
print '    - no attrs           %d' % skipped_noattr
print '    - md names           %d' % skipped_mdnames
print '    - no as vector       %d' % skipped_novector
print '    - dup md             %d' % skipped_dupmd
print '  * error:               %d' % error

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please check the documentation at https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-3-0/index.html"
