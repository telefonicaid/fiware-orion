# -*- coding: utf-8 -*-
"""
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
"""

__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'

from iotqautils.cb_utils import PayloadUtils
from integration.tools.general_utils import check_world_attribute_is_not_none
from lettuce import step, world


@step('build the standard entity creation payload with the previous data')
def build_the_standard_entity_creation_payload_with_the_previous_data(step):
    """
    Build the payload, taking into account the context_elements has to be defined in steps before
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['context_elements'])
    world.payloads[world.payloads_count] = PayloadUtils.build_standard_entity_creation_payload(
        world.context_elements)


@step('build the standard entity update payload with the previous data')
def build_the_standard_entity_update_payload_with_the_previous_data(step):
    """
    Build the payload, taking into account the context_elements has to be defined in steps before
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['context_elements'])
    world.payloads[world.payloads_count] = PayloadUtils.build_standard_entity_update_payload(
        world.context_elements)


@step('build the standard entity delete payload with the previous data')
def build_the_standard_entity_delete_payload_with_the_previous_data(step):
    """
    Build the payload, taking into account the context_elements has to be defined in steps before
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['context_elements'])
    world.payloads[world.payloads_count] = PayloadUtils.build_standard_entity_delete_payload(
        world.context_elements)


@step('build the standard query context payload with the previous data$')
@step('build the standard query context payload with the previous data and the following attributes')
def build_the_standard_query_context_payload_with_the_previous_data(step):
    """
    Build the payload, taking into account the entities has to be defined in steps before
    || attributes(list, separated by comma)(optional) ||
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['entities'])
    if len(step.hashes) > 0:
        if 'attributes' in step.hashes[0]:
            rows = len(step.hashes)
            if rows != 1:
                raise ValueError('The table of the step has to have only 1 row but it has {rows}'.format(rows=rows))
            world.payloads[world.payloads_count] = PayloadUtils.build_standard_query_context_payload(
                world.entities, step.hashes[0]['attributes'].split(','))
        else:
            raise ValueError('The column expected in the table is "attributes"')
    else:
        world.payloads[world.payloads_count] = PayloadUtils.build_standard_query_context_payload(
            world.entities)


@step('build the standard context subscription ontime payload with the previous data and the following')
def build_the_standard_context_subscription_ontime_payload_with_the_previous_data_and_the_following(step):
    """
    Build the payload taking into account the entities and notify condition have to be defined before
    | attributes(list, separated by comma) | reference | duration |
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['entities', 'notify_conditions'])
    if len(step.hashes) == 1:
        columns = ['attributes', 'reference', 'duration']
        for column in columns:
            if column not in step.hashes[0]:
                raise ValueError(
                    'The column {column} has to exist in the table. The table is: {hashes}'.format(column=column,
                                                                                                   hashes=step.hashes))
        world.payloads[world.payloads_count] = PayloadUtils.build_standard_subscribe_context_payload(
            world.entities, step.hashes[0]['attributes'].split(','),
            'http://{ip}:{port}{path}'.format(ip=world.config['mock']['host'], port=world.config['mock']['port'],
                                              path=step.hashes[0]['reference']),
            step.hashes[0]['duration'],
            world.notify_conditions)


# NGSI 9

@step('build the standard context registration payload with the previous data and duration "([^"]*)"')
def build_the_standard_context_registration_payload_with_the_previous_data(step, duration):
    """
    Build the payload, taking into account the context_registration has to be defined in steps before
    || attributes(list, separated by comma)(optional) ||
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['context_registrations'])
    world.payloads[world.payloads_count] = PayloadUtils.build_context_registration_payload(
        world.context_registrations, duration)


@step('build the standard discover context availability payload with the previous data')
def build_the_standard_discover_context_availability_payload_with_the_previous_data(step):
    """
    Build the payload, taking into account the entities has to be defined in steps before
    || attributes(list, separated by comma)(optional) ||
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['entities'])
    world.payloads[world.payloads_count] = PayloadUtils.build_discover_context_availability_payload(
        world.entities)
    

