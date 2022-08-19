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

from iotqatools.helpers_utils import *
from iotqatools.mongo_utils import Mongo
from tools.NGSI_v2 import NGSI
from tools.properties_config import Properties

# constants
CONTEXT_BROKER_ENV = u'context_broker_env'
MONGO_ENV = u'mongo_env'

properties_class = Properties()

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


# ------------------------- delete steps ----------------------------

@step(u'delete an entity with id "([^"]*)"')
def delete_an_entity_by_id(context, entity_id):
    """
    delete an entity by id
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: entity id name
    """
    __logger__.debug("Deleting entity: %s..." % entity_id)
    context.resp = context.cb.delete_entities_by_id(context, entity_id)
    __logger__.info("...\"%s\" entity has been deleted" % entity_id)


@step(u'delete an attribute "([^"]*)" in the entity with id "([^"]*)"')
def delete_an_attribute_in_entity_with_id(context, attribute_name, entity_id):
    """
    delete an attribute in an entity
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: entity id name
    :param attribute_name: attribute name to delete
    """
    __logger__.debug("Deleting an attribute in an entity defined by ID...")
    # if delete a single attribute in several entities a response list is returned, else only one response is returned.
    context.resp = context.cb.delete_entities_by_id(context, entity_id, attribute_name)
    __logger__.info("... an attribute is deleted in an entity defined by ID")


# ------------------------- verification steps ----------------------------

@step(u'verify that the attribute is deleted into mongo in the defined entity')
@step(u'verify that the attribute is deleted into mongo')
def verify_that_the_attribute_is_deleted_into_mongo(context):
    """
    verify that the attribute is deleted into mongo
    """
    props_mongo = properties_class.read_properties()[MONGO_ENV]  # mongo properties dict
    __logger__.debug("Verifying if the atribute is deleted...")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
                  password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_attribute_is_deleted(mongo, context.cb.get_entity_context(), context.cb.get_headers(), context.cb.get_entities_parameters())
    __logger__.info("...verified that the attribute is deleted")
