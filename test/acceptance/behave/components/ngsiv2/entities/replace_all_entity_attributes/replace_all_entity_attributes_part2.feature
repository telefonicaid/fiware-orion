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

 #  -------------------------- entity id --------------------------------------------------

  @entity_id_replace
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value         |
      | entities_type    | <entity_type> |
      | entities_id      | <entity_id>   |
      | attributes_name  | temperature   |
      | attributes_value | 34            |
      | attributes_type  | celsius       |
      | metadatas_number | 2             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | hot           |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_type | entity_id  |
      | room_1      | room       |
      | room_2      | 34         |
      | room_3      | false      |
      | room_4      | true       |
      | room_5      | 34.4E-34   |
      | room_6      | temp.34    |
      | room_7      | temp_34    |
      | room_8      | temp-34    |
      | room_9      | TEMP34     |
      | room_10     | house_flat |
      | room_11     | house.flat |
      | room_12     | house-flat |
      | room_13     | house@flat |
      | room_17     | random=10  |
      | room_18     | random=100 |
      | room_19     | random=256 |

  @entity_id_not_ascii_plain
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with not ascii plain in entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | entity_id  |
      | habitación |
      | españa     |
      | barça      |

  @entity_id_unknown @BUG_1320
  Scenario:  try to replace attributes by entity ID using NGSI v2 with unknown entity id values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_replace_entity_id_unknown |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "sdfdsfsd" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @entity_id_replace_invalid
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | entity_id           |
      | house<flat>         |
      | house=flat          |
      | house'flat'         |
      | house\'flat\'       |
      | house;flat          |
      | house(flat)         |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |
      | house_&             |
      | my house            |

  @entity_id_replace_invalid @BUG_1782
  Scenario:  try to replace attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "house_?" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | Empty right-hand-side for URI param //attrs/ |

  @entity_id_replace_invalid @BUG_1782
  Scenario:  try to replace attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "house_/" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

  @entity_id_replace_invalid @BUG_1782 @ISSUE_2075 @skip
  Scenario:  try to replace attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
    When replace attributes by ID "house_/" if it exists and with "normalized" mode
    Then verify that receive an "Method not allowed" http code
    And verify an error response
      | parameter   | value            |
      | error       | MethodNotAllowed |
      | description | No defined yet   |

  # --------------------- attribute name  ------------------------------------

  @attribute_name_replace @BUG_1323
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute names
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attribute_name_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value    |
      | entities_type    | house    |
      | entities_id      | room     |
      | attributes_name  | pressure |
      | attributes_value | 34       |
      | attributes_type  | celsius  |
      | metadatas_number | 2        |
      | metadatas_name   | very_hot |
      | metadatas_type   | alarm    |
      | metadatas_value  | hot      |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_name |
      | temperature     |
      | temp.48         |
      | temp_49         |
      | temp-50         |
      | TEMP51          |
      | house_flat      |
      | house.flat      |
      | house-flat      |
      | house@flat      |
      | random=10       |
      | random=100      |
      | random=256      |

  @attribute_name_not_ascii_plain
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with not ascii plain in attribute name
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attribute_name_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value    |
      | entities_type    | house    |
      | entities_id      | room     |
      | attributes_name  | pressure |
      | attributes_value | 34       |
      | attributes_type  | celsius  |
      | metadatas_number | 2        |
      | metadatas_name   | very_hot |
      | metadatas_type   | alarm    |
      | metadatas_value  | hot      |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |
    Examples:
      | attributes_name |
      | habitación      |
      | españa          |
      | barça           |

  @attribute_name_exceed_max_length
  Scenario:  try to replace attributes by entity ID using NGSI v2 with attribute name exceed max length
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attribute_name_replace |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
 # These properties below are used in create request
    And properties to entities
      | parameter        | value    |
      | entities_type    | house    |
      | entities_id      | room     |
      | attributes_name  | pressure |
      | attributes_value | 34       |
      | attributes_type  | celsius  |
      | metadatas_number | 2        |
      | metadatas_name   | very_hot |
      | metadatas_type   | alarm    |
      | metadatas_value  | hot      |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_name  | random=257 |
      | attributes_value | 80         |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute name length: 257, max length supported: 256 |

  @attribute_name_invalid
  Scenario Outline:  try to replace attributes using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_attribute_name_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
 # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
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
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |
    Examples:
      | entity_id | attributes_name |
      | room_2    | house<flat>     |
      | room_3    | house=flat      |
      | room_4    | house"flat"     |
      | room_5    | house'flat'     |
      | room_6    | house;flat      |
      | room_8    | house(flat)     |

  @attribute_name_replace_error
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with invalid attribute names
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_attribute_name_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
  # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | true              |
    When replace attributes by ID "room" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_name |
      | rewrewr         |
      | SDFSDFSDF       |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

  @attribute_name_empty
  Scenario:  try to replace attribute using NGSI v2 API with empty attribute name
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_attribute_name_replace_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_name  |       |
      | attributes_value | 80    |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                          |
      | error       | BadRequest                     |
      | description | no 'name' for ContextAttribute |

  # --------------------- attribute type  ------------------------------------

  @attribute_type_replace @BUG_1212
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attributes type
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | attribute_type_replace |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
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
      | parameter        | value             |
      | attributes_name  | pressure          |
      | attributes_value | 80                |
      | attributes_type  | <attributes_type> |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | dfsdf           |
      | 34              |
      | 34.4E-34        |
      | temp.34         |
      | temp_34         |
      | temp-34         |
      | TEMP34          |
      | house_flat      |
      | house.flat      |
      | house-flat      |
      | house@flat      |
      | random=10       |
      | random=100      |
      | random=256      |

  @attribute_type_replace_not_ascii_plain
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with not ascii plain attributes type
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | attribute_type_replace |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
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
      | parameter        | value             |
      | attributes_name  | pressure          |
      | attributes_value | 80                |
      | attributes_type  | <attributes_type> |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    Examples:
      | attributes_type |
      | habitación      |
      | españa          |
      | barça           |

  @attribute_type_replace_exceed_max_length
  Scenario:  try to replace attributes by entity ID using NGSI v2 with attributes type exceed max length allowed (256)
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | attribute_type_replace |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
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
      | parameter        | value      |
      | attributes_name  | pressure   |
      | attributes_value | 80         |
      | attributes_type  | random=257 |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute type length: 257, max length supported: 256 |

  @attribute_type_replace_forbidden @BUG_1212 @BUG_1260
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with forbidden attributes type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_replace_attribute_type_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | pressure          |
      | attributes_value | 80                |
      | attributes_type  | <attributes_type> |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    Examples:
      | entity_id | attributes_type |
      | room_1    | house<flat>     |
      | room_2    | house=flat      |
      | room_3    | house"flat"     |
      | room_4    | house'flat'     |
      | room_5    | house;flat      |
      | room_6    | house(flat)     |
      | room_7    | house_?         |
      | room_8    | house_&         |
      | room_9    | house_/         |
      | room_10   | house_#         |
      | room_11   | my house        |

  @attribute_type_replace_wrong
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attributes type
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_type_error_ii |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 67          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | "pressure"        |
      | attributes_value | true              |
      | attributes_type  | <attributes_type> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_type |
      | room1     | rewrewr         |
      | room2     | SDFSDFSDF       |

  @attribute_type_replace_invalid @BUG_1212
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with invalid attributes type
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_attribute_type_error_iii |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 67          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | "pressure"        |
      | attributes_value | true              |
      | attributes_type  | <attributes_type> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | invalid JSON type for attribute type |
    Examples:
      | entity_id | attributes_type |
      | room3     | false           |
      | room4     | true            |
      | room5     | 34              |
      | room6     | {"a":34}        |
      | room7     | ["34", "a", 45] |

  # --------------------- attribute metadata name  ------------------------------------

  @attribute_metadata_name_replace_without_meta_type @BUG_1217
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata name and with attribute type but without attribute metadata type in replace request
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_name_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 80                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | dsfsdfsd             |
      | 34                   |
      | 34.4E-34             |
      | temp.34              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house.flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=256           |

  @attribute_metadata_name_replace_not_ascii_plain
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with not ascii plain attribute metadata name and with attribute type but without attribute metadata type in replace request
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_name_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 80                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | attributes_meta_name |
      | habitación           |
      | españa               |
      | barça                |

  @attribute_metadata_name_replace_exceed_max_length
  Scenario:  try to replace attributes by entity ID using NGSI v2 with attribute metadata name exceed max length allowed (256)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_name_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
      | parameter        | value      |
      | attributes_name  | pressure   |
      | attributes_value | 80         |
      | metadatas_name   | random=257 |
      | metadatas_value  | 5678       |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata name length: 257, max length supported: 256 |

  @attribute_metadata_name_replace_with_meta_type @BUG_1217
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata name and with attribute metadata type nor attribute type in replace request
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_name_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 80                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | bar                    |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | fsdfsf               |
      | 34                   |
      | 34.4E-34             |
      | temp.34              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house.flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=256           |

  @attribute_metadata_name_replace_forbidden_without_meta_type @BUG_1220
  Scenario Outline:  try to replace attribute by entity ID using NGSI v2 with forbidden attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_metadata_name_replace_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 80                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
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

  @attribute_metadata_name_replace_forbidden_with_meta_type @BUG_1220
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with forbidden attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                      |
      | Fiware-Service     | test_attribute_metadata_name_replace_error |
      | Fiware-ServicePath | /test                                      |
      | Content-Type       | application/json                           |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 80                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | metatype               |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
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

  @attribute_metadata_name_replace_wrong @BUG_1220
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                      |
      | Fiware-Service     | test_attribute_metadata_name_replace_error |
      | Fiware-ServicePath | /test                                      |
      | Content-Type       | application/json                           |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 67          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "pressure"             |
      | attributes_value | true                   |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | "5678"                 |
    When replace attributes by ID "room" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_meta_name |
      | rewrewr              |
      | SDFSDFSDF            |
      | false                |
      | true                 |
      | 34                   |
      | {"a":34}             |
      | ["34", "a", 45]      |
      | null                 |

  @attribute_metadata_name_replace_wrong @BUG_1220
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                      |
      | Fiware-Service     | test_attribute_metadata_name_replace_error |
      | Fiware-ServicePath | /test                                      |
      | Content-Type       | application/json                           |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 67          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "pressure"             |
      | attributes_value | true                   |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | "5678"                 |
      | metadatas_type   | "metatype"             |
    When replace attributes by ID "room" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_meta_name |
      | rewrewr              |
      | SDFSDFSDF            |
      | false                |
      | true                 |
      | 34                   |
      | {"a":34}             |
      | ["34", "a", 45]      |
      | null                 |

  @attribute_metadata_name_replace_empty @BUG_1220
  Scenario:  try to replace attributes by entity ID using NGSI v2 with empty attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_metadata_name_replace_empty |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
      | metadatas_name   |          |
      | metadatas_value  | 5678     |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

  @attribute_metadata_name_replace_empty @BUG_1220
  Scenario:  try to replace attributes by entity ID using NGSI v2 with empty attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_metadata_name_replace_empty |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | pressure |
      | attributes_value | 80       |
      | metadatas_name   |          |
      | metadatas_value  | 5678     |
      | metadatas_type   | celsius  |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

    # --------------------- attribute metadata type  ------------------------------------

  @metadata_type_replace_wo_type @BUG_1216
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata type without attribute type
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_type_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 89                     |
      | metadatas_name   | very_cold              |
      | metadatas_value  | 678                    |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_type |
      | sdfsdf               |
      | 34                   |
      | 34.4E-34             |
      | temp.34              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house.flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=256           |

  @attribute_metadata_type_not_ascii_plain
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with not ascii plain attribute metadata type
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_type_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 89                     |
      | metadatas_name   | very_cold              |
      | metadatas_value  | 678                    |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    Examples:
      | attributes_meta_type |
      | habitación           |
      | españa               |
      | barça                |

  @attribute_metadata_type_exceed_max_length
  Scenario:  replace attributes by entity ID using NGSI v2 with attribute metadata type exceed max length allowed (256)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_type_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_name  | pressure   |
      | attributes_value | 89         |
      | metadatas_name   | very_cold  |
      | metadatas_value  | 678        |
      | metadatas_type   | random=257 |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata type length: 257, max length supported: 256 |

  @metadata_type_replace_w_type @BUG_1216
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute metadata type with attribute type
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_metadata_type_replace |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | pressure               |
      | attributes_value | 89                     |
      | attributes_type  | bar                    |
      | metadatas_name   | very_cold              |
      | metadatas_value  | 678                    |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_type |
      | ddsgdf               |
      | 34                   |
      | 34.4E-34             |
      | temp.34              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house.flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=256           |

  @metadata_type_update_forbidden @BUG_1232
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with forbidden attribute metadata type
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_metadata_type_update |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 45                     |
      | metadatas_name   | very_hot_0             |
      | metadatas_value  | 678                    |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    And verify that entities are not stored in mongo
    Examples:
      | entity_id | attributes_meta_type |
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

  @attribute_metadata_type_update_wrong
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "temperature"          |
      | attributes_value | 45                     |
      | metadatas_name   | "very_hot_0"           |
      | metadatas_value  | "hot"                  |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_meta_type |
      | room1     | rewrewr              |
      | room2     | SDFSDFSDF            |

  @attribute_metadata_type_update_wrong @BUG_1232
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "temperature"          |
      | attributes_value | 45                     |
      | metadatas_name   | "very_hot_0"           |
      | metadatas_value  | "hot"                  |
      | metadatas_type   | <attributes_meta_type> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | invalid JSON type for attribute metadata type |
    Examples:
      | entity_id | attributes_meta_type |
      | room3     | false                |
      | room4     | true                 |
      | room5     | 34                   |
      | room6     | {"a":34}             |
      | room7     | ["34", "a", 45]      |
