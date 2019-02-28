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

# This script requires:
#
#   pymongo==3.0.3
#   geojson==2.4.1

__author__ = 'fermin'

import sys
import json
import pymongo
import geojson
from datetime import datetime


##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

uri = 'mongodb://localhost:27017'

autofix = False

verbose = False

# END of the configuration part (don't touch below this line ;)
##############################################################


ATTRS = 'attrs'
GEO_TYPES = [ 'geo:point', 'geo:line', 'geo:box', 'geo:polygon', 'geo:json']

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


def fix_location_geopoint(entity, geo_attr, coords):
    """
    Fix location entity (geo:point), adding the missing field.

    :param entity: entity to fix
    :param geo_attr: name of the geo:point attribute
    :param coords: coordinates of the geo:point attribute
    :return: "OK" if update when ok, "FAIL xxxx" otherwise
    """

    # Location example for reference:
    # { "attrName" : "position", "coords" : { "type" : "Point", "coordinates" : [ -1.52363, 42.9113 ] } }

    # Get coordinates and revert it (GeoJSON uses that)
    try:
        # Sanity check
        if len(coords.split(',')) != 2:
            raise ValueError

        coordinates = [float(coords.split(',')[1]), float(coords.split(',')[0])]
    except ValueError:
        return 'FAIL coordinates parsing error <%s>' % coords

    location = {
        'attrName': geo_attr,
        'coords': {
            'type': 'Point',
            'coordinates': coordinates
        }
    }

    try:
        # Update document with the new attribute fields
        db[COL].update(flatten(entity['_id']), {'$set': {'location': location}})

        # Check update was ok (this is not an exhaustive checking that is better than nothing :)
        check_doc = db[COL].find_one(flatten(entity['_id']))

        if update_ok(check_doc, entity[ATTRS]):
            return 'OK'
        else:
            return 'FAIL attrs check after update in DB'

    except pymongo.errors.WriteError as e:
        return 'FAIL mongo error %d - location was %s' % (e.code, json.dumps(location))


def fix_empty_geopoint(entity, geo_attr):
    """
    Fix location with empty geo:point, seting it to 0,0

    :param entity: entity to fix
    :param geo_attr: name of the geo:point attribute
    :return: "OK" if update when ok, "FAIL xxxx" otherwise
    """

    # Location example for reference:
    # { "attrName" : "position", "coords" : { "type" : "Point", "coordinates" : [ -1.52363, 42.9113 ] } }

    entity[ATTRS][geo_attr]['value'] = '0,0'

    location = {
        'attrName': geo_attr,
        'coords': {
            'type': 'Point',
            'coordinates': [0, 0]
        }
    }

    try:
        # Update document with the new attribute fields
        db[COL].update(flatten(entity['_id']), {'$set': {ATTRS: entity[ATTRS], 'location': location}})

        # Check update was ok (this is not an exhaustive checking that is better than nothing :)
        check_doc = db[COL].find_one(flatten(entity['_id']))

        if update_ok(check_doc, entity[ATTRS]):
            return 'OK'
        else:
            return 'FAIL attrs check after update in DB'

    except pymongo.errors.WriteError as e:
        return 'FAIL mongo error %d - location was %s' % (e.code, json.dumps(location))


def str2number(d):
    """
    Returns a vector of parsed numbers (other elements are not touched)

    :param d: the vector to parse
    :return: the parsed vector
    """

    if type(d) == list:
        return map(str2number, d)
    else:
        try:
            return float(d)
        except:  # Not a float, leave as it is
            return d


def fix_location_geojson(entity, geo_attr, geo_json):
    """
    Fix location entity (geo:json), adding the missing field.

    :param entity: entity to fix
    :param geo_attr: name of the geo:json attribute
    :param geo_json: GeoJSON object to use for location
    :return: "OK" if update when ok, "FAIL xxxx" otherwise
    """

    # Location example for reference:
    # { "attrName" : "position", "coords" : { "type" : "Point", "coordinates" : [ -1.52363, 42.9113 ] } }

    if type(geo_json) != dict or not 'type' in geo_json.keys() or not 'coordinates' in geo_json.keys():
        return 'FAIL not a valid GeoJSON: %s' % json.dumps(geo_json)

    # If the geo:json was updated using NGSIv1, numbers would be string and the GeoJSON will not be valid. Thus,
    # we first parse strings to number
    geo_json_parsed = {
        'type': geo_json['type'],
        'coordinates': str2number(geo_json['coordinates'])
    }

    # This is weird... :)
    #
    # As far as I have checked and at least for this version of geojson module (2.4.1), the geojson.loads(string
    # function returns either a geojson object (if string corresponds to a valid GeoJSON) or the string
    # passed as argument. In the first case, is_valid is a property of the object to check for True. In the
    # second case a AttributeError: 'dict' object has no attribute 'is_valid' error is raised

    try:
        if not geojson.loads(json.dumps(geo_json_parsed)).is_valid:
            return 'FAIL not a valid GeoJSON: %s' % json.dumps(geo_json_parsed)
    except:
        return 'FAIL not a valid GeoJSON: %s' % json.dumps(geo_json_parsed)

    location = {
        'attrName': geo_attr,
        'coords': geo_json_parsed
    }

    try:
        # Update document with the new attribute fields
        db[COL].update(flatten(entity['_id']), {'$set': {'location': location}})

        # Check update was ok (this is not an exhaustive checking that is better than nothing :)
        check_doc = db[COL].find_one(flatten(entity['_id']))

        if update_ok(check_doc, entity[ATTRS]):
            return 'OK'
        else:
            return 'FAIL attrs check after update in DB'

    except pymongo.errors.WriteError as e:
        return 'FAIL mongo error %d - location was %s' % (e.code, json.dumps(location))


def add_loc_point_attr(entity, location):
    """
    Fix entity adding an attribute that match the location (of type Point)

    :param entity: the entity to fix
    :param location: the location field
    :return:
    """

    # Location example for reference:
    # { "attrName" : "position", "coords" : { "type" : "Point", "coordinates" : [ -1.52363, 42.9113 ] } }

    attr_name = location['attrName']

    # Get coordinates and revert it (GeoJSON uses that)
    coords0 = location['coords']['coordinates'][0]
    coords1 = location['coords']['coordinates'][1]

    # Note coordinates are in opposite orden in attribute that in location
    attr = {
        'type': 'geo:point',
        'value': "%d, %d" % (coords1, coords0)
    }

    entity[ATTRS][attr_name] = attr

    try:
        # Update document with the new attribute fields
        db[COL].update(flatten(entity['_id']), {'$set': {ATTRS: entity[ATTRS]}})

        # Check update was ok (this is not an exhaustive checking that is better than nothing :)
        check_doc = db[COL].find_one(flatten(entity['_id']))

        if update_ok(check_doc, entity[ATTRS]):
            return 'OK'
        else:
            return 'FAIL attrs check after update in DB'

    except pymongo.errors.WriteError as e:
        return 'FAIL mongo error %d - location was %s' % (e.code, json.dumps(location))



def extract_geo_attr(attrs):
    """
    Given a key-value of attributes, returns the attribute with geo: type or None
    if no geo: attribute is found.

    :param attrs: key-value of attribute to process
    :return: an attribute name or None
    """

    for attr in attrs.keys():
        if attrs[attr]['type'] in GEO_TYPES:
            return attr

    return None


def check_ngsiv1_location(attrs, location):
    """
    Check if there is an attribute in the entity which match location field in the "NGSIv1 location way", i.e.
    attribute type doesn't means anything but the value coordinates format matching the location

    :param attrs: entity attributes object to look
    :param location: location field
    :return: TRue if check is ok, False otherwise
    """

    # Location example for reference:
    # { "attrName" : "position", "coords" : { "type" : "Point", "coordinates" : [ -1.52363, 42.9113 ] } }

    # Only Point is allowed in NGSIv1
    if location['coords']['type'] != 'Point':
        return False

    loc_attr = None
    for attr in attrs.keys():
        if attr == location['attrName']:
            loc_attr = attrs[attr]
            break

    if loc_attr is None:
        return False

    value = loc_attr['value']

    # Coordinates format check
    if len(value.split(',')) != 2:
        return False

    # Check coordinates are equal
    try:
        if float(value.split(',')[0]) != location['coords']['coordinates'][1]:
            return False
        if float(value.split(',')[1]) != location['coords']['coordinates'][0]:
            return False

    except ValueError:
        # Format error: some of the token is not a valid float
        return False

    return True


def date2string(time):
    """
    Convert date to string

    :param time: date time (as timestamp)
    :return: string representing the date time
    """

    return datetime.fromtimestamp(time).strftime("%Y-%m-%dT%H:%M:%SZ")


def safe_add(d, k):
    """
    Add or create a key to dictionary if it doesn't already exists.

    :param d:
    :param k:
    :return:
    """

    if not k in d.keys():
        d[k] = True


def msg(m):
    """
    Print a message if verbose is enabled
    :param m: message to print
    :return:
    """

    if verbose:
        print m


##########################
# Main program starts here

if len(sys.argv) != 2:
    print "invalid number of arguments, use: ./check_location_coherence.py <database_name>"
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

client = pymongo.MongoClient(uri)
db = client[DB]

need_help = False
processed = 0

# At the end, processed = no id + id duplicate attrs + id single attrs
counter_analysis = {
    'ngeo-nloc': 0,
    'geo-loc': 0,
    'geopoint-nloc': 0,
    'geojson-nloc': 0,
    'ngeo-loc': 0,
    'legacy': 0,
    'ngeo-locpoint': 0,
    'unknown-geo': 0,
    'corrupted-location': 0,
    'emptygeopoint': 0,
}
# At the end, processed = untouched + changed + error
counter_update = {
    'untouched': 0,
    'changed': 0,
    'error': 0,
}

not_fixable_types_found = {}
location_types_found = {}

total = db[COL].count()

print "- processing entities collection (%d entities) looking for location field coherence, this may take a while... " % total

# The sort() is a way of ensuring that a modified document doesn't enters again at the end of the cursor (we have
# observed that this may happen with large collections, e.g ~50,000 entities). In addition, we have to use
# batch_size so the cursor doesn't expires at server (see http://stackoverflow.com/questions/10298354/mongodb-cursor-id-not-valid-error).
# The used batch_size value is an heuristic
for doc in db[COL].find().sort([('_id.id', 1), ('_id.type', -1), ('_id.servicePath', 1)]).batch_size(100):

    processed += 1

    # Progress meter
    #sys.stdout.write('- processing entity: %d/%d   \r' % (processed, total) )
    #sys.stdout.flush()

    location = None
    geo_attr = extract_geo_attr(doc[ATTRS])
    if 'location' in doc:
        location = doc['location']

    if location is None:
        if geo_attr is None:
            # Entity without geo: attribute and without location field. It's ok.
            counter_analysis['ngeo-nloc'] += 1
            counter_update['untouched'] += 1
        else:  # geo_attr is not None
            geo_type = doc[ATTRS][geo_attr]['type']
            geo_value = doc[ATTRS][geo_attr]['value']

            if geo_type == 'geo:point':

                if geo_value == '':
                    # Entity with empty geo:point is a degenerated case. Fixable, setting 0,0 as coordinates.
                    counter_analysis['emptygeopoint'] += 1
                    if autofix:
                        result = fix_empty_geopoint(doc, geo_attr)
                        msg('   - {0}: fixing empty geo:point {1} ({2}): {3}'.format(processed, json.dumps(doc['_id']),
                                                                                     date2string(doc['modDate']), result))
                        if result == 'OK':
                            counter_update['changed'] += 1
                        else:
                            counter_update['error'] += 1
                            need_help = True
                    else:
                        msg('   - {0}: empty geo:point {1} ({2})'.format(processed, json.dumps(doc['_id']),
                                                                         date2string(doc['modDate'])))

                        counter_update['untouched'] += 1
                        need_help = True
                else:
                    # Entity with geo:point attribute but without location field. Fixable
                    counter_analysis['geopoint-nloc'] += 1
                    if autofix:
                        result = fix_location_geopoint(doc, geo_attr, geo_value)
                        msg('   - {0}: fixing loc (geo:point) {1} ({2}): {3}'.format(processed, json.dumps(doc['_id']),
                                                                                     date2string(doc['modDate']), result))
                        if result == 'OK':
                            counter_update['changed'] += 1
                        else:
                            counter_update['error'] += 1
                            need_help = True
                    else:
                        msg('   - {0}: entity w/ geo:point but wo/ location  {1} ({2})'.format(processed, json.dumps(doc['_id']),
                                                                                               date2string(doc['modDate'])))
                        counter_update['untouched'] += 1
                        need_help = True

            elif geo_type == 'geo:json':
                # Entity with geo:point attribute but without location field. Fixable
                counter_analysis['geojson-nloc'] += 1
                if autofix:
                    result = fix_location_geojson(doc, geo_attr, geo_value)
                    msg('   - {0}: fixing loc (geo:json) {1} ({2}): {3}'.format(processed, json.dumps(doc['_id']),
                                                                                date2string(doc['modDate']), result))
                    if result == 'OK':
                        counter_update['changed'] += 1
                    else:
                        counter_update['error'] += 1
                        need_help = True
                else:
                    msg('   - {0}: entity w/ geo:json but wo/ location  {1} ({2})'.format(processed,
                                                                                          json.dumps(doc['_id']),
                                                                                          date2string(doc['modDate'])))
                    counter_update['untouched'] += 1
                    need_help = True

            else:
                # Entity with geo: attribute different than geo:point or geo:json but without location field. Not fixable
                msg('   - {0}: unfixable {1} {2} ({3})'.format(processed, geo_type, json.dumps(doc['_id']),
                                                               date2string(doc['modDate'])))


                safe_add(not_fixable_types_found, geo_type)

                counter_analysis['unknown-geo'] += 1
                counter_update['untouched'] += 1
                need_help = True

    else:  # location is not None
        try:
            loc_type = location['coords']['type']
            loc_attr = location['attrName']

            if geo_attr is None:
                # Entity without geo: attribute location but with location field. They may come from NGSIv1
                # or be a real problem.
                if loc_type == 'Point' and check_ngsiv1_location(doc[ATTRS], location):
                    counter_analysis['legacy'] += 1
                    counter_update['untouched'] += 1
                else:
                    if loc_type == 'Point' and not loc_attr in doc[ATTRS]:
                        # Location type is Point and attribute doesn't not exist. We can fix adding the corresponding
                        # attribute
                        counter_analysis['ngeo-locpoint'] += 1
                        if autofix:
                            result = add_loc_point_attr(doc, location)
                            msg('   - {0}: fixing loc (Point) {1} ({2}): {3}'.format(processed, json.dumps(doc['_id']),
                                                     date2string(doc['modDate']), result))
                            if result == 'OK':
                                counter_update['changed'] += 1
                            else:
                                counter_update['error'] += 1
                                need_help = True
                        else:
                            msg('   - {0}: entity w/ location Point but wo/ geo:point or NGSIv1 point  {1} ({2})'.format(processed,
                                                                                              json.dumps(doc['_id']),
                                                                                              date2string(doc['modDate'])))
                            counter_update['untouched'] += 1
                            need_help = True
                    else:
                        # Location type is not point. Not fixable
                        counter_analysis['ngeo-loc'] += 1
                        counter_update['untouched'] += 1
                        safe_add(location_types_found, loc_type)
                        need_help = True

            else:  # geo_attr is not None
                # Entity with geo: attribute and with location field. It's ok
                # FIXME: it may happen that the GeoJSON at location field doesn't correspond with the one calculated
                # from geo: attribute. However, we consider that possibility very rare so we are not checking it
                counter_analysis['geo-loc'] += 1
                counter_update['untouched'] += 1

        except KeyError:
            # Location format is wrong
            counter_analysis['corrupted-location'] += 1
            counter_update['untouched'] += 1

            msg('   - {0}: entity w/ corrupted location {1} ({2}): {3}'.format(processed, json.dumps(doc['_id']),
                                                                               date2string(doc['modDate']),
                                                                               json.dumps(location)))

            need_help = True

print '- processing entity: %d/%d' % (processed, total)
print '- documents analyzed:                                                          %d' % processed
print '  * entities wo/ geo: attr & wo/ loc field                              (ok)   %d' % counter_analysis['ngeo-nloc']
print '  * entities w/  geo: attr & w/ loc field                               (ok)   %d' % counter_analysis['geo-loc']
print '  * entities wo/ geo: attr & w/ loc field - coherent (leg NGSIv1)       (ok)   %d' % counter_analysis['legacy']
print '  ! entities wo/ geo: attr & w/ loc field - not coherent           (fixable)   %d' % counter_analysis['ngeo-locpoint']
print '  ! entities wo/ geo: attr & w/ loc field - not coherent         (unfixable)   %d' % counter_analysis['ngeo-loc']
print '  ! entities w/ geo:point attr & wo/ loc field                     (fixable)   %d' % counter_analysis['geopoint-nloc']
print '  ! entities w/ empty string in geo:point attr                     (fixable)   %d' % counter_analysis['emptygeopoint']
print '  ! entities w/ geo:json attr and wo/ loc field                    (fixable)   %d' % counter_analysis['geojson-nloc']
print '  ! entities w/ other geo: attr and wo/ loc field                (unfixable)   %d' % counter_analysis['unknown-geo']
print '  ! entities w/ detected corrupted location                      (unfixable)   %d' % counter_analysis['corrupted-location']

if len(not_fixable_types_found.keys()) > 0:
    print '* geo: types found without associated location field (except geo:point):       %s' % ','.join(not_fixable_types_found.keys())

if len(location_types_found.keys()) > 0:
    print '* loc types found without associated geo: attr not fixable:                    %s' % ','.join(location_types_found.keys())


print '- documents processed:                                                         %d' % processed
print '  * untouched:                                                                 %d' % counter_update['untouched']
print '  * changed:                                                                   %d' % counter_update['changed']
print '  * attempt to change but error:                                               %d' % counter_update['error']

if need_help:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Ask for help!"
