# -*- coding: utf-8 -*-

# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

# Missing Tests:
#   - verification of headers response
#   - verification of Special Attribute Types
#


Feature: create entities requests (POST) using NGSI v2. "POST" - /v2/entities/ plus payload and queries parameters
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

  # ---------- attribute metadata name --------------------------------

  @attributes_metadata_name
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata name without metadata type
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_name |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_name |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
      # In metadata name always is added a suffix in code. Ex: _0
      | random=254    |

  @attributes_metadata_name_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes metadata name without metadata type but with not plain ascii
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_name |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | metadata_name |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_exists @BUG_1112
  Scenario:  try to create entities using NGSI v2 if attributes metadata exists but without metadata type but the new one has metadata type
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_name |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | metadatas_name   | very_hot    |
      | metadatas_value  | true        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | metadatas_name   | very_hot    |
      | metadatas_value  | true        |
      | metadatas_type   | alarm       |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unprocessable Entity" http code
    And verify several error responses
      | parameter   | value          |
      | error       | Unprocessable  |
      | description | Already Exists |

  @attributes_metadata_name_with_type
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata name with metadata type
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_metadata_name_with_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | metadatas_type   | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_name |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
       # In metadata name always is added a suffix in code. Ex: _0
      | random=254    |

  @attributes_metadata_name_with_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes metadata name with metadata type and not plain ascii
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_metadata_name_with_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | metadatas_type   | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | metadata_name |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_name_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes metadata name that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | celsius     |
      | metadatas_name   | random=257  |
      | metadatas_value  | random=5    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata name length: 257, max length supported: 256 |

  @attributes_metadata_name_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata name without metadata type
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_metadata_name_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_name |
      | room_1      | house<flat>   |
      | room_2      | house=flat    |
      | room_3      | house"flat"   |
      | room_4      | house'flat'   |
      | room_5      | house;flat    |
      | room_6      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_name_with_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata name with metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_metadata_name_without_type_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | attributes_type  | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_name |
      | room_1      | house<flat>   |
      | room_2      | house=flat    |
      | room_3      | house"flat"   |
      | room_4      | house'flat'   |
      | room_5      | house;flat    |
      | room_6      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_name_no_string_error
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes metadata name without metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | "my_default"    |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_name   |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

  @attributes_metadata_name_with_type_no_string_error
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes metadata name with metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | "my_default"    |
      | attributes_type  | "nothing"       |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_name   |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

   # ---------- attribute metadata value --------------------------------

  @attributes_metadata_value
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata value without metadata type
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_metadata_value |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | room2            |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_value |
      | dgdgdfgd       |
      | 34             |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | habitación     |
      | españa         |
      | barça          |
      | random=10      |
      | random=100     |
      | random=1000    |
      | random=10000   |
      | random=50000   |
      | random=100000  |

  @attributes_metadata_value_with_type
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | room2            |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | random=6         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_value |
      | 34             |
      | dgdgdfgd       |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | habitación     |
      | españa         |
      | barça          |
      | random=10      |
      | random=100     |
      | random=1000    |
      | random=10000   |
      | random=50000   |
      | random=100000  |

  @attributes_metadata_value_without @BUG_1200
  Scenario:  try to create entities using NGSI v2 without attributes metadata value nor metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   | random=5    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attributes_metadata_value_without_and_with_type @BUG_1200
  Scenario:  try to create entities using NGSI v2 without attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   | random=5    |
      | metadatas_type   | random=6    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attributes_metadata_value_special.row<row.id>
  @attributes_metadata_value_special @BUG_1106 @BUG_1713
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata special values without metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value             |
      | "room1"   | true                       |
      | "room2"   | false                      |
      | "room3"   | 34                         |
      | "room4"   | -34                        |
      | "room5"   | 5.00002                    |
      | "room6"   | -5.00002                   |
      | "room7"   | "41.3763726, 2.1864475,14" |
      | "room8"   | "2017-06-17T07:21:24.238Z" |
      | "room9"   | null                       |

  @attributes_metadata_value_special_object @ISSUE_1712 @ISSUE_1068
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values without metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value                                                                |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_special_4 @BUG_1106 @BUG_1713
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata special values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value             |
      | "room1"   | true                       |
      | "room2"   | false                      |
      | "room3"   | 34                         |
      | "room4"   | -34                        |
      | "room5"   | 5.00002                    |
      | "room6"   | -5.00002                   |
      | "room15"  | "41.3763726, 2.1864475,14" |
      | "room16"  | "2017-06-17T07:21:24.238Z" |
      | "room17"  | null                       |

  @attributes_metadata_value_special_object @ISSUE_1712 @ISSUE_1068
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | "<entity_id>"    |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value                                                                |
      | room7     | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | room8     | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | room9     | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | room10    | {"x": "x1","x2": "b"}                                                         |
      | room11    | {"x": {"x1": "a","x2": "b"}}                                                  |
      | room12    | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | room13    | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | room14    | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_error @BUG_1093 @BUG_1200
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata value without metadata type
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_metadata_value_error |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | <entities_id>    |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_value |
      | room_2      | house<flat>    |
      | room_3      | house=flat     |
      | room_4      | house"flat"    |
      | room_5      | house'flat'    |
      | room_6      | house;flat     |
      | room_8      | house(flat)    |

  @attributes_metadata_value_error_with_type @BUG_1093 @BUG_1200
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_metadata_value_with_type_error |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | <entities_id>    |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | random=6         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_value |
      | room_2      | house<flat>    |
      | room_3      | house=flat     |
      | room_4      | house"flat"    |
      | room_5      | house'flat'    |
      | room_6      | house;flat     |
      | room_8      | house(flat)    |

  @attributes_metadata_value_special_wrong @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong special attributes metadata values without metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | "room_1"  | rwerwer        |
      | "room_2"  | True           |
      | "room_3"  | TRUE           |
      | "room_4"  | False          |
      | "room_5"  | FALSE          |
      | "room_6"  | 34r            |
      | "room_7"  | 5_34           |
      | "room_8"  | ["a", "b"      |
      | "room_9"  | ["a" "b"]      |
      | "room_10" | "a", "b"]      |
      | "room_11" | ["a" "b"}      |
      | "room_12" | {"a": "b"      |
      | "room_13" | {"a" "b"}      |
      | "room_14" | "a": "b"}      |

  @attributes_metadata_value_special_not_allowed @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several compound special attributes metadata values without metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | invalid JSON type for attribute metadata value |
    Examples:
      | entity_id | metadata_value                                                                |
      | "room15"  | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room16"  | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room17"  | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room18"  | {"x": "x1","x2": "b"}                                                         |
      | "room19"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room20"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room21"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room22"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_special_wrong @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong special attributes metadata values with metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | "room_1"  | rwerwer        |
      | "room_2"  | True           |
      | "room_3"  | TRUE           |
      | "room_4"  | False          |
      | "room_5"  | FALSE          |
      | "room_6"  | 34r            |
      | "room_7"  | 5_34           |
      | "room_8"  | ["a", "b"      |
      | "room_9"  | ["a" "b"]      |
      | "room_10" | "a", "b"]      |
      | "room_11" | ["a" "b"}      |
      | "room_12" | {"a": "b"      |
      | "room_13" | {"a" "b"}      |
      | "room_14" | "a": "b"}      |

  @attributes_metadata_value_special_not_allowed @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several compound special attributes metadata values with metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | invalid JSON type for attribute metadata value |
    Examples:
      | entity_id | metadata_value                                                                |
      | "room15"  | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room16"  | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room17"  | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room18"  | {"x": "x1","x2": "b"}                                                         |
      | "room19"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room20"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room21"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room22"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  # ---------- attribute metadata type --------------------------------

  @attributes_metadata_type
  Scenario Outline:  create entities using NGSI v2 with several attributes with metadata types
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | room2           |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_type |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
      | random=256    |

  @attributes_metadata_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes with metadata types and not plain ascii
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | room2           |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    Examples:
      | metadata_type |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_type_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes metadata type that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | celsius     |
      | metadatas_name   | temperature |
      | metadatas_value  | random=5    |
      | metadatas_type   | random=257  |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata type length: 257, max length supported: 256 |

  @attributes_metadata_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata type
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_metadata_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_type |
      | room_2      | house<flat>   |
      | room_3      | house=flat    |
      | room_4      | house"flat"   |
      | room_5      | house'flat'   |
      | room_6      | house;flat    |
      | room_8      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_type_no_string_error @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several invalid attributes metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | "default"       |
      | metadatas_type   | <metadata_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_type |
      | "room1"   | rewrewr       |
      | "room2"   | SDFSDFSDF     |

  @attributes_metadata_type_no_string_error @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several not allowed attributes metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | "default"       |
      | metadatas_type   | <metadata_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | invalid JSON type for attribute metadata type |
    Examples:
      | entity_id | metadata_type   |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

  # ---------- Queries Parameters - options=keyValues --------------------------------

  @qp_key_values
  Scenario Outline:  create entities using NGSI v2 with keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter         | value       |
      | entities_type     | room        |
      | entities_id       | room2       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 46          |
      | attributes_type   | celsius     |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
     # queries parameters
      | qp_options        | keyValues   |
    When create entity group with "3" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_key_values_wrong
  Scenario Outline:  try to create several entities using NGSI v2 with wrong keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value     |
      | entities_type    | room      |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | 54        |
     # queries parameters
      | qp_options       | <options> |
    When create entity group with "1" entities in "<format>" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |
    Examples:
      | options  | format     |
      | sdfdsfsd | normalized |
      | sdfdsfsd | keyValues  |
      | house_?  | normalized |
      | house_?  | keyValues  |
      | house_&  | normalized |
      | house_&  | keyValues  |
      | house_/  | normalized |
      | house_/  | keyValues  |
      | house_#  | normalized |
      | house_#  | keyValues  |
      | my house | normalized |
      | my house | keyValues  |

  @qp_key_values_invalid
  Scenario Outline:  try to create several entities using NGSI v2 with wrong keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value     |
      | entities_type    | room      |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | 54        |
     # queries parameters
      | qp_options       | <options> |
    When create entity group with "1" entities in "<format>" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | options     | format     |
      | house<flat> | normalized |
      | house<flat> | keyValues  |
      | house=flat  | normalized |
      | house=flat  | keyValues  |
      | house"flat" | normalized |
      | house"flat" | keyValues  |
      | house'flat' | normalized |
      | house'flat' | keyValues  |
      | house;flat  | normalized |
      | house;flat  | keyValues  |
      | house(flat) | normalized |
      | house(flat) | keyValues  |

  @qp_key_values_off_only_value.row<row.id>
  @qp_key_values_off_only_value @BUG_1716 @BUG_1789
  Scenario Outline:  try to create an entity using NGSI v2 without keyValues mode activated, but in only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | attribute must be a JSON object, unless keyValues option is used |
    Examples:
      | entity_id | attribute_value                                                               |
      | "room0"   | "34"                                                                          |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room11"  | null                                                                          |
      | "room12"  | "sdfsdf.sdfsdf"                                                               |
      | "room13"  | "41.3763726, 2.1864475,14"                                                    |

  @qp_key_values_off_only_value_2 @BUG_1716 @BUG_1789 @BUG_1892
  Scenario Outline:  try to create an entity using NGSI v2 without keyValues mode activated, but in only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                       |
      | error       | BadRequest                                  |
      | description | unrecognized property for context attribute |
    Examples:
      | entity_id | attribute_value                                |
      | "room14"  | {"x": {"x1": "a","x2": "b"}}                   |
      | "room15"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}      |
      | "room16"  | {"x": ["a", 45, "rt"],"x2": "b"}               |
      | "room17"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"} |
      | "room18"  | {"x": "x1","x2": "b"}                          |

  @qp_key_values_on_only_value
  Scenario Outline:  create an entity using NGSI v2 with keyValues mode activated, but in only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
      # queries parameters
      | qp_options       | keyValues         |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attribute_value                                                               |
      | "room0"   | "34"                                                                          |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |
      | "room18"  | "sdfsdf.sdfsdf"                                                               |
