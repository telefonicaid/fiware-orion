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

from iotqautils.cb_utils import EntitiesConsults, AttributesConsults, AttributesCreation, ContextElements, \
    NotifyConditions, ContextRegistrations
from integration.tools.general_utils import check_world_attribute_is_not_none
from lettuce import world, step


@step('the following entities to consult')
def entities_to_consult(step):
    """
    Step to create an instance of EntitiesConsults
    the following entities to consult
    | entity_id | entity_type | is_pattern(optional) |
    :param step:
    :return:
    """
    world.entities = EntitiesConsults()
    for line in step.hashes:
        if "is_pattern" in line:
            world.entities.add_entity(line['entity_id'], line['entity_type'], line['is_pattern'])
        else:
            world.entities.add_entity(line['entity_id'], line['entity_type'])


@step('the following attributes to consult')
def attributes_to_consult(step):
    """
    Step to create an instance of AttributeConsults
    the following entities to consult
    | attribute_name | attribute_type |
    :param step:
    :return:
    """
    world.attributes_consult = AttributesConsults()
    for line in step.hashes:
        world.attributes_consult.add_attribute(line['attribute_name'],
                                               line['attribute_type'])


@step('the following attributes to create')
def attributes_to_create(step):
    """
    Step to create an instance of AttributesCreation
    the following entities to consult
    | attribute_name | attribute_type | attribute_value |
    :param step:
    :return:
    """
    world.attributes_creation = AttributesCreation()
    for line in step.hashes:
        world.attributes_creation.add_attribute(line['attribute_name'],
                                                line['attribute_type'],
                                                line['attribute_value'])


@step('a context elements with the before attrs and the following entities')
def add_a_context_element_with_the_before_attrs_and_the_following_entities(step):
    """
    Step to create a contextElement if doesnt exist, and add the attributes data defined before
    add a context element with the before attrs and the followin entity properties
    | entity_id | entity_type | is_pattern(optional) |
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['attributes_creation'])
    if not isinstance(world.context_elements, ContextElements):
        world.context_elements = ContextElements()
    for line in step.hashes:
        if "is_pattern" in line:
            world.context_elements.add_context_element(line['entity_id'], line['entity_type'],
                                                       world.attributes_creation,
                                                       line['is_pattern'])
        else:
            world.context_elements.add_context_element(line['entity_id'], line['entity_type'],
                                                       world.attributes_creation)


@step('a notify conditions with the following data')
def a_notify_conditions_with_the_following_data(step):
    """
    Step to create a notifyConditions
    a notify conditions with the following data
    | type(ontime or onchange) | time (ontime type) || attributes(list, separated by comma) |
    :param step:
    :return:
    """
    world.notify_conditions = NotifyConditions()
    for line in step.hashes:
        if line['type'] == 'ontime':
            world.notify_conditions.add_notify_condition_ontime(line['time'])
        elif line['type'] == 'onchange':
            world.notify_conditions.add_notify_condition_onchange(
                line['attributes'].split(',').strip())
        else:
            raise ValueError(
                'The type specified "{type_specified}" is not valid, it has to be "ontime" or "onchange"'.format(
                    type_specified=line['type']))


@step('a context registrations with the before entities and attributes and the following providing applications')
def a_context_registrations_with_the_before_entities_and_attributes_and_the_following_providing_applications(step):
    """
    Step to create a constextRegistrations with the attributes and entities setted before, one for each providing application
    a context registrations with the before entities and attributes and the following providing applications
    | providing_application |
    :param step:
    :return:
    """
    check_world_attribute_is_not_none(['entities', 'attributes_consult'])
    world.context_registrations = ContextRegistrations()
    for line in step.hashes:
        world.context_registrations.add_context_registration(
            world.entities, world.attributes_consult,
            'http://{ip}:{port}{path}'.format(ip=world.config['mock']['host'], port=world.config['mock']['port'],
                                              path=line['providing_application']))

