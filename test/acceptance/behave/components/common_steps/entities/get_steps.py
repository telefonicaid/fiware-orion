# -*- coding: utf-8 -*-
"""
 Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U

 This file is part of Orion Context Broker.

 Orion Context Broker is free software: you can redistribute it and/or
 modify it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 Orion Context Broker is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.

 For those usages not covered by this license please contact with
 iot_support at tid dot es
"""
__author__ = 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

import behave
from behave import step
import copy

from iotqatools.helpers_utils import *
from tools.NGSI_v2 import NGSI


# constants
CONTEXT_BROKER_ENV = u'context_broker_env'
MONGO_ENV = u'mongo_env'


behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

# ------------------------- list entities ----------------------------

@step(u'get all entities')
def get_all_entities(context):
    """
    list all entities
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("getting a list with all entities in a service...")
    context.resp = context.cb.list_all_entities(context)
    __logger__.info("...returned a list with all entities in a service")


@step(u'get an entity by ID "([^"]*)"')
def get_an_entity_by_id(context, entity_id):
    """
    get an entity by ID
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: entity ID
    """
    __logger__.debug("getting an entity by id...")
    context.resp = context.cb.list_an_entity_by_id(context, entity_id)
    __logger__.info("...returned an entity by id")


@step(u'get an attribute "([^"]*)" by ID "([^"]*)"')
def get_an_attribute_by_id(context, attribute_name, entity_id):
    """
    get an attribute by ID
    :param attribute_name:
    :param entity_id:
    """
    __logger__.debug("getting an attribute by id...")
    context.resp = context.cb.list_an_attribute_by_id(attribute_name, entity_id)
    __logger__.info("...returned an attribute by id")


@step(u'initialize the accumulator context of entities')
def initialize_the_accumulator_context_of_entities(context):
    """
    initialize the accumulator context of entities
    """
    context.entities_accumulate = []

@step(u'accumulate context of entities for use with lists')
def accumulate_entities_to_list(context):
    """
    accumulate context of entities for use with the returned lists
    :param context:It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    entity = {}
    entity = context.cb.get_entity_context()
    context.entities_accumulate.append(copy.deepcopy(entity))
    __logger__.debug("accumulate after: %s" % str(context.entities_accumulate))


# ------------------------------------- validations ----------------------------------------------

@step(u'verify that any entities are returned')
@step(u'verify that all entities are returned')
def verify_get_all_entities(context):
    """
    verify get all entities
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying all entities are returned in get request...")
    queries_parameters = context.cb.get_entities_parameters()
    ngsi = NGSI()
    ngsi.verify_get_all_entities(queries_parameters, context.entities_accumulate, context.resp)
    __logger__.info("...Verified all entities are returned in get request...")


@step(u'verify an entity in raw mode with type "([^"]*)" in attribute value from http response')
def verify_http_response_in_raw_mode_witn_type(context, field_type):
    """
    verify http response in raw mode and type in attribute value from http response
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param field_type: field type (bool | int | float | list | dict | str | NoneType)
    """
    global cb, resp
    __logger__.debug("Verifying http response in raw mode from http response...")
    entities_context = context.cb.get_entity_context()
    ngsi = NGSI()
    ngsi.verify_entity_raw_mode_http_response(entities_context, context.resp, field_type)
    __logger__.info("...Verified http response in raw mode from http response...")


@step(u'verify that the entity by ID is returned')
def verify_that_the_entity_by_id_is_returned(context):
    """
    verify that the entity by ID is returned
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying an entity by ID returned from a request...")
    queries_parameters = context.cb.get_entities_parameters()
    entities_context = context.cb.get_entity_context()
    entity_id_to_request = context.cb.get_entity_id_to_request()
    ngsi = NGSI()
    ngsi.verify_an_entity_by_id(queries_parameters, entities_context, context.resp, entity_id_to_request)
    __logger__.info("...Verified an entity by ID returned from a request...")


@step(u'verify that the attribute by ID is returned')
def verify_that_the_attribute_by_id_is_returned(context):
    """
    verify that the attribute by ID is returned
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying an attribute by ID returned from a request...")
    entities_context = context.cb.get_entity_context()
    attribute_name_to_request = context.cb.get_attribute_name_to_request()
    ngsi = NGSI()
    ngsi.verify_an_attribute_by_id(entities_context, context.resp, attribute_name_to_request)
    __logger__.info("...Verified an attribute by ID returned from a request...")


@step(u'verify an attribute by ID in raw mode with type "([^"]*)" in attribute value from http response')
def verify_an_attribute_by_id_in_raw_mode_from_http_response(context, field_type):
    """
    verify an attribute by ID in raw mode with type in attribute value from http response
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param field_type: field type to verify (bool | int | float | list | dict | str | NoneType)
    """
    __logger__.debug("Verifying an attribute by ID returned in raw mode from http response...")
    entities_context = context.cb.get_entity_context()
    attribute_name_to_request = context.cb.get_attribute_name_to_request()
    ngsi = NGSI()
    ngsi.verify_an_attribute_by_id_in_raw_mode_http_response(entities_context, context.resp, attribute_name_to_request, field_type)
    __logger__.info("...Verified an attribute by ID returned in raw mode from http response...")
