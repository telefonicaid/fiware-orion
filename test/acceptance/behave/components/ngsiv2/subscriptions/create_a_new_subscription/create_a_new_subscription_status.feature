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

  @status_without
  Scenario:  create a new subscription using NGSI v2 without status value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_status |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | subject_type           | room                    |
      | subject_idPattern      | .*                      |
      | condition_attrs        | temperature             |
      | condition_attrs_number | 3                       |
      | notification_http_url  | http://localhost:1234   |
      | notification_attrs     | temperature             |
      | expires                | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @status
  Scenario Outline:  create a new subscription using NGSI v2 with status value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_status |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | subject_type           | room                    |
      | subject_idPattern      | .*                      |
      | condition_attrs        | temperature             |
      | condition_attrs_number | 3                       |
      | notification_http_url  | http://localhost:1234   |
      | notification_attrs     | temperature             |
      | expires                | 2016-04-05T14:00:00.00Z |
      | status                 | <status>                |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | status   |
      | active   |
      | inactive |

  @status_wrong
  Scenario Outline:  try to create a new subscription using NGSI v2 with wrong status value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_status |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | subject_type           | room                    |
      | subject_idPattern      | .*                      |
      | condition_attrs        | temperature             |
      | condition_attrs_number | 3                       |
      | notification_http_url  | http://localhost:1234   |
      | notification_attrs     | temperature             |
      | expires                | 2016-04-05T14:00:00.00Z |
      | status                 | <status>                |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                        |
      | error       | BadRequest                                                   |
      | description | status is not valid (it has to be either active or inactive) |
    Examples:
      | status        |
      |               |
      | fsdfsdf       |
      | expired       |
      | random=100000 |

  @status_not_string
  Scenario Outline:  try to create a new subscription using NGSI v2 with not string type in status
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_status |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | ".*"                    |
      | condition_attrs       | "temperature"           |
      | notification_http_url | "http://localhost:1234" |
      | notification_attrs    | "temperature"           |
      | status                | <status>                |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | status is not a string |
    Examples:
      | status        |
      | true          |
      | ["er", 34]    |
      | {"value": 34} |
      | 234324324     |

  @status_forbidden_chars
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden chars in status value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_status |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | status                 | <status>                |
      | subject_type           | room                    |
      | subject_idPattern      | .*                      |
      | condition_attrs        | temperature             |
      | condition_attrs_number | 3                       |
      | notification_http_url  | http://localhost:1234   |
      | notification_attrs     | temperature             |
      | expires                | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                        |
      | error       | BadRequest                                                   |
      | description | status is not valid (it has to be either active or inactive) |
    Examples:
      | status      |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
