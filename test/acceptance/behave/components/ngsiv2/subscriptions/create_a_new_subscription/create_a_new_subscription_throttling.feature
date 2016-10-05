# -*- coding: utf-8 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# author: 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

#
#  Note: the "skip" tag is to skip the scenarios that still are not developed or failed
#        -tg=-skip
#

Feature: create new subscriptions (POST) using NGSI v2. "POST" - /v2/subscriptions plus payload
  As a context broker user
  I would like to create new subscriptions (POST) using NGSI v2
  So that I can manage and use them in my scripts

  Actions Before the Feature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file
  Setup: start ContextBroker
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  Actions After each Scenario:
  Setup: delete database in mongo

  Actions After the Feature:
  Setup: stop ContextBroker

  @throttling_without
  Scenario:  create a new subscription using NGSI v2 without throttling field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_csub_throttling |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | without condition field |
      | notification_http_url | http://localhost:1234   |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @throttling
  Scenario Outline:  create a new subscription using NGSI v2 with throttling value
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_csub_throttling |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | without condition field |
      | notification_http_url | http://localhost:1234   |
      | throttling            | <throttling>            |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | throttling |
      | 0          |
      | 1          |
      | 100        |
      | 1000       |
      | 10000      |
      | 100000     |
      | 1000000    |
      | 10000000   |

  @throttling_not_int
  Scenario Outline:  try to create a new subscription using NGSI v2 with not int type in throttling
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | ".*"                    |
      | condition_attrs       | "temperature"           |
      | notification_http_url | "http://localhost:1234" |
      | notification_attrs    | "temperature"           |
      | throttling            | <throttling>            |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | throttling is not an int |
    Examples:
      | throttling    |
      | true          |
      | ["er", 34]    |
      | {"value": 34} |
      | "234324324"   |
      | 23432.45      |

  @throttling_invalid
  Scenario Outline:  try to create a new subscription using NGSI v2 with invalid value in throttling
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | ".*"                    |
      | condition_attrs       | "temperature"           |
      | notification_http_url | "http://localhost:1234" |
      | notification_attrs    | "temperature"           |
      | throttling            | <throttling>            |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | throttling |
      | 23432,45   |
      | (23432,45) |
