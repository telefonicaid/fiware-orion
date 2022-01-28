#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
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

# Hint: use 'PYTHONIOENCODING=utf8 python check_legacy_location_metadata.py'
# if you are going to redirect the output of this script to a file


__author__ = 'fermin'

from pymongo import MongoClient
import sys


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


def entity_id(doc):
    """
    Extracts entity identification keys and returns an object with them

    :param doc: entity document, as taken from DB
    :return: {id, type, servicePath} object
    """
    id = doc['_id']['id']
    type = doc['_id']['type']
    sp = doc['_id']['servicePath']

    return {'id': id, 'type': type, 'servicePath': sp}


##########################
# Main program starts here

autofix = False

if len(sys.argv) != 2:
    print "invalid number of arguments, please check https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_3-5-0/index.html"
    sys.exit()

DB = sys.argv[1]

# Warn user
if autofix:
    print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
    print "do a backup of your database before using it as described in https://fiware-orion.readthedocs.io/en/master/admin/database_admin/index.html#backup. Use this script at your own risk."
    print "If you are sure you want to continue type 'yes' and press Enter"

    confirm = raw_input()

    if confirm != 'yes':
        sys.exit()

uri = 'mongodb://localhost:27017'
client = MongoClient(uri)
db = client[DB]

need_fix = False
corrupted = False
verbose = True

# Counters
processed = 0
counter_location_ngsiv2 = 0
counter_location_md_wgs84 = 0
counter_location_md_not_wgs84 = 0
counter_location_more_than_one = 0
counter_untouched = 0
counter_changed = 0

corrupted_entities = []
affected_entities = []

total = db['entities'].count()

print "- processing entities collection (%d entities) looking for attributes with ID metadata, this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enter again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expire at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is a heuristic
for entity in db['entities'].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    processed += 1

    sys.stdout.write('- processing entity: %d/%d   \r' % (processed, total))
    sys.stdout.flush()

    # It may happen that entity doesn't have any attribute. We early detect that situation and skip in that case
    if len(entity['attrs'].keys()) == 0:
        # print '- %d: entity without attributes %s. Skipping' % (processed, json.dumps(entity['_id']))
        continue  # entities loop

    wgs84_attr = None
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr] and 'location' in entity['attrs'][attr]['md']:
            if entity['attrs'][attr]['md']['location']['value'] == 'WGS84' \
                    or entity['attrs'][attr]['md']['location']['value'] == 'WSG84':
                if wgs84_attr is not None:
                    # more than one location metadata is not allowed due to the checks done by CB
                    # in processLocationAtEntityCreation() function. However, we check in
                    # any case as CB could be buggy. Note in this case we don't append to
                    # affected_entities as that was done first time the location metadata was detected
                    counter_location_more_than_one += 1
                    corrupted = True
                    corrupted_entities.append(entity_id(entity))
                    continue  # entities loop
                else:
                    # Note that location metadata doesn't have any semantic in NGSIv2, so we can
                    # have an entity created with NGSIv2 with location metadata but not location geo-index.
                    # We need to detect that situation
                    if "location" in entity:
                        counter_location_md_wgs84 += 1
                        affected_entities.append(entity_id(entity))
                        wgs84_attr = attr
                    else:
                        counter_location_ngsiv2 += 1
            else:
                # location metadata with a value different to WGS84 is not possible taking into
                # account the checks done by CB in processLocationAtEntityCreation() function
                # However, we check in any case as CB could be buggy
                counter_location_md_not_wgs84 += 1
                corrupted = True
                corrupted_entities.append(entity_id(entity))
                continue  # entities loop

    if wgs84_attr is not None:
        if autofix:
            # Autofix consist on:
            # 1) Remove location metadata (key in 'md' and item in 'mdNames')
            # 2) Change attribute type by "geo:point"

            attr = entity['attrs'][wgs84_attr]
            attr['mdNames'] = list(filter(lambda x: x != 'location', attr['mdNames']))

            attr['md'].pop('location', None)
            if len(attr['md'].keys()) == 0:
                attr.pop('md', None)

            attr['type'] = 'geo:point'

            # it would be easier db['entities'].save(entity) but it seems it has problems when
            # _id value is an object and may lead to duplicating entities instead of updating
            # note we removed _id from update doc, to avoid possible problems reported by
            # pymongo such as "the (immutable) field '_id' was found to have been altered"
            query = flatten(entity['_id'])
            entity.pop('_id', None)
            db['entities'].update(query, entity)
            counter_changed += 1
        else:
            # Fix should be done by the user
            counter_untouched += 1
            need_fix = True
    else:
        counter_untouched += 1

print '- processing entity: %d/%d' % (processed, total)
print '- documents analyzed:                                                  %d' % processed
print '  * entities w/ location md w/  WGS84/WSG84 value:                     %d' % counter_location_md_wgs84
print '  * entities w/ location md w/o WGS84/WSG84 value (DB corruption!):    %d' % counter_location_md_not_wgs84
print '  * entities w/ more than one location md (DB corruption!):            %d' % counter_location_more_than_one
print '  * entities w/ meaningless location md (created by NGSIv2)            %d' % counter_location_ngsiv2
print '- documents processed:                                                 %d' % processed
print '  * untouched:                                                         %d' % counter_untouched
print '  * changed:                                                           %d' % counter_changed

if verbose:
    if len(affected_entities) > 0:
        print '- Affected entities:'
        for entity in affected_entities:
            print '  * ' + str(entity)
    if len(corrupted_entities) > 0:
        print '- Corrupted entities:'
        for entity in corrupted_entities:
            print '  * ' + str(entity)

if need_fix or corrupted:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please check the documentation at https://fiware-orion.readthedocs.io/en/master/admin/upgrading_crossing_3-5-0/index.html"
