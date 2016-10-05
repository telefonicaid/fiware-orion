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


  # ------------ notification - metadata field ---------------------
  @notification_metadata_without
  Scenario:  create a new subscription using NGSI v2 without notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_metadata
  Scenario Outline:  create a new subscription using NGSI v2 with notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | metadata                                   |
      | *                                          |
      | previousValue                              |
      | previousValue,*                            |
      | actionType                                 |
      | actionType,*                               |
      | actionType,previousValue                   |
      | actionType,previousValue,*                 |
      | temperature                                |
      | sfsdfsd,asdasdas,previousValue,actionType* |

  @notification_metadata_empty_array
  Scenario:  create a new subscription using NGSI v2 with an empty array in notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | array is empty                  |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_metadata_empty
  Scenario:  try to create a new subscription using NGSI v2 with empty notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata |                                 |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | attrs element is empty |

  @notification_metadata_wrong_types
  Scenario Outline:  try to create a new subscription using NGSI v2 with wrong structure types in notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                             |
      | subject_idPattern     | ".*"                              |
      | condition_attrs       | "temperature"                     |
      | notification_http_url | "http://replace_host:1045/notify" |
      | notification_metadata | <metadata>                        |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | attrs is not an array |
    Examples:
      | metadata     |
      | 189.56       |
      | "xcvxcvxvxc" |
      | true         |
      | {"a":3}      |

  @notification_metadata_forbidden
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden notification - metadata field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | attrs element has forbidden char |
    Examples:
      | metadata    |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house flat  |

