#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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
from deepdiff import DeepDiff
from datetime import datetime
import argparse
import logging
import json
import sys
import re


# Helper functions


def is_geo_type(attr_type):
    """
    Return True if attr type passed as argument is a geo type

    :param attr_type: the attr type to evaluate
    """

    return attr_type == 'geo:point' or attr_type == 'geo:line' or attr_type == 'geo:box' \
        or attr_type == 'geo:polygon' or attr_type == 'geo:json'


def ignore_type(attr):
    """
    Return true if attribute has the ignoreType metadata
    """
    return 'md' in attr and 'ignoreType' in attr['md']


def is_geo_attr(attr):
    """
    Check if a given attr is of geo type, i.e. following conditions are true:
    * It has a geo type (geo:json, et.c9
    * It doesn't use the ignoreType metadata
    * Its value is not null
    """
    return is_geo_type(attr['type']) and not ignore_type(attr) and attr['value'] is not None


def to_geo_json(attr):
    """
    Return the GeoJSON corresponding to an attribute location, taking into account the type

    Useful ref: https://github.com/telefonicaid/fiware-orion/blob/3.9.0/doc/manuals/orion-api.md

    :return: either a dict (containing a GeoJSON) or a string (in the case of error)
    """

    if attr['type'] == 'geo:point':
        # "value": "41.3763726, 2.186447514",
        coords = attr['value'].split(",")
        return {
            'type': 'Point',
            'coordinates': [float(coords[1]), float(coords[0])]
        }
    elif attr['type'] == 'geo:line':
        # "value": [
        #    "40.63913831188419, -8.653321266174316",
        #    "40.63881265804603, -8.653149604797363"
        # ]
        coordinates = []
        for item in attr['value']:
            coords = item.split(",")
            coordinates.append([float(coords[1]), float(coords[0])])
        return {
            'type': 'LineString',
            'coordinates': coordinates
        }
    elif attr['type'] == 'geo:box':
        # "value": [
        #    "40.63913831188419, -8.653321266174316",
        #    "40.63881265804603, -8.653149604797363"
        # ]
        # The first pair is the lower corner, the second is the upper corner.
        lower_corner_0 = attr['value'][0].split(",")
        upper_corner_0 = attr['value'][1].split(",")
        lower_corner_1 = [upper_corner_0[0], lower_corner_0[1]]
        upper_corner_1 = [lower_corner_0[0], upper_corner_0[1]]
        return {
            'type': 'Polygon',
            'coordinates': [[
                [float(lower_corner_0[1]), float(lower_corner_0[0])],
                [float(lower_corner_1[1]), float(lower_corner_1[0])],
                [float(upper_corner_0[1]), float(upper_corner_0[0])],
                [float(upper_corner_1[1]), float(upper_corner_1[0])],
                [float(lower_corner_0[1]), float(lower_corner_0[0])]
            ]]
        }
    elif attr['type'] == 'geo:polygon':
        # "value": [
        #    "40.63913831188419, -8.653321266174316",
        #    "40.63881265804603, -8.653149604797363",
        #    "40.63913831188419, -8.653321266174316"
        # ]
        coordinates = []
        for item in attr['value']:
            coords = item.split(",")
            last = [float(coords[1]), float(coords[0])]
            coordinates.append([float(coords[1]), float(coords[0])])
        coords.append(last)  # so we have a closed shape
        return {
            'type': 'Polygon',
            'coordinates': [coordinates]
        }
    elif attr['type'] == 'geo:json':
        # Feature is a special type, see https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/orion-api.md#geojson
        if attr['value']['type'] == 'Feature':
            if 'geometry' in attr['value']:
                return attr['value']['geometry']
            else:
                return "geo:json Feature does not have geometry element"
        # FeatureCollection is a special type, see https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/orion-api.md#geojson
        if attr['value']['type'] == 'FeatureCollection':
            if 'features' not in attr['value']:
                return "geo:json FeatureCollection does not have features element"
            if len(attr['value']['features']) == 0:
                return "geo:json FeatureCollection features has no elements"
            if len(attr['value']['features']) > 1:
                return "geo:json FeatureCollection features has more than one element"
            if 'geometry' in attr['value']['features'][0]:
                return attr['value']['features'][0]['geometry']
            else:
                return "geo:json FeatureCollection feature does not have geometry element"
        else:
            return attr['value']

    else:
        return f"unknown geo location type: {attr['type']}"


def convert_strings_to_numbers(data):
    """
    Generated by ChatGPT :)
    """
    if isinstance(data, dict):
        # If it's a dictionary, apply the function recursively to its values
        return {key: convert_strings_to_numbers(value) for key, value in data.items()}
    elif isinstance(data, list):
        # If it's a list, apply the function recursively to its elements
        return [convert_strings_to_numbers(item) for item in data]
    elif isinstance(data, str):
        # If it's a string, try converting it to a number
        try:
            return int(data)
        except ValueError:
            try:
                return float(data)
            except ValueError:
                # If it's not a valid number, leave it as it is
                return data
    else:
        # If it's neither a dictionary, list, nor string, leave it as it is
        return data


def check_id(id):
    """
    Common checks for several rules

    Ref: https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/orion-api.md#identifiers-syntax-restrictions
    """
    # Minimum field length is 1 character
    l = len(id)
    if l == 0:
        return f'length ({l}) shorter than minimum allowed (1)'
    # Maximum field length is 256 characters
    if l > 256:
        return f'length ({l}) greater than maximum allowed (256)'

    # Following chars are not used (general): < > " ' = ; ( )
    if re.search('[<>"\'=;()]', id):
        return f'contains forbidden chars (general)'

    # Following chars are not used (identifiers): whitespace, & ? / #
    if re.search('[\s&?/#]', id):
        return f'contains forbidden chars (identifiers)'

    return None


# Rules functions
def rule10(entity):
    """
    Rule 10: `_id` field consistency

    See README.md for an explanation of the rule
    """
    missing_fields = []

    for field in ['id', 'type', 'servicePath']:
        if field not in entity['_id']:
            missing_fields.append(field)

    if len(missing_fields) > 0:
        return f"missing subfields in _id: {', '.join(missing_fields)}"
    else:
        return None


def rule11(entity):
    """
    Rule 11: mandatory fields in entity

    See README.md for an explanation of the rule
    """
    missing_fields = []

    for field in ['attrNames', 'creDate', 'modDate']:
        if field not in entity:
            missing_fields.append(field)

    if len(missing_fields) > 0:
        return f"missing fields: {', '.join(missing_fields)}"
    else:
        return None


def rule12(entity):
    """
    Rule 12: mandatory fields in attribute

    See README.md for an explanation of the rule
    """
    s = []
    for attr in entity['attrs']:
        missing_fields = []

        for field in ['mdNames', 'creDate', 'modDate']:
            if field not in entity['attrs'][attr]:
                missing_fields.append(field)

        if len(missing_fields) > 0:
            s.append(f"in attribute '{attr}' missing fields: {', '.join(missing_fields)}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule13(entity):
    """
    Rule 13: `attrNames` field consistency

    See README.md for an explanation of the rule
    """
    # note the omission of attrNames is checked by another rule. In this rule we include
    # some guards for not breaking in that case

    # attrNames in attrs
    attrnames_not_in_attrs = []
    if 'attrNames' in entity:
        for attr in entity['attrNames']:
            if attr.replace('.', '=') not in entity['attrs']:
                attrnames_not_in_attrs.append(attr)

    # attrs in attrNames
    attrs_not_in_attrnames = []
    if 'attrs' in entity:
        for attr in entity['attrs']:
            if 'attrNames' not in entity or attr.replace('=', '.') not in entity['attrNames']:
                attrs_not_in_attrnames.append(attr)

    s = []
    if len(attrnames_not_in_attrs) > 0:
        s.append(f"attributes in attrNames not found in attrs object: {','.join(attrnames_not_in_attrs)}")
    if len(attrs_not_in_attrnames) > 0:
        s.append(f"attributes in attrs object not found in attrNames: {','.join(attrs_not_in_attrnames)}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule14(entity):
    """
    Rule 14: `mdNames` field consistency

    See README.md for an explanation of the rule
    """
    # note the omission of mdNames is checked by another rule. In this rule we include
    # some guards for not breaking in that case

    s = []
    for item in entity['attrs']:
        attr = entity['attrs'][item]
        # mdNames in md
        if 'mdNames' in attr:
            mdnames_not_in_md = []
            for md in attr['mdNames']:
                if md.replace('.', '=') not in attr['md']:
                    mdnames_not_in_md.append(md)

        # md in mdNames
        md_not_in_mdnames = []
        if 'md' in attr:
            for md in attr['md']:
                if 'mdNames' not in attr or md.replace('=', '.') not in attr['mdNames']:
                    md_not_in_mdnames.append(md)

        if len(mdnames_not_in_md) > 0:
            s.append(
                f"in attribute '{item}' metadata in mdNames not found in md object: {', '.join(mdnames_not_in_md)}")
        if len(md_not_in_mdnames) > 0:
            s.append(
                f"in attribute '{item}' metadata in md object not found in mdNames: {', '.join(md_not_in_mdnames)}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule15(entities_collection):
    """
    Rule 15: not swapped subkeys in `_id`

    See README.md for an explanation of the rule

    This is a global rule, so the parameter is not an individual entity but the whole entities collection.
    """
    s = []
    for entity in entities_collection.aggregate(
            [
                {
                    '$group': {
                        '_id': {'id': '$_id.id', 'type': '$_id.type', 'servicePath': '$_id.servicePath'},
                        'count': {'$sum': 1}
                    }
                },
                {
                    '$match': {'count': {'$gt': 1}}
                }
            ]
    ):
        id = entity['_id']['id']
        type = entity['_id']['type']
        service_path = entity['_id']['servicePath']
        count = entity['count']
        s.append(
            f"_id uniqueness violation for entity id='{id}' type='{type}' servicePath='{service_path}' found {count} times")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule16(entity):
    """
    Rule 16: `location` field consistency

    See README.md for an explanation of the rule
    """
    # check that as much as one attribute is using geo type
    geo_attrs = []
    for attr in entity['attrs']:
        # type existence in attribute is checked by another rule
        if 'type' in entity['attrs'][attr] and is_geo_attr(entity['attrs'][attr]):
            geo_attrs.append(attr)

    if len(geo_attrs) > 1:
        return f"more than one attribute with geo type: {', '.join(geo_attrs)}"

    if len(geo_attrs) == 1:
        # If geo attr found, then check that there is consistent location field
        geo_attr = geo_attrs[0]
        geo_type = entity['attrs'][geo_attr]['type']

        if 'location' not in entity:
            return f"geo location '{geo_attr}' ({geo_type}) not null but location field not found in entity"
        if entity['location']['attrName'] != geo_attr:
            return f"location.attrName ({entity['location']['attrName']}) differs from '{geo_attr}'"

        geo_json = to_geo_json(entity['attrs'][geo_attr])
        if type(geo_json) == str:
            return geo_json

        # https://www.testcult.com/deep-comparison-of-json-in-python/
        diff = DeepDiff(geo_json, entity['location']['coords'], ignore_order=True)
        if diff:
            # A typical difference is that attribute value uses strings and location uses numbers
            # (this happens when the location was created/updated using NGSIv1). We try to identify that case
            geo_json = convert_strings_to_numbers(geo_json)
            if not DeepDiff(geo_json, entity['location']['coords'], ignore_order=True):
                return f"location.coords and GeoJSON derived from '{geo_attr}' ({geo_type}) is consistent, but value " \
                        f"should use numbers for coordinates instead of strings"
            else:
                # Other causes
                return f"location.coords and GeoJSON derived from '{geo_attr}' ({geo_type}) value: {diff}"
    else:  # len(geo_attrs) == 0
        # If no geo attr found, check there isn't a location field
        if 'location' in entity:
            return f"location field detected but no geo attribute is present (maybe metadata location is used?)"


def rule17(entity):
    """
    Rule 17: `lastCorrelator` existence

    See README.md for an explanation of the rule
    """
    if 'lastCorrelator' not in entity:
        return f"missing lastCorrelator"
    else:
        return None


def rule20(entity):
    """
    Rule 20: entity id syntax

    See README.md for an explanation of the rule
    """

    # The existence of id in _id is checked by another rule
    if 'id' in entity['_id']:
        r = check_id(entity['_id']['id'])
        if r is not None:
            return f"entity id ({entity['_id']['id']}) syntax violation: {r}"

    return None


def rule21(entity):
    """
    Rule 21: entity type syntax

    See README.md for an explanation of the rule
    """

    # The existence of type in _id is checked by another rule
    if 'type' in entity['_id']:
        r = check_id(entity['_id']['type'])
        if r is not None:
            return f"entity type ({entity['_id']['type']}) syntax violation: {r}"

    return None


def rule22(entity):
    """
    Rule 22: entity servicePath syntax

    See README.md for an explanation of the rule

    Ref: https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/orion-api.md#entity-service-path
    """

    # servicePath existence is checked by another rule
    if 'servicePath' not in entity['_id']:
        return None

    sp = entity['_id']['servicePath']
    # Scope must start with / (only "absolute" scopes are allowed)
    if not sp.startswith('/'):
        return f"servicePath '{sp}' does not starts with '/'"

    # This special case can be problematic (as split will detect always a level and the minimum length rule
    # will break. So early return
    if sp == '/':
        return None

    # 10 maximum scope levels in a path
    sp_levels = sp[1:].split('/')
    if len(sp_levels) > 10:
        return f"servicePath has {len(sp_levels)} tokens but the limit is 10"

    # 50 maximum characters in each level (1 char is minimum), only alphanumeric and underscore allowed
    for i in range(len(sp_levels)):
        if len(sp_levels[i]) == 0:
            return f'servicePath level #{i} length is 0 but minimum is 1'
        if len(sp_levels[i]) > 50:
            return f'servicePath level #{i} length is {len(sp_levels[i])} but maximum is 50'
        if re.search('[^a-zA-Z0-9_]', sp_levels[i]):
            return f"unallowed characters in '{sp_levels[i]}' in servicePath level #{i}"


def rule23(entity):
    """
    Rule 23: attribute name syntax

    See README.md for an explanation of the rule
    """
    s = []

    # attrNames-attrs consistency is checked by another rule. In the present rule we cannot assume
    # they are consistent, so we do a union of both sets
    attrs_to_check = set()
    if 'attrNames' in entity:
        attrs_to_check.update(entity['attrNames'])
    for attr in entity['attrs']:
        attrs_to_check.add(attr.replace('=', '.'))

    # check in the resulting union set
    for attr in attrs_to_check:
        r = check_id(attr)
        if r is not None:
            s.append(f"attribute name ({attr}) syntax violation: {r}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule24(entity):
    """
    Rule 24: attribute type syntax

    See README.md for an explanation of the rule
    """
    s = []
    for attr in entity['attrs']:
        if 'type' not in entity['attrs'][attr]:
            s.append(f"in attribute '{attr}' type is missing")
        else:
            type = entity['attrs'][attr]['type']
            r = check_id(type)
            if r is not None:
                s.append(f"in attribute '{attr}' type ({type}) syntax violation: {r}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule25(entity):
    """
    Rule 25: metadata name syntax

    See README.md for an explanation of the rule
    """
    s = []
    for attr in entity['attrs']:
        # mdNames-md consistency is checked by another rule. In the present rule we cannot assume
        # they are consistent, so we do a union of both sets
        md_to_check = set()
        if 'mdNames' in entity['attrs'][attr]:
            md_to_check.update(entity['attrs'][attr]['mdNames'])
        if 'md' in entity['attrs'][attr]:
            for md in entity['attrs'][attr]['md']:
                md_to_check.add(md.replace('=', '.'))

        # check in the resulting union set
        for md in md_to_check:
            r = check_id(md)
            if r is not None:
                s.append(f"in attribute '{attr}' metadata name ({md}) syntax violation: {r}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule26(entity):
    """
    Rule 26: metadata type syntax

    See README.md for an explanation of the rule
    """
    s = []
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr]:
            for md in entity['attrs'][attr]['md']:
                if 'type' not in entity['attrs'][attr]['md'][md]:
                    s.append(f"in attribute '{attr}' metadata '{md}' type is missing")
                else:
                    type = entity['attrs'][attr]['md'][md]['type']
                    r = check_id(type)
                    if r is not None:
                        s.append(f"in attribute '{attr}' metadata '{md}' type ({type}) syntax violation: {r}")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule90(entity):
    """
    Rule 90: detect usage of `geo:x` attribute type where `x` different from `json`

    See README.md for an explanation of the rule
    """

    # note we could have more than one case, as ignoreType can be in use
    s = []
    for attr in entity['attrs']:
        # type existence in attribute is checked by another rule
        if 'type' in entity['attrs'][attr]:
            type = entity['attrs'][attr]['type']
            if is_geo_type(type) and type != 'geo:json':
                s.append(f"{attr} ({type})")

    #
    if len(s) > 0:
        return f"usage of deprecated geo type in attributes: {', '.join(s)}"


def rule91(entity):
    """
    Rule 91: detect usage of more than one legacy `location` metadata

    See README.md for an explanation of the rule
    """
    attrs = []
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr]:
            if 'location' in entity['attrs'][attr]['md']:
                attrs.append(attr)

    if len(attrs) > 1:
        return f"location metadata found {len(attrs)} times in attributes: {', '.join(attrs)} (maximum should be just 1)"
    else:
        return None


def rule92(entity):
    """
    Rule 92: detect legacy `location` metadata should be `WGS84` or `WSG84`

    See README.md for an explanation of the rule
    """
    s = []
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr]:
            if 'location' in entity['attrs'][attr]['md']:
                location_value = entity['attrs'][attr]['md']['location']['value']
                if location_value != 'WGS84' and location_value != 'WSG84':
                    s.append(
                        f"in attribute '{attr}' location metadata value is {location_value} (should be WGS84 or WSG84)")

    if len(s) > 0:
        return ', '.join(s)
    else:
        return None


def rule93(entity):
    """
    Rule 93: detect usage of redundant legacy `location`

    See README.md for an explanation of the rule
    """
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr]:
            for md in entity['attrs'][attr]['md']:
                if md == 'location' and is_geo_type(entity['attrs'][attr]['type']):
                    return f"in attribute '{attr}' redundant location metadata found (attribute is already using {entity['attrs'][attr]['type']} type)"

    return None


def rule94(entity):
    """
    Rule 94: detect usage of not redundant legacy `location`

    See README.md for an explanation of the rule
    """
    for attr in entity['attrs']:
        if 'md' in entity['attrs'][attr]:
            for md in entity['attrs'][attr]['md']:
                if md == 'location' and not is_geo_type(entity['attrs'][attr]['type']):
                    return f"in attribute '{attr}' location metadata found (attribute type is {entity['attrs'][attr]['type']})"

    return None


rules = [
    # Rules 1x
    {
        'label': 'Rule10',
        'global': False,
        'func': rule10
    },
    {
        'label': 'Rule11',
        'global': False,
        'func': rule11
    },
    {
        'label': 'Rule12',
        'global': False,
        'func': rule12
    },
    {
        'label': 'Rule13',
        'global': False,
        'func': rule13
    },
    {
        'label': 'Rule14',
        'global': False,
        'func': rule14
    },
    {
        'label': 'Rule15',
        'global': True,
        'func': rule15
    },
    {
        'label': 'Rule16',
        'global': False,
        'func': rule16
    },
    {
        'label': 'Rule17',
        'global': False,
        'func': rule17
    },
    # Rules 2x
    {
        'label': 'Rule20',
        'global': False,
        'func': rule20
    },
    {
        'label': 'Rule21',
        'global': False,
        'func': rule21
    },
    {
        'label': 'Rule22',
        'global': False,
        'func': rule22
    },
    {
        'label': 'Rule23',
        'global': False,
        'func': rule23
    },
    {
        'label': 'Rule24',
        'global': False,
        'func': rule24
    },
    {
        'label': 'Rule25',
        'global': False,
        'func': rule25
    },
    {
        'label': 'Rule26',
        'global': False,
        'func': rule26
    },
    # Rules 9x
    {
        'label': 'Rule90',
        'global': False,
        'func': rule90
    },
    {
        'label': 'Rule91',
        'global': False,
        'func': rule91
    },
    {
        'label': 'Rule92',
        'global': False,
        'func': rule92
    },
    {
        'label': 'Rule93',
        'global': False,
        'func': rule93
    },
    {
        'label': 'Rule94',
        'global': False,
        'func': rule94
    }
]


def process_db(logger, db_name, db_conn, include_entity_date, query, rules_exp):
    """
    Process an individual DB

    :param logger: logger object
    :param db_name: the name of the DB to process
    :param db_conn: connection to MongoDB
    :param include_entity_date: if True, include entity modification date in log traces
    :param query: query to filter entities to be processed
    :param rules_exp: regular expression to filter rules to apply
    :return: fails
    """

    logger.info(f'processing {db_name}')
    n = 0
    failed_entities = 0
    fails = 0

    # check collection existence
    if 'entities' not in db_conn[db_name].list_collection_names():
        logger.warning(f'collections entities not found in {db_name} database, nothing to do')
        return

    # apply global rules
    for rule in rules:
        if rules_exp is not None and not re.search(rules_exp, rule['label']):
            continue

        if rule['global']:
            s = rule['func'](db_conn[db_name]['entities'])
            if s is not None:
                logger.warning(f'DB {db_name} {rule["label"]} violation in entities collection: {s}')
                fails += 1

    # apply per-entity rules
    for entity in db_conn[db_name]['entities'].find(query):
        n += 1
        entity_fail = False

        id_string = json.dumps(entity['_id'])
        if include_entity_date:
            if 'modDate' in entity:
                id_string = f"({datetime.fromtimestamp(entity['modDate']).strftime('%Y-%m-%dT%H:%M:%SZ')}) {id_string}"
            else:
                id_string = f"(<no date>)) {id_string}"

        logger.debug(f'* processing entity {id_string}')
        for rule in rules:
            if rules_exp is not None and not re.search(rules_exp, rule['label']):
                continue

            if not rule['global']:
                s = rule['func'](entity)
                if s is not None:
                    logger.warning(f'DB {db_name} {rule["label"]} violation for entity {id_string}: {s}')
                    entity_fail = True
                    fails += 1

        if entity_fail:
            failed_entities += 1

    if n > 0:
        logger.info(
            f'processed {db_name}: {failed_entities}/{n} ({round(failed_entities / n * 100, 2)}%) failed entities with {fails} rule violations')
    else:
        logger.warning(f'{db_name} has 0 entities (maybe it should be cleaned up?)')

    return fails


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='entities_consistency',
        description='Check consistency in Orion entities collection in DB')

    parser.add_argument('--mongoUri', dest='mongo_uri', default='mongodb://localhost:27017',
                        help='MongoDB URI. Default is mongodb://localhost:27017')
    parser.add_argument('--db', dest='db',
                        help='DB name to check. If omitted all DBs starting with "orion" will be checked.')
    parser.add_argument('--include-entities-date', dest='include_entities_date', default=False, action='store_true',
                        help='include entity modification time in log traces')
    parser.add_argument('--query', dest='query', default='{}',
                        help='query to filter entities to check, in JSON MongoDB query language. By default, '
                             'all entities in the collection will be checked.')
    parser.add_argument('--rules-exp', dest='rules_exp',
                        help='Specifies the rules to apply, as a regular expression. By default all rules are applied.')
    parser.add_argument('--logLevel', dest='log_level', choices=['DEBUG', 'INFO', 'WARN', 'ERROR'], default='INFO',
                        help='log level. Default is INFO')
    args = parser.parse_args()

    # sets the logging configuration
    logging.basicConfig(
        level=logging.getLevelName(args.log_level),
        format="time=%(asctime)s | lvl=%(levelname)s | msg=%(message)s",
        handlers=[
            logging.StreamHandler()
        ]
    )
    logger = logging.getLogger()

    # connect to MongoDB
    mongo_client = MongoClient(args.mongo_uri)
    db_names = mongo_client.list_database_names()

    # to remove starting and trailing ' char, in case it is used
    query = json.loads(args.query.replace("'", ""))

    fails = 0
    if args.db is not None:
        if args.db in db_names:
            fails += process_db(logger, args.db, mongo_client, args.include_entities_date, query, args.rules_exp)
        else:
            logger.fatal(f'database {args.db} does not exist')
            sys.exit(1)
    else:
        # Process all Orion databases
        for db_name in db_names:
            if db_name.startswith('orion-'):
                fails += process_db(logger, db_name, mongo_client, args.include_entities_date, query, args.rules_exp)

    logger.info(f'total rule violations: {fails}')
