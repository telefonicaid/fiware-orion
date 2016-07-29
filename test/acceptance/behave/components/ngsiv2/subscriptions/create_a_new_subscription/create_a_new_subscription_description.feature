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

  @description_without
  Scenario:  create a new subscription using NGSI v2 without description value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
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
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @description
  Scenario Outline:  create a new subscription using NGSI v2 with description value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | description            | <desc>                  |
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
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | desc                  |
      |                       |
      | my first subscription |
      | random=1024           |

  @description_length_exceed
  Scenario:  try to create a new subscription using NGSI v2 with length exceed in description value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | description            | random=1025             |
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
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | max description length exceeded |

  @description_forbidden_chars @BUG_2308
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden chars in description value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_csub_desc   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | description            | <desc>                  |
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
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | forbidden characters in description |
    Examples:
      | desc        |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  @description_not_string
  Scenario Outline:  try to create a new subscription using NGSI v2 with not string type in description
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
      | description           | <desc>                  |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                       |
      | error       | BadRequest                  |
      | description | description is not a string |
    Examples:
      | desc          |
      | true          |
      | ["er", 34]    |
      | {"value": 34} |
      | 234324324     |
