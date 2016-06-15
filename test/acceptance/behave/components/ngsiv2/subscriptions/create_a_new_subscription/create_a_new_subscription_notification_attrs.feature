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


  # ------------ notification - attrs field ---------------------
  @notification_attrs_without @BUG_1952
  Scenario:  create a new subscription using NGSI v2 without notification attrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_attrs_array_empty
  Scenario:  create a new subscription using NGSI v2 with array is empty in notification attrs field. the ”ONANYCHANGE” case.
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | array is empty          |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_attrs
  Scenario Outline:  create a new subscription using NGSI v2 with notification attrs field values
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | <attributes>            |
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

  @notification_attrs_empty @BUG_2018
  Scenario:  try to create a new subscription using NGSI v2 with empty notification attrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    |                         |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | attrs element is empty |

  @notification_attrs_multiples
  Scenario Outline:  try to create a new subscription using NGSI v2 with multiples names in notification attributes field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_attrs_number | <number>                |
      | notification_attrs        | temperature             |
      | expires                   | 2016-04-05T14:00:00.00Z |
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

  @notification_attrs_forbidden @BUG_2095
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden values in notification attrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notification_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | <attribute>             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | attrs element has forbidden char |
    Examples:
      | attribute   |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  @notification_attrs_not_plain_ascii @BUG_2099
  Scenario Outline:  try to create a new subscription using NGSI v2 with not plain ascii values in notification attrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notification_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | <attribute>             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | attrs element has forbidden char |
    Examples:
      | attribute  |
      | habitación |
      | españa     |
      | barça      |

  @notification_attrs_length_exceed @BUG_2100 @skip
  Scenario:  try to create a new subscription using NGSI v2 with length max exceed (256) in notification attrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notification_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | notification_attrs    | random=257              |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                              |
      | error       | BadRequest                                                         |
      | description | notification attribute name length: 257, max length supported: 256 |

  @notification_attrs_two_equals @BUG_2101 @skip
  Scenario:  try to create a new subscription using NGSI v2 with attributes name equals in notification attrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notification_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                       |
      | subject_idPattern     | ".*"                        |
      | condition_attrs       | "temperature"               |
      | notification_http_url | "http://localhost:1234"     |
      | notification_attrs    | "temperature","temperature" |
      | expires               | "2016-04-05T14:00:00.00Z"   |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | BadRequest      |
      | description | not definet yet |

  @notification_attrs_n_equals @BUG_2101 @skip
  Scenario:  try to create a new subscription using NGSI v2 with attributes name equals in notification attrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notification_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                                      |
      | subject_idPattern     | ".*"                                                                       |
      | condition_attrs       | "temperature"                                                              |
      | notification_http_url | "http://localhost:1234"                                                    |
      | notification_attrs    | "temperature","temperature","pressure","temperature","speed","temperature" |
      | expires               | "2016-04-05T14:00:00.00Z"                                                  |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | BadRequest      |
      | description | not definet yet |

  # ------------ notification - exceptAttrs field ---------------------
  @notification_except_attrs_without
  Scenario:  create a new subscription using NGSI v2 without notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | http://localhost:1234   |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_except_attrs
  Scenario Outline:  create a new subscription using NGSI v2 with notification exceptAttrs field values
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs | <attributes>            |
      | expires                   | 2016-04-05T14:00:00.00Z |
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

  @notification_except_attrs_empty
  Scenario:  try to create a new subscription using NGSI v2 with empty notification excptAttrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs |                         |
      | expires                   | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | attrs element is empty |

  @notification_except_attrs_array_empty
  Scenario:  try to create a new subscription using NGSI v2 with array empty notification excptAttrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs | array is empty          |
      | expires                   | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | http notification has exceptAttrs is empty |

  @notification_except_attrs_multiples
  Scenario Outline:  try to create a new subscription using NGSI v2 with multiples names in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_attrs_number | <number>                |
      | notification_except_attrs | temperature             |
      | expires                   | 2016-04-05T14:00:00.00Z |
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

  @notification_except_attrs_forbidden
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden values in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_notif_except_attrs |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs | <attribute>             |
      | expires                   | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | attrs element has forbidden char |
    Examples:
      | attribute   |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  @notification_except_attrs_not_plain_ascii
  Scenario Outline:  try to create a new subscription using NGSI v2 with not plain ascii values in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notif_except_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs | <attribute>             |
      | expires                   | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | attrs element has forbidden char |
    Examples:
      | attribute  |
      | habitación |
      | españa     |
      | barça      |

  @notification_except_attrs_length_exceed @BUG_2100 @skip
  Scenario:  try to create a new subscription using NGSI v2 with length max exceed (256) in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notif_except_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                   |
      | subject_idPattern         | .*                      |
      | condition_attrs           | temperature             |
      | notification_http_url     | http://localhost:1234   |
      | notification_except_attrs | random=257              |
      | expires                   | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                              |
      | error       | BadRequest                                                         |
      | description | notification attribute name length: 257, max length supported: 256 |

  @notification_except_attrs_two_equals @BUG_2101 @skip
  Scenario:  try to create a new subscription using NGSI v2 with attributes name equals in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notif_except_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
      # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                       |
      | subject_idPattern         | ".*"                        |
      | condition_attrs           | "temperature"               |
      | notification_http_url     | "http://localhost:1234"     |
      | notification_except_attrs | "temperature","temperature" |
      | expires                   | "2016-04-05T14:00:00.00Z"   |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | BadRequest      |
      | description | not definet yet |

  @notification_except_attrs_n_equals @BUG_2101 @skip
  Scenario:  try to create a new subscription using NGSI v2 with attributes name equals in notification exceptAttrs field
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_notif_except_attrs_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                                                                      |
      | subject_idPattern         | ".*"                                                                       |
      | condition_attrs           | "temperature"                                                              |
      | notification_http_url     | "http://localhost:1234"                                                    |
      | notification_except_attrs | "temperature","temperature","pressure","temperature","speed","temperature" |
      | expires                   | "2016-04-05T14:00:00.00Z"                                                  |
    When create a new subscription in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | BadRequest      |
      | description | not definet yet |

