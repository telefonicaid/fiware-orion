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


Feature: update an attribute value by entity ID and attribute name if it exists using NGSI v2 API. "PUT" - /v2/entities/<entity_id>/attrs/<attr_name>/value plus payload
  As a context broker user
  I would like to update an attribute value by entity ID and attribute name if it exists using NGSI v2 API
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
  Scenario:  update an attribute value by entity ID and attribute name if it exists using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @without_payload
  Scenario:  try to update an attribute value by entity ID and attribute name if it exists using NGSI v2 but without payload
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
    Then verify that receive an "Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | LengthRequired                                   |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  @maximum_size
  Scenario:  try to update attributes value by ID and attribute name using NGSI v2 with maximum size in payload
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_update_max_size |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value          |
      | attributes_value | random=1048564 |
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048577, max size supported: 1048576 |

  # ------------------------ Service ----------------------------------------------

  @service_update
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
  Scenario:  update attributes value by entity ID and attribute name using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_update_error
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                                                                         |
      | error       | BadRequest                                                                                                                                    |
      | description | tenant name not accepted - a tenant string must not be longer than 50 characters and may only contain underscores and alphanumeric characters |
    Examples:
      | service                         |
      | service.sr                      |
      | Service-sr                      |
      | Service(sr)                     |
      | Service=sr                      |
      | Service<sr>                     |
      | Service,sr                      |
      | greater than max length allowed |

 # ------------------------ Service path ----------------------------------------------

  @service_path_update @BUG_1423 @skip
  Scenario Outline:  update attributes value by entity ID and attribute nameusing NGSI v2 with several service header values
    Given a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
  Scenario:  update attributes value by entity ID and attribute name using NGSI v2 without service header
    Given  a definition of headers
      | parameter      | value                    |
      | Fiware-Service | test_update_service_path |
      | Content-Type   | application/json         |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_update_error
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
  Scenario:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_service_path_error       |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #  -------------------------- entity id --------------------------------------------------

  @entity_id_update
  Scenario Outline:  update attributes value by entity ID and attribute name using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And create "1" entities with "3" attributes
      | parameter        | value         |
      | entities_type    | <entity_type> |
      | entities_id      | <entity_id>   |
      | attributes_name  | temperature   |
      | attributes_value | 34            |
      | attributes_type  | celcius       |
      | metadatas_number | 2             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | hot           |
    And verify that receive several "Created" http code
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
      | room_14     | habitación |
      | room_15     | españa     |
      | room_16     | barça      |
      | room_17     | random=10  |
      | room_18     | random=100 |
      | room_19     | random=960 |

  @entity_not_exists @BUG_1360 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name but the entitiy id does not exist using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_not_exists |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @more_entities_update @BUG_1387 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with more than one entity id with the same id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_update_more_entities |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                                          |
      | error       | TooManyResults                                                                 |
      | description | There is more than one entity that match the update. Please refine your query. |

  @entity_id_update_invalid @BUG_1351 @skip
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
      | house_?             |
      | house_&             |
      | house_/             |
      | house_#             |

  @entity_id_empty @ISSUE_1487 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 API with empty entity_id
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "" and attribute name "speed" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

 # --------------------- attribute name  ------------------------------------

  @attribute_name_update
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "1" entities with "1" attributes
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | <attributes_name> |
      | attributes_value | 34                |
      | attributes_type  | celcius           |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | hot               |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "the same value of the previous request" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
      | habitación      |
      | españa          |
      | barça           |
      | random=10       |
      | random=100      |
      | random=10000    |
      | random=50000    |

  @attribute_name_not_exists @BUG_1360 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 but the attribute name does not exist
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "1" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "wrwerwerw" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | invalid character in URI |

  @attribute_name_invalid @BUG_1351 @skip
  Scenario Outline:  try to update an attribute value by entity id and attribute name using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "<attributes_name>" if it exists
      | parameter        | value |
      | attributes_value | 80    |
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
      | room_7    | house_?             |
      | room_8    | house_&             |
      | room_9    | house_/             |
      | room_10   | house_#             |
      | room_11   | {\'a\':34}          |
      | room_12   | [\'34\', \'a\', 45] |

  @attribute_name_empty @ISSUE_1487 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 API with empty attribute name
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "" if it exists
      | parameter        | value |
      | attributes_value | 80    |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

# --------------------- attribute value  ------------------------------------

  @attribute_value_update_without_attribute_type
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_update_attr_value_without_attr_type |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | fdgdfgfd         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | habitación       |
      | españa           |
      | barça            |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=50000     |

  @attribute_value_update_with_attribute_type
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and with attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_update_attr_value_with_attr_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celcius     |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | gsdfggff         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | habitación       |
      | españa           |
      | barça            |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=50000     |

  @attribute_value_update_with_metadatas
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and with metadatas
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_attr_value_with_metadata |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | tytryrty         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | habitación       |
      | españa           |
      | barça            |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=50000     |

  @attribute_value_update_without_attribute_type_special @BUG_1106 @skip
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And create "1" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_value                                                              |
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

  @attribute_value_update_with_attribute_type_special @BUG_1106 @skip
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with attribute type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And create "1" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 67          |
      | attributes_type  | celcius     |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_value                                                              |
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

  @attribute_value_update_with_metadata_special @BUG_1106 @skip
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with metadatas
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And create "1" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "No Content" http code
    Examples:
      | entity_id | attributes_value                                                              |
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

  @attribute_value_update_with_attribute_type_in_update
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with attribute type in update request
    Given  a definition of headers
      | parameter          | value                                           |
      | Fiware-Service     | test_update_attr_value_with_attr_type_in_update |
      | Fiware-ServicePath | /test                                           |
      | Content-Type       | application/json                                |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value   |
      | attributes_value | 90      |
      | attributes_type  | celcius |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | unexpected JSON field - accepting only /value/ |

  @attribute_value_update_with_metadata_in_update
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with metadata in update request
    Given  a definition of headers
      | parameter          | value                                           |
      | Fiware-Service     | test_update_attr_value_with_attr_type_in_update |
      | Fiware-ServicePath | /test                                           |
      | Content-Type       | application/json                                |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value    |
      | attributes_value | 90       |
      | attributes_type  | celcius  |
      | metadatas_name   | very_hot |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | unexpected JSON field - accepting only /value/ |

  @attribute_value_update_with_metadata_type_in_update
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with metadata type in update request
    Given  a definition of headers
      | parameter          | value                                           |
      | Fiware-Service     | test_update_attr_value_with_attr_type_in_update |
      | Fiware-ServicePath | /test                                           |
      | Content-Type       | application/json                                |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
      | parameter        | value    |
      | attributes_value | 90       |
      | attributes_type  | celcius  |
      | metadatas_name   | very_hot |
      | metadatas_type   | alarm    |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | unexpected JSON field - accepting only /value/ |

  @attribute_value_invalid @BUG_1424 @skip
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 without invalid attribute values in update request
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_update_attr_value_error |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid characters in attribute value |
    Examples:
      | entity_id | attributes_value |
      | room_2    | house<flat>      |
      | room_3    | house=flat       |
      | room_4    | house"flat"      |
      | room_5    | house'flat'      |
      | room_6    | house;flat       |
      | room_8    | house(flat)      |

  @attribute_value_error_special
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with wrong attribute values in update request (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists in raw mode
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_value |
      | room_1    | rwerwer          |
      | room_2    | True             |
      | room_3    | TRUE             |
      | room_4    | False            |
      | room_5    | FALSE            |
      | room_6    | 34r              |
      | room_7    | 5_34             |
      | room_8    | ["a", "b"        |
      | room_9    | ["a" "b"]        |
      | room_10   | "a", "b"]        |
      | room_11   | ["a" "b"}        |
      | room_12   | {"a": "b"        |
      | room_13   | {"a" "b"}        |
      | room_14   | "a": "b"}        |

  @attribute_value_error_special_2 @BUG_1490 @skip
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with a dot in attribute values as dict in update request (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And verify that receive several "Created" http code
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists in raw mode
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_value  |
      | room_1    | {"rt.ty": "5678"} |
