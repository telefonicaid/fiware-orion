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

Feature: delete an entity request using NGSI v2 API. "DELETE" - /v2/entities/
  As a context broker user
  I would like to delete entities requests using NGSI v2 API
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
  Scenario: Delete several entities using NGSI v2 API
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And create "3" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | attributes_type  | date        |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When delete entities with id "room"
      | parameter       | value                                  |
      | entities_number | the same value of the previous request |
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo

  # ------------------------ Service ----------------------------------------------

  @service_delete
  Scenario Outline: Delete entities by ID using NGSI v2 with several service header values
    Given a definition of headers
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
    When delete entities with id "room"
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_delete_without
  Scenario: Delete entities by ID using NGSI v2 without service header
    Given a definition of headers
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
    When delete entities with id "room"
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo

  @service_delete_error
  Scenario Outline: Try to delete entities by ID using NGSI v2 with wrong service header values
    Given a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    When delete entities with id "room"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
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

  @service_path_delete @BUG_1423 @skip
  Scenario Outline: Delete entities by ID using NGSI v2 with several service path header values
    Given a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_delete_service_path |
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
    When delete entities with id "room"
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo
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
  Scenario: Delete entities by ID using NGSI v2 without service path header
    Given a definition of headers
      | parameter      | value                    |
      | Fiware-Service | test_delete_service_path |
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
    When delete entities with id "room"
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo

  @service_path_delete_error
  Scenario Outline: Try to delete entities by ID using NGSI v2 with wrong service header path values
    Given a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_delete_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    When delete entities with id "room"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
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
  Scenario Outline: Try to delete entities by ID using NGSI v2 with wrong service path header values
    Given a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_delete_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    When delete entities with id "room"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_delete_error
  Scenario Outline: Try to delete entities by ID using NGSI v2 with wrong service path header values
    Given a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_replace_service_path_error |
      | Fiware-ServicePath | <service_path>                  |
      | Content-Type       | application/json                |
    When delete entities with id "room"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_delete_error
  Scenario: Try to delete entities by ID using NGSI v2 with wrong service path header values
    Given a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_service_path_error      |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    When delete entities with id "room"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

 #  -------------------------- entity id --------------------------------------------------

  @entity_id_delete
  Scenario Outline: Delete entities by ID using NGSI v2 with several entity id values
    Given a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_delete_entity_id |
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
    When delete entities with id "the same value of the previous request"
    Then verify that receive several "No Content" http code
    And verify that entities are not stored in mongo
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

  @entity_not_exists
  Scenario: Try to delete an entity by ID but it does not exists using NGSI v2
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And create "1" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | attributes_type  | date        |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When delete entities with id "dsfsdfds"
    Then verify that receive several "Not Found" http code
    And verify several error responses
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @more_entities_delete @BUG_1346 @skip
  Scenario: Try to delete an entity by ID using NGSI v2 with more than one entity with the same id
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_delete_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
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
    When delete entities with id "room"
    Then verify that receive several "Conflict" http code
    And verify several error responses
      | parameter   | value                                                                          |
      | error       | TooManyResults                                                                 |
      | description | There is more than one entity that match the delete. Please refine your query. |

  @entity_id_delete_invalid
  Scenario Outline: Try to delete entity by ID using NGSI v2 with invalid entity id values
    Given a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_replace_entity_id |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    When delete entities with id "<entity_id>"
    Then verify that receive several "Bad Request" http code
    And verify several error responses
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
