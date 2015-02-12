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

from iotqautils.cb_utils import CbNgsi10Utils, CbNgsi9Utils
import requests
from integration.tools.general_utils import check_key_value, drop_database
from lettuce import step, world


@step('a new NGSI version "([^"]*)" petition with the service "([^"]*)" and the subservice "([^"]*)"')
def a_new_ngsi10_petition_with_the_service_and_the_subservice(step, ngsi_version, service, subservice):
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
    if ngsi_version == '10':
        world.cb[world.cb_count] = CbNgsi10Utils(world.config['context_broker']['host'], world.service,
                                                 world.subservice,
                                                 port=world.config['context_broker']['port'])
    elif ngsi_version == '9':
        world.cb[world.cb_count] = CbNgsi9Utils(world.config['context_broker']['host'], world.service, world.subservice,
                                                port=world.config['context_broker']['port'])
    else:
        raise ValueError(
            'The version of ngsi api have to be \'9\' or \'10\', not {ngsi_version}'.format(ngsi_version=ngsi_version))


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


# Response utils
@step('check the response has the key "([^"]*)" with the value "([^"]*)"')
def check_the_response_has_the_key_with_the_value(step, key, value):
    assert check_key_value(world.responses[world.response_count].json(), key, value)


@step('check the response has not the key "([^"]*)" with the value "([^"]*)"')
def check_the_response_has_the_key_with_the_value(step, key, value):
    assert check_key_value(world.responses[world.response_count].json(), key, value) == False


# Mongho utils
@step('clean the mongo database of the service "([^"]*)"')
def clean_the_mongo_database_of_the_service(step, service):
    drop_database(world.config['mongo']['host'], int(world.config['mongo']['port']), service)

