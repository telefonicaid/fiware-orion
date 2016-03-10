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

   #   -------------- queries parameters ------------------------------------------
   #   ---  type query parameter ---

  @more_entities @BUG_1346  @skip
  Scenario:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute metadata name without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attr_more_entities |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5678    |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @qp_type
  Scenario:  update an attribute by entity ID and attribute name using NGSI v2 with "type" query parameter
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5678    |
      # query parameter
      | qp_type          | house_1 |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_type_empty
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with "type" query parameter with empty value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5678    |
      # query parameter
      | qp_type          |         |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |

  @qp_unknown @BUG_1831 @skip
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with "type" query parameter with empty value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5678    |
      # query parameter
      | qp_<parameter>   | house_1 |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |
    Examples:
      | parameter |
      |           |
      | dfgdfgfgg |
      | house_&   |
      | my house  |
      | p_/       |
      | p_#       |

  @qp_invalid
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with "type" query parameter with invalid value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 78      |
      | metadatas_name   | my_meta |
      | metadatas_value  | 5678    |
      # query parameter
      | qp_<parameter>   | house_1 |
    When update an attribute by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | parameter           |
      | ========            |
      | p<flat>             |
      | p=flat              |
      | p'flat'             |
      | p\'flat\'           |
      | p;flat              |
      | p(flat)             |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |


