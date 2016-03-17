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


Feature: replace attributes by entity ID using NGSI v2. "PUT" - /v2/entities/<entity_id> plus payload
  Queries parameters:
  tested: options=keyValues
  pending: type
  As a context broker user
  I would like to replace attributes by entity ID using NGSI v2
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

  # --------------------- attribute metadata value  ------------------------------------

  @attribute_metadata_value_replace_wo_attr_value @BUG_1789
  Scenario:  replace attributes by entity ID using NGSI v2 with new attribute metadata values without attribute value nor metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value     |
      | attributes_name | pressure  |
      | metadatas_name  | very_cold |
      | metadatas_value | 45        |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_value_replace_wo_attr_value @BUG_1789
  Scenario:  replace attributes by entity ID using NGSI v2 with new attribute metadata values without attribute value but with metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value     |
      | attributes_name | pressure  |
      | metadatas_name  | very_cold |
      | metadatas_value | 45        |
      | metadatas_type  | nothing   |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_value_replace_without_meta_type @BUG_1220
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | pressure                |
      | attributes_value | 90                      |
      | metadatas_name   | very_cold               |
      | metadatas_value  | <attributes_meta_value> |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_value |
      | fdgfdgdfg             |
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
      | random=1000000        |

  @attribute_metadata_value_replace_with_meta_type @BUG_1232
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
     # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | pressure                |
      | attributes_value | 90                      |
      | metadatas_name   | very_cold               |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | my_metatype             |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_value |
      | fdgfdgdfg             |
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
      | random=1000000        |

  @attributes_metadata_replace_compound_value @ISSUE_1712 @skip
 # The json values still are not allowed.
  Scenario Outline: replace attributes by entity ID using NGSI v2 with several attributes metadata json values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value         |
      | entities_type    | "room"        |
      | entities_id      | <entity_id>   |
      | attributes_name  | "temperature" |
      | attributes_value | "34"          |
      | metadatas_name   | "alarm"       |
      | metadatas_value  | 67            |
      | metadatas_type   | "warning"     |
    And create an entity in raw and "normalized" modes
    And verify that receive an "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value            |
      | attributes_name  | "temperature"    |
      | metadatas_name   | "alarm"          |
      | attributes_value | 34               |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
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

  @attribute_metadata_value_replace_special_without_meta_type @BUG_1220
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "pressure"              |
      | attributes_value | true                    |
      | metadatas_name   | "very_cold"             |
      | metadatas_value  | <attributes_meta_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value |
      | room1     | true                  |
      | room2     | false                 |
      | room3     | 34                    |
      | room4     | -34                   |
      | room5     | 5.00002               |
      | room6     | -5.00002              |

  @attribute_metadata_value_replace_special_with_meta_type @BUG_1220
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "pressure"              |
      | metadatas_name   | "very_cold"             |
      | attributes_value | 45                      |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | "alarm"                 |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value |
      | room1     | true                  |
      | room2     | false                 |
      | room3     | 34                    |
      | room4     | -34                   |
      | room5     | 5.00002               |
      | room6     | -5.00002              |

  @attribute_metadata_value_replace_forbidden @BUG_1216
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with forbidden attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
     # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | pressure                |
      | metadatas_name   | very_cold               |
      | metadatas_value  | <attributes_meta_value> |
      | attributes_value | 90                      |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    Examples:
      | entity_id | attributes_meta_value |
      | room_1    | house<flat>           |
      | room_2    | house=flat            |
      | room_3    | house"flat"           |
      | room_4    | house'flat'           |
      | room_5    | house;flat            |
      | room_6    | house(flat)           |

  @attribute_metadata_value_replace_forbidden @BUG_1216
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with forbidden attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | pressure                |
      | metadatas_name   | very_cold               |
      | metadatas_value  | <attributes_meta_value> |
      | attributes_value | 90                      |
      | metadatas_type   | bar                     |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    Examples:
      | entity_id | attributes_meta_value |
      | room_2    | house<flat>           |
      | room_3    | house=flat            |
      | room_4    | house"flat"           |
      | room_5    | house'flat'           |
      | room_6    | house;flat            |
      | room_8    | house(flat)           |

  @metadata_value_replace_wrong
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "pressure"              |
      | attributes_value | true                    |
      | metadatas_name   | "very_cold"             |
      | metadatas_value  | <attributes_meta_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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

  @metadata_value_replace_wrong
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "pressure"              |
      | attributes_value | true                    |
      | metadatas_name   | "very_cold"             |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | "bar"                   |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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

  # -------------- queries parameters ----------------------
  # ----- options=keyValues query parameter ----

  @qp_options_key_values
  Scenario:  replace attributes by entity ID if it exists using NGSI v2 with options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
        # queries parameters
      | qp_options        | keyValues   |
    When replace attributes by ID "room_1" if it exists and with "keyValues" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_options_key_values_error
  Scenario:  try to replace attributes by entity ID if it exists using NGSI v2 without options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When replace attributes by ID "room_1" if it exists and with "keyValues" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | attribute must be a JSON object, unless keyValues option is used |

  @qp_options_key_value_attr_names_duplicated @BUG_1433
  Scenario:  replace attributes by entity ID if it exists using NGSI v2 with options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      # queries parameters
      | qp_options        | keyValues   |
    When replace attributes by ID "room_1" if it exists and with "keyValues" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
