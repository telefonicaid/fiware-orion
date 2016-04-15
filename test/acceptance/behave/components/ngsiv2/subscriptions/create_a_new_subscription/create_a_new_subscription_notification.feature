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

  # ------------ notification field and its sub-fields ---------------------

  @notification_without
  Scenario:  try to create a new subscription using NGSI v2 without notification field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                      |
      | subject_idPattern       | .*                         |
      | condition_attributes    | temperature                |
      | notification_callback   | without notification field |
      | notification_attributes | temperature                |
      | expires                 | 2016-04-05T14:00:00.00Z    |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | no notification for subscription specified |

  # ------------ notification - callback field ---------------------
  @notification_callback
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification callback field values
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_notification_callback |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | <callback>              |
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
      | callback                                  |
      | http://localhost                          |
      | http://localhost:1234                     |
      | http://localhost:1234/my_path             |
      | http://localhost:1234/my_path/multiple    |
      | https://localhost:1234/my_path/multiple   |
      | http://192.10.10.1:2134                   |
      | http://255.255.255.254:65535              |
      | http://255.255.255.254:65535/my_path      |
      | http://192.168.0.1:1234/my_path/multiple  |
      | https://192.168.0.1:1234/my_path/multiple |
      | http://my_host                            |
      | http://my_host.server                     |
      | http://my_host-2                          |
      | http://my_host-2.server                   |
      | http://my_host_2                          |
      | http://my_host-2:2134                     |
      | http://my_host:3455/my_path               |
      | http://my_host:3455/my_path/multiple      |

  @notification_callback_empty @BUG_2007 @skip
  Scenario:  try to create a new subscription using NGSI v2 with empty values in notification callback
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_callback_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   |                         |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value          |
      | error       | BadRequest     |
      | description | Empty callback |

  @notification_callback_without
  Scenario:  try to create a new subscription using NGSI v2 without notification callback
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_callback_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value               |
      | error       | BadRequest          |
      | description | callback is missing |

  @notification_callback_invalid.row<row.id>
  @notification_callback_invalid @BUG_2006 @BUG_2009 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification callback field  invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_callback_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | <callback>              |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL in notification callback |
    Examples:
      | callback                   |
      | ws://                      |
      | ://localhost               |
      | http//localhost            |
      | http:/localhost            |
      | http:localhost             |
      | http://localhost:          |
      | http://localhost:dsfsdf    |
      | http://localhost\my_path   |
      | http://e34.56.45.34        |
      | http://34,56.45.34         |
      | http://34.56.45.34;3454    |
      | http://34.56:3454          |
      | http://                    |
      | http://.                   |
      | http://..                  |
      | http://../                 |
      | http://?                   |
      | http://??/                 |
      | http://#                   |
      | http://##/                 |
      | //                         |
      | //a                        |
      | ///a                       |
      | ///                        |
      | http:///a                  |
      | foo.com                    |
      | rdar://1234                |
      | h://test                   |
      | http://                    |
      | ://                        |
      | http://foo.bar/foo(bar)baz |
      | ftps://foo.bar/            |
      | http://-error-.invalid/    |
      | http://a.b--c.de/          |
      | http://-a.b.co             |
      | http://a.b-.co             |
      | http://0.0.0.0             |
      | http://10.1.1.255          |
      | http://1.1.1.1.1           |
      | http://3441.2344.1231.1123 |
      | http://123.123.123         |
      | http://3628126748          |
      | http://.www.foo.bar/       |
      | http://www.foo.bar./       |
      | http://.www.foo.bar./      |

  @notification_callback_forbidden @BUG_2006 @BUG_2009 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification callback field forbidden values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_callback_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | <callback>              |
      | notification_attributes | temperature             |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL in notification callback |
    Examples:
      | callback    |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  # ------------ notification - attributes field ---------------------
  @notification_attributes
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification attributes field values
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_notification_attributes |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | <attributes>            |
      | expires                 | 2016-04-05T14:00:00.00Z |
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

  @notification_attributes_array_empty
  Scenario:  try to create a new subscription using NGSI v2 with array is empty in notification attributes field
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_notification_attributes |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes | array is empty          |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_attributes_without @BUG_1952 @skip
  Scenario:  try to create a new subscription using NGSI v2 without notification attributes field
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_notification_attributes |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attributes  | temperature             |
      | notification_callback | http://localhost:1234   |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo

  @notification_attributes_empty @BUG_2018 @skip
  Scenario:  try to create a new subscription using NGSI v2 with empty notification attributes field
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_notification_attributes |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                   |
      | subject_idPattern       | .*                      |
      | condition_attributes    | temperature             |
      | notification_callback   | http://localhost:1234   |
      | notification_attributes |                         |
      | expires                 | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | invalid payload: empty attribute name |

  @notification_attributes_multiples
  Scenario Outline:  try to create a new subscription using NGSI v2 with multiples names in notification attributes field
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_notification_attributes |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                      | value                   |
      | subject_idPattern              | .*                      |
      | condition_attributes           | temperature             |
      | notification_callback          | http://localhost:1234   |
      | notification_attributes_number | <number>                |
      | notification_attributes        | temperature             |
      | expires                        | 2016-04-05T14:00:00.00Z |
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
      | 50000  |


 #  --- (Pending to changes and/ or development) still missing tests for:
 #  - notification field and its subfields
 #  - expires field
 #  - status field



