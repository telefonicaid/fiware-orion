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

  @expires_http_url
  Scenario:  create a new subscription using NGSI v2 with expires value con http url
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | description            | my first subscription   |
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

  @expires_httpCustom_url
  Scenario:  create a new subscription using NGSI v2 with expires value con httpCustom url
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | description                  | my first subscription   |
      | subject_type                 | room                    |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | condition_attrs_number       | 3                       |
      | notification_http_custom_url | http://localhost:1234   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @expires_empty
  Scenario:  create a new subscription using NGSI v2 with empty value in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                 |
      | description            | my first subscription |
      | subject_type           | room                  |
      | subject_idPattern      | .*                    |
      | condition_attrs        | temperature           |
      | condition_attrs_number | 3                     |
      | notification_http_url  | http://localhost:1234 |
      | notification_attrs     | temperature           |
      | expires                |                       |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @expires_permanent @ISSUE_1949
  Scenario:  create a new subscription using NGSI v2 with infinite duration (permanent)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                 |
      | description            | my first subscription |
      | subject_type           | room                  |
      | subject_idPattern      | .*                    |
      | condition_attrs        | temperature           |
      | condition_attrs_number | 3                     |
      | notification_http_url  | http://localhost:1234 |
      | notification_attrs     | temperature           |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @expires_invalid_format
  Scenario:  create a new subscription using NGSI v2 with invalid format (not date) in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | ".*"                    |
      | condition_attrs       | "temperature"           |
      | notification_http_url | "http://localhost:1234" |
      | notification_attrs    | "temperature"           |
      | expires               | "gfdgfdg"               |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                         |
      | error       | BadRequest                    |
      | description | expires has an invalid format |

  @expires_not_string
  Scenario Outline:  create a new subscription using NGSI v2 with not string type in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | ".*"                    |
      | condition_attrs       | "temperature"           |
      | notification_http_url | "http://localhost:1234" |
      | notification_attrs    | "temperature"           |
      | expires               | <espires>               |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                   |
      | error       | BadRequest              |
      | description | expires is not a string |
    Examples:
      | espires       |
      | true          |
      | ["er", 34]    |
      | {"value": 34} |
      | 234324324     |

  @expires_forbidden_chars
  Scenario Outline:  create a new subscription using NGSI v2 with forbidden chars in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                 |
      | description            | my first subscription |
      | subject_type           | room                  |
      | subject_idPattern      | .*                    |
      | condition_attrs        | temperature           |
      | condition_attrs_number | 3                     |
      | notification_http_url  | http://localhost:1234 |
      | notification_attrs     | temperature           |
      | expires                | <expires>             |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                         |
      | error       | BadRequest                    |
      | description | expires has an invalid format |
    Examples:
      | expires                   |
      | 2016-04-05T14:<00>:00.00Z |
      | 2016-04-05T14=00:00.00Z   |
      | 2016-04-05T14:"00":00.00Z |
      | 2016-04-05T14:00:'00'.00Z |
      | 2016-04-05T14:00;00.00Z   |
      | 2016-04-05T(14):00:00.00Z |

  @expires_bad_format
  Scenario Outline:  create a new subscription using NGSI v2 with bad format in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                 |
      | description            | my first subscription |
      | subject_type           | room                  |
      | subject_idPattern      | .*                    |
      | condition_attrs        | temperature           |
      | condition_attrs_number | 3                     |
      | notification_http_url  | http://localhost:1234 |
      | notification_attrs     | temperature           |
      | expires                | <expires>             |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                         |
      | error       | BadRequest                    |
      | description | expires has an invalid format |
    Examples:
      | expires                 |
      | 2r16-04-05T14:00:00.00Z |
      | -04-05T14:00:00.00Z     |
      | 2016_04-05T14:00:00.00Z |
      | 2016-64-05T14:00:00.00Z |
      | 2016--05T14:00:00.00Z   |
      | 2016-r4-05T14:00:00.00Z |
      | 2016-04-55T14:00:00.00Z |
      | 2016-04-r5T14:00:00.00Z |
      | 2016-04-T14:00:00.00Z   |
      | 2016-04-05K14:00:00.00Z |
      | 2016-04-05T54:00:00.00Z |
      | 2016-04-05Tr4:00:00.00Z |
      | 2016-04-05T:00:00.00Z   |
      | 2016-04-05T14;00:00.00Z |
      | 2016-04-05T14:r0:00.00Z |
      | 2016-04-05T14::00.00Z   |
      | 2016-04-05T14:10-00.00Z |
      | 2016-04-05T14:10:x0.00Z |
      | 2016-04-05T14:10:.00Z   |

  @expires_bad_format @BUG_2303 @skip
  Scenario Outline:  create a new subscription using NGSI v2 with bad format in expires
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_expires |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                 |
      | description            | my first subscription |
      | subject_type           | room                  |
      | subject_idPattern      | .*                    |
      | condition_attrs        | temperature           |
      | condition_attrs_number | 3                     |
      | notification_http_url  | http://localhost:1234 |
      | notification_attrs     | temperature           |
      | expires                | <expires>             |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                         |
      | error       | BadRequest                    |
      | description | expires has an invalid format |
    Examples:
      | expires                  |
      | 2016-04-05T14:10:0x.00Z  |
      | 2016-04-05T14:10:00,00Z  |
      | 2016-04-05T14:10:00.h00Z |
      | 2016-04-05T14:10:00.0h0Z |
      | 2016-04-05T14:10:00.,00L |