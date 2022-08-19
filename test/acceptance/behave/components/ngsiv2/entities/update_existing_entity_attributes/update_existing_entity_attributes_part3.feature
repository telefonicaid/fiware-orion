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


Feature: update an attribute by entity ID if it exists using NGSI v2. "PATCH" - /v2/entities/<entity_id> plus payload
  As a context broker user
  I would like to update an attribute by entity ID if it exists using NGSI v2
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
  @attribute_metadata_name_without_meta_type @BUG_1217 @ISSUE_1786
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata name without metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | room                   |
      | attributes_name  | temperature            |
      | attributes_value | 345                    |
      | attributes_type  | celsius                |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_type   | alarm                  |
      | metadatas_value  | hot                    |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | cold                   |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | 34                   |
      | my_meta              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=254           |

 @attribute_metadata_name_without_meta_type @BUG_1217 @ISSUE_1786 @BUG_2394
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata name without metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | room                   |
      | attributes_name  | temperature            |
      | attributes_value | 345                    |
      | attributes_type  | celsius                |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_type   | alarm                  |
      | metadatas_value  | hot                    |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | cold                   |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | 34.4E-34             |
      | temp.34              |
      | house.flat           |

  @attribute_metadata_name_not_ascii_plain
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with not ascii plain attribute metadata name without metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 345         |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | my_metadata |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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

  @attribute_metadata_name_not_ascii_plain
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with not ascii plain attribute metadata name with metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 345         |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | my_metadata |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | alarm                  |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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

  @attribute_metadata_name_exceed_max_length_allowed
  Scenario:  try to update an attribute by entity ID using NGSI v2 with attribute metadata name exceed max length allowed with metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 345         |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | my_metadata |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 106         |
      | metadatas_number | 2           |
      | metadatas_name   | random=255  |
      | metadatas_value  | 5678        |
      | metadatas_type   | alarm       |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata name length: 257, max length supported: 256 |

  @attribute_metadata_name_with_meta_type @BUG_1217 @ISSUE_1786
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata name with metadata type and attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | room                   |
      | attributes_name  | temperature            |
      | attributes_value | 345                    |
      | attributes_type  | celsius                |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_type   | alarm                  |
      | metadatas_value  | hot                    |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | nothing                |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | 34                   |
      | my_meta              |
      | temp_34              |
      | temp-34              |
      | TEMP34               |
      | house_flat           |
      | house-flat           |
      | house@flat           |
      | random=10            |
      | random=100           |
      | random=254           |

  @attribute_metadata_name_with_meta_type @BUG_1217 @ISSUE_1786 @BUG_2394
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata name with metadata type and attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value                  |
      | entities_type    | house                  |
      | entities_id      | room                   |
      | attributes_name  | temperature            |
      | attributes_value | 345                    |
      | attributes_type  | celsius                |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_type   | alarm                  |
      | metadatas_value  | hot                    |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 106                    |
      | metadatas_number | 2                      |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | nothing                |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_name |
      | 34.4E-34             |
      | temp.34              |
      | house.flat           |

  @attribute_metadata_name_without_attr_value @BUG_1217 @ISSUE_1786 @BUG_1789
  Scenario:  try to update an attribute by entity ID using NGSI v2 with several attribute metadata name without metadata type nor attribute value
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | warning     |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value       |
      | attributes_name | temperature |
      | metadatas_name  | my_meta     |
      | metadatas_value | 5678        |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_name_without_attr_value @BUG_1217 @ISSUE_1786 @BUG_1789
  Scenario:  try to update an attribute by entity ID using NGSI v2 with several attribute metadata name with metadata type nor attribute value
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | warning     |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value       |
      | attributes_name | temperature |
      | metadatas_name  | my_meta     |
      | metadatas_value | 5678        |
      | metadatas_type  | nothing     |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_replace_without_meta_type @BUG_1788
  Scenario:  update an attribute by entity ID using NGSI v2 and append a new metadata without metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | warning     |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value       |
      | attributes_name | temperature |
      | metadatas_name  | my_meta     |
      | metadatas_value | 5678        |
    When update attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_replace_with_meta_type @BUG_1788
  Scenario:  update an attribute by entity ID using NGSI v2 and append a new metadata with metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | warning     |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | metadatas_name   | my_meta     |
      | metadatas_value  | 5678        |
      | metadatas_type   | nothing     |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @attribute_metadata_name_update_forbidden @BUG_1220
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with forbidden attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 34                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
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
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 34                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | celsius                |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
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

  @attribute_metadata_name_update_wrong @BUG_1220
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "temperature"          |
      | attributes_value | 34                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
    When update attributes by ID "room2" if it exists in raw and "normalized" modes
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

  @attribute_metadata_name_update_wrong @BUG_1220
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | "temperature"          |
      | attributes_value | 34                     |
      | metadatas_name   | <attributes_meta_name> |
      | metadatas_value  | 5678                   |
      | metadatas_type   | "celsius"              |
    When update attributes by ID "room2" if it exists in raw and "normalized" modes
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

  @attribute_metadata_name_update_empty @BUG_1220
  Scenario:  try to update an attribute by entity ID using NGSI v2 with empty attribute metadata name without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   |             |
      | metadatas_value  | 5678        |
    When update attributes by ID "room2" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

  @attribute_metadata_name_update_empty @BUG_1220
  Scenario:  try to update an attribute by entity ID using NGSI v2 with empty attribute metadata name with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_attribute_metadata_name_update_error |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   |             |
      | metadatas_value  | 5678        |
      | metadatas_type   | celsius     |
    When update attributes by ID "room2" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                 |
      | error       | BadRequest            |
      | description | missing metadata name |

    # --------------------- attribute metadata value  ------------------------------------

  @attribute_metadata_value_update @BUG_1220
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | attribute_metadata_value_update |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
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
      | parameter        | value                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_value |
      | vbcvbcvb              |
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

  @attribute_metadata_value_update @BUG_1232
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | attribute_metadata_value_update |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
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
      | parameter        | value                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_type   | warning                 |
      | metadatas_value  | <attributes_meta_value> |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_value |
      | fdgdfgdfg             |
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

  @attribute_metadata_value_update_special_without_meta_type @BUG_1220
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and without attribute metadata type
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "temperature"           |
      | attributes_value | 34                      |
      | metadatas_name   | "very_hot_0"            |
      | metadatas_value  | <attributes_meta_value> |
    When update attributes by ID "<entity_id>_1" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value |
      | room1     | true                  |
      | room2     | false                 |
      | room3     | 34                    |
      | room4     | -34                   |
      | room5     | 5.00002               |
      | room6     | -5.00002              |

  @attribute_metadata_value_update_special_with_meta_type @BUG_1220
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with special metadata attribute values (compound, vector, boolean, etc) and with attribute metadata type
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | "temperature"           |
      | attributes_value | 34                      |
      | metadatas_name   | "very_hot_0"            |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | "warning"               |
    When update attributes by ID "<entity_id>_1" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_meta_value |
      | room1     | true                  |
      | room2     | false                 |
      | room3     | 34                    |
      | room4     | -34                   |
      | room5     | 5.00002               |
      | room6     | -5.00002              |

  @attributes_metadata_compound_value_object @ISSUE_1712 @ISSUE_1068
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values without metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value         |
      | entities_type    | "room"        |
      | entities_id      | "<entity_id>" |
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
      | attributes_value | 34               |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
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

  @attributes_metadata_compound_value_object @ISSUE_1712 @ISSUE_1068
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value         |
      | entities_type    | "room"        |
      | entities_id      | "<entity_id>" |
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
      | attributes_value | 34               |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "No Content" http code
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

  @attribute_metadata_value_update_forbidden @BUG_1216
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with forbidden attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
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
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with forbidden attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
  # These properties below are used in update request
    And properties to entities
      | parameter        | value                   |
      | attributes_name  | temperature             |
      | attributes_value | 34                      |
      | metadatas_name   | very_hot_0              |
      | metadatas_value  | <attributes_meta_value> |
      | metadatas_type   | celsius                 |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
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

  @attribute_metadata_value_update_wrong
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong attributes metadata values without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value            |
      | attributes_name  | "temperature"    |
      | attributes_value | 34               |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | room_1    | rwerwer        |
      | room_2    | True           |
      | room_3    | TRUE           |
      | room_4    | False          |
      | room_5    | FALSE          |
      | room_6    | 34r            |
      | room_7    | 5_34           |
      | room_8    | ["a", "b"      |
      | room_9    | ["a" "b"]      |
      | room_10   | "a", "b"]      |
      | room_11   | ["a" "b"}      |
      | room_12   | {"a": "b"      |
      | room_13   | {"a" "b"}      |
      | room_14   | "a": "b"}      |

  @attribute_metadata_value_update_wrong
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong attributes metadata values with attribute metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | attribute_metadata_value_update_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value            |
      | attributes_name  | "temperature"    |
      | attributes_value | 34               |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | room_1    | rwerwer        |
      | room_2    | True           |
      | room_3    | TRUE           |
      | room_4    | False          |
      | room_5    | FALSE          |
      | room_6    | 34r            |
      | room_7    | 5_34           |
      | room_8    | ["a", "b"      |
      | room_9    | ["a" "b"]      |
      | room_10   | "a", "b"]      |
      | room_11   | ["a" "b"}      |
      | room_12   | {"a": "b"      |
      | room_13   | {"a" "b"}      |
      | room_14   | "a": "b"}      |

     # --------------------- attribute metadata type  ------------------------------------
  @attribute_metadata_type_update @BUG_1216
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attribute metadata type
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value                  |
      | attributes_name  | temperature            |
      | attributes_value | 345                    |
      | metadatas_name   | very_hot_0             |
      | metadatas_value  | 678                    |
      | metadatas_type   | <attributes_meta_type> |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_meta_type |
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
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with not ascii plain attribute metadata type but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 345         |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | my_metadata |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_name  | temperature     |
      | attributes_value | 106             |
      | metadatas_number | 2               |
      | metadatas_name   | my_metadata     |
      | metadatas_value  | 56              |
      | metadatas_type   | <metadata_type> |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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

  @attribute_metadata_type_exceed_max_length_allowed
  Scenario:  try to update an attribute by entity ID using NGSI v2 with attribute metadata type exceed max length allowed but with attribute value in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_attribute_metadata_name_update |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 345         |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | my_metadata |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 106         |
      | metadatas_number | 2           |
      | metadatas_name   | my_meta     |
      | metadatas_value  | 5678        |
      | metadatas_type   | random=257  |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata type length: 257, max length supported: 256 |

  @attribute_metadata_type_update_forbidden @BUG_1232 @skip
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with forbidden attribute metadata type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | attribute_metadata_type_update |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
  # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_name  | temperature     |
      | attributes_value | 106             |
      | metadatas_number | 2               |
      | metadatas_name   | my_meta         |
      | metadatas_value  | 5678            |
      | metadatas_type   | <metadata_type> |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    And verify that entities are not stored in mongo
    Examples:
      | entity_id | metadata_type |
      | room_2    | house<flat>   |
      | room_3    | house=flat    |
      | room_4    | house"flat"   |
      | room_5    | house'flat'   |
      | room_6    | house;flat    |
      | room_8    | house(flat)   |
      | room_7    | house_?       |
      | room_8    | house_&       |
      | room_9    | house_/       |
      | room_10   | house_#       |
      | room_11   | my house      |

  @attribute_metadata_type_update_wrong
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_name  | "temperature"   |
      | attributes_value | 34              |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | true            |
      | metadatas_type   | <metadata_type> |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_type |
      | room1     | rewrewr       |
      | room2     | SDFSDFSDF     |

  @attribute_metadata_type_update_wrong @BUG_1232 @skip
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with wrong metadata attribute types
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value           |
      | attributes_name  | "temperature"   |
      | attributes_value | 34              |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | true            |
      | metadatas_type   | <metadata_type> |
    When update attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
