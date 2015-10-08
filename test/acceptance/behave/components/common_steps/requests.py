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
from iotqatools.cb_v2_utils import CB
from iotqatools.mongo_utils import Mongo

from tools.properties_config import Properties  # methods in properties class
from tools.NGSI_v2 import NGSI


# constants
CONTEXT_BROKER_ENV = u'context_broker_env'

# HTTP status code
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
                'Length Required': 411,
                'Request Entity Too Large': 413,
                'Unsupported Media Type': 415,
                'Unprocessable Entity': 422,
                'Internal Server Error': 500}


# ------------- general_operations.feature -----------------------------------------
behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

# --------------- general_operations ----------------------


@step(u'send a API entry point request')
def send_a_base_request(context):
    """
    send a API entry point request
    :param context:
    """
    global cb, resp
    __logger__.debug("Sending a API entry point request: /v2 ...")
    properties_class = Properties()
    props = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props["CB_PROTOCOL"], host=props["CB_HOST"], port=props["CB_PORT"])
    resp = cb.get_base_request()
    __logger__.info("...Sent a API entry point request: /v2 correctly")


@step(u'send a version request')
def send_a_version_request(context):
    """
    send a version request

    """
    global cb, resp, props_cb_env
    __logger__.debug("Sending a version request...")
    properties_class = Properties()
    props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props_cb_env["CB_PROTOCOL"], host=props_cb_env["CB_HOST"], port=props_cb_env["CB_PORT"])
    resp = cb.get_version_request()
    __logger__.info("..Sent a version request correctly")


@step(u'send a statistics request')
def send_a_statistics_request(context):
    """
    send a statistics request
    :param context:
    """
    global cb, resp, props_cb_env
    __logger__.debug("Sending a statistics request...")
    properties_class = Properties()
    props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    cb = CB(protocol=props_cb_env["CB_PROTOCOL"], host=props_cb_env["CB_HOST"], port=props_cb_env["CB_PORT"])
    resp = cb.get_statistics_request()
    __logger__.info("..Sent a statistics request correctly")

# ------------------ create_entities ------------------------------------------------


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


@step(u'create an entity and attribute with special values in raw')
def create_an_entity_and_attribute_with_special_values(context):
    """
    create an entity and attribute with special values in raw
       ex:
             "value": true
             "value": false
             "value": 34
             "value": 5.00002
             "value": [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]
             "value": {"x": {"x1": "a","x2": "b"}}
             "value": "41.3763726, 2.1864475,14"  -->  "type": "geo:point"
             "value": "2017-06-17T07:21:24.238Z"  -->  "type: "date"
        Some cases are not parsed correctly to dict in python
    :param context: context variable (optional parameters)
    """
    global cb, resp
    resp = cb.create_entity_raw(context)


@step(u'delete database in mongo')
def delete_database_in_mongo(context):
    """
    Delete database used in mongo
    """
    fiware_service_header = u'Fiware-Service'
    orion_prefix = u'orion'
    database_name = EMPTY
    global cb

    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    m = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    headers = cb.get_headers()
    __logger__.debug("Deleting database in mongo...")
    if fiware_service_header in headers:
        if headers[fiware_service_header] == EMPTY:
            database_name = orion_prefix
        elif headers[fiware_service_header].find(".") < 0:
            database_name = "%s-%s" % (orion_prefix, headers[fiware_service_header])
    else:
        database_name = orion_prefix
    if database_name != EMPTY:
        m.connect(database_name.lower())
        m.drop_database()
        m.disconnect()
        __logger__.debug("...Database \"%s\" is deleted" % database_name.lower())

# ------------------------- list entities ----------------------------


@step(u'get all entities')
def get_all_entities(context):
    """
    list all entities
    """
    global cb, resp
    __logger__.debug("getting a list with all entities in a service...")
    resp = cb.list_all_entities(context)
    __logger__.info("...returned a list with all entities in a service")


@step(u'get an entity by ID "([^"]*)"')
def get_an_entity_by_id(context, entity_id):
    """
    get an entity by ID
    """
    global cb, resp
    __logger__.debug("getting an entity by id...")
    resp = cb.list_an_entity_by_ID(context, entity_id)
    __logger__.debug("...returned an entity by id...")


@step(u'get an attribute "([^"]*)" by ID "([^"]*)"')
def get_an_attribute_by_id(context, attribute_name, entity_id):
    """
    get an attribute by ID
    :param context:
    :param attribute_name:
    :param entity_id:
    """
    global cb, resp
    __logger__.debug("getting an attribute by id...")
    resp = cb.list_an_attribute_by_ID(attribute_name, entity_id)
    __logger__.debug("...returned an attribute by id...")


# ------------------------ update and append -----------------------------------------------


@step(u'update or append attributes by ID "([^"]*)"')
def update_or_append_an_attribute_by_id(context, entity_id):
    """
    update or append attributes by ID
    :param context:
    :param entity_id:
    """
    global cb, resp
    __logger__.debug("updating or appending an attribute by id...")
    resp = cb.update_or_append_an_attribute_by_id("POST", context, entity_id)
    __logger__.debug("...updated or appended an attribute by id...")


@step(u'update or append attributes by ID "([^"]*)" in raw mode')
def update_or_append_an_attribute_by_ID_in_raw_mode(context, entity_id):
    """
    update or append attributes by ID in raw mode
    :param context:
    :param entity_id:
    """
    global cb, resp
    __logger__.debug("updating or appending an attribute by id in raw mode...")
    resp = cb.update_or_append_an_attribute_in_raw_by_id("POST", context, entity_id)
    __logger__.debug("...updated or appended an attribute by id in raw mode...")


@step(u'update an attribute by ID "([^"]*)" if it exists')
def update_an_attribute_by_ID_if_it_exists(context, entity_id):
    """
    update an attribute by ID if it exists
    :param context:
    :param entity_id: id name
    """
    global cb, resp
    __logger__.debug("updating or appending an attribute by id if it exists...")
    resp = cb.update_or_append_an_attribute_by_id("PATCH", context, entity_id)
    __logger__.debug("...updated or appended an attribute by id if it exists...")


@step(u'update an attribute by ID "([^"]*)" if it exists in raw mode')
def update_an_attribute_by_ID_if_it_exists_in_raw_mode(context, entity_id):
    """
    update or append attributes by ID in raw mode
    :param context:
    :param entity_id:
    """
    global cb, resp
    __logger__.debug("updating or appending an attribute by id in raw mode if it exists...")
    resp = cb.update_or_append_an_attribute_in_raw_by_id("PATCH", context, entity_id)
    __logger__.debug("...updated or appended an attribute by id in raw mode if it exists...")


@step(u'replace attributes by ID "([^"]*)"')
def replace_attributes_by_id(context, entity_id):
    """
    replace attributes by ID
    :param context:
    :param entity_id: id name
    """
    global cb, resp
    __logger__.debug("replacing attributes by id...")
    resp = cb.update_or_append_an_attribute_by_id("PUT", context, entity_id)
    __logger__.debug("...replaced attributes by id...")


@step(u'replace attributes by ID "([^"]*)" in raw mode')
def replace_attributes_by_idin_raw_mode(context, entity_id):
    """
    replace attributes by ID
    :param context:
    :param entity_id: id name
    """
    global cb, resp
    __logger__.debug("replacing attributes by id in raw mode...")
    resp = cb.update_or_append_an_attribute_in_raw_by_id("PUT", context, entity_id)
    __logger__.debug("...replaced attributes by id in raw mode...")

# ------------------------------------- validations ----------------------------------------------


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
    __logger__.info('...Verified that http code returned is "%s"' % http_code)


@step(u'verify "([^"]*)" url with "([^"]*)" value in response')
def verify_entry_point(context, url, value):
    """
    verify API entry point response.
         Ex:
            {
                "entities_url":"/v2/entities",
                "types_url":"/v2/types",
                "subscriptions_url":"/v2/subscriptions",
                "registrations_url":"/v2/registrations"
            }
    :param context:
    :param url:
    :param value:
    """
    global resp
    __logger__.debug("Verifying url in API entry point response...")
    resp_dict = convert_str_to_dict(resp.text, "JSON")
    assert resp_dict[url] == value, " ERROR - in \"%s\" url with  \"%s\" value " % (url, value)
    __logger__.info("...Verified url in API entry point response")


@step(u'verify statistics "([^"]*)" field does exists')
def verify_stat_fields(context, field):
    """
    verify statistics fields in response.
    Ex:
            {
              "orion" : {
                    "xmlRequests" : "5",
                    "versionRequests" : "1",
                    "statisticsRequests" : "2",
                    "uptime_in_secs" : "364",
                    "measuring_interval_in_secs" : "364"
              }
            }
    :param context:
    :param field: field to verify if it does exists
    """
    global resp
    __logger__.debug("Verifying statistics field: %s does exists..." % field)
    resp_dict = convert_str_to_dict(resp.text, "JSON")
    assert "orion" in resp_dict, "ERROR - orion field does no exist in statistics response"
    assert field in resp_dict["orion"], "ERROR - %s field does no exist in statistics response" % field
    __logger__.info("...Verified that statistics field %s is correct" % field)


@step(u'verify version "([^"]*)" field does exists')
def verify_version_fields(context, field):
    """
    verify version fields in response.
    Ex:
            {
              "orion" : {
                  "version" : "0.23.0_20150722131636",
                  "uptime" : "0 d, 0 h, 4 m, 46 s",
                  "git_hash" : "3c0767f91997a25925229b836dc48bba0f4801ba",
                  "compile_time" : "Wed Jul 22 13:18:54 CEST 2015",
                  "compiled_by" : "develenv",
                  "compiled_in" : "ci-fiware-01"
              }
            }
    :param context:
    :param field: field to verify if it does exists
    """
    global resp
    __logger__.debug("Verifying version field: %s does exists..." % field)
    resp_dict = convert_str_to_dict(resp.text, "JSON")
    assert "orion" in resp_dict, "ERROR - orion field does no exist in version response"
    assert field in resp_dict["orion"], "ERROR - %s field does no exist in version response" % field
    __logger__.info("...Verified that version field %s is correct" % field)


@step(u'verify if version is the expected')
def verify_if_version_is_the_expected(context):
    """
    verify if version is the expected
    """
    global resp, props_cb_env
    resp_dict = convert_str_to_dict(str(resp.text), "JSON")
    assert resp_dict["orion"]["version"].find(props_cb_env["CB_VERSION"]) >= 0, \
        " ERROR in context broker version value, \n" \
        " expected: %s \n" \
        " installed: %s" % (props_cb_env["CB_VERSION"], resp_dict["orion"]["version"])
    __logger__.debug("-- version %s is correct in version request" % props_cb_env["CB_VERSION"])


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
    for i in range(int(entities_context["entities_number"])):
        assert resp_list[i].status_code == status_codes[http_code], " ERROR - http code is wrong in position: %s \n" \
                                                                    " expected: %s \n" \
                                                                    " received: %s" % (
                                                                    str(i), str(status_codes[http_code]),
                                                                    str(resp_list[i].status_code))
        __logger__.debug(" -- status code \"%s\" is the expected in position: %s" % (http_code, str(i)))
    __logger__.info("...Verified that http code returned in all entities are %s" % http_code)


@step(u'verify that entities are stored in default tenant at mongo')
@step(u'verify that entities are stored in mongo')
def entities_are_stored_in_mongo(context):
    """
    verify that entities are stored in mongo
    """
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    __logger__.debug(" >> verifying entities are stored in mongo")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_entities_stored_in_mongo(mongo, cb.get_entity_context(), cb.get_headers())
    __logger__.info(" >> verified entities are stored in mongo")


@step(u'verify that entities are not stored in mongo')
def entities_are_not_stored_in_mongo(context):
    """
    verify that entities are not stored in mongo
    """
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    __logger__.debug(" >> verifying entities are not stored in mongo")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_entities_stored_in_mongo(mongo, cb.get_entity_context(), cb.get_headers(), False)
    __logger__.info(" >> verified entities are not stored in mongo")


@step(u'verify that an entity is updated in mongo')
def verify_that_an_entity_is_updated_in_mongo(context):
    """
    verify that an entity is updated in mongo
    """
    global cb
    properties_class = Properties()
    props_mongo = properties_class.read_properties()["mongo_env"]  # mongo properties dict
    __logger__.debug(" >> verifying that an entity is updating in mongo")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_entity_updated_in_mongo(mongo, cb.get_entity_context(), cb.get_headers())
    __logger__.info(" >> verified that an entity is updated in mongo")

@step(u'verify an error response')
def verify_error_message(context):
    """
    verify error response
    :param context: parameters to evaluate
    """
    global cb, resp
    __logger__.debug("Verifying error message ...")
    ngsi = NGSI()
    ngsi.verify_error_response(context, resp)
    __logger__.info("...Verified that error message is the expected")


@step(u'verify several error responses')
def verify_error_message(context):
    """
    verify error response
    :param context: parameters to evaluate
    """
    global cb, resp_list
    __logger__.debug("Verifying error message in several entities...")
    entities_context = cb.get_entity_context()
    ngsi = NGSI()
    for i in range(int(entities_context["entities_number"])):
        ngsi.verify_error_response(context, resp_list[i])
    __logger__.info("...Verified that error message is the expected in all entities ")


@step(u'verify that all entities are returned')
def verify_get_all_entities(context):
    """
    verify get all entities
    """
    global cb, resp
    __logger__.debug("Verifying all entities are returned in get request...")
    queries_parameters = cb.get_entities_parameters()
    entities_context = cb.get_entity_context()
    ngsi = NGSI()
    ngsi.verify_get_all_entities(queries_parameters, entities_context, resp)
    __logger__.info("...Verified all entities are returned in get request...")


@step(u'verify an entity in raw mode with type "([^"]*)" in attribute value from http response')
def verify_http_response_in_raw_mode_witn_type(context, field_type):
    """
    verify http response in raw mode and type in attribute value from http response
    """
    global cb, resp
    __logger__.debug("Verifying http response in raw mode from http response...")
    entities_context = cb.get_entity_context()
    ngsi = NGSI()
    ngsi.verify_entity_raw_mode_http_response(entities_context, resp, field_type)
    __logger__.info("...Verified http response in raw mode from http response...")


@step(u'verify that the entity by ID is returned')
def verify_that_the_entity_by_id_is_returned(context):
    """
    verify that the entity by ID is returned
    """
    global cb, resp
    __logger__.debug("Verifying an entity by ID returned from a request...")
    queries_parameters = cb.get_entities_parameters()
    entities_context = cb.get_entity_context()
    entity_id_to_request = cb.get_entity_id_to_request()
    ngsi = NGSI()
    ngsi.verify_an_entity_by_id(queries_parameters, entities_context, resp, entity_id_to_request)
    __logger__.info("...Verified an entity by ID returned from a request...")


@step(u'verify that the attribute by ID is returned')
def verify_that_the_attribute_by_id_is_returned(context):
    """
    verify that the attribute by ID is returned
    """
    global cb, resp
    __logger__.debug("Verifying an attribute by ID returned from a request...")
    entities_context = cb.get_entity_context()
    attribute_name_to_request = cb.get_attribute_name_to_request()
    ngsi = NGSI()
    ngsi.verify_an_attribute_by_id(entities_context, resp, attribute_name_to_request)
    __logger__.info("...Verified an attribute by ID returned from a request...")


@step(u'verify an attribute by ID in raw mode with type "([^"]*)" in attribute value from http response')
def verify_an_attribute_by_id_in_raw_mode_from_http_response(context, field_type):
    """
    verify an attribute by ID in raw mode with type in attribute value from http response
    :param context:
    :param field_type:
    :return:
    """
    global cb, resp
    __logger__.debug("Verifying an attribute by ID returned in raw mode from http response...")
    entities_context = cb.get_entity_context()
    attribute_name_to_request = cb.get_attribute_name_to_request()
    ngsi = NGSI()
    ngsi.verify_an_attribute_by_id_in_raw_mode_http_response(entities_context, resp, attribute_name_to_request, field_type)
    __logger__.info("...Verified an attribute by ID returned in raw mode from http response...")
