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

  @happy_path
  Scenario:  update an attribute by entity ID and attribute name if it exists using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
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
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @without_payload @BUG_1385
  Scenario:  try to update an attribute by entity ID and attribute name if it exists using NGSI v2 but without_payload
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
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
      | parameter          |
      | without_properties |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Content Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | ContentLengthRequired                            |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  @maximum_size @BUG_1385
  Scenario:  try to update attributes by ID and attribute name using NGSI v2 with maximum size in payload
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_update_max_size |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
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
      | parameter        | value          |
      | attributes_value | random=1048378 |
      | attributes_type  | Fahrenheit     |
      | metadatas_number | 3              |
      | metadatas_name   | very_hot       |
      | metadatas_type   | alarm          |
      | metadatas_value  | cold           |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048577, max size supported: 1048576 |

  # ------------------------ Service ----------------------------------------------

  @service_update
  Scenario Outline:  update attributes by entity ID and atrribute name using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
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
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_update_without
  Scenario:  update attributes by entity ID and attribute name using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
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
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_update_error @BUG_1873
  Scenario Outline:  try to update attributes by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                  |
      | error       | BadRequest                                                                             |
      | description | bad character in tenant name - only underscore and alphanumeric characters are allowed |
    Examples:
      | service     |
      | service.sr  |
      | Service-sr  |
      | Service(sr) |
      | Service=sr  |
      | Service<sr> |
      | Service,sr  |
      | service#sr  |
      | service%sr  |
      | service&sr  |

  @service_update_bad_length
  Scenario:  try to update attributes by entity ID and attribute name using NGSI v2 with bad length service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

 # ------------------------ Service path ----------------------------------------------

  @service_path_update @BUG_1423
  Scenario Outline:  update attributes by entity ID and attribute nameusing NGSI v2 with several service header values
    Given a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
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
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | service_path                                                  |
      |                                                               |
      | /                                                             |
      | /service_path                                                 |
      | /service_path_12                                              |
      | /Service_path                                                 |
      | /SERVICE                                                      |
      | /serv1/serv2/serv3/serv4/serv5/serv6/serv7/serv8/serv9/serv10 |
      | max length allowed                                            |
      | max length allowed and ten levels                             |

  @service_path_update_without
  Scenario:  update attributes by entity ID and attribute name using NGSI v2 without service header
    Given  a definition of headers
      | parameter      | value                    |
      | Fiware-Service | test_update_service_path |
      | Content-Type   | application/json         |
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
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_update_error
  Scenario Outline:  try to update attributes by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | a component of ServicePath contains an illegal character |
    Examples:
      | service_path |
      | /service.sr  |
      | /service;sr  |
      | /service=sr  |
      | /Service-sr  |
      | /serv<45>    |
      | /serv(45)    |

  @service_path_update_error
  Scenario Outline:  try to update attributes by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_update_error
  Scenario Outline:  try to update attributes by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_update_error
  Scenario:  try to update attributes by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_service_path_error       |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #  -------------------------- entity id --------------------------------------------------

  @entity_id_update
  Scenario Outline:  update attributes by entity ID and attribute name using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value         |
      | entities_type     | <entity_type> |
      | entities_id       | <entity_id>   |
      | attributes_number | 3             |
      | attributes_name   | temperature   |
      | attributes_value  | 34            |
      | attributes_type   | celsius       |
      | metadatas_number  | 2             |
      | metadatas_name    | very_hot      |
      | metadatas_type    | alarm         |
      | metadatas_value   | hot           |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 3          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "the same value of the previous request" and attribute name "temperature_0" if it exists
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

  @entity_id_update_not_ascii_plain
  Scenario Outline:  update attributes by entity ID and attribute name using NGSI v2 with not ascii plain entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "<entity_id>" and attribute name "temperature_0" if it exists
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

  @entity_not_exists @BUG_1360
  Scenario:  try to update an attribute by entity ID and attribute name but the entitiy id does not exist using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_not_exists |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "sdfsdfs" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @more_entities_update @BUG_1387 @skip
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with more than one entity id  with the same id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_update_more_entities |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "room" and attribute name "temperature_0" if it exists
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                                          |
      | error       | TooManyResults                                                                 |
      | description | There is more than one entity that match the update. Please refine your query. |

  @entity_id_update_invalid @BUG_1351
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "<entity_id>" and attribute name "temperature_0" if it exists
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

  @entity_id_update_invalid @BUG_1351
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "house_?" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | Empty right-hand-side for URI param //attrs/temperature_0// |

  @entity_id_update_invalid @BUG_1351
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "house_/" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

  @entity_id_update_invalid @BUG_1351 @ISSUE_2080 @skip
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "house_#" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | attribute must be a JSON object, unless keyValues option is used |

  @entity_id_empty @ISSUE_1426
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 API with empty entity_id
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute by ID "" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | entity id length: 0, min length supported: 1 |

 # --------------------- attribute name  ------------------------------------

  @attribute_name_update
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | <attributes_name> |
      | attributes_value | 34                |
      | attributes_type  | celsius           |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | hot               |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room" and attribute name "the same value of the previous request" if it exists
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

  @attribute_name_update_not_ascii_plain
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with not ascii plain attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "<attributes_name>" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | attributes_name |
      | habitación      |
      | españa          |
      | barça           |

  @attribute_name_not_exists @BUG_1360
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 but the attribute name does not exist
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
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
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room_1" and attribute name "ewrewrwrw" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @attribute_name_invalid.row<row.id>
  @attribute_name_invalid @BUG_1351
  Scenario Outline:  try to update an attribute by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "<entity_id>_1" and attribute name "<attributes_name>" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      # \' is replaced in code by ". Limitation of Behave. See case 3.
      | entity_id | attributes_name     |
      | room_1    | house<flat>         |
      | room_2    | house=flat          |
      | room_3    | house\'flat\'       |
      | room_4    | house'flat'         |
      | room_5    | house;flat          |
      | room_6    | house(flat)         |
      | room_11   | {\'a\':34}          |
      | room_12   | [\'34\', \'a\', 45] |

  @attribute_name_invalid
  Scenario:  try to update an attribute by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room_1" and attribute name "house_&" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |

  @attribute_name_invalid @BUG_1360
  Scenario:  try to update an attribute by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room_1" and attribute name "house_?" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                   |
      | error       | BadRequest                              |
      | description | Empty right-hand-side for URI param /// |

  @attribute_name_invalid @BUG_1360
  Scenario:  try to update an attribute by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room_1" and attribute name "house_/" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @attribute_name_invalid @BUG_1360
  Scenario:  try to update an attribute by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room_1" and attribute name "house_#" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @attribute_name_empty
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 API with empty attribute name
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 25    |
    When update an attribute by ID "room" and attribute name "" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | attribute must be a JSON object, unless keyValues option is used |

 # --------------------- attribute type  ------------------------------------

  @attribute_type_update_wo_attr_type
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attributes type, without attribute type nor metadata previously in create request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | dfgfdgfd        |
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

  @attribute_type_update_not_ascii_plain.row<row.id>
  @attribute_type_update_not_ascii_plain @BUG_1844
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with not ascii plain attributes type, without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value         |
      | entities_type    | <entity_type> |
      | entities_id      | room          |
      | attributes_name  | temperature   |
      | attributes_value | 56            |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "room_1" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    Examples:
      | entity_type | attributes_type |
      | house_1     | habitación      |
      | house_2     | españa          |
      | house_3     | barça           |

  @attribute_type_update_max_length_allowed @BUG_1845
  Scenario:  try to update an attribute by entity ID and attribute name using NGSI v2 with max_length_allowed attributes type, without attribute type nor metadatas previously in attribute
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | celsius     |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 25         |
      | attributes_type  | random=257 |
    When update an attribute by ID "room_1" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute type length: 257, max length supported: 256 |

  @attribute_type_update_w_type
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attributes type, with attribute type previously in create request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
      | attributes_type   | celsius     |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | dfgfdgfd        |
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

  @attribute_type_update_w_metadata
  Scenario Outline:  update an attribute by entity ID and attribute name using NGSI v2 with several attributes type, with metadata previously in create request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
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
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | cvxcvxcv        |
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

  @attribute_type_update_forbidden.row<row.id>
  @attribute_type_update_forbidden @BUG_1212 @BUG_1847
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with forbidden attributes type
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
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists
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

  @attribute_type_update_wrong @BUG_1428
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with wrong attributes type
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
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_type |
      | room1     | rewrewr         |
      | room2     | SDFSDFSDF       |

  @attribute_type_update_invalid @BUG_1212
  Scenario Outline:  try to update an attribute by entity ID and attribute name using NGSI v2 with invalid attributes type
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
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | 25                |
      | attributes_type  | <attributes_type> |
    When update an attribute by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
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
