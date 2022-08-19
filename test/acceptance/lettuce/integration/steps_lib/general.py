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
import time

__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'

from iotqautils.cb_utils import CbNgsi10Utils, CbNgsi9Utils
from integration.tools.general_utils import check_key_value, drop_database
from lettuce import step, world


@step('a new "([^"]*)" api request with the service "([^"]*)" and the subservice "([^"]*)"')
def a_new_request_with_the_service_and_the_subservice(step, ngsi_version, service, subservice):
    world.entities = None
    world.attributes_consult = None
    world.attributes_creation = None
    world.context_elements = None
    world.notify_conditions = None
    world.context_registration = None
    world.mock_data = None
    if service == 'empty':
        world.service = None
    else:
        world.service = service
    if subservice == 'empty':
        world.subservice = None
    else:
        world.subservice = subservice
    world.payloads_count += 1
    world.response_count += 1
    world.cb_count += 1
    if ngsi_version == 'NGSI10':
        world.cb[world.cb_count] = CbNgsi10Utils(world.config['context_broker']['host'], world.service,
                                                 world.subservice,
                                                 port=world.config['context_broker']['port'], log_verbosity='ERROR', log_instance=world.log)
    elif ngsi_version == 'NGSI9':
        world.cb[world.cb_count] = CbNgsi9Utils(world.config['context_broker']['host'], world.service, world.subservice,
                                                port=world.config['context_broker']['port'], log_verbosity='ERROR', log_instance=world.log)
    else:
        raise ValueError(
            'The version of ngsi api have to be \'NGSI9\' or \'NGSI10\', not {ngsi_version}'.format(ngsi_version=ngsi_version))


@step('add the following headers to the request')
def add_the_following_headers_to_the_request(step):
    """
    Add headers of the table to the cb instance, the format of the table is
    | header | value |
    :param step:
    :return:
    """
    for line in step.hashes:
        if 'header' in line and 'value' in line:
            world.cb[world.cb_count].headers.update({line['header']: line['value']})


@step('print the request and the response')
def print_the_request_and_the_response(step):
    print "###################################"
    for i in range(0, world.payloads_count + 1):
        print world.payloads[i]
        print "***************************"
    print "-----------------------------------"
    for i in range(0, world.response_count + 1):
        print world.responses[i].text
        print "***************************"
    print "-----------------------------------"

@step('wait "([^"]*)" seconds')
def wait_seconds(step, seconds):
    time.sleep(int(seconds))


# Response utils
@step('check the response has the key "([^"]*)" with the value "([^"]*)"')
def check_the_response_has_the_key_with_the_value(step, key, value):
    assert check_key_value(world.responses[world.response_count].json(), key,
                           value), 'The key {key} is not in the response or has not the value {value}. \
                           Response: {response}'.format(
        key=key, value=value, response=world.responses[world.response_count])


@step('check the response has not the key "([^"]*)" with the value "([^"]*)"')
def check_the_response_has_not_the_key_with_the_value(step, key, value):
    assert check_key_value(world.responses[world.response_count].json(), key,
                           value) is False, 'The key {key} is in the response and has the value {value}. \
                           Response: {response}'.format(
        key=key, value=value, response=world.responses[world.response_count])


@step('check the response has not the key "([^"]*)"$')
def check_the_response_has_not_the_key(step, key):
    assert key not in world.responses[world.response_count].json(), 'The key {key} is in the response. Response: \
    {response}'.fromat(key=key, respose=world.responses[world.response_count])


@step('check the response has the key "([^"]*)"$')
def check_the_response_has_the_key(step, key):
    assert key in world.responses[
        world.response_count].json(), 'The key {key} is not in the response. \
        Response: {response}'.fromat(key=key, response=world.responses[world.response_count])


# Mongho utils
@step('clean the mongo database of the service "([^"]*)"')
def clean_the_mongo_database_of_the_service(step, service):
    drop_database(world.config['mongo']['host'], int(world.config['mongo']['port']), service)

