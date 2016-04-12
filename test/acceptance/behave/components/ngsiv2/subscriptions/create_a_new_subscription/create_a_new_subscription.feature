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

  @happy_path
  Scenario:  create a new subscription using NGSI v2
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_casub_happy_path |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                      | value                                                              |
      | description                    | my first subscription                                              |
      | subject_type                   | room                                                               |
      | subject_idPattern              | .*                                                                 |
      | subject_entities_number        | 2                                                                  |
      | subject_entities_prefix        | type                                                               |
      | condition_attributes           | temperature                                                        |
      | condition_attributes_number    | 3                                                                  |
      | condition_expression           | q>>>temperature>40&georel>>>near&geometry>>>point&coords>>>40.6391 |
      | notification_callback          | http://localhost:1234                                              |
      | notification_attributes        | temperature                                                        |
      | notification_attributes_number | 3                                                                  |
      | notification_throttling        | 5                                                                  |
      | expires                        | 2016-04-05T14:00:00.00Z                                            |
      | status                         | active                                                             |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @maximum_size
  Scenario:  try to create a new subscription using NGSI v2 with maximum size in payload
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                      | value                                                              |
      | description                    | random=1048078                                                     |
      | subject_type                   | room                                                               |
      | subject_idPattern              | .*                                                                 |
      | subject_entities_number        | 2                                                                  |
      | subject_entities_prefix        | type                                                               |
      | condition_attributes           | temperature                                                        |
      | condition_attributes_number    | 3                                                                  |
      | condition_expression           | q>>>temperature>40&georel>>>near&geometry>>>point&coords>>>40.6391 |
      | notification_callback          | http://localhost:1234                                              |
      | notification_attributes        | temperature                                                        |
      | notification_attributes_number | 3                                                                  |
      | notification_throttling        | 5                                                                  |
      | expires                        | 2016-04-05T14:00:00.00Z                                            |
      | status                         | active                                                             |
    When create a new subscription
    Then verify that receive a "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048577, max size supported: 1048576 |

  @length_required
  Scenario:  try to create a new subscription using NGSI v2 wihout payload
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_length_required |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter   | value           |
      | description | without payload |
    When create a new subscription
    Then verify that receive a "Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | LengthRequired                                   |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  # ---------- content-type header --------------------------------

  @content_type_without
  Scenario:  try to create a new subscription using NGSI v2 without content-type header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                                                           |
      | error       | UnsupportedMediaType                                                            |
      | description | Content-Type header not used, default application/octet-stream is not supported |

  @content_type_error
  Scenario Outline:  try to create a new subscription using NGSI v2 with wrong content-type header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | <content_type>    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | UnsupportedMediaType                       |
      | description | not supported content type: <content_type> |
    Examples:
      | content_type                      |
      | application/x-www-form-urlencoded |
      | application/xml                   |
      | multipart/form-data               |
      | text/plain                        |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  # ---------- Service header --------------------------------

  @service_without
  Scenario:  create a new subscription using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @service
  Scenario Outline:  create a new subscription using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_error
  Scenario Outline:  try to create subscriptions using NGSI v2 with several wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                  |
      | error       | BadRequest                                                                             |
      | description | bad character in tenant name - only underscore and alphanumeric characters are allowed |
    Examples:
      | service       |
      | servicedot.sr |
      | Service-sr    |
      | Service(sr)   |
      | Service=sr    |
      | Service<sr>   |
      | Service,sr    |
      | service#sr    |
      | service%sr    |
      | service&sr    |

  @service_bad_length
  Scenario:  try to create subscriptions using NGSI v2 with bad length in service header
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ---------- Services path header --------------------------------

  @service_path
  Scenario Outline:  create subscriptions using NGSI v2 with several service path header value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath | <service_path>    |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | service_path                                                  |
      | /                                                             |
      | /service_path                                                 |
      | /service_path_12                                              |
      | /Service_path                                                 |
      | /SERVICE                                                      |
      | /serv1/serv2/serv3/serv4/serv5/serv6/serv7/serv8/serv9/serv10 |
      | max length allowed                                            |
      | max length allowed and ten levels                             |

  @service_path_empty @BUG_1961
  Scenario:  create subscriptions using NGSI v2 with empty value in service path header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath |                   |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @service_path_without @BUG_2024 @skip
  Scenario:  create subscriptions using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value             |
      | Fiware-Service | test_service_path |
      | Content-Type   | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @service_path_error
  Scenario Outline:  try to create subscription using NGSI v2 with wrong service path header values
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | a component of ServicePath contains an illegal character |
    Examples:
      | service_path |
      | /service.sr  |
      | /service;sr  |
      | /service=sr  |
      | /Service-sr  |
      | /serv<45>    |
      | /serv(45)    |

  @service_path_error
  Scenario Outline:  try to create subscription using NGSI v2 with wrong service paths header vslues
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_error
  Scenario Outline:  try to create subscriptions using NGSI v2 with component-name too long in service path header values
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    And verify that entities are not stored in mongo
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_error
  Scenario:  try to create entitsubscrptions using NGSI v2 with too many components in service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_service_path_error              |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |
