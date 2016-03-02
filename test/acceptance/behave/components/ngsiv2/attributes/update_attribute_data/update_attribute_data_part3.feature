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


Feature: update an attribute by entity ID and attribute name if it exists using NGSI v2 API. "PUT" - /v2/entities/<entity_id>/attrs/<attr_name> plus payload
  queries parameters:
  tested: type
  As a context broker user
  I would like to update an attribute by entity ID and attribute name if it exists using NGSI v2 API
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

 # --------------------- attribute metadata name  ------------------------------------

  @attribute_metadata_name_update
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata name without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_1 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_name |
      | room_0    | dfsdfsdf             |
      | room_1    | 34                   |
      | room_2    | 34.4E-34             |
      | room_3    | temp.34              |
      | room_4    | temp_34              |
      | room_5    | temp-34              |
      | room_6    | TEMP34               |
      | room_7    | house_flat           |
      | room_8    | house.flat           |
      | room_9    | house-flat           |
      | room_10   | house@flat           |
      | room_14   | random=10            |
      | room_15   | random=100           |
      | room_16   | random=256           |

  @attribute_metadata_name_update_not_ascii_plain
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with not ascii plain attribute metadata name without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_1 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | entity_id | attributes_meta_name |
      | room_11   | habitación           |
      | room_12   | españa               |
      | room_13   | barça                |

  @attribute_metadata_name_update_max_length_exceed
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with attribute metadata name exceed max length allowed (256) without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_1 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 78         |
      | metadatas_name   | random=257 |
      | metadatas_value  | 5678       |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata name length: 257, max length supported: 256 |

  @attribute_metadata_name_update_without_attr_value @BUG_1868 @skip
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with attribute metadata and  without attribute value
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_1 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter       | value   |
      | metadatas_name  | my_meta |
      | metadatas_value | 5678    |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |

  @attribute_metadata_name_update_2.row<row.id>
  @attribute_metadata_name_update_2 @BUG_1217
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata name with attribute type previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_2 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_name |
      | room_0    | dfsdfsdf             |
      | room_1    | 34                   |
      | room_2    | 34.4E-34             |
      | room_3    | temp.34              |
      | room_4    | temp_34              |
      | room_5    | temp-34              |
      | room_6    | TEMP34               |
      | room_7    | house_flat           |
      | room_8    | house.flat           |
      | room_9    | house-flat           |
      | room_10   | house@flat           |
      | room_14   | random=10            |
      | room_15   | random=100           |
      | room_16   | random=256           |

  @attribute_metadata_name_update_3
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata name with metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_3 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | <entity_id>            |
      | attributes_name  | temperature            |
      | attributes_value | 34                     |
      | attributes_type  | celsius                |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | hot                    |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_name |
      | room_0    | dfsdfsdf             |
      | room_1    | 34                   |
      | room_2    | 34.4E-34             |
      | room_3    | temp.34              |
      | room_4    | temp_34              |
      | room_5    | temp-34              |
      | room_6    | TEMP34               |
      | room_7    | house_flat           |
      | room_8    | house.flat           |
      | room_9    | house-flat           |
      | room_10   | house@flat           |
      | room_14   | random=10            |
      | room_15   | random=100           |
      | room_16   | random=256           |

  @attribute_metadata_name_update_4 @BUG_1433
  Scenario Outline: update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata name with metadatas with metadata types previously in attribute
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attribute_metadata_name_update_4 |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | <entity_id>            |
      | attributes_name  | temperature            |
      | attributes_value | 34                     |
      | attributes_type  | celsius                |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_type   | alarm                  |
      | metadatas_value  | hot                    |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_name |
      | room_0    | dfsdfsdf             |
      | room_1    | 34                   |
      | room_2    | 34.4E-34             |
      | room_3    | temp.34              |
      | room_4    | temp_34              |
      | room_5    | temp-34              |
      | room_6    | TEMP34               |
      | room_7    | house_flat           |
      | room_8    | house.flat           |
      | room_9    | house-flat           |
      | room_10   | house@flat           |
      | room_14   | random=10            |
      | room_15   | random=100           |
      | room_16   | random=256           |

  @attribute_metadata_name_update_forbidden @BUG_1220
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | entity_id | attributes_meta_name |
      | room_1    | house<flat>          |
      | room_2    | house=flat           |
      | room_3    | house"flat"          |
      | room_4    | house'flat'          |
      | room_5    | house;flat           |
      | room_6    | house(flat)          |
      | room_7    | house_?              |
      | room_8    | house_&              |
      | room_9    | house_/              |
      | room_10   | house_#              |
      | room_11   | my house             |

  @attribute_metadata_name_update_forbidden @BUG_1220
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | celsius                |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | entity_id | attributes_meta_name |
      | room_1    | house<flat>          |
      | room_2    | house=flat           |
      | room_3    | house"flat"          |
      | room_4    | house'flat'          |
      | room_5    | house;flat           |
      | room_6    | house(flat)          |
      | room_7    | house_?              |
      | room_8    | house_&              |
      | room_9    | house_/              |
      | room_10   | house_#              |
      | room_11   | my house             |

  @attribute_metadata_name_update_wrong @BUG_1428
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_meta_name |
      | room_1    | rewrewr              |
      | room_2    | SDFSDFSDF            |
      | room_3    | false                |
      | room_4    | true                 |
      | room_5    | 34                   |
      | room_6    | {"a":34}             |
      | room_7    | ["34", "a", 45]      |
      | room_8    | null                 |

  @attribute_metadata_name_update_wrong @BUG_1428
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_value | 78                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | "celsius"              |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_meta_name |
      | room_1    | rewrewr              |
      | room_2    | SDFSDFSDF            |
      | room_3    | false                |
      | room_4    | true                 |
      | room_5    | 34                   |
      | room_6    | {"a":34}             |
      | room_7    | ["34", "a", 45]      |
      | room_8    | null                 |

  @attribute_metadata_name_update_empty @BUG_1438
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with empty attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 78    |
      | metadatas_name   |       |
      | metadatas_value  | 5678  |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

  @attribute_metadata_name_update_empty @BUG_1438
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with empty attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 84          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   |         |
      | metadatas_value  | 5678    |
      | metadatas_type   | celsius |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

   # --------------------- attribute metadata value  ------------------------------------

  @attribute_metadata_value_update_1 @BUG_1216
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | attribute_metadata_value_update |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_value |
      | room_0    | gdfgdfg               |
      | room_1    | 34                    |
      | room_2    | 34.4E-34              |
      | room_3    | temp.34               |
      | room_4    | temp_34               |
      | room_5    | temp-34               |
      | room_6    | TEMP34                |
      | room_7    | house_flat            |
      | room_8    | house.flat            |
      | room_9    | house-flat            |
      | room_10   | house@flat            |
      | room_11   | habitación            |
      | room_12   | españa                |
      | room_13   | barça                 |
      | room_14   | random=10             |
      | room_15   | random=100            |
      | room_16   | random=1000           |
      | room_17   | random=10000          |
      | room_18   | random=100000         |
      | room_19   | random=1000000        |

  @attribute_metadata_value_update_2 @BUG_1216
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | attribute_metadata_value_update |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_type   | alarm                   |
      | metadatas_value  | <attributes_meta_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id | attributes_meta_value |
      | room_0    | gdfgdfg               |
      | room_1    | 34                    |
      | room_2    | 34.4E-34              |
      | room_3    | temp.34               |
      | room_4    | temp_34               |
      | room_5    | temp-34               |
      | room_6    | TEMP34                |
      | room_7    | house_flat            |
      | room_8    | house.flat            |
      | room_9    | house-flat            |
      | room_10   | house@flat            |
      | room_11   | habitación            |
      | room_12   | españa                |
      | room_13   | barça                 |
      | room_14   | random=10             |
      | room_15   | random=100            |
      | room_16   | random=1000           |
      | room_17   | random=10000          |
      | room_18   | random=100000         |
      | room_19   | random=1000000        |

  @attributes_metadata_compound_value @ISSUE_1712 @skip
  # The json values still are not allowed.
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with compound attribute metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
 # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value            |
      | attributes_name  | "temperature"    |
      | attributes_value | 78               |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
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

  @attributes_metadata_compound_value @ISSUE_1712 @skip
  # The json values still are not allowed.
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with compound attribute metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
 # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value            |
      | attributes_value | 78               |
      | metadatas_name   | "alarm"          |
      | metadatas_type   | "nothing"        |
      | metadatas_value  | <metadata_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
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

  @attribute_metadata_value_update_special_without_meta_type @BUG_1220
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and without attribute metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
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
      | parameter        | value            |
      | attributes_value | 78               |
      | metadatas_name   | "very_hot_0"     |
      | metadatas_value  | <metadata_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | metadata_value |
      | room1     | true           |
      | room2     | false          |
      | room3     | 34             |
      | room4     | -34            |
      | room5     | 5.00002        |
      | room6     | -5.00002       |

  @attribute_metadata_value_update_special_with_meta_type @BUG_1220
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and with attribute metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
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
      | parameter        | value            |
      | attributes_value | 78               |
      | metadatas_name   | "very_hot_0"     |
      | metadatas_type   | "nothing"        |
      | metadatas_value  | <metadata_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | metadata_value |
      | room1     | true           |
      | room2     | false          |
      | room3     | 34             |
      | room4     | -34            |
      | room5     | 5.00002        |
      | room6     | -5.00002       |

  @attribute_metadata_value_update_forbidden @BUG_1216
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
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

  @attribute_metadata_value_update_forbidden @BUG_1216
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | celsius                 |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
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

  @attribute_metadata_value_update_wrong @BUG_1428
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | "very_hot_0"            |
      | metadatas_value  | <attributes_meta_value> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
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

  @attribute_metadata_value_update_wrong @BUG_1428
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_value | 78                      |
      | metadatas_name   | "very_hot_0"            |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | "celsius"               |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
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

  @attribute_metadata_value_update_without
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 without attributes metadata values and without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 78         |
      | metadatas_name   | very_hot_0 |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attribute_metadata_value_update_without
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 without attributes metadata values and without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
# These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 78         |
      | metadatas_name   | very_hot_0 |
      | metadatas_type   | alarm      |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

      # --------------------- attribute metadata type  ------------------------------------
  @attribute_metadata_type_update @BUG_1216
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
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
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | very_hot_0      |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | metadata_type |
      | dgdfgdfg      |
      | 34            |
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
      | random=256    |

  @attribute_metadata_type_update_not_ascii_plain
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with not ascii plain attribute metadata type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
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
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | very_hot_0      |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    Examples:
      | metadata_type |
      | habitación    |
      | españa        |
      | barça         |

  @attribute_metadata_type_update_exceed_max_length
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with attribute metadata type exceed max length allowed (256)
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
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
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 78         |
      | metadatas_name   | very_hot_0 |
      | metadatas_value  | 678        |
      | metadatas_type   | random=257 |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata type length: 257, max length supported: 256 |

  @attribute_metadata_type_new @BUG_1216
  Scenario Outline:  update an existent attribute by entity ID and attribute name using NGSI v2 API with a new attribute metadata and different attribute metadata type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
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
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | my_meta         |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | metadata_type |
      | sdfsdf        |
      | 34            |
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
      | random=256    |

  @attribute_metadata_type_update_error @BUG_1232
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attribute metadata type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | my_meta         |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    Examples:
      | entity_id | metadata_type |
      | room_1    | house<flat>   |
      | room_2    | house=flat    |
      | room_3    | house"flat"   |
      | room_4    | house'flat'   |
      | room_5    | house;flat    |
      | room_6    | house(flat)   |
      | room_7    | house_?       |
      | room_8    | house_&       |
      | room_9    | house_/       |
      | room_10   | house_#       |
      | room_11   | my house      |

  @attribute_metadata_type_update_wrong @BUG_1232
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | "my_meta"       |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_type |
      | room1     | rewrewr       |
      | room2     | SDFSDFSDF     |

  @attribute_metadata_type_update_wrong @BUG_1232
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_value | 78              |
      | metadatas_name   | "my_meta"       |
      | metadatas_value  | 678             |
      | metadatas_type   | <metadata_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | invalid JSON type for attribute metadata type |
    Examples:
      | entity_id | metadata_type   |
      | room3     | false           |
      | room4     | true            |
      | room5     | 34              |
      | room6     | {"a":34}        |
      | room7     | ["34", "a", 45] |

  @attribute_metadata_type_update_wo_meta_value
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with metadata types and without metadata value
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_type   | alarm   |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attribute_metadata_type_update_empty
  Scenario:  update an attribute by entity ID and attribute name using NGSI v2 with empty metadata types and without metadata value
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
 # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5654    |
      | metadatas_type   |         |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

