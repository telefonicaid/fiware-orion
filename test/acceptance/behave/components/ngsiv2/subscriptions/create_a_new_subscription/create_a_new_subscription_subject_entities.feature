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

  # ------------ subject field and its sub-fields ---------------------

  @subject_without
  Scenario:  try to create a new subscription using NGSI v2 without subject field
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_csub_subject |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | without subject field   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | no subject for subscription specified |

  # ------------ subject - entities field and its sub-fields ---------------------

  @entities_without
  Scenario:  try to create a new subscription using NGSI v2 without entities field
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_csub_entities |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | without entities field  |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                         |
      | error       | BadRequest                    |
      | description | no subject entities specified |

# ------------ subject - entities - type ----------------------

  @type_without
  Scenario:  create a new subscription using NGSI v2 without type field and with idPattern field
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @type_without
  Scenario:  create a new subscription using NGSI v2 without type field and with id field
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | room2                   |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @type_only @BUG_1939
  Scenario:  create a new subscription using NGSI v2 with only type field and without id and idPattern fields
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | house                   |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | BadRequest                                              |
      | description | subject entities element does not have id nor idPattern |

  @type_id_pattern
  Scenario Outline:  create new subscriptions using NGSI v2 with several type values and idPattern
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | <type>                  |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | type       |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @type_id
  Scenario Outline:  create new subscriptions using NGSI v2 with several type values and id
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | <type>                  |
      | subject_id            | room45                  |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | type       |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @type_raw_whitespaces
  Scenario:  create an subscription using NGSI v2 with whitespaces in entities type using raw mode
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
      # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                                               |
      | description           | "my first subscription"                                                                             |
      | subject_type          | "                   "                                                                               |
      | subject_idPattern     | ".*"                                                                                                |
      | condition_attrs       | "temperature"                                                                                       |
      | condition_expression  | "q">>>"temperature>40"&"georel">>>"near;minDistance:1000"&"geometry">>>"point"&"coords">>>"40,6391" |
      | notification_http_url | "http://localhost:1234"                                                                             |
      | notification_attrs    | "temperature"                                                                                       |
      | throttling            | 5                                                                                                   |
      | expires               | "2016-04-05T14:00:00.00Z"                                                                           |
      | status                | "active"                                                                                            |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | forbidden characters in subject entities element type |

  @type_not_plain_ascii @BUG_1964
  Scenario Outline:  try to create subscriptions using NGSI v2 with not plain ascii type values
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | <type>                  |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | forbidden characters in subject entities element type |
    Examples:
      | type       |
      | habitación |
      | españa     |
      | barça      |

  @type_length_minimum @BUG_1985 @skip
  Scenario:  try to create subscription using NGSI v2 with entities type length minimum allowed (1)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_condition_attrs_limit_exceed |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          |                         |
      | subject_id            | room                    |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | BadRequest      |
      | description | not defined yet |

  @type_length_exceed @BUG_1965
  Scenario:  try to create subscriptions using NGSI v2 with type length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_entities_type_exceed |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | random=257              |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | max type length exceeded |

  @type_wrong @BUG_1967
  Scenario Outline:  try to create a subscription using NGSI v2 with several wrong type values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_entities_id_error |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | <type>                  |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | forbidden characters in subject entities element type |
    Examples:
      | type        |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_&     |
      | house_/     |
      | house_#     |
      | my house    |

  @type_wrong_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with wrong json in entities type
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                                               |
      | description           | "my first subscription"                                                                             |
      | subject_type          | <type>                                                                                              |
      | subject_idPattern     | ".*"                                                                                                |
      | condition_attrs       | "temperature"                                                                                       |
      | condition_expression  | "q">>>"temperature>40"&"georel">>>"near;minDistance:1000"&"geometry">>>"point"&"coords">>>"40,6391" |
      | notification_http_url | "http://localhost:1234"                                                                             |
      | notification_attrs    | "temperature"                                                                                       |
      | throttling            | 5                                                                                                   |
      | expires               | "2016-04-05T14:00:00.00Z"                                                                           |
      | status                | "active"                                                                                            |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | type      |
      | rewrewr   |
      | SDFSDFSDF |

  @type_invalid_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with invalid entities type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
      # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                                               |
      | description           | "my first subscription"                                                                             |
      | subject_type          | <type>                                                                                              |
      | subject_idPattern     | ".*"                                                                                                |
      | condition_attrs       | "temperature"                                                                                       |
      | condition_expression  | "q">>>"temperature>40"&"georel">>>"near;minDistance:1000"&"geometry">>>"point"&"coords">>>"40,6391" |
      | notification_http_url | "http://localhost:1234"                                                                             |
      | notification_attrs    | "temperature"                                                                                       |
      | throttling            | 5                                                                                                   |
      | expires               | "2016-04-05T14:00:00.00Z"                                                                           |
      | status                | "active"                                                                                            |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | subject entities element type is not a string |
    Examples:
      | type            |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

  @type_multiples
  Scenario Outline:  create new subscriptions using NGSI v2 with multiples entities types
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_entities_type_multiples |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_idPattern       | .*                      |
      | subject_entities_number | <number>                |
      | subject_entities_prefix | type                    |
      | condition_attrs         | temperature             |
      | notification_http_url   | http://localhost:1234   |
      | notification_attrs      | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | number |
      | 2      |
      | 10     |
      | 50     |
      | 100    |
      | 1000   |
      | 10000  |

# ------------ subject - entities - id ----------------------

  @id_and_id_pattern_without
  Scenario:  try to create a new subscription using NGSI v2 without id, type nor idPattern fields
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | BadRequest                                              |
      | description | subject entities element does not have id nor idPattern |

  @id_idPattern
  Scenario:  try to create a new subscription using NGSI v2 with id and idPattern fields
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | room2                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | subject entities element has id and idPattern |

  @id
  Scenario Outline:  create entities using NGSI v2 with several entities id values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | <id>                    |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | id         |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @id_raw_whitespaces
  Scenario:  create an subscription using NGSI v2 with whitespaces in entities id using raw mode
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
      # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                                               |
      | description           | "my first subscription"                                                                             |
      | subject_type          | "house"                                                                                             |
      | subject_id            | "                    "                                                                              |
      | condition_attrs       | "temperature"                                                                                       |
      | condition_expression  | "q">>>"temperature>40"&"georel">>>"near;minDistance:1000"&"geometry">>>"point"&"coords">>>"40,6391" |
      | notification_http_url | "http://localhost:1234"                                                                             |
      | notification_attrs    | "temperature"                                                                                       |
      | throttling            | 5                                                                                                   |
      | expires               | "2016-04-05T14:00:00.00Z"                                                                           |
      | status                | "active"                                                                                            |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                               |
      | error       | BadRequest                                          |
      | description | forbidden characters in subject entities element id |

  @id_not_plain_ascii @BUG_1973
  Scenario Outline:  try to create subscription using NGSI v2 with several entities id values with not plain ascii chars
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | <id>                    |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                               |
      | error       | BadRequest                                          |
      | description | forbidden characters in subject entities element id |
    Examples:
      | id         |
      | habitación |
      | españa     |
      | barça      |

  @id_length_minimum @BUG_1984
  Scenario:  try to create subscription using NGSI v2 with entities id length minimum allowed (1)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_condition_attrs_limit_exceed |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            |                         |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | subject entities element id is empty |

  @id_length_exceed @BUG_1974
  Scenario:  try to create subscription using NGSI v2 with with entities id length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_entities_id_limit_exceed |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | random=257              |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | max id length exceeded |

  @id_wrong @BUG_1975
  Scenario Outline:  try to create subscription using NGSI v2 with wrong entities id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_entities_id_error |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | <id>                    |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                               |
      | error       | BadRequest                                          |
      | description | forbidden characters in subject entities element id |
    Examples:
      | id          |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_&     |
      | house_/     |
      | house_#     |
      | my house    |

  @id_invalid_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with several invalid entities id (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_entities_id_error |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_id            | <id>                      |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | id        |
      | rewrewr   |
      | SDFSDFSDF |

  @id_not_allowed_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with several not allowed entities id  (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_entities_id_error |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_id            | <id>                      |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                       |
      | error       | BadRequest                                  |
      | description | subject entities element id is not a string |
    Examples:
      | id              |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

  @id_multiples
  Scenario Outline:  create new subscriptions using NGSI v2 with multiples entities ids
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_type            | room                    |
      | subject_id              | room2                   |
      | subject_entities_number | <number>                |
      | subject_entities_prefix | id                      |
      | condition_attrs         | temperature             |
      | notification_http_url   | http://localhost:1234   |
      | notification_attrs      | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | number |
      | 2      |
      | 10     |
      | 50     |
      | 100    |
      | 1000   |
      | 10000  |

# ------------ subject - entities - idPattern ----------------------

  @idpattern
  Scenario Outline:  create entities using NGSI v2 with several entities idPattern values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_id_pattern |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | "<id_pattern>"            |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | id_pattern   |
      | room         |
      | 34           |
      | false        |
      | true         |
      | 34.4E-34     |
      | temp.34      |
      | temp_34      |
      | temp-34      |
      | TEMP34       |
      | house_flat   |
      | house.flat   |
      | house-flat   |
      | house@flat   |
      | random=10    |
      | random=100   |
      | random=256   |
      | veh.*        |
      | ^3.?         |
      | .*als.*      |
      | .*ru.*       |
      | .*34         |
      | ^te+.*       |
      | [A-Za-z0-9]* |
      | house(flat)  |
      | house(f*)    |
      | .*           |

  @idpattern_backslash @BUG_2142 @skip
  Scenario Outline:  create entities using NGSI v2 with several entities idPattern values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_id_pattern |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | "<id_pattern>"            |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | id_pattern |
      | \a         |
      | \w+        |
      | \w*        |
      | [\w]+      |
      | (\w)+      |
      | \d*        |
      | \D*        |

  @idpattern_wrong_regex
  Scenario Outline:  create entities using NGSI v2 with wrong regex in idPattern values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_id_pattern |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | "<id_pattern>"            |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid regex for entity id pattern |
    Examples:
      | id_pattern   |
      | [A-Za-z0-9)* |

  @id_pattern_raw_whitespaces
  Scenario:  create an subscription using NGSI v2 with whitespaces in entities idPattern using raw mode
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
      # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                                               |
      | description           | "my first subscription"                                                                             |
      | subject_type          | "house"                                                                                             |
      | subject_idPattern     | "                      "                                                                            |
      | condition_attrs       | "temperature"                                                                                       |
      | condition_expression  | "q">>>"temperature>40"&"georel">>>"near;minDistance:1000"&"geometry">>>"point"&"coords">>>"40,6391" |
      | notification_http_url | "http://localhost:1234"                                                                             |
      | notification_attrs    | "temperature"                                                                                       |
      | throttling            | 5                                                                                                   |
      | expires               | "2016-04-05T14:00:00.00Z"                                                                           |
      | status                | "active"                                                                                            |
    When create a new subscription in raw mode
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo

  @id_pattern_not_plain_ascii @BUG_1976 @skip
  Scenario Outline:  try to create subscription using NGSI v2 with several entities idPattern values with not plain ascii chars
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_id_pattern |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | <idPattern>             |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |
    Examples:
      | idPattern    |
      | habitación   |
      | españa       |
      | barça        |
      | house(barça) |
      | ^aña+.*      |

  @id_pattern_length_minimum @BUG_1986
  Scenario:  try to create subscription using NGSI v2 with entities idPattern length minimum allowed (1)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_condition_attrs_limit_exceed |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     |                         |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                       |
      | error       | BadRequest                                  |
      | description | subject entities element idPattern is empty |

  @id_pattern_wrong @BUG_1978 @skip
  Scenario Outline:  try to create subscription using NGSI v2 with wrong entities idPattern values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_entities_id_pattern_error |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | <id_pattern>            |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |
    Examples:
      | id_pattern  |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_&     |
      | house_/     |
      | house_#     |
      | my house    |

  @id_pattern_invalid_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with several invalid entities idPattern (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_entities_id_pattern_error |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_id_pattern    | <id_pattern>              |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | id_pattern |
      | rewrewr    |
      | SDFSDFSDF  |

  @id_pattern_not_allowed_raw
  Scenario Outline:  try to create an subscription using NGSI v2 with several not allowed entities idPattern  (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_entities_id_pattern_error |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | <id_pattern>              |
      | condition_attrs       | "temperature"             |
      | notification_http_url | "http://localhost:1234"   |
      | notification_attrs    | "temperature"             |
      | expires               | "2016-04-05T14:00:00.00Z" |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | BadRequest                                         |
      | description | subject entities element idPattern is not a string |
    Examples:
      | id_pattern      |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |
