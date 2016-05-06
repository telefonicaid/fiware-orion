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


@step(u'get attributes in an entity by ID "([^"]*)"')
def get_attributes_in_an_entity_by_id(context, entity_id):
    """
    get attributes in an entity by ID
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: entity ID
    """
    __logger__.debug("getting attributes in an entity by id...")
    context.resp = context.cb.list_an_entity_by_id(context, entity_id, "/attrs")
    __logger__.info("...returned attributes in an entity by id")


@step(u'get an attribute "([^"]*)" by ID "([^"]*)"')
def get_an_attribute_by_id(context, attribute_name, entity_id):
    """
    get an attribute by ID
    :param attribute_name: name of the attribute
    :param entity_id: id of the entity
    """
    __logger__.debug("getting an attribute by id...")
    context.resp = context.cb.list_an_attribute_by_id(context, attribute_name, entity_id)
    __logger__.info("...returned an attribute by id")


@step(u'initialize entity groups recorder')
def initialize_the_entity_group_recorder(context):
    """
    initialize entity groups recorder
    """
    context.entities_accumulate = []


@step(u'record entity group')
def accumulate_entity_group(context):
    """
    record context of entities for use with validation steps
    :param context:It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    entity = {}
    entity = context.cb.get_entity_context()
    context.entities_accumulate.append(copy.copy(entity))
    __logger__.debug("entity groups record after: %s" % str(context.entities_accumulate))


@step(u'get an attribute value by ID "([^"]*)" and attribute name "([^"]*)" if it exists')
def get_an_attribute_value_by_id_and_attribute_name_if_it_exists(context, entity_id, attribute_name):
    """
    get an attribute value by ID and attribute name if it exists
    :param context:It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: id of the entity
    :param attribute_name: name of the attribute
    """
    __logger__.debug("getting an attribute value by id and attribute name...")
    context.resp = context.cb.list_an_attribute_by_id(context, attribute_name, entity_id, "value")
    __logger__.info("...returned an attribute value by id and attribute name")


@step(u'get entity types')
def get_entity_types(context):
    """
    get entity types
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("getting entity types...")
    context.resp = context.cb.get_entity_types(context)
    __logger__.info("...returned entity type with counter")


@step(u'get an entity type by type "([^"]*)"')
def get_entity_types(context, entity_type):
    """
    get an entity type with its attribute types
    :param entity_type: type used
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("getting an entity type...")
    context.resp = context.cb.get_an_entity_type(context, entity_type)
    __logger__.info("...returned an entity type")




# ------------------------------------- validations ----------------------------------------------


@step(u'verify that "([^"]*)" entities are returned')
def verify_get_all_entities(context, entities_returned):
    """
    verify get all entities
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying %s entities are returned in get request..." % entities_returned)
    queries_parameters = context.cb.get_entities_parameters()
    ngsi = NGSI()
    ngsi.verify_get_all_entities(queries_parameters, context.entities_accumulate, context.resp, entities_returned)
    __logger__.info("...Verified %s entities are returned in get request" % entities_returned)


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


@step(u'verify that attributes in an entity by ID are returned')
def verify_that_attributes_in_an_entity_by_id_are_returned(context):
    """
    verify that attributes in an entity by ID are returned
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying that attributes in an entity by ID are returned from a request...")
    queries_parameters = context.cb.get_entities_parameters()
    entities_context = context.cb.get_entity_context()
    entity_id_to_request = context.cb.get_entity_id_to_request()
    ngsi = NGSI()
    ngsi.verify_an_entity_by_id(queries_parameters, entities_context, context.resp, entity_id_to_request, attrs=True)
    __logger__.info("...Verified that attributes in an entity by ID are returned from a request...")


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


@step(u'verify that the attribute value by ID is returned')
def verify_that_the_attribute_value_by_id_is_returned(context):
    """
    verify that the attribute value by ID is returned
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying an attribute value by ID returned from a request...")
    entities_context = context.cb.get_entity_context()
    ngsi = NGSI()
    ngsi.verify_an_attribute_value_by_id(entities_context, context.resp)
    __logger__.info("...Verified the attribute value by ID returned from a request...")


@step(u'verify that entity types returned in response are: "([^"]*)"')
def verify_that_entity_types_are_returned_in_response(context, types):
    """
    verify that entity types are returned in response
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying that entities types are returned from a request...")
    ngsi = NGSI()
    ngsi.verify_entity_types(types, context.resp)
    __logger__.info("...Verified that entities types are returned from a request...")


@step(u'verify that attributes types are returned in response based on the info in the recorder')
def verify_that_attribute_types_are_returned_in_response(context):
    """
    verify that attributes types are returned in response based on the info in the recorder
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying that attribute types are returned from a request...")
    queries_parameters = context.cb.get_entities_parameters()
    prefixes = context.cb.get_entities_prefix()
    ngsi = NGSI()
    ngsi.verify_attributes_types_with_entity_types(queries_parameters, context.entities_accumulate, prefixes, context.resp)
    __logger__.info("...Verified that attribute types are returned from a request...")

@step(u'verify that attributes types by entity type are returned in response based on the info in the recorder')
def verify_that_attributes_types_by_entity_type_are_returned_in_response_based_on_the_info_in_the_recorder(context):
    """
    verify that attributes types by entity type are returned in response based on the info in the recorder
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying that attribute types by entity type are returned in response...")
    entity_type = context.cb.get_entity_type_to_request()
    ngsi = NGSI()
    ngsi.verify_attributes_types_by_entity_types(entity_type, context.entities_accumulate, context.resp)
    __logger__.info("...Verified that attribute types by entity type are returned in response...")
