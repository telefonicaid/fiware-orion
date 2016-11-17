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


Feature: Attribute metadata value in update Batch operation using NGSI v2. "POST" - /v2/op/update plus payload and queries parameters
  As a context broker user
  I would like to  create entities requests using NGSI v2
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

  #  --------------------- attribute metadata value ---------------------------

  @attribute_metadata_value
  Scenario Outline:  update entities with update batch operations using NGSI v2 with several attribute metadata values and without metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_op_update_attribute_meta_value |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room1                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | warning                 |
      | metadatas_value  | <attributes_meta_value> |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_meta_value |
      | fsdfsd                |
      | 34                    |
      | 34.4E-34              |
      | temp.34               |
      | temp_34               |
      | temp-34               |
      | TEMP34                |
      | house_flat            |
      | house.flat            |
      | house-flat            |
      | house@flat            |
      | habitación            |
      | españa                |
      | barça                 |
      | random=10             |
      | random=100            |
      | random=1000           |
      | random=10000          |
      | random=100000         |
      | random=500000         |
      | random=1000000        |

  @attribute_metadata_value
  Scenario Outline:  update entities with update batch operations using NGSI v2 with several attribute metadata values and with metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_op_update_attribute_meta_value |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room1                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | warning                 |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | alarm                   |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_meta_value |
      | fsdfsd                |
      | 34                    |
      | 34.4E-34              |
      | temp.34               |
      | temp_34               |
      | temp-34               |
      | TEMP34                |
      | house_flat            |
      | house.flat            |
      | house-flat            |
      | house@flat            |
      | habitación            |
      | españa                |
      | barça                 |
      | random=10             |
      | random=100            |
      | random=1000           |
      | random=10000          |
      | random=100000         |
      | random=500000         |
      | random=1000000        |


  @attribute_metadata_value_without
  Scenario:  try update entities with update batch operations using NGSI v2 without attributes values
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_op_update_attribute_meta_value |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | romm_2      |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | metadatas_name   | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attribute_metadata_value_without_with_type
  Scenario:  update entities with update batch operations using NGSI v2 without attributes values but with attribute type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_op_update_attribute_meta_value |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room_3      |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | metadatas_name   | warning     |
      | metadatas_type   | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attribute_metadata_value_special
  Scenario Outline:  create an entity using NGSI v2 with special attributes metatada values without type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_op_update_attribute_meta_value_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | "house"                 |
      | entities_id      | "<entity_id>"           |
      | attributes_name  | "temperature"           |
      | attributes_value | 56                      |
      | metadatas_name   | "warning"               |
      | metadatas_value  | <attributes_meta_value> |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value                                                         |
      | room1     | true                                                                          |
      | room2     | false                                                                         |
      | room3     | 34                                                                            |
      | room4     | -34                                                                           |
      | room5     | 5.00002                                                                       |
      | room6     | -5.00002                                                                      |
      | room7     | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | room8     | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | room9     | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | room10    | {"x": "x1","x2": "b"}                                                         |
      | room11    | {"x": {"x1": "a","x2": "b"}}                                                  |
      | room12    | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | room13    | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | room14    | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | room15    | "41.3763726, 2.1864475,14"                                                    |
      | room16    | "2017-06-17T07:21:24.238Z"                                                    |
      | room17    | null                                                                          |

  @attribute_metadata_value_special
  Scenario Outline:  create an entity using NGSI v2 with several attributes special values with type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_op_update_attribute_meta_value_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | "house"                 |
      | entities_id      | "<entity_id>"           |
      | attributes_name  | "temperature"           |
      | attributes_value | 67                      |
      | attributes_type  | "celsius"               |
      | metadatas_name   | "warning"               |
      | metadatas_type   | "alarm"                 |
      | metadatas_value  | <attributes_meta_value> |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value                                                         |
      | room1     | true                                                                          |
      | room2     | false                                                                         |
      | room3     | 34                                                                            |
      | room4     | -34                                                                           |
      | room5     | 5.00002                                                                       |
      | room6     | -5.00002                                                                      |
      | room7     | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | room8     | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | room9     | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | room10    | {"x": "x1","x2": "b"}                                                         |
      | room11    | {"x": {"x1": "a","x2": "b"}}                                                  |
      | room12    | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | room13    | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | room14    | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | room15    | "41.3763726, 2.1864475,14"                                                    |
      | room16    | "2017-06-17T07:21:24.238Z"                                                    |
      | room17    | null                                                                          |

  @attribute_metadata_value_wrong @BUG_2682
  Scenario Outline:  try to update entities with update batch operations using NGSI v2 with several wrong attributes values
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_op_update_attribute_meta_value_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
    # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | random=3                |
      | attributes_name  | temperature             |
      | attributes_value | 56                      |
      | metadatas_name   | warning                 |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | alarm                   |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    And verify that entities are not stored in mongo
    Examples:
      | attributes_meta_value |
      | house<flat>           |
      | house=flat            |
      | house"flat"           |
      | house'flat'           |
      | house;flat            |
      | house(flat)           |

  @attribute_metadata_value_wrong_without_type
  Scenario Outline:  try to update entities with update batch operations using NGSI v2 with several wrong attributes metadata special values without metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_op_update_attribute_meta_value_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | "house"                 |
      | entities_id      | "<entity_id>"           |
      | attributes_name  | "temperature"           |
      | attributes_value | 78                      |
      | metadatas_name   | "warning"               |
      | metadatas_value  | <attributes_meta_value> |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_meta_value |
      | room_1    | rwerwer               |
      | room_2    | True                  |
      | room_3    | TRUE                  |
      | room_4    | False                 |
      | room_5    | FALSE                 |
      | room_6    | 34r                   |
      | room_7    | 5_34                  |
      | room_8    | ["a", "b"             |
      | room_9    | ["a" "b"]             |
      | room_10   | "a", "b"]             |
      | room_11   | ["a" "b"}             |
      | room_12   | {"a": "b"             |
      | room_13   | {"a" "b"}             |
      | room_14   | "a": "b"}             |

  @attribute_metadata_value_wrong_with_type
  Scenario Outline:  try to update entities with update batch operations using NGSI v2 with several wrong attributes special values with attribute type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_op_update_attribute_meta_value_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value                   |
      | entities_type    | "house"                 |
      | entities_id      | "<entity_id>"           |
      | attributes_name  | "temperature"           |
      | attributes_value | 78                      |
      | attributes_type  | "celsius"               |
      | metadatas_name   | "warning"               |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | "alarm"                 |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_meta_value |
      | room_1    | rwerwer               |
      | room_2    | True                  |
      | room_3    | TRUE                  |
      | room_4    | False                 |
      | room_5    | FALSE                 |
      | room_6    | 34r                   |
      | room_7    | 5_34                  |
      | room_8    | ["a", "b"             |
      | room_9    | ["a" "b"]             |
      | room_10   | "a", "b"]             |
      | room_11   | ["a" "b"}             |
      | room_12   | {"a": "b"             |
      | room_13   | {"a" "b"}             |
      | room_14   | "a": "b"}             |
