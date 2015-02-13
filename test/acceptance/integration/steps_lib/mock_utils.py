# -*- coding: utf-8 -*-
"""
# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

__author__ = 'Jon Calderin Goñi (jcaldering@gmail.com)'

import requests
from integration.tools.general_utils import start_mock, check_world_attribute_is_not_none
from lettuce import step, world
from integration.tools.responses_predefined import responses

# Mock start
@step('a started mock')
def start_mock_step(step):
    world.mock = start_mock()

@step('set the response of the context provider mock as "([^"]*)"')
def set_the_Response_of_the_context_provider_mock_as(step, response):
    assert 'Response set' == requests.request('post', 'http://{mock_ip}:{mock_port}/set_response'.format(
        mock_ip=world.config['mock']['host'], mock_port=world.config['mock']['port']), data=responses[response])


@step('retrieve information from the mock')
def get_mock_information(step):
    world.mock_data = requests.request('get', 'http://{mock_ip}:{mock_port}/get_data'.format(
        mock_ip=world.config['mock']['host'], mock_port=world.config['mock']['port']))


@step('the path in the last mock petition contains "([^"]*)"')
def the_path_in_the_last_petition_contains(step, content):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)
    elements = len(mock_response.keys())
    try:
        path = mock_response[str(elements - 1)]['path']
        assert path.find(
            content) >= 0, 'The content "{content}" is not in the path of the last petition in the mock "{path}"'.format(
            content=str(content), path=str(path))
    except KeyError as e:
        print mock_response
        raise e


@step('headers of the last mock petition contains the head "([^"]*)" with the value "([^"]*)"')
def the_path_in_the_last_petition_contains(step, head, value):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)
    elements = len(mock_response.keys())
    try:
        headers = mock_response[str(elements - 1)]['headers']
        print headers
        assert head in headers and headers[head] == value, \
            'The head {head} does not exist or the value is not {value},\
             check the headers forwarded {headers}' \
                .format(head=head, value=value, headers=headers)
    except KeyError as e:
        print 'The response received is: {response}'.format(response=mock_response)
        raise e


@step('headers of the last mock petition not contains the head "([^"]*)"$')
def the_path_in_the_last_petition_contains(step, head):
    check_world_attribute_is_not_none(['mock_data'])
    mock_response = eval(world.mock_data.text)
    elements = len(mock_response.keys())
    try:
        headers = mock_response[str(elements - 1)]['headers']
        assert head not in headers, \
            'The head {head} exist in the headers forwarded,\
             check the headers forwarded {headers}' \
                .format(head=head, headers=headers)
    except KeyError as e:
        print 'The response received is: {response}'.format(response=mock_response)
        raise e


@step('there is "([^"]*)" petitions requested to the mock')
def there_is_petitions_requested_to_the_mock(step, number_requests):
    check_world_attribute_is_not_none(['mock_data'])
    number_mock_requests = len(eval(world.mock_data.text).keys())
    assert str(number_requests) == str(
        number_mock_requests), 'The requests to the mock were {number_mock_requests} and the expeted are {number_expected}'.format(
        number_mock_requests=number_mock_requests, number_expected=number_requests)

