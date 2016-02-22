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
import json
import time

__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'

import requests
from integration.tools.general_utils import start_mock, check_world_attribute_is_not_none, pretty, check_key_value
from lettuce import step, world
from integration.tools.responses_predefined import responses

# Mock start
@step('a started mock')
def start_mock_step(step):
    world.mock = start_mock()
    time.sleep(1)


@step('set the response of the mock in the path "([^"]*)" as "([^"]*)"')
def set_the_Response_of_the_context_provider_mock_as(step, url, response):
    mock_url = 'http://{mock_ip}:{mock_port}{url}/mock_configurations'.format(
        mock_ip=world.config['mock']['host'], mock_port=world.config['mock']['port'], url=url)
    resp = requests.request('post', mock_url, data=json.dumps(responses[response]))
    assert 204 == resp.status_code


@step('retrieve information from the mock')
def get_mock_information(step):
    world.mock_data = requests.request('get', 'http://{mock_ip}:{mock_port}/queues'.format(
        mock_ip=world.config['mock']['host'], mock_port=world.config['mock']['port']))


@step('the path in the last mock request contains "([^"]*)"')
def the_path_in_the_last_request_contains(step, content):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)

    try:
        path = mock_response['requests'].keys()[0]
        assert path.find(
            content) >= 0, 'The content "{content}" is not in the path of the last request in the mock "{path}"'.format(
            content=str(content), path=str(path))
    except KeyError as e:
        world.log.error(mock_response)
        raise e


@step('headers of the last mock request contains the head "([^"]*)" with the value "([^"]*)"')
def the_path_in_the_last_request_contains(step, head, value):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)['requests']
    elements = len(mock_response.keys())
    try:
        headers = mock_response[mock_response.keys()[elements - 1]][0]['headers']
        assert head.lower() in headers and headers[head.lower()] == value, \
            'The head {head} does not exist or the value is not {value},\
             check the headers forwarded {headers}' \
                .format(head=head, value=value, headers=headers)
    except KeyError as e:
        world.log.error('The response received is: {response}'.format(response=mock_response))
        raise e


@step('headers of the last mock request not contains the head "([^"]*)"$')
def the_path_in_the_last_request_contains(step, head):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)['requests']
    elements = len(mock_response.keys())
    try:
        headers = mock_response[mock_response.keys()[elements - 1]][0]['headers']
        assert head.lower() not in headers, \
            'The head {head} exist in the headers forwarded,\
             check the headers forwarded {headers}' \
                .format(head=head.lower(), headers=headers)
    except KeyError as e:
        world.log.error('The response received is: {response}'.format(response=mock_response))
        raise e


@step('there is "([^"]*)" requests sent to the mock')
def there_is_requests_requested_to_the_mock(step, number_requests):
    check_world_attribute_is_not_none(['mock_data'])
    requests = json.loads(world.mock_data.text)['requests']
    number_mock_requests = 0
    for url in requests.keys():
        number_mock_requests += len(requests[url])
    assert str(number_requests) == str(
        number_mock_requests), 'The requests to the mock were {number_mock_requests} and the expeted are {number_expected}'.format(
        number_mock_requests=number_mock_requests, number_expected=number_requests)

@step('print the information stored in the mock')
def print_the_information_stored_in_the_mock(step):
    print pretty(json.loads(world.mock_data.text))

@step('the "([^"]*)" requests of the mock has the key "([^"]*)" with the value "([^"]*)"')
def the_requests_of_the_mock_has_the_key_with_the_value(step, number_request, key, value):
    assert check_key_value(world.mock_data.json()['requests'][world.mock_data.json()['requests'].keys()[0]][int(number_request)-1], key,
                           value) is False, 'The key {key} is in the response and has the value {value}. \
                           Response: {response}'.format(
        key=key, value=value, response=world.responses[world.response_count])

