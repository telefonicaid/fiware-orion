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


Feature: update attributes by entity ID if it exists using NGSI v2. "PATCH" - /v2/entities/<entity_id> plus payload
  As a context broker user
  I would like to update attributes by entity ID if it exists using NGSI v2
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
  Scenario:  update attributes by entity ID if it exists using NGSI v2
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
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @entity_not_exists @BUG_1260
  Scenario:  try to update attributes by entity ID but it does not exists using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_not_exists |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
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
    When update attributes by ID "speed" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @more_entities_update @BUG_1260
  Scenario:  try to update attributes by entity ID using NGSI v2 with more than one entity with the same id
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
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @without_payload @BUG_1257
  Scenario:  try to update any attribute by entity ID using NGSI v2 without payload
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_without_payload |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # update properties are not defined, empty payload
    And properties to entities
      | parameter          |
      | without_properties |
    When update attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | LengthRequired                                   |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  @maximum_size
   # 4708 attributes is a way of generating a request longer than 1MB (in fact, 1048774 bytes)
  Scenario:  try to update attributes using NGSI v2 with maximum size in payload (4708 attributes = 1048774 bytes)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | my_type     |
      | metadatas_number | 1           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | 1234567890  |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
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
    When update attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048774, max size supported: 1048576 |

   # ------------------------ Service ----------------------------------------------
  @service_update.row<row.id>
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
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that entities are stored in mongo
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
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that entities are stored in mongo

  @service_update_error @BUG_1873
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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
  Scenario:  try to update attributes by entity ID using NGSI v2 with bad length service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ------------------------ Service path ----------------------------------------------

  @service_path_update.row<row.id>
  @service_path_update @BUG_1423
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_update_service_path |
      | Fiware-ServicePath | <service_path>           |
      | Content-Type       | application/json         |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house_3     |
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
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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
  Scenario:  update attributes by entity ID using NGSI v2 without service header
    Given  a definition of headers
      | parameter      | value                    |
      | Fiware-Service | test_update_service_path |
      | Content-Type   | application/json         |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house_3     |
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
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_update_error @BUG_1280
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
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

  @service_path_update_error @BUG_1280
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_update_error @BUG_1280
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | application/json               |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_update_error @BUG_1280
  Scenario:  try to update attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_service_path_error       |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #  -------------------------- entity id --------------------------------------------------

  @entity_id_update.row<row.id>
  @entity_id_update
  Scenario Outline:  update attributes by entity ID using NGSI v2 with several entity id values
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
      | parameter        | value         |
      | attributes_name  | temperature_1 |
      | attributes_value | 80            |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
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
      | room_14     | random=10  |
      | room_15     | random=100 |
      | room_16     | random=256 |

  @entity_id_no_exists @BUG_1260
  Scenario Outline:  try to update attribute that doesn't previously exists in the entity using NGSI v2 API with several entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | <entity_id> |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_name  | speed |
      | attributes_value | 80    |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |
    Examples:
      | entity_id |
      | room_0    |
      | room_1    |
      | room_2    |

  @entity_id_unknown @BUG_1260
  Scenario:  try to update attributes by entity ID using NGSI v2 with unknown entity id values
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_update_entity_id_unknown |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room3       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When update attributes by ID "utyuty" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @entity_id_update_invalid.row<row.id>
  @entity_id_update_invalid @BUG_1280
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
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

  @entity_id_update_invalid @BUG_1280 @BUG_1782
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
   # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When update attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |
    Examples:
      | entity_id |
      | house_?   |
      | house_/   |
      | house_#   |

  @entity_id_update_empty @BUG_1783 @skip
  Scenario:  try to update attributes by entity ID using NGSI v2 with empty entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When update attributes by ID "" if it exists and with "normalized" mode
    Then verify that receive an "Method not allowed" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |

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
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value                                  |
      | attributes_name  | the same value of the previous request |
      | attributes_value | 80                                     |
    When update attributes by ID "room" if it exists and with "normalized" mode
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

  @attribute_name_invalid
  Scenario Outline:  try to update attribute that doesn't previously exists in the entity using NGSI v2 API with invalid attribute names
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | <entity_id> |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | <attributes_name> |
      | attributes_value | 80                |
    When update attributes by ID "room" if it exists and with "normalized" mode
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
      | room_5    | house'flat'     |
      | room_4    | house;flat      |
      | room_5    | house(flat)     |
      | room_6    | house_?         |
      | room_7    | house_&         |
      | room_8    | house_/         |
      | room_9    | house_#         |
      | room_10   | my house        |

  @attribute_name_update_error
  Scenario Outline:  try to update attributes by entity ID using NGSI v2 with invalid attribute names
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
    When update attributes by ID "room" if it exists in raw and "normalized" modes
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
  Scenario:  try to update attribute that doesn't previously exists in the entity using NGSI v2 API with empty attribute name
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
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_name  |       |
      | attributes_value | 80    |
    When update attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                          |
      | error       | BadRequest                     |
      | description | no 'name' for ContextAttribute |

  @append_attributes_without_attr_type @BUG_1784 @skip
  Scenario:  try to append new attributes by entity ID using NGSI v2 without attribute type nor metadata in update request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_new_attrs |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @append_attributes_with_attr_type @BUG_1784 @skip
  Scenario:  try to append new attributes by entity ID using NGSI v2 with attribute type but without metadata in update request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_new_attrs |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
      | attributes_type   | celsius     |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @append_attributes_with_metatadata @BUG_1784 @skip
  Scenario:  try to append new attributes by entity ID using NGSI v2 with metadata in update request
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_new_attrs |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When update attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |
