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

Feature: delete an attribute request using NGSI v2 API. "DELETE" - /v2/entities/<entity_id>/attrs/<attr_name>
  Queries parameters:
  tested: type
  As a context broker user
  I would like to delete an attribute request using NGSI v2 API
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
  Scenario: Delete an attribute in several entities using NGSI v2 API
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
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
    And modify headers and keep previous values "false"
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive an "No Content" http code
    And verify that the attribute is deleted into mongo in the defined entity

  # ------------------------ Content-Type header ----------------------------------------------
  @with_content_type @BUG_2128
  Scenario Outline: Delete an attribute in several entities using NGSI v2 API with Content-Type header
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
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
    And modify headers and keep previous values "false"
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | <content_type>         |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                        |
      | error       | BadRequest                                                                                   |
      | description | Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden |
    Examples:
      | content_type                      |
      | application/json                  |
      | application/xml                   |
      | application/x-www-form-urlencoded |
      | multipart/form-data               |
      | text/plain                        |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  @with_empty_content_type @BUG_2364 @skip
  Scenario: Delete an attribute in several entities using NGSI v2 API with Content-Type header and empty value
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
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
    And modify headers and keep previous values "false"
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       |                        |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                        |
      | error       | BadRequest                                                                                   |
      | description | Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden |

  # ------------------------ Service header ----------------------------------------------
  @service_delete
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with several service header values
    Given a definition of headers
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
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | the same value of the previous request |
      | Fiware-ServicePath | /test                                  |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_delete_without
  Scenario: Delete an attribute by entity ID using NGSI v2 without service header
    Given a definition of headers
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
    And modify headers and keep previous values "false"
      | parameter          | value |
      | Fiware-ServicePath | /test |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo

  @service_delete_error @BUG_1873
  Scenario Outline: Try to delete an attribute by entity ID using NGSI v2 with wrong service header values
    Given a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
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

  @service_delete_bad_length
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with bad length service header values
    Given a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ------------------------ Service path header ----------------------------------------------

  @service_path_delete @BUG_1423
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with several service path header values
    Given a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_delete_service_path |
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
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | test_delete_service_path               |
      | Fiware-ServicePath | the same value of the previous request |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo
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

  @service_path_delete_without
  Scenario: Delete an attribute by entity ID using NGSI v2 without service path header
    Given a definition of headers
      | parameter      | value                    |
      | Fiware-Service | test_delete_service_path |
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
    And modify headers and keep previous values "false"
      | parameter      | value                    |
      | Fiware-Service | test_delete_service_path |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo

  @service_path_delete_error
  Scenario Outline: Try to delete an attribute by entity ID using NGSI v2 with wrong service header path values
    Given a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_delete_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
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

  @service_path_delete_error
  Scenario Outline: Try to delete an attribute by entity ID using NGSI v2 with wrong service path header values
    Given a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_delete_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_delete_error
  Scenario Outline: Try to delete an attribute by entity ID using NGSI v2 with service path header values max length allowed exceed
    Given a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_replace_service_path_error |
      | Fiware-ServicePath | <service_path>                  |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_delete_error
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with too many components in service path header values
    Given a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_service_path_error      |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When delete an attribute "temperature_0" in the entity with id "room_1"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

#  -------------------------- entity id --------------------------------------------------

  @entity_id_delete
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with several entity id values
    Given a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
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
    And modify headers and keep previous values "false"
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
      | Fiware-ServicePath | /test                 |
    When delete an attribute "temperature_0" in the entity with id "the same value of the previous request"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo
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

  @entity_id_delete_not_ascii_plain
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with not ascii plain entity id values
    Given a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
      | Fiware-ServicePath | /test                 |
    When delete an attribute "temperature_0" in the entity with id "<entity_id>"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | entity_id  |
      | habitación |
      | españa     |
      | barça      |

  @entity_not_exists
  Scenario: Try to delete an attribute by entity ID but it does not exists using NGSI v2
    Given a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
      | Fiware-ServicePath | /test                 |
    When delete an attribute "temperature_0" in the entity with id "fdgdfgfdgd"
    Then verify that receive a "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @more_entities_delete @BUG_1346
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with more than one entity with the same id
    Given a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
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
      | type   | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
      | Fiware-ServicePath | /test                 |
    When delete an attribute "temperature_0" in the entity with id "room"
    Then verify that receive a "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @entity_id_delete_invalid
  Scenario Outline: Try to delete an attribute by entity ID using NGSI v2with invalid entity id values
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
    When delete an attribute "temperature_0" in the entity with id "<entity_id>"
    Then verify that receive a "Bad Request" http code
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

  @entity_id_delete_invalid
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with empty right-hand-side for entity id
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
    When delete an attribute "temperature_0" in the entity with id "house_?"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | BadRequest                                                 |
      | description | Empty right-hand-side for URI param //attrs/temperature_0/ |

  @entity_id_delete_invalid
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with invalid entity id values
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
    When delete an attribute "temperature_0" in the entity with id "house_/"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

  @entity_id_delete_invalid
  Scenario: Try to delete an attribute by entity ID using NGSI v2 with invalid entity id values
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
    When delete an attribute "temperature_0" in the entity with id "house_#"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

 # --------------------- attribute name  ------------------------------------

  @attribute_name_delete
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with several attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_delete |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_delete |
      | Fiware-ServicePath | /test                      |
    When delete an attribute "the same value of the previous request" in the entity with id "room_1"
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo
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

  @attribute_name_delete_not_ascii_plain
  Scenario Outline: Delete an attribute by entity ID using NGSI v2 with not ascii plain attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_delete |
      | Fiware-ServicePath | /test                      |
    When delete an attribute "<attributes_name>" in the entity with id "room_1"
    Then verify that receive a "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |
    Examples:
      | attributes_name |
      | habitación      |
      | españa          |
      | barça           |

  @attribute_name_not_exists
  Scenario:  Try to delete an attribute by entity ID using NGSI v2 but the attribute name does not exist
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_delete |
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
    And create entity group with "2" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_delete |
      | Fiware-ServicePath | /test                      |
    When delete an attribute "wrwerwerw" in the entity with id "room_1"
    Then verify that receive a "Not Found" http code
    And verify an error response
      | parameter   | value               |
      | error       | NotFound            |
      | description | Attribute not found |

  @attribute_name_invalid
  Scenario Outline:  try to delete an attribute by entity ID using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
    When delete an attribute "<attributes_name>" in the entity with id "<entity_id>"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      # \' is replaced in code by " . Limitation of Behave. See case 3.
      | entity_id | attributes_name     |
      | room_1    | house<flat>         |
      | room_2    | house=flat          |
      | room_3    | house\'flat\'       |
      | room_4    | house'flat'         |
      | room_5    | house;flat          |
      | room_6    | house(flat)         |
      | room_7    | {\'a\':34}          |
      | room_8    | [\'34\', \'a\', 45] |

  @attribute_name_invalid @BUG_1351
  Scenario Outline:  try to delete an attribute by entity ID using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
    When delete an attribute "<attributes_name>" in the entity with id "<entity_id>"
    Then verify that receive a "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |
    Examples:
      | entity_id | attributes_name |
      | room_9    | house_?         |
      | room_10   | house_&         |
      | room_11   | house_/         |
      | room_12   | house_#         |

  @attribute_name_empty @ISSUE_2078
  Scenario:  try to delete an attribute by entity ID using NGSI v2 API with empty attribute name
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
    When delete an attribute "" in the entity with id "room"
    Then verify that receive an "Method not allowed" http code
    And verify an error response
      | parameter   | value              |
      | error       | MethodNotAllowed   |
      | description | method not allowed |

   #   -------------- queries parameters ------------------------------------------
   #   ---  type query parameter ---

  @qp_type
  Scenario Outline:  delete an attribute by entity ID using NGSI v2 with "type" query parameter
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
    When delete an attribute "temperature" in the entity with id "room"
      | parameter | value   |
      | type      | <types> |
    Then verify that receive a "No Content" http code
    And verify that the attribute is deleted into mongo
    Examples:
      | types   |
      | house_0 |
      | house_1 |
      | house_2 |

  @qp_type_empty
  Scenario:  try to delete an attribute by entity ID using NGSI v2 with "type" query parameter with empty value
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
    When delete an attribute "temperature" in the entity with id "room"
      | parameter | value |
      | type      |       |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |

  @qp_unknown @BUG_1831 @skip
  Scenario Outline:  try to delete an attribute by entity ID using NGSI v2 with unknown query parameter
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
    When delete an attribute "temperature" in the entity with id "room"
      | parameter   | value   |
      | <parameter> | house_1 |
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
  Scenario Outline:  try to delete an attribute by entity ID using NGSI v2 with "type" query parameter with invalid value
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
    When delete an attribute "temperature" in the entity with id "room"
      | parameter   | value   |
      | <parameter> | house_1 |
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
