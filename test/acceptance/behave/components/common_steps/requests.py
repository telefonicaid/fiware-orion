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


from iotqautils.helpers_utils import *
from iotqautils.CB_v2_utils import CB
from iotqautils.mongo_utils import Mongo

from tools.properties_config import Properties  # methods in properties class
from tools.NGSI_v2 import NGSI


# constants
CONTEXT_BROKER_ENV = u'context_broker_env'

#HTTP status code
status_codes = {'OK': 200,
                'Created': 201,
                'No Content': 204,
                'Moved Permanently': 301,
                'Redirect': 307,
                'Bad Request': 400,
                'unauthorized': 401,
                'Not Found': 404,
                'Bad Method': 405,
                'Not Acceptable': 406,
                'Conflict': 409,
                'Unsupported Media Type': 415,
                'Internal Server Error': 500}


# ------------- base.feature -----------------------------------------
behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


@step(u'send a base request')
def send_a_base_request(context):
    """
    send a base request
    :param context:
    """
    global cb, resp
    __logger__.debug("Sending a base request: /v2 ...")
    properties_class = Properties()
    props = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props["CB_PROTOCOL"], host=props["CB_HOST"], port=props["CB_PORT"])
    resp = cb.get_base_request()
    __logger__.info("...Sent a base request: /v2 correctly")


@step(u'send a version request')
def send_a_base_request(context):
    """
    send a version request
    :param context:
    """
    global cb, resp, props_cb_env
    __logger__.debug("Sending a version request...")
    properties_class = Properties()
    props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props_cb_env["CB_PROTOCOL"], host=props_cb_env["CB_HOST"], port=props_cb_env["CB_PORT"])
    resp = cb.get_version_request()
    __logger__.info("..Sent a version request correctly")


@step(u'send a statistics request')
def send_a_base_request(context):
    """
    send a version request
    :param context:
    """
    global cb, resp, props_cb_env
    __logger__.debug("Sending a statistics request...")
    properties_class = Properties()
    props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props_cb_env["CB_PROTOCOL"], host=props_cb_env["CB_HOST"], port=props_cb_env["CB_PORT"])
    resp = cb.get_statistics_request()
    __logger__.info("..Sent a statistics request correctly")


@step(u'a definition of headers')
def service_and_service_path(context):
    """
    configuration of service an service path in headers
    :param context:
    """
    global cb
    properties_class = Properties()
    props = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props["CB_PROTOCOL"], host=props["CB_HOST"], port=props["CB_PORT"])
    cb.definition_headers(context)


@step(u'create "([^"]*)" entities with "([^"]*)" attributes')
def create_entities_with_properties(context, entities_number, attributes_number):
    """
    create N entities with N attributes and another optional parameters:
        -  entity_type, entity_id, attributes_name, attributes_value, attribute_type, metadata_name and metadata_value
    :param context: context variable (optional parameters)
    :param entities_number: number of entities
    :param attributes_number: number of attributes
    """
    global cb, resp_list
    resp_list = cb.create_entities(context, entities_number, attributes_number)


@step(u'delete database in mongo')
def delete_database_in_mongo(context):
    """
    Delete database used in mongo
    """
    fiware_service_header = u'Fiware-Service'
    orion_prefix = u'orion'
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    m = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    headers = cb.get_headers()
    if fiware_service_header in headers and (headers[fiware_service_header].find(".") < 0):
        __logger__.debug("Deleting database in mongo...")
        if fiware_service_header in headers:
            database_name = "%s-%s" % (orion_prefix, headers[fiware_service_header])
        else:
            database_name = orion_prefix
        m.connect(database_name.lower())
        m.drop_database()
        m.disconnect()
        __logger__.debug("...Database \"%s\" is deleted" % database_name.lower())


#  ------------------------------------- validations ----------------------------------------------
@step(u'verify that receive an "([^"]*)" http code')
def verify_that_receive_an_http_code(context, http_code):
    """
    verify that receive an http code
    :param context:
    :param http_code:
    """
    global resp
    __logger__.debug("Verifying that return an http codes...")
    assert resp.status_code == status_codes[http_code], " ERROR - http code is wrong\n" \
                                                        " expected: %s \n" \
                                                        " received: %s" % (
                                                        str(status_codes[http_code]), str(resp.status_code))
    __logger__.info("...Verified that http code returned is %s" % http_code)


@step(u'verify main paths')
def verify_main_paths(context):
    """
    verify main paths
    :param context:
    """
    global resp
    __logger__.debug("Verifying main paths...")
    ngsi = NGSI()
    ngsi.verify_main_paths(resp)
    __logger__.info("...Verified that main paths are correct")


@step(u'verify statistics')
def verify_main_paths(context):
    """
    verify statistics
    :param context:
    """
    global resp
    __logger__.debug("Verifying statistics fields...")
    ngsi = NGSI()
    ngsi.verify_statistics(resp)
    __logger__.info("...Verified that statistics fields are correct")


@step(u'verify if version is the expected')
def verify_if_version_is_the_expected(context):
    """
    verify if version is the expected
    """
    global resp, props_cb_env

    if props_cb_env["CB_VERIFY_VERSION"].lower() == "true":
        resp_dict = convert_str_to_dict(str(resp.text), "JSON")
        assert resp_dict["orion"]["version"].find(
            props_cb_env["CB_VERSION"]) >= 0, " ERROR in context broker version  value, \n" \
                                              " expected: %s \n" \
                                              " installed: %s" % (
                                              props_cb_env["CB_VERSION"], resp_dict["orion"]["version"])
        __logger__.debug("-- version %s is correct in base request v2" % props_cb_env["CB_VERSION"])


@step(u'verify that receive several "([^"]*)" http code')
def verify_that_receive_several_http_codes(context, http_code):
    """
    verify that receive several http codes in multi entities
    :param context:
    :param http_code: http code in all entities
    """
    global cb, resp_list
    __logger__.debug("Verifying that return an http code in several entities...")
    entities_context = cb.get_entity_context()
    for i in range(entities_context["entities_number"]):
        assert resp_list[i].status_code == status_codes[http_code], " ERROR - http code is wrong in position: %s \n" \
                                                                    " expected: %s \n" \
                                                                    " received: %s" % (
                                                                    str(i), str(status_codes[http_code]),
                                                                    str(resp_list[i].status_code))
        __logger__.debug(" -- status code \"%s\" is the expected in position: %s" % (http_code, str(i)))

    __logger__.info("...Verified that http code returned in all entities are %s" % http_code)


@step(u'verify that entities are stored in mongo')
def entities_are_stored_in_mongo(context):
    """
    verify that entities are stored in mongo
    """
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    __logger__.debug(" >> verifying entities are stored in mongo")
    m = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_entities_stored_in_mongo(m, cb.get_entity_context(), cb.get_headers())
    __logger__.info(" >> verified entities are stored in mongo")


@step(u'verify that entities are not stored in mongo')
def entities_are_not_stored_in_mongo(context):
    """
    verify that entities are stored in mongo
    """
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    __logger__.debug(" >> verifying entities are not stored in mongo")
    m = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_entities_stored_in_mongo(m, cb.get_entity_context(), cb.get_headers(), False)
    __logger__.info(" >> verified entities are not stored in mongo")


@step(u'verify error response')
def verify_error_message(context):
    """
    verify error response
    :param context: parameters to evaluate
    """
    global resp_list
    ngsi = NGSI()
    ngsi.verify_error_response(context, resp_list)