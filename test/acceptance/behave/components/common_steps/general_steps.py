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
from iotqatools.remote_log_utils import Remote_Log
from iotqatools.fabric_utils import FabricSupport

from tools.properties_config import Properties  # methods in properties class
from tools.NGSI_v2 import NGSI

# constants
properties_class = Properties()
CONTEXT_BROKER_ENV = u'context_broker_env'
MONGO_ENV = u'mongo_env'

# HTTP status code
status_codes = {'OK': 200,
                'Created': 201,
                'No Content': 204,
                'Moved Permanently': 301,
                'Redirect': 307,
                'Bad Request': 400,
                'unauthorized': 401,
                'Not Found': 404,
                'Method Not Allowed': 405,
                'Not Acceptable': 406,
                'Conflict': 409,
                'Content Length Required': 411,
                'Request Entity Too Large': 413,
                'Unsupported Media Type': 415,
                'Unprocessable Entity': 422,
                'Internal Server Error': 500}


behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

# --------------- general_operations ----------------------


@step(u'send a API entry point request')
def send_a_base_request(context):
    """
    send a API entry point request
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Sending a API entry point request: /v2 ...")
    props = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    context.cb = CB(protocol=props["CB_PROTOCOL"], host=props["CB_HOST"], port=props["CB_PORT"])
    context.resp = context.cb.get_base_request()
    __logger__.info("...Sent a API entry point request: /v2 correctly")


@step(u'send a version request')
def send_a_version_request(context):
    """
    send a version request
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Sending a version request...")
    context.props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    context.cb = CB(protocol=context.props_cb_env["CB_PROTOCOL"], host=context.props_cb_env["CB_HOST"], port=context.props_cb_env["CB_PORT"])
    context.resp = context.cb.get_version_request()
    __logger__.info("..Sent a version request correctly")
send_a_version_request = step(u'send a version request')(send_a_version_request)


@step(u'send a statistics request')
def send_a_statistics_request(context):
    """
    send a statistics request
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Sending a statistics request...")
    context.props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    context.cb = CB(protocol=context.props_cb_env["CB_PROTOCOL"], host=context.props_cb_env["CB_HOST"], port=context.props_cb_env["CB_PORT"])
    context.resp = context.cb.get_statistics_request()
    __logger__.info("..Sent a statistics request correctly")


@step(u'send a cache statistics request')
def send_a_cache_statistics_request(context):
    """
    send a cache statistics request
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Sending a statistics request...")
    context.props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    context.cb = CB(protocol=context.props_cb_env["CB_PROTOCOL"], host=context.props_cb_env["CB_HOST"], port=context.props_cb_env["CB_PORT"])
    context.resp = context.cb.get_cache_statistics_request()
    __logger__.info("..Sent a statistics request correctly")


@step(u'delete database in mongo')
def delete_database_in_mongo(context):
    """
    Delete database used in mongo
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    fiware_service_header = u'Fiware-Service'
    orion_prefix = u'orion'
    database_name = orion_prefix
    props_mongo = properties_class.read_properties()[MONGO_ENV]  # mongo properties dict
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
                  password=props_mongo["MONGO_PASS"])
    headers = context.cb.get_headers()

    if fiware_service_header in headers:
        if headers[fiware_service_header] != EMPTY:
            if headers[fiware_service_header].find(".") < 0:
                database_name = "%s-%s" % (database_name, headers[fiware_service_header].lower())
            else:
                postfix = headers[fiware_service_header].lower()[0:headers[fiware_service_header].find(".")]
                database_name = "%s-%s" % (database_name, postfix)

    __logger__.debug("Deleting database \"%s\" in mongo..." % database_name)
    mongo.connect(database_name)
    mongo.drop_database()
    mongo.disconnect()
    __logger__.info("...Database \"%s\" is deleted" % database_name)


@step(u'check in log, label "([^"]*)" and message "([^"]*)"')
def check_in_log_label_and_text(context, label, text):
    """
    Verify in log file if a label with a message exists
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param label: label to find
    :param text: text to find (begin since the end)
    """
    __logger__.debug("Looking for in log the \"%s\" label and the \"%s\" text..." % (label, text))
    props_cb_env = properties_class.read_properties()[CONTEXT_BROKER_ENV]
    remote_log = Remote_Log(file="%s/contextBroker.log" % props_cb_env["CB_LOG_FILE"], fabric=context.my_fab)
    line = remote_log.find_line(label, text)
    assert line is not None, " ERROR - the \"%s\" label and the \"%s\" text do not exist in the log" % (label, text)
    __logger__.info("log line: \n%s" % line)
    ngsi = NGSI()
    ngsi.verify_log(context, line)
    __logger__.info("...confirmed traces in log")


@step(u'delay for "([^"]*)" seconds')
def delay_for_seconds(context, seconds):
    """
    delay for N seconds

    :param seconds: seconds to delay
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.info("delay for \"%s\" seconds" % seconds)
    time.sleep(int(seconds))


# ------------------------------------- validations ----------------------------------------------


@step(u'verify that receive a.? "([^"]*)" http code')
def verify_that_receive_an_http_code(context, http_code):
    """
    verify that receive an http code
    :param http_code:  http code expected
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("context: %s" % repr(context.resp.text))
    __logger__.debug("Verifying that return an http codes...")
    assert context.resp.status_code == status_codes[http_code], \
        " ERROR - http code is wrong\n" \
        " expected: %s \n" \
        " received: %s" % (str(status_codes[http_code]), str(context.resp.status_code))
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
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param url: url key to verify
    :param value: value expected

    """
    __logger__.debug("Verifying url in API entry point response...")
    resp_dict = convert_str_to_dict(context.resp.text, "JSON")
    assert resp_dict[url] == value, " ERROR - in \"%s\" url with  \"%s\" value " % (url, value)
    __logger__.info("...Verified url in API entry point response")


@step(u'verify statistics "([^"]*)" field does exists')
def verify_stat_fields(context, field_to_test):
    """
    verify statistics and cache statistics fields in response.
    Ex: /statistics
     {
        "uptime_in_secs":2,
        "measuring_interval_in_secs":2
     }
       /cache/statistics
     {
        "ids":"",
        "refresh":1,
        "inserts":0,
        "removes":0,
        "updates":0,
        "items":0
     }
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param field_to_test: field to verify if it does exists
    """
    __logger__.debug("Verifying statistics field: %s does exists..." % field_to_test)
    resp_dict = convert_str_to_dict(context.resp.text, "JSON")
    assert field_to_test in resp_dict.keys(), "ERROR - \"%s\" field does no exist in statistics response" % field_to_test
    __logger__.info("...Verified that statistics field %s is correct" % field_to_test)


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
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param field: field to verify if it does exists
    """
    __logger__.debug("Verifying version field: %s does exists..." % field)
    resp_dict = convert_str_to_dict(context.resp.text, "JSON")
    assert "orion" in resp_dict, "ERROR - orion field does no exist in version response"
    assert field in resp_dict["orion"], "ERROR - %s field does no exist in version response" % field
    __logger__.info("...Verified that version field %s is correct" % field)


@step(u'verify if version is the expected')
def verify_if_version_is_the_expected(context):
    """
    verify if version is the expected
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    resp_dict = convert_str_to_dict(str(context.resp.text), "JSON")
    assert resp_dict["orion"]["version"].find(context.props_cb_env["CB_VERSION"]) >= 0, \
        " ERROR in context broker version value, \n" \
        " expected: %s \n" \
        " installed: %s" % (context.props_cb_env["CB_VERSION"], resp_dict["orion"]["version"])
    __logger__.info("-- version %s is correct in version request" % context.props_cb_env["CB_VERSION"])


@step(u'verify that receive several "([^"]*)" http code')
def verify_that_receive_several_http_codes(context, http_code):
    """
    verify that receive several http codes in multi entities
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param http_code: http code in all entities
    """
    __logger__.debug("Verifying that return an http code in several entities...")
    entities_context = context.cb.get_entity_context()
    for i in range(int(entities_context["entities_number"])):
        assert context.resp_list[i].status_code == status_codes[http_code], \
            " ERROR - http code is wrong in position: %s \n" \
            "expected: %s \n" \
            " received: %s" % (str(i), str(status_codes[http_code]), str(context.resp_list[i].status_code))
        __logger__.debug(" -- status code \"%s\" is the expected in position: %s" % (http_code, str(i)))
    __logger__.info("...Verified that http code returned in all entities are %s" % http_code)


@step(u'verify an error response')
def verify_error_message(context):
    """
    verify error response
    :param context: parameters to evaluate. It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying error message ...")
    ngsi = NGSI()
    ngsi.verify_error_response(context, context.resp)
    __logger__.info("...Verified that error message is the expected")


@step(u'verify several error responses')
def verify_error_message(context):
    """
    verify error response
    :param context: parameters to evaluate. It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying error message in several entities...")
    entities_context = context.cb.get_entity_context()
    ngsi = NGSI()
    for i in range(int(entities_context["entities_number"])):
        ngsi.verify_error_response(context, context.resp_list[i])
    __logger__.info("...Verified that error message is the expected in all entities ")


@step(u'verify headers in response')
def verify_headers_in_response(context):
    """
    verify headers in response
    Ex:
          | parameter          | value                |
          | fiware-total-count | 5                    |
          | location           | /v2/subscriptions/.* |
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Verifying headers in response...")
    ngsi = NGSI()
    ngsi.verify_headers_response(context)
    __logger__.info("...Verified headers in response")
