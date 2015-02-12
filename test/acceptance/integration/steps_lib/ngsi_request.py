# -*- coding: utf-8 -*-
"""
Copyright 2014 Telefonica Investigación y Desarrollo, S.A.U

This file is part of fiware-orion

fiware-orion is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

fiware-orion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public
License along with fiware-orion.
If not, seehttp://www.gnu.org/licenses/.

For those usages not covered by the GNU Affero General Public License
please contact with::[iot_support@tid.es]
"""

__author__ = 'Jon Calderin Goñi (jcaldering@gmail.com)'


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


@step('a standard context registration is asked with the before information')
def a_standard_context_registration_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].register_context(
        world.payloads[world.payloads_count])


@step('a standard disconver context availability is asked with the before information')
def a_standard_discover_context_availability_is_asked_with_the_before_information(step):
    world.responses[world.response_count] = world.cb[world.cb_count].discover_context_availability(
        world.payloads[world.payloads_count])
