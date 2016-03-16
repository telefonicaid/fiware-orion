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


Feature: update an attribute value by entity ID and attribute name if it exists using NGSI v2 API. "PUT" - /v2/entities/<entity_id>/attrs/<attr_name>/value plus payload
  queries parameters:
  tested:type
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @without_payload
  Scenario:  try to update an attribute value by entity ID and attribute name if it exists using NGSI v2 but without payload
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
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value          |
      | attributes_value | random=1048575 |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048577, max size supported: 1048576 |

  # ------------------------ content-Type header ----------------------------------------------

  @content_types
  Scenario Outline:  update an attribute value by entity ID and attribute name if it exists using NGSI v2 with several content-type headers
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value          |
      | Content-Type | <content_type> |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_value | <attribute_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | content_type     | attribute_value         |
      | text/plain       | 80                      |
      | text/plain       | "dfsdf"                 |
      | text/plain       | true                    |
      | text/plain       | null                    |
      | application/json | ["45", "fdgdg", "677"]  |
      | application/json | {"a":"45", "b":"fdgdg"} |

  @content_types_wrong
  Scenario Outline:  try to update an attribute value by entity ID and attribute name if it exists using NGSI v2 with wrong content-type headers
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value          |
      | Content-Type | <content_type> |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | "80"  |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | UnsupportedMediaType                       |
      | description | not supported content type: <content_type> |
    Examples:
      | content_type                      |
      | application/x-www-form-urlencoded |
      | application/xml                   |
      | multipart/form-data               |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  # ------------------------ Service header ----------------------------------------------

  @service_update
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with several service header values
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_update_error
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value      |
      | Fiware-Service     | <service>  |
      | Fiware-ServicePath | /test      |
      | Content-Type       | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
  Scenario:  try to update attributes value by entity ID and attribute name using NGSI v2 with bad length service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | text/plain                      |
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

 # ------------------------ Service path header ----------------------------------------------

  @service_path_update @BUG_1423
  Scenario Outline:  update attributes value by entity ID and attribute nameusing NGSI v2 with several service path header values
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
  Scenario:  update attributes value by entity ID and attribute name using NGSI v2 without service path header
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_update_wrong
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service path header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | text/plain                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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

  @service_path_update_wrong
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with wrong service path header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | text/plain                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
  Scenario Outline:  try to update attributes value by entity ID and attribute name using NGSI v2 with component-name too long in service path header values
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_update_service_path_error |
      | Fiware-ServicePath | <service_path>                 |
      | Content-Type       | text/plain                     |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
  Scenario:  try to update attributes value by entity ID and attribute name using NGSI v2 with too many components in service path header value
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_service_path_error       |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | text/plain                           |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature_0" if it exists
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
  Scenario Outline:  update attributes value by entity ID and attribute name using NGSI v2 with not ascii plain entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | text/plain            |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
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

  @entity_not_exists @BUG_1360 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name but the entitiy id does not exist using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_not_exists |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | text/plain             |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "dfgfdg" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @more_entities_update @BUG_1387
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with more than one entity id with the same id
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room" and attribute name "temperature_0" if it exists
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @entity_id_update_invalid @BUG_1351
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | text/plain            |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
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

  @entity_id_update_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | text/plain            |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "house_#" and attribute name "temperature_0" if it exists
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | UnsupportedMediaType                   |
      | description | not supported content type: text/plain |

  @entity_id_update_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | text/plain            |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "house_?" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | Empty right-hand-side for URI param //attrs/temperature_0/value/ |

  @entity_id_update_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with invalid entity id values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | text/plain            |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "house_/" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | unrecognized request |

  @entity_id_empty @ISSUE_1487 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 API with empty entity_id
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attribute_name_update_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | text/plain                       |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "" and attribute name "temperature_0" if it exists
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "the same value of the previous request" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_name |
      | room            |
      | 34              |
      | false           |
      | true            |
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

  @attribute_name_update_not_ascii_plain
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with not ascii plain attribute names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attribute_name_update |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | text/plain                 |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "<attributes_name>" if it exists
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

  @attribute_name_not_exists @BUG_1360 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 but the attribute name does not exist
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "fdgdfgdfg" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | invalid character in URI |

  @attribute_name_invalid @BUG_1351
  Scenario Outline:  try to update an attribute value by entity id and attribute name using NGSI v2 API with invalid attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "<attributes_name>" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      # \' is replaced in code by ". Limitation of Behave. See case 3.
      | attributes_name     |
      | house<flat>         |
      | house=flat          |
      | house\'flat\'       |
      | house'flat'         |
      | house;flat          |
      | house(flat)         |
      | house_&             |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |
      | my house            |

  @attribute_name_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity id and attribute name using NGSI v2 API with invalid attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "house_?" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | Empty right-hand-side for URI param //value/ |

  @attribute_name_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity id and attribute name using NGSI v2 API with invalid attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "house_/" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | unrecognized request |

  @attribute_name_invalid @BUG_1351
  Scenario:  try to update an attribute value by entity id and attribute name using NGSI v2 API with invalid attribute names
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "house_#" if it exists
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | UnsupportedMediaType                   |
      | description | not supported content type: text/plain |

  @attribute_name_empty @ISSUE_1487 @skip
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 API with empty attribute name
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room_1" and attribute name "" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |
