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
from datetime import datetime
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


def extract_attr_with_md_id(attrs):
    """
    Given a key-value of attributes, returns a dictionary with all attributes with metadata ID.
    The key in the dictionary is the attribute name and the value a list of its IDs. If attrs
    doesn't have any metadata ID, then an empty dictionary will be returned

    :param attrs: key-value of attribute to process
    :return: a dictionary as described above
    """

    r = {}
    for attr in attrs.keys():
        if attr.find('()') > 0:
            tokens = attr.split("()")
            name = tokens[0]
            md_id = tokens[1]
            if name in r.keys():
                r[name].append(md_id)
            else:
                r[name] = [md_id]
    return r


def date2string(time):
    """
    Convert date to string

    :param time: date time (as timestamp)
    :return: string representing the date time
    """

    return datetime.fromtimestamp(time).strftime("%Y-%m-%dT%H:%M:%SZ")


def fix_entity(entity, attrs_with_md_id):
    """
    Attemp to fix an entity.
    :params entity: entity document
    :param attrs_with_md_id: the attrs used to fix
    :return: 'untouched', 'changed', 'error', depending of the result
    """

    new_attrs = dict(entity[ATTRS])
    new_attrnames = list(entity[ATTRNAMES])
    check_attrs = []
    entity_touched = False
    if autofix == 'as_new_attrs':
        for base_name in attrs_with_md_id.keys():
            for md_id in attrs_with_md_id[base_name]:
                # Composing old name and new name, eg. temperature()ID1 and temperature:ID1
                old_name = base_name + '()' + md_id
                new_name = base_name + ':' + md_id

                # Remove old base-name from attrNames list (e.g. temperature)...
                if base_name in new_attrnames:
                    new_attrnames.remove(base_name)

                # ...and add the new one (temperature:ID1). Previous check of temperature:ID1 is not already
                # in the list: that will be an unsolvable situation
                if new_name in new_attrnames:
                    print "     * ERROR: attribute <{}> already exist in entity. Cannot be automatically fixed".format(new_name)
                    return 'error'
                else:
                    new_attrnames.append(new_name)

                    # Changing temperature()ID1 for temperature:ID1 in the attrs key-map
                    new_attrs[new_name] = new_attrs.pop(old_name)

                    check_attrs.append(new_name)

                    entity_touched = True

    elif autofix == 'as_metadata':
        # This is possible only if there isn't any attribute with more than one ID
        for base_name in attrs_with_md_id.keys():
            if len(attrs_with_md_id[base_name]) > 1:
                print '     * ERROR: entity has at least one attribute with more than one ID. Cannot be automaticaly fixed.'
                return 'error'

        # Composing old name and new name, eg. temperature()ID1 and temperature:ID1
        md_id = attrs_with_md_id[base_name][0]
        old_name = base_name + '()' + md_id

        # Changing temperature()ID1 for temperature in the attrs key-map...
        new_attrs[base_name] = new_attrs.pop(old_name)

        # ...and add the corresponding metadata ID (as regular metatada)
        new_md_id = {
            'type': 'string',   # This is the hardwired type used for metadata ID in Orion <= 1.13.0
            'value': md_id
        }
        if 'md' in new_attrs[base_name].keys():
            new_attrs[base_name]['md']['id'] = new_md_id
        else:
            new_attrs[base_name]['md'] = {'id': new_md_id}

        entity_touched = True

    # Now that new_attrs and new_attrsname are prepared, lest's do the actual update at MongoDB
    if entity_touched:
        # Update document with the new attribute fields
        db[COL].update(flatten(doc['_id']), {'$set': {ATTRS: new_attrs, ATTRNAMES: new_attrnames}})

        # Check update was ok (this is not an exhaustive checking that is better than nothing :)
        check_doc = db[COL].find_one(flatten(doc['_id']))

        if update_ok(check_doc, check_attrs):
            return 'changed'
        else:
            print '     * ERROR: document <{}> change attempt failed!'.format(json.dumps(check_doc['_id']))
            return 'error'
    else:
        return 'untouched'


##########################
# Main program starts here

autofix = None

if len(sys.argv) != 2:
    print "invalid number of arguments, please check https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-14-0/index.html"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

# Warn user
if autofix is not None:
    print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
    print "do a backup of your database before using it as described in https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html#backup. Use this script at your own risk."
    print "If you are sure you want to continue type 'yes' and press Enter"

    confirm = raw_input()

    if (confirm != 'yes'):
        sys.exit()

uri = 'mongodb://localhost:27017'
client = MongoClient(uri)
db = client[DB]

need_fix = False
processed = 0

# At the end, processed = no id + id duplicate attrs + id single attrs
counter_analysis = {
    'no': 0,
    'dup': 0,
    'single': 0,
}
# At the end, processed = untouched + changed + error
counter_update = {
    'untouched': 0,
    'changed': 0,
    'error': 0,
}

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
        counter_analysis['no'] += 1
        counter_update['untouched'] += 1
        continue   # entities loop

    attrs_with_md_id = extract_attr_with_md_id(doc[ATTRS])
    if len(attrs_with_md_id.keys()) > 0:
        print '- {}: entity {} ({}): metadata ID detected'.format(processed, json.dumps(doc['_id']),
                                                                  date2string(doc['modDate']))

        max_number_of_ids = 0
        for attr in attrs_with_md_id.keys():
            print '     {}: [ {} ]'.format(attr, ', '.join(attrs_with_md_id[attr]))
            if len(attrs_with_md_id[attr]) > max_number_of_ids:
                max_number_of_ids = len(attrs_with_md_id[attr])

        if max_number_of_ids > 1:
            counter_analysis['dup'] += 1
            autofix_as_metadata_not_possible = True
        else:
            counter_analysis['single'] += 1
    else:
        counter_analysis['no'] += 1
        counter_update['untouched'] += 1
        continue  # entities loop

    # Try to fix the entity
    result = fix_entity(doc, attrs_with_md_id)
    counter_update[result] += 1
    if result == 'error':
        need_fix = True

print '- processing entity: %d/%d' % (processed, total)
print '- documents analyzed:                                                 %d' % processed
print '  * entities w/o any attr w/ md ID:                                   %d' % counter_analysis['no']
print '  * entities w/ at least one attr w/ more than one ID:                %d' % counter_analysis['dup']
print '  * entities w/ at least one attr w/ md ID all them w/ single md ID:  %d' % counter_analysis['single']
print '- documents processed:                                                %d' % processed
print '  * untouched:                                                        %d' % counter_update['untouched']
print '  * changed: entities w/ at least one attr w/ more than one ID:       %d' % counter_update['changed']
print '  * attempt to change but error:                                      %d' % counter_update['error']

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please check the documentation at https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_1-14-0/index.html"
