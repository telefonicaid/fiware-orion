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

  # ------------ subject - condition ---------------------

  @condition_without @ISSUE_1946 @skip
  Scenario:  try to create a new subscription using NGSI v2 without condition field
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_condition   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | without condition field |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  # ------------ subject - condition - attributes ---------------------

  @condition_attrs_without @ISSUE_1946 @skip
  Scenario:  try to create a new subscription using NGSI v2 without condition attributes field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_condition_attrs |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @condition_attrs_empty @ISSUE_1946
  Scenario:  try to create a new subscription using NGSI v2 without condition attributes field with empty array
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_condition_attrs |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | array is empty          |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @condition_attrs
  Scenario Outline:  create entities using NGSI v2 with several entities id values
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_condition_attrs |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_id            | room2                   |
      | condition_attrs       | <attributes>            |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | attributes |
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

  @condition_attrs_multiples
  Scenario Outline:  create a subscription using NGSI v2 with multiple condition attributes
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_condition_attrs |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                   |
      | subject_id             | room2                   |
      | condition_attrs        | temperature             |
      | condition_attrs_number | <number>                |
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
      | number |
      | 2      |
      | 10     |
      | 100    |
      | 1000   |
      | 10000  |

  @condition_attrs_not_plain_ascii @BUG_1979 @skip
  Scenario Outline:  try to create subscription using NGSI v2 with condition attributes values with not plain ascii chars
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_condition_attrs_ascii |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | <attributes>            |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Invalid characters in condition attributes |
    Examples:
      | attributes |
      | habitación |
      | españa     |
      | barça      |

  @condition_attrs_length_minimum @BUG_1983 @skip
  Scenario:  try to create subscription using NGSI v2 with condition-attrs length minimum allowed (1)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_condition_attrs_limit_exceed |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       |                         |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | condition attributes length: 257, max length supported: 256 |

  @condition_attrs_length_exceed @BUG_1980 @skip
  Scenario:  try to create subscription using NGSI v2 with condition-attrs length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_condition_attrs_limit_exceed |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | random=257              |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | condition attributes length: 257, max length supported: 256 |

  @condition_attrs_wrong @BUG_1981 @skip
  Scenario Outline:  try to create subscription using NGSI v2 with wrong condition attributes values
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_condition_attrs_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | <attributes>            |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Invalid characters in condition attributes |
    Examples:
      | attributes  |
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

  @condition_attrs_invalid_raw
  Scenario Outline:  try to create a new subscription using NGSI v2 with several invalid condition attributes (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_condition_attrs_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | .*                        |
      | condition_attrs       | <attributes>              |
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
      | attributes |
      | rewrewr    |
      | SDFSDFSDF  |

  @condition_attrs_not_allowed_raw
  Scenario Outline:  try to create a new subscription using NGSI v2 with several not allowed condition-attrs (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_condition_attrs_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                     |
      | subject_idPattern     | .*                        |
      | condition_attrs       | <attributes>              |
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
      | attributes               |
      | false                    |
      | true                     |
      | 34                       |
      | {"a":34}                 |
      | ["34", "a", 45]          |
      | null                     |
      | "temperature";"pressure" |
      | "temperature             |
      | "temperature"&"pressure" |
      | "temperature" "pressure" |

  # ------------ subject - condition - expression ---------------------

  @condition_expression_without
  Scenario:  try to create a new subscription using NGSI v2 without condition expression field
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_condition_expression |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @condition_expression_empty @ISSUE_1946 @skip
  Scenario:  try to create a new subscription using NGSI v2 without condition expression field with empty object
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_condition_expression |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_expression  | object is empty         |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  # ------------ subject - condition - expression - q ---------------------
  @condition_expression_q.row<row.id>
  @condition_expression_q
  Scenario Outline: create a new subscription using NGSI v2 with "q" condition expression and several values
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_condition_expression |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | q                                                         |
      | temperature>40                                            |
      | temperature>=40                                           |
      | temperature<=40                                           |
      | temperature<40                                            |
      | temperature==40                                           |
      | temperature!=40                                           |
      | temperature==40.56                                        |
      | temperature==cool                                         |
      | !speed;!timestamp                                         |
      | temperature=='40'                                         |
      | color!='black,ligth'                                      |
      | color!='black,ligth',orange                               |
      | speed==69..90                                             |
      | speed!=69..90                                             |
      | my_time==2016-04-05T14:00:00.00Z..2016-05-05T14:00:00.00Z |
      | my_time!=2016-04-05T14:00:00.00Z..2016-05-05T14:00:00.00Z |
      | my_time>2016-04-05T14:00:00.00Z                           |
      | my_time>=2016-04-05T14:00:00.00Z                          |
      | my_time<2016-04-05T14:00:00.00Z                           |
      | my_time<=2016-04-05T14:00:00.00Z                          |
      | speed==79..85;!myAttr;otherAttr                           |
      | 34                                                        |
      | false                                                     |
      | true                                                      |
      | temp_34                                                   |
      | temp-34                                                   |
      | TEMP34                                                    |
      | random=10                                                 |
      | random=100                                                |
      | random=256                                                |

  @condition_expression_q_escaped @BUG_1988 @skip
  Scenario:  try to create a new subscription using NGSI v2 with "q" condition expression but with escaped string
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_condition_expression |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                      |
      | subject_type          | "room2"                    |
      | subject_idPattern     | ".*"                       |
      | condition_attrs       | "temperature"              |
      | condition_expression  | "q">>>"temperature>\"40\"" |
      | notification_http_url | "http://localhost:1234"    |
      | notification_attrs    | "temperature"              |
      | expires               | "2016-04-05T14:00:00.00Z"  |
    When create a new subscription in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid query expression |

  @condition_expression_q_parse_error @BUG_1989 @BUG_2106
  Scenario Outline:  try to create a new subscription using NGSI v2 with "q" condition expression but with parse errors
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_condition_expression_error |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value         |
      | error       | BadRequest    |
      | description | <description> |
    Examples:
      | q       | description                     |
      | speed== | empty right-hand-side in q-item |
      | speed!= | empty right-hand-side in q-item |
      | speed>= | empty right-hand-side in q-item |
      | speed<= | empty right-hand-side in q-item |
      | speed>  | empty right-hand-side in q-item |
      | speed<  | empty right-hand-side in q-item |
      | ==speed | empty left-hand-side in q-item  |
      | !=speed | empty left-hand-side in q-item  |
      | >=speed | empty left-hand-side in q-item  |
      | <=speed | empty left-hand-side in q-item  |
      | >speed  | empty left-hand-side in q-item  |
      | <speed  | empty left-hand-side in q-item  |

  @condition_expression_q_range_error.row<row.id>
  @condition_expression_q_range_error @1991 @BUG_2106
  Scenario Outline:  try to create a new subscription using NGSI v2 with "q" condition expression but with invalid operator in ranges
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_condition_expression_error |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value         |
      | error       | BadRequest    |
      | description | <description> |
    Examples:
      | q                | description                         |
      | speed>=69..90    | ranges only valid for == and != ops |
      | speed>69..90     | ranges only valid for == and != ops |
      | speed<=69..79    | ranges only valid for == and != ops |
      | speed<99..190    | ranges only valid for == and != ops |
      | speed==99....190 | invalid range: types mixed          |
      | speed==99,,,190  | empty item in list                  |

  @condition_expression_q_range_error_invested.row<row.id>
  @condition_expression_q_range_error_invested @BUG_2106 @2117 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with "q" condition expression but with invalid operator in ranges (invested range)
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_condition_expression_error |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value         |
      | error       | BadRequest    |
      | description | <description> |
    Examples: # now all return 201 and the subsc is created
      | q              | description    |
      | speed==100..1  | invested range |
      | speed==100..-1 | invested range |

  @condition_expression_q_invalid_chars.row<row.id>
  @condition_expression_q_invalid_chars @BUG_2106 @BUG_1994 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with "q" condition expression but with invalid chars
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_condition_expression_error |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
      | Accept             | application/json                |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | invalid character found in URI param /q/ |
    Examples: # now all return 201 and the subsc is created
      | q           |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_/     |
      | house_#     |
      | my house    |

  @condition_expression_q_invalid_date.row<row.id>
  @condition_expression_q_invalid_date @BUG_2106 @BUG_1996 @skip
  # FIXME: below Examples only represent at the "Complete date plus hours, minutes, seconds and a decimal fraction of a second" level in https://www.w3.org/TR/NOTE-datetime
  Scenario Outline:  try to create a new subscription using NGSI v2 with "q" condition expression but with invalid DateTime values (ISO8601)
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_condition_expression_error_II |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | q>>><q>                 |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value         |
      | error       | BadRequest    |
      | description | <description> |
    Examples:
      | q                                                          | description                        |
      | my_time<>2016-04-05T14:00:00.00Z                           | TBD                                |
      | my_time>=2r16-04-05T14:00:00.00Z                           | TBD                                |
      | my_time>=-04-05T14:00:00.00Z                               | TBD                                |
      | my_time>=2016_04-05T14:00:00.00Z                           | TBD                                |
      | my_time>=2016-64-05T14:00:00.00Z                           | TBD                                |
      | my_time>=2016--05T14:00:00.00Z                             | TBD                                |
      | my_time>=2016-r4-05T14:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-55T14:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-r5T14:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-T14:00:00.00Z                             | TBD                                |
      | my_time>=2016-04-05K14:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-05T54:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-05Tr4:00:00.00Z                           | TBD                                |
      | my_time>=2016-04-05T:00:00.00Z                             | TBD                                |
      | my_time>=2016-04-05T14;00:00.00Z                           | TBD                                |
      | my_time>=2016-04-05T14:r0:00.00Z                           | TBD                                |
      | my_time>=2016-04-05T14:44:00.00Z                           | TBD                                |
      | my_time>=2016-04-05T14::00.00Z                             | TBD                                |
      | my_time>=2016-04-05T14:10-00.00Z                           | TBD                                |
      | my_time>=2016-04-05T14:10:x0.00Z                           | TBD                                |
      | my_time>=2016-04-05T14:10:0x.00Z                           | TBD                                |
      | my_time>=2016-04-05T14:10:.00Z                             | TBD                                |
      | my_time>=2016-04-05T14:10:00,00Z                           | lists only valid for == and != ops |
      | my_time>=2016-04-05T14:10:00.h00Z                          | TBD                                |
      | my_time>=2016-04-05T14:10:00.0h0Z                          | TBD                                |
      | my_time>=2016-04-05T14:10:00.,00L                          | lists only valid for == and != ops |
      | my_time==2016-04-05T14:00:00.00Z,,2017-04-05T14:00:00.00Z  | empty item in list                 |
      | my_time==2016-04-05T14:00:00.00Z...2017-04-05T14:00:00.00Z | invalid range: types mixed         |

  # ------------ subject - condition - expression - georel ---------------------
  @condition_expression_georel.row<row.id>
  @condition_expression_georel
  Scenario Outline: create a new subscription using NGSI v2 with "georel" condition expression and allowed values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_condition_expression_georel |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                     |
      | subject_type          | room                                                      |
      | subject_idPattern     | .*                                                        |
      | condition_attrs       | temperature                                               |
      | condition_expression  | georel>>><georel>&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                     |
      | notification_attrs    | temperature                                               |
      | expires               | 2016-04-05T14:00:00.00Z                                   |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | georel                | geometry | coords                                                      |
      | near;MaxDistance:1000 | point    | 25.774,-80.190                                              |
      | near;MinDistance:1000 | point    | 25.774,-80.190                                              |
      | coveredBy             | box      | 25.774,-80.190;18.466,-66.118                               |
      | coveredBy             | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | intersects            | point    | 25.774,-80.190                                              |
      | intersects            | line     | 25.774,-80.190;18.466,-66.118                               |
      | intersects            | box      | 25.774,-80.190;18.466,-66.118                               |
      | intersects            | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | equals                | point    | 25.774,-80.190                                              |
      | equals                | line     | 25.774,-80.190;18.466,-66.118                               |
      | equals                | box      | 25.774,-80.190;18.466,-66.118                               |
      | equals                | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | disjoint              | point    | 25.774,-80.190                                              |
      | disjoint              | line     | 25.774,-80.190;18.466,-66.118                               |
      | disjoint              | box      | 25.774,-80.190;18.466,-66.118                               |
      | disjoint              | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |

  @condition_expression_georel_without_geometry @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression and without geometry field
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | georel>>><georel>       |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                  |
      | error       | BadRequest                                                             |
      | description | Expression not supported: /georel/ field used without /geometry/ field |
    Examples:
      | georel                |
      | near;MaxDistance:1000 |
      | near;MinDistance:1000 |
      | coveredBy             |
      | intersects            |
      | equals                |
      | disjoint              |

  @condition_expression_georel_unknown @ISSUE_2002 @skip
  Scenario: try to create a new subscription using NGSI v2 with "georel" condition expression and unknown value
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                     |
      | subject_type          | room                                                      |
      | subject_idPattern     | .*                                                        |
      | condition_attrs       | temperature                                               |
      | condition_expression  | georel>>>fsdfsdf&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                     |
      | notification_attrs    | temperature                                               |
      | expires               | 2016-04-05T14:00:00.00Z                                   |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                              |
      | error       | BadRequest                                                         |
      | description | Expression not supported: Invalid selector in georel specification |

  @condition_expression_geometry_not_allowed @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression and not allowed values
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                      |
      | subject_type          | room                                                       |
      | subject_idPattern     | .*                                                         |
      | condition_attrs       | temperature                                                |
      | condition_expression  | georel>>><georel>&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                      |
      | notification_attrs    | temperature                                                |
      | expires               | 2016-04-05T14:00:00.00Z                                    |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                              |
      | error       | BadRequest                                                         |
      | description | Expression not supported: Invalid selector in georel specification |
    Examples:
      | georel     |
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

  @condition_expression_georel_near_only @ISSUE_2002 @skip
  Scenario: try to create a new subscription using NGSI v2 with "georel" condition expression with "near" as value and without either minDistance nor maxDistance
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                  |
      | subject_type          | room                                                   |
      | subject_idPattern     | .*                                                     |
      | condition_attrs       | temperature                                            |
      | condition_expression  | georel>>>near&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                  |
      | notification_attrs    | temperature                                            |
      | expires               | 2016-04-05T14:00:00.00Z                                |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                              |
      | error       | BadRequest                                                                         |
      | description | Expression not supported: georel /near/ without either minDistance nor maxDistance |

  @condition_expression_georel_near_not_point @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression with "near" as value and with geometry different of point value
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                        |
      | subject_type          | room                                                                         |
      | subject_idPattern     | .*                                                                           |
      | condition_attrs       | temperature                                                                  |
      | condition_expression  | georel>>>near;minDistance:5000&geometry>>><geometry>&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                                        |
      | notification_attrs    | temperature                                                                  |
      | expires               | 2016-04-05T14:00:00.00Z                                                      |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                           |
      | error       | BadRequest                                                                      |
      | description | Expression not supported: georel /near/ used with geometry different than point |
    Examples:
      | geometry |
      | line     |
      | polygon  |
      | box      |

  @condition_expression_georel_coveredBy_not_supported @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression with "coveredBy" as value and with geometry value not supported
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                      |
      | subject_type          | room                                                       |
      | subject_idPattern     | .*                                                         |
      | condition_attrs       | temperature                                                |
      | condition_expression  | georel>>>coveredBy&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                      |
      | notification_attrs    | temperature                                                |
      | expires               | 2016-04-05T14:00:00.00Z                                    |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                              |
      | error       | BadRequest                                                                         |
      | description | Expression not supported: <geometry> geometry cannot be used with coveredBy georel |
    Examples:
      | geometry | coords                        |
      | point    | 25.774,-80.190                |
      | line     | 25.774,-80.190;18.466,-66.118 |

  @condition_expression_georel_near_not_point @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression with "near" as value and with invalid selector
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                             |
      | subject_type          | room                                                                              |
      | subject_idPattern     | .*                                                                                |
      | condition_attrs       | temperature                                                                       |
      | condition_expression  | georel>>>near;minDistance<separator>5000&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                                             |
      | notification_attrs    | temperature                                                                       |
      | expires               | 2016-04-05T14:00:00.00Z                                                           |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                              |
      | error       | BadRequest                                                         |
      | description | Expression not supported: Invalid selector in georel specification |
    Examples:
      | separator |
      | ;         |
      | =         |
      | ==        |
      | _         |
      | -         |

  @condition_expression_georel_empty @ISSUE_2002 @skip
  Scenario: try to create a new subscription using NGSI v2 with "georel" condition expression with empty value
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                              |
      | subject_type          | room                                               |
      | subject_idPattern     | .*                                                 |
      | condition_attrs       | temperature                                        |
      | condition_expression  | georel>>>&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                              |
      | notification_attrs    | temperature                                        |
      | expires               | 2016-04-05T14:00:00.00Z                            |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | Empty right-hand-side for /georel/ Expression not supported |

  @condition_expression_georel_forbidden @ISSUE_2002 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "georel" condition expression with forbidden values
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_georel_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                      |
      | subject_type          | room                                                       |
      | subject_idPattern     | .*                                                         |
      | condition_attrs       | temperature                                                |
      | condition_expression  | georel>>><georel>&geometry>>>point&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                      |
      | notification_attrs    | temperature                                                |
      | expires               | 2016-04-05T14:00:00.00Z                                    |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                          |
      | error       | BadRequest                                                     |
      | description | Expression not supported: Invalid characters in /georel/ field |
    Examples:
      | georel      |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  # ------------ subject - condition - expression - geometry ---------------------
  @condition_expression_geometry
  Scenario Outline: create a new subscription using NGSI v2 with "geometry" condition expression and allowed values
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_condition_expression_geometry |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                     |
      | subject_type          | room                                                      |
      | subject_idPattern     | .*                                                        |
      | condition_attrs       | temperature                                               |
      | condition_expression  | georel>>><georel>&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                     |
      | notification_attrs    | temperature                                               |
      | expires               | 2016-04-05T14:00:00.00Z                                   |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | georel                | geometry | coords                                                      |
      | near;MaxDistance:1000 | point    | 40.6391                                                     |
      | near;MinDistance:1000 | point    | 40.6391                                                     |
      | intersects            | point    | 25.774,-80.190                                              |
      | equals                | point    | 25.774,-80.190                                              |
      | disjoint              | point    | 25.774,-80.190                                              |
      | intersects            | line     | 25.774,-80.190;18.466,-66.118                               |
      | equals                | line     | 25.774,-80.190;18.466,-66.118                               |
      | disjoint              | line     | 25.774,-80.190;18.466,-66.118                               |
      | coveredBy             | box      | 25.774,-80.190;18.466,-66.118                               |
      | intersects            | box      | 25.774,-80.190;18.466,-66.118                               |
      | equals                | box      | 25.774,-80.190;18.466,-66.118                               |
      | disjoint              | box      | 25.774,-80.190;18.466,-66.118                               |
      | coveredBy             | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | intersects            | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | equals                | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | disjoint              | polygon  | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |

  @condition_expression_geometry_wo_coords @ISSUE_ @skip
  Scenario: try to create a new subscription using NGSI v2 with "geometry" condition expression and without "coords" field
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_condition_expression_geometry_error |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_type          | room                    |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | condition_expression  | geometry>>>point        |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                  |
      | error       | BadRequest                                                             |
      | description | Expression not supported: /geometry/ field used without /coords/ field |

  @condition_expression_geometry_empty @ISSUE_ @skip
  Scenario: try to create a new subscription using NGSI v2 with "geometry" condition expression and with empty value
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_condition_expression_geometry_error |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                              |
      | subject_type          | room                                                               |
      | subject_idPattern     | .*                                                                 |
      | condition_attrs       | temperature                                                        |
      | condition_expression  | georel>>>near;maxDistance:5000&geometry>>>&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                              |
      | notification_attrs    | temperature                                                        |
      | expires               | 2016-04-05T14:00:00.00Z                                            |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                         |
      | error       | BadRequest                                                    |
      | description | Empty right-hand-side for /geometry/ Expression not supported |

  @condition_expression_geometry_not_allowed @BUG_1999 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "geometry" condition expression and not allowed values
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_condition_expression_geometry_error |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                        |
      | subject_type          | room                                                                         |
      | subject_idPattern     | .*                                                                           |
      | condition_attrs       | temperature                                                                  |
      | condition_expression  | georel>>>near;maxDistance:5000&geometry>>><geometry>&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                                        |
      | notification_attrs    | temperature                                                                  |
      | expires               | 2016-04-05T14:00:00.00Z                                                      |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                |
      | error       | BadRequest                                                           |
      | description | Expression not supported: Invalid selector in geometry specification |
    Examples:
      | geometry   |
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

  @condition_expression_geometry_forbidden @BUG_2000 @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "geometry" condition expression and forbidden values
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_condition_expression_geometry_error |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                        |
      | subject_type          | room                                                                         |
      | subject_idPattern     | .*                                                                           |
      | condition_attrs       | temperature                                                                  |
      | condition_expression  | georel>>>near;maxDistance:5000&geometry>>><geometry>&coords>>>25.774,-80.190 |
      | notification_http_url | http://localhost:1234                                                        |
      | notification_attrs    | temperature                                                                  |
      | expires               | 2016-04-05T14:00:00.00Z                                                      |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                     |
      | error       | BadRequest                                                                |
      | description | Expression not supported: Invalid characters in in geometry specification |
    Examples:
      | geometry    |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  # ------------ subject - condition - expression - coords ---------------------
  @condition_expression_coords_invalid_point @ISSUE_ @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "coords" condition expression and invalid point
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_condition_expression_coords |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                  |
      | subject_type          | room                                                   |
      | subject_idPattern     | .*                                                     |
      | condition_attrs       | temperature                                            |
      | condition_expression  | georel>>>equals&geometry>>><geometry>&coords>>>40.6391 |
      | notification_http_url | http://localhost:1234                                  |
      | notification_attrs    | temperature                                            |
      | expires               | 2016-04-05T14:00:00.00Z                                |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                     |
      | error       | BadRequest                                                |
      | description | Expression not supported: invalid point in /coords/ field |
    Examples:
      | geometry |
      | point    |
      | line     |
      | box      |
      | polygon  |

  @condition_expression_coords_invalid_point @ISSUE_ @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "coords" condition expression and invalid point
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_coords_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                |
      | subject_type          | room                                                 |
      | subject_idPattern     | .*                                                   |
      | condition_attrs       | temperature                                          |
      | condition_expression  | georel>>>equals&geometry>>>polygon&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                |
      | notification_attrs    | temperature                                          |
      | expires               | 2016-04-05T14:00:00.00Z                              |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                     |
      | error       | BadRequest                                                |
      | description | Expression not supported: invalid point in /coords/ field |
    Examples:
      | coords |
      | ,      |
      | .      |
      | ;      |

  @condition_expression_coords_invalid_coordenates @ISSUE_ @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "coords" condition expression and invalid number of coordinates
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_coords_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                   |
      | subject_type          | room                                                    |
      | subject_idPattern     | .*                                                      |
      | condition_attrs       | temperature                                             |
      | condition_expression  | georel>>>equals&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                   |
      | notification_attrs    | temperature                                             |
      | expires               | 2016-04-05T14:00:00.00Z                                 |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Expression not supported: invalid number of coordinates for /<geometry>/ |
    Examples:
      | geometry | coords                                                      |
      | point    | 25.774,-80.190;18.466,-66.118                               |
      | point    | 25.774,-80.190;18.466,-66.118;32.321,-64.757;25.774,-80.190 |
      | line     | 25.774,-80.190                                              |
      | box      | 25.774,-80.190                                              |
      | box      | 25.774,-80.190;18.466,-66.118;32.321,-64.757;               |

  @condition_expression_coords_few_coords_polygon @ISSUE_ @skip
  Scenario Outline: try to create a new subscription using NGSI v2 with "coords" condition expression and too few coordinates for polygon
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_condition_expression_coords_error |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                |
      | subject_type          | room                                                 |
      | subject_idPattern     | .*                                                   |
      | condition_attrs       | temperature                                          |
      | condition_expression  | georel>>>equals&geometry>>>polygon&coords>>><coords> |
      | notification_http_url | http://localhost:1234                                |
      | notification_attrs    | temperature                                          |
      | expires               | 2016-04-05T14:00:00.00Z                              |
    When create a new subscription
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                     |
      | error       | BadRequest                                                |
      | description | Expression not supported: Too few coordinates for polygon |
    Examples:
      | coords                                       |
      | 25.774,-80.190                               |
      | 25.774,-80.190;18.466,-66.118;               |
      | 25.774,-80.190;18.466,-66.118;32.321,-64.757 |
