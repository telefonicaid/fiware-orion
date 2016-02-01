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


from lettuce import step, world


@step('a standard context entity creation is asked with the before information')
def a_standard_context_entity_Creation_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].standard_entity_creation(
        world.payloads[world.payloads_count])


@step('a standard context entity update is asked with the before information')
def a_standard_context_entity_update_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].standard_entity_update(
        world.payloads[world.payloads_count])


@step('a standard context entity delete is asked with the before information')
def a_standard_context_entity_delete_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].standard_entity_delete(
        world.payloads[world.payloads_count])


@step('a standard query context is asked with the before information')
def a_standard_query_context_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].standard_query_context(
        world.payloads[world.payloads_count])
    
@step('a standard context subscription is asked with the before information')
def a_standard_context_subscription_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].standard_subscribe_context_ontime(
        world.payloads[world.payloads_count])

# NFGSI 9

@step('a standard context registration is asked with the before information')
def a_standard_context_registration_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].register_context(world.payloads[world.payloads_count])


@step('a standard disconver context availability is asked with the before information')
def a_standard_discover_context_availability_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].discover_context_availability(
        world.payloads[world.payloads_count])

# ***************
# Convenience

@step('a convenience query context is asked with the following data')
def a_convenience_query_context_is_asked_with_the_following_data(step):
    """
    Execute a convenience query context with the information in the table. The format is:
    | entity_id(optional) | entity_type(optional) | attirbute(optional) |
    :param step:
    :return:
    """
    rows = len(step.hashes)
    if rows != 1:
        raise ValueError('The table for this steps has to have only 1 row but it has {rows}'.format(rows=rows))
    kargs = dict()
    if 'entity_id' in step.hashes[0]:
        kargs.update({'entity_id': step.hashes[0]['entity_id']})
    if 'entity_type' in step.hashes[0]:
        kargs.update({'entity_type': step.hashes[0]['entity_type']})
    if 'attribute' in step.hashes[0]:
        kargs.update({'attribute': step.hashes[0]['attribute']})
    world.responses[world.response_count] = world.cb[world.cb_count].convenience_query_context(**kargs)


@step('a convenience delete context is asked with the following data')
def a_convenience_delete_entity_is_asked_with_the_following_information(step):
    """
    Execute a convenience query context with the information in the table. The format is:
    | entity_id | entity_type(optional) |
    :param step:
    :return:
    """
    rows = len(step.hashes)
    if rows != 1:
        raise ValueError('The table for this steps has to have only 1 row but it has {rows}'.format(rows=rows))
    kargs = dict()
    if 'entity_id' in step.hashes[0]:
        kargs.update({'entity_id': step.hashes[0]['entity_id']})
    else:
        raise ValueError('The entity_id is mandatory. Table: {table}'.format(table=step.hashes))
    if 'entity_type' in step.hashes[0]:
        kargs.update({'entity_type': step.hashes[0]['entity_type']})
    world.responses[world.response_count] = world.cb[world.cb_count].convenience_entity_delete_url_method(**kargs)

