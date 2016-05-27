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


Feature: update or append an attribute by entity ID using NGSI v2. "POST" - /v2/entities/<entity_id> plus payload
  As a context broker user
  I would like to update or append an attribute by entity ID
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
  Scenario:  update and append attributes by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
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
      | parameter         | value       |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @more_entities_update @BUG_1198
  Scenario:  try to update an attribute by entity ID using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_update_more_entities |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
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
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
    When update or append attributes by ID "room" and with "normalized" mode
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @more_entities_append @BUG_1198
  Scenario:  try to append an attribute by entity ID using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_update_more_entities |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "room" and with "normalized" mode
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @length_required @BUG_1199 @BUG_1203
  Scenario:  try to update or append an attribute by entity ID using NGSI v2 without payload
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_length_required |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in update request
    And properties to entities
      | parameter          |
      | without_properties |
    When update or append attributes by ID "room" and with "normalized" mode
    Then verify that receive an "Content Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | ContentLengthRequired                            |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  @maximum_size @BUG_1199
    # 4708 attributes is a way of generating a request longer than 1MB (in fact, 1048774 bytes)
  Scenario:  try to update or append attributes using NGSI v2 with maximum size in payload (4708 attributes = 1048774 bytes)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
   # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 4708        |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update or append attributes by ID "room" and with "normalized" mode
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048774, max size supported: 1048576 |

  # ------------------------ Service ----------------------------------------------

  @service_update
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several service header values
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
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "room_1" and with "normalized" mode
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

  @service_append
  Scenario Outline:  append new attributes by entity ID using NGSI v2 with several service header values
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
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
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
  Scenario:  update attributes by entity ID using NGSI v2 without service header
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
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_append_without
  Scenario:  append attributes by entity ID using NGSI v2 without service header
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
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_update_append_error @BUG_1873
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
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

  @service_update_append_bad_length
  Scenario:  try to update or append attributes by entity ID using NGSI v2 with bad length service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
   # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ------------------------ Service path ----------------------------------------------

  @service_path_update @BUG_1423
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several service path header values
    Given  a definition of headers
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
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "room_1" and with "normalized" mode
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

  @service_path_append @BUG_1423
  Scenario Outline:  append attributes by entity ID using NGSI v2 with several service path header values
    Given  a definition of headers
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
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
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
  Scenario:  update attributes by entity ID using NGSI v2 without service path header
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
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_append_without
  Scenario:  append attributes by entity ID using NGSI v2 without service path header
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
      | parameter         | value    |
      | attributes_number | 3        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_update_append_illegal
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with illegal service path header values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
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

  @service_path_update_append_not_allowed
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with not allowed service path header values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_update_append_too_long
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with too long service path header values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_update_many_components
  Scenario:  try to update or append attributes by entity ID using NGSI v2 with too many service path header values
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_service_path             |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #  -------------------------- entity id --------------------------------------------------

  @entity_id_update
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
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
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id  |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @entity_id_update_not_ascii_plain
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update or append attributes by ID "<entity_id>" and with "normalized" mode
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

  @entity_id_append
  Scenario Outline:  append attributes by entity ID using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | entity_id  |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @entity_id_unknown @BUG_1206 @BUG_1817
  Scenario:  try to append entity and attributes by entity ID using NGSI v2 with unknown entity
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_update_entity_id_uknown |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "trretre" and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                 |
      | error       | NotFound              |
      | description | Entity does not exist |

  @entity_id_update_invalid
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "<entity_id>" and with "normalized" mode
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

  @entity_id_update_invalid @BUG_1782 @BUG_1817
  Scenario:  try to update or append attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "house_?" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | Empty right-hand-side for URI param //attrs/ |

  @entity_id_update_invalid @BUG_1782 @BUG_1817
  Scenario:  try to update or append attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "house_/" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

  @entity_id_update_invalid @BUG_1782 @BUG_1817 @ISSUE_2075 @skip
  Scenario:  try to update or append attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When update or append attributes by ID "house_#" and with "normalized" mode
    Then verify that receive an "Method not allowed" http code
    And verify an error response
      | parameter   | value            |
      | error       | MethodNotAllowed |
      | description | No defined yet   |

  # ----------------------- attributes ---------------------------------------

  @attribute_multiples
  Scenario Outline:  append multiples attributes by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value              |
      | attributes_number | <attribute_number> |
      | attributes_name   | himidity           |
      | attributes_value  | 80                 |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attribute_number |
      | 1                |
      | 5                |
      | 10               |
      | 50               |
      | 100              |
      | 500              |

  # --------------------- attribute name  ------------------------------------

  @attribute_name_update
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When update or append attributes by ID "room_1" and with "normalized" mode
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
  Scenario Outline:  update attributes by entity ID using NGSI v2 with not ascii plain attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When update or append attributes by ID "room_1" and with "normalized" mode
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

  @attribute_name_update_exceed_max_length
  Scenario:  update attributes by entity ID using NGSI v2 with attribute names exceed max length allowed (256)
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_name  | random=257 |
      | attributes_value | 80         |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute name length: 257, max length supported: 256 |

  @attribute_name_append
  Scenario Outline:  append attributes by entity ID using NGSI v2 with several attribute names
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
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_name |
      | humidity        |
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

  @attribute_name_update_append_invalid @BUG_1200
  Scenario Outline:  try to append attributes by entity ID using NGSI v2 with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When update or append attributes by ID "<entity_id>" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |
    Examples:
      | entity_id | attributes_name |
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

  @attribute_name_update_append_invalid
  Scenario Outline:  try to update or append attributes by entity ID using NGSI v2 with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | true              |
    When update or append attributes by ID "room" in raw and "normalized" modes
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

  @attribute_name_update_append_empty
  Scenario:  try to append attributes by entity ID using NGSI v2 with empty attribute name
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_name  |       |
      | attributes_value | 80    |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                          |
      | error       | BadRequest                     |
      | description | no 'name' for ContextAttribute |

   # --------------------- attribute type  ------------------------------------

  @attribute_type_update @BUG_1212
  Scenario Outline:  update an attribute by entity ID using NGSI v2 with several attributes type
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
      | parameter        | value             |
      | attributes_name  | temperature       |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | werwerwe        |
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

  @attribute_type_update_not_ascii_plain
  Scenario Outline:  try to update an attribute by entity ID using NGSI v2 with not ascii plain attributes type
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | temperature       |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    Examples:
      | attributes_type |
      | habitación      |
      | españa          |
      | barça           |

  @attribute_type_update_exceed_max_length
  Scenario:  try to update an attribute by entity ID using NGSI v2 with attributes type exceed max length allowed (256)
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_update |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_type  | random=257  |
      | attributes_value | 789         |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute type length: 257, max length supported: 256 |

  @attribute_type_append @BUG_1212
  Scenario Outline:  append an attribute by entity ID using NGSI v2 with several attributes type
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | attribute_type_append |
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
      | parameter        | value             |
      | attributes_name  | humidity          |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_type |
      | dfgfdgd         |
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

  @attribute_type_update_forbidden @BUG_1212
  Scenario Outline:  try to update or append an attribute by entity ID using NGSI v2 with forbidden attributes type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | humidity          |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "<entity_id>" and with "normalized" mode
    Then verify that receive a "Bad Request" http code
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

  @attribute_type_update_wrong
  Scenario Outline:  try to update or append an attribute by entity ID using NGSI v2 with wrong attributes type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | "humidity"        |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "<entity_id>" in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_type |
      | room1     | rewrewr         |
      | room2     | SDFSDFSDF       |

  @attribute_type_update_error @BUG_1212
  Scenario Outline:  try to update or append an attribute by entity ID using NGSI v2 with invalid attributes type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | "humidity"        |
      | attributes_type  | <attributes_type> |
      | attributes_value | 789               |
    When update or append attributes by ID "<entity_id>" in raw and "normalized" modes
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
