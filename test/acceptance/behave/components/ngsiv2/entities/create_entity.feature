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

# Missing Tests:
#   - verification of Maximum Length Allowed in Fields
#   - verification of Special Attribute Types
#


Feature: create entities requests (POST) using NGSI v2. "POST" - /v2/entities/ plus payload and queries parameters
  As a context broker user
  I would like to  create entities requests using NGSI v2
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
  Scenario:  create several entities using NGSI v2
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_happy_path  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter         | value                   |
      | entities_type     | room                    |
      | entities_id       | room2                   |
      | attributes_number | 2                       |
      | attributes_name   | random=5                |
      | attributes_value  | 017-06-17T07:21:24.238Z |
      | attributes_type   | date                    |
      | metadatas_number  | 2                       |
      | metadatas_name    | very_hot                |
      | metadatas_type    | alarm                   |
      | metadatas_value   | hot                     |
    When create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  @already_exists_1 @BUG_1158
  Scenario:  try  to create an entity using NGSI v2 but this entity already exists with only entity id
    Given  a definition of headers
      | parameter      | value               |
      | Fiware-Service | test_already_exists |
      | Content-Type   | application/json    |
    And properties to entities
      | parameter        | value       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And properties to entities
      | parameter        | value       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unprocessable Entity" http code
    And verify several error responses
      | parameter   | value                 |
      | error       | InvalidModification   |
      | description | Entity already exists |

  @already_exists_2 @BUG_1158
  Scenario:  try  to create an entity using NGSI v2 but this entity already exists with entity id and type
    Given  a definition of headers
      | parameter      | value               |
      | Fiware-Service | test_already_exists |
      | Content-Type   | application/json    |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unprocessable Entity" http code
    And verify several error responses
      | parameter   | value                 |
      | error       | InvalidModification   |
      | description | Entity already exists |

  @already_exists_3 @BUG_1158
  Scenario:  try  to create an entity using NGSI v2 but this entity already exists with entity id and type and service_path
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_already_exists |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unprocessable Entity" http code
    And verify several error responses
      | parameter   | value                 |
      | error       | InvalidModification   |
      | description | Entity already exists |

  @maximum_size @BUG_1199
    # 8013 is a way of generating a request longer than 1MB (in fact, 1048624 bytes)
  Scenario:  try to create a new entity NGSI v2 with maximum size in payload (8013 attributes = 1048624 bytes)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And properties to entities
      | parameter         | value      |
      | entities_type     | room       |
      | entities_id       | room2      |
      | attributes_number | 8013       |
      | attributes_name   | max_size   |
      | attributes_value  | temperatur |
      | attributes_type   | my_type    |
      | metadatas_number  | 1          |
      | metadatas_name    | very_hot   |
      | metadatas_type    | alarm      |
      | metadatas_value   | 1234567890 |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Request Entity Too Large" http code
    And verify several error responses
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048624, max size supported: 1048576 |
    And verify that entities are not stored in mongo

  @content_type_without @BUG_1199
  Scenario:  try to create entities using NGSI v2 without content-type header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unsupported Media Type" http code
    And verify several error responses
      | parameter   | value                                                                           |
      | error       | UnsupportedMediaType                                                            |
      | description | Content-Type header not used, default application/octet-stream is not supported |

  @content_type_error @BUG_1199
  Scenario Outline:  try to create entities using NGSI v2 with wrong content-type header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | <content_type>    |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Unsupported Media Type" http code
    And verify several error responses
      | parameter   | value                                      |
      | error       | UnsupportedMediaType                       |
      | description | not supported content type: <content_type> |
    Examples:
      | content_type                      |
      | application/x-www-form-urlencoded |
      | application/xml                   |
      | multipart/form-data               |
      | text/plain                        |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  @length_required @BUG_1199 @BUG_1203
  Scenario:  try to create several entities using NGSI v2 wihout payload
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_length_required |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Length Required" http code
    And verify several error responses
      | parameter   | value                                            |
      | error       | LengthRequired                                   |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  # ---------- Services --------------------------------

  @service_without
  Scenario: create entities using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in default tenant at mongo

  @service
  Scenario Outline:  create entities using NGSI v2 with several services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    And delete database in mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_error @BUG_1087
  Scenario Outline:  try to create entities using NGSI v2 with several wrong services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                                                  |
      | error       | BadRequest                                                                             |
      | description | bad character in tenant name - only underscore and alphanumeric characters are allowed |
    And verify that entities are not stored in mongo
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

  @service_bad_length @BUG_1087
  Scenario:  try to create entities using NGSI v2 with several bad length in services headers
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |
    And verify that entities are not stored in mongo

  # ---------- Services path --------------------------------

  @service_path
  Scenario Outline:  create entities using NGSI v2 with several service paths headers
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath | <service_path>    |
      | Content-Type       | application/json  |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
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

  @service_path_without
  Scenario:  create entities using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                     |
      | Fiware-Service | test_service_path_without |
      | Content-Type   | application/json          |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  @service_path_error @BUG_1092
  Scenario Outline:  try to create entities using NGSI v2 with several wrong service paths headers
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | a component of ServicePath contains an illegal character |
    And verify that entities are not stored in mongo
    Examples:
      | service_path |
      | /service.sr  |
      | /service;sr  |
      | /service=sr  |
      | /Service-sr  |
      | /serv<45>    |
      | /serv(45)    |

  @service_path_error @BUG_1092
  Scenario Outline:  try to create entities using NGSI v2 with several wrong service paths headers
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    And verify that entities are not stored in mongo
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_error @BUG_1092
  Scenario Outline:  try to create entities using NGSI v2 with several wrong service paths headers
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_service_path_error |
      | Fiware-ServicePath | <service_path>          |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    And verify that entities are not stored in mongo
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_error @BUG_1092
  Scenario:  try to create entities using NGSI v2 with several wrong service paths headers
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_service_path_error              |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |
    And verify that entities are not stored in mongo

  # ---------- entities and attributes number --------------------------------

  @entities_number
  Scenario Outline:  create several entities using NGSI v2
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities    |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "<entities>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | entities |
      | 1        |
      | 10       |
      | 50       |
      | 100      |

  @attributes_number
  Scenario Outline:  create entities using NGSI v2 with several attributes
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_attributes  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter         | value        |
      | entities_type     | house        |
      | entities_id       | room2        |
      | attributes_number | <attributes> |
      | attributes_name   | temperature  |
      | attributes_value  | 34           |
    When create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes |
      | 1          |
      | 10         |
      | 50         |
      | 100        |

  @metadatas_number
  Scenario Outline:  create entities using NGSI v2 with several attributes metadatas
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_attributes  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter         | value        |
      | entities_type     | house        |
      | entities_id       | room2        |
      | attributes_number | <attributes> |
      | attributes_name   | temperature  |
      | attributes_value  | 34           |
      | metadatas_number  | <metadatas>  |
      | metadatas_name    | very_hot     |
      | metadatas_type    | alarm        |
      | metadatas_value   | hot          |
    When create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes | metadatas |
      | 1          | 1         |
      | 10         | 10        |
      | 50         | 50        |
      | 100        | 100       |
      | 10         | 1000      |
      | 1000       | 10        |

  @entity_without_aattribute
  Scenario:  create entities using NGSI v2 without any attributes
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_attributes_error |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And properties to entities
      | parameter     | value |
      | entities_type | house |
      | entities_id   | room2 |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  # ---------- entity type "type" --------------------------------

  @entities_type.row<row.id>
  @entities_type
  Scenario Outline:  create entities using NGSI v2 with several entities type values
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | entities_id | entities_type |
      | room_1      | room          |
      | room_2      | 34            |
      | room_3      | false         |
      | room_4      | true          |
      | room_5      | 34.4E-34      |
      | room_6      | temp.34       |
      | room_7      | temp_34       |
      | room_8      | temp-34       |
      | room_9      | TEMP34        |
      | room_10     | house_flat    |
      | room_11     | house.flat    |
      | room_12     | house-flat    |
      | room_13     | house@flat    |
      | room_14     | random=10     |
      | room_15     | random=100    |
      | room_16     | random=256    |

  @entities_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several entities type values with not plain ascii
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_entities_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | Invalid characters in entity type |
    Examples:
      | entities_id | entities_type |
      | room_17     | habitación    |
      | room_18     | españa        |
      | room_19     | barça         |

  @entities_type_length_exceed @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with entity type length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_entities_type_key_limit |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value       |
      | entities_type    | random=257  |
      | entities_id      | room_1      |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                              |
      | error       | BadRequest                                         |
      | description | entity type length: 257, max length supported: 256 |

  @entities_type_without
  Scenario:  create entities using NGSI v2 without entities type values
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_type_without |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value       |
      | entities_id      | room_2      |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  @entities_type_error.row<row.id>
  @entities_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong entities type values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | Invalid characters in entity type |
    And verify that entities are not stored in mongo
    Examples:
      | entities_type |
      | house<flat>   |
      | house=flat    |
      | house"flat"   |
      | house'flat'   |
      | house;flat    |
      | house(flat)   |
      | house_?       |
      | house_&       |
      | house_/       |
      | house_#       |
      | my house      |

  @entities_type_wrong_raw @BUG_1108
  Scenario Outline:  try to create an entity using NGSI v2 with wrong json in entities_type
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_type_error_2 |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value         |
      | entities_type    | <entity_type> |
      | entities_id      | "room"        |
      | attributes_name  | "temperature" |
      | attributes_value | true          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_type |
      | rewrewr     |
      | SDFSDFSDF   |

  @entities_type_invalid_raw @BUG_1108
  Scenario Outline:  try to create an entity using NGSI v2 with invalid entities_type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_type_error_2 |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value         |
      | entities_type    | <entity_type> |
      | entities_id      | "room"        |
      | attributes_name  | "temperature" |
      | attributes_value | true          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | invalid JSON type for entity type |
    Examples:
      | entity_type     |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

   # ---------- entity id "id" --------------------------------

  @entities_id
  Scenario Outline:  create entities using NGSI v2 with several entities id values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | entities_type | entities_id |
      | room_1        | room        |
      | room_2        | 34          |
      | room_3        | false       |
      | room_4        | true        |
      | room_5        | 34.4E-34    |
      | room_6        | temp.34     |
      | room_7        | temp_34     |
      | room_8        | temp-34     |
      | room_9        | TEMP34      |
      | room_10       | house_flat  |
      | room_11       | house.flat  |
      | room_12       | house-flat  |
      | room_13       | house@flat  |
      | room_17       | random=10   |
      | room_18       | random=100  |
      | room_19       | random=256  |

  @entities_id_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several entities id values with not plain ascii
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_entities_id |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |

    Examples:
      | entities_type | entities_id |
      | room_14       | habitación  |
      | room_15       | españa      |
      | room_16       | barça       |

  @entities_id_length_exceed @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with with entity id length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_id_key_limit |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | random=257  |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                            |
      | error       | BadRequest                                       |
      | description | entity id length: 257, max length supported: 256 |

  @entities_id_without
  Scenario:  try to create entities using NGSI v2 without entities id values
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_entities_id_without |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | no entity id specified |
    And verify that entities are not stored in mongo

  @entities_id_wrong @BUG_1093 @BUG_1200 @BUG_1351  @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong entities id values
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_entities_id_error |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And properties to entities
      | parameter        | value         |
      | entities_type    | room          |
      | entities_id      | <entities_id> |
      | attributes_name  | temperature   |
      | attributes_value | 34            |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_&     |
      | house_/     |
      | house_#     |
      | my house    |

  @entities_id_invalid_raw @BUG_1108
  Scenario Outline:  try to create an entity using NGSI v2 with several invalid entities id without attribute type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_type_error_2 |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | <entity_id>   |
      | attributes_name  | "temperature" |
      | attributes_value | true          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id |
      | rewrewr   |
      | SDFSDFSDF |

  @entities_id_invalid_raw_2 @BUG_1108
  Scenario Outline:  try to create an entity using NGSI v2 with several not allowed entities id without attribute type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_entities_type_error_2 |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | <entity_id>   |
      | attributes_name  | "temperature" |
      | attributes_value | true          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | invalid JSON type for entity id |
    Examples:
      | entity_id       |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

  # ---------- attribute name --------------------------------

  @attributes_name @ISSUE_1090
  Scenario Outline:  create entities using NGSI v2 with several attributes names
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_name |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | <attributes_name> |
      | attributes_value | 34                |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_name |
      | temperature     |
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

  @attributes_name_not_allowed @ISSUE_1090
  Scenario Outline:  try to create entities using NGSI v2 with several attributes names with not plain ascii
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_name |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | <attributes_name> |
      | attributes_value | 34                |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |
    Examples:
      | attributes_name |
      | habitación      |
      | españa          |
      | barça           |

  @attributes_name_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes name that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value      |
      | entities_type    | house      |
      | entities_id      | room       |
      | attributes_name  | random=257 |
      | attributes_value | 56         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute name length: 257, max length supported: 256 |

  @attributes_name_error @BUG1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attributes_name_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | <entities_id>     |
      | attributes_name  | <attributes_name> |
      | attributes_value | 34                |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute name |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | attributes_name |
      | room_1      | house<flat>     |
      | room_2      | house=flat      |
      | room_3      | house"flat"     |
      | room_4      | house'flat'     |
      | room_5      | house;flat      |
      | room_6      | house(flat)     |
      | room_7      | house_?         |
      | room_8      | house_&         |
      | room_9      | house_/         |
      | room_10     | house_#         |
      | room_11     | my house        |

  @attributes_name_no_string_error
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes name (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | <attributes_name> |
      | attributes_value | true              |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_name |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |
      | "room8"   | null            |

  @attributes_name_without
  Scenario:  create entities using NGSI v2 without attributes names
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attributes_name_without |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter     | value |
      | entities_type | house |
      | entities_id   | room  |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  @attributes_name_empty
  Scenario:  try to create entities using NGSI v2 with empty attributes names
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attributes_name_empty |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value |
      | entities_type    | house |
      | entities_id      | room  |
      | attributes_name  |       |
      | attributes_value | 34    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                          |
      | error       | BadRequest                     |
      | description | no 'name' for ContextAttribute |
    And verify that entities are not stored in mongo

  # ---------- attribute value --------------------------------

  @attributes_value
  Scenario Outline:  create entities using NGSI v2 with several attributes values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_attributes_value |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And properties to entities
      | parameter        | value              |
      | entities_type    | house              |
      | entities_id      | room               |
      | attributes_name  | temperature        |
      | attributes_value | <attributes_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_value |
      | fsdfsd           |
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
      | random=100000    |
      | random=500000    |
      | random=1000000   |

  @attributes_value_wrong @BUG_1093 @BUG_1200
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_value_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value              |
      | entities_type    | house              |
      | entities_id      | <entities_id>      |
      | attributes_name  | temperature        |
      | attributes_value | <attributes_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid characters in attribute value |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | attributes_value |
      | room_1      | house<flat>      |
      | room_2      | house=flat       |
      | room_3      | house"flat"      |
      | room_4      | house'flat'      |
      | room_5      | house;flat       |
      | room_6      | house(flat)      |

  @attributes_value_without
  Scenario:  try to create entities using NGSI v2 without attributes values
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_attributes_value_without |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter       | value       |
      | entities_type   | house       |
      | entities_id     | room        |
      | attributes_name | temperature |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |
    And verify that entities are not stored in mongo

  @attributes_value_without_with_type @BUG_1195
  Scenario:  try to create entities using NGSI v2 without attributes values but with attribute type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_attributes_value_without |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter       | value       |
      | entities_type   | house       |
      | entities_id     | room        |
      | attributes_name | temperature |
      | attributes_type | date        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |
    And verify that entities are not stored in mongo

  @attributes_value_without_with_metadata @BUG_1195
  Scenario:  try to create entities using NGSI v2 without attributes values but with metadata
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_attributes_value_without |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |
    And verify that entities are not stored in mongo

  @attributes_value_special @BUG_1106
  Scenario Outline:  create an entity using NGSI v2 with several attributes special values without type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attributes_special |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attributes_value                                                              |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |

  @attributes_value_special_type @BUG_1106
  Scenario Outline:  create an entity using NGSI v2 with several attributes special values with type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_attributes_special_with_type |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "example"          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attributes_value                                                              |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |

  @attributes_value_special_metadata @BUG_1106
  Scenario Outline:  create an entity using NGSI v2 with several attributes special values with metadata (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_attributes_special_with_meta |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "default"          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attributes_value                                                              |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |

  @attributes_value_special_type_and_metadata @BUG_1106
  Scenario Outline:  create an entity using NGSI v2 with several attributes special values with type and metadata (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                                      |
      | Fiware-Service     | test_attributes_special_with_type_and_meta |
      | Fiware-ServicePath | /test                                      |
      | Content-Type       | application/json                           |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "example"          |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "default"          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attributes_value                                                              |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |

  @attributes_value_wrong_without_type
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes special values without attribute type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_value_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_value |
      | "room_1"  | rwerwer          |
      | "room_2"  | True             |
      | "room_3"  | TRUE             |
      | "room_4"  | False            |
      | "room_5"  | FALSE            |
      | "room_6"  | 34r              |
      | "room_7"  | 5_34             |
      | "room_8"  | ["a", "b"        |
      | "room_9"  | ["a" "b"]        |
      | "room_10" | "a", "b"]        |
      | "room_11" | ["a" "b"}        |
      | "room_12" | {"a": "b"        |
      | "room_13" | {"a" "b"}        |
      | "room_14" | "a": "b"}        |

  @attributes_value_wrong_with_type
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes special values with attribute type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attributes_value_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "example"          |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
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

  @attributes_value_wrong_with_metadata
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes special values with metadata
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attributes_value_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
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

  @attributes_wrong_with_metadata_and_type
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes special values with type and metadata
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attributes_value_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "example"          |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
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

  # ---------- attribute type --------------------------------

  @attributes_type
  Scenario Outline:  create entities using NGSI v2 with several attributes type
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | 56                |
      | attributes_type  | <attributes_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_type |
      | temperature     |
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
      | random=150      |
      | random=256      |

  @attributes_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes type with not plain ascii
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | 56                |
      | attributes_type  | <attributes_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    Examples:
      | attributes_type |
      | habitación      |
      | españa          |
      | barça           |

  @attributes_type_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes type that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | random=257  |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                 |
      | error       | BadRequest                                            |
      | description | attribute type length: 257, max length supported: 256 |

  @attributes_type_without
  Scenario:  create entities using NGSI v2 without attributes type
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attributes_type_without |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo

  @attributes_type_wrong_p @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes types
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_attributes_type_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | <entities_id>     |
      | attributes_name  | temperature       |
      | attributes_value | 56                |
      | attributes_type  | <attributes_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in attribute type |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | attributes_type |
      | room_1      | house<flat>     |
      | room_2      | house=flat      |
      | room_3      | house"flat"     |
      | room_4      | house'flat'     |
      | room_5      | house;flat      |
      | room_6      | house(flat)     |
      | room_7      | house_?         |
      | room_8      | house_&         |
      | room_9      | house_/         |
      | room_10     | house_#         |
      | room_11     | my house        |

  @attributes_type_invalid @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several invalid attributes type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "room"            |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | true              |
      | attributes_type  | <attributes_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | attributes_type |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |

  @attributes_type_no_allowed @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several not allowed attributes type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "room"            |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | true              |
      | attributes_type  | <attributes_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | invalid JSON type for attribute type |
    Examples:
      | entity_id | attributes_type |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

  # ---------- attribute metadata name --------------------------------

  @attributes_metadata_name
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata name without metadata type
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_name |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_name |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
      # In metadata name always is added a suffix in code. Ex: _0
      | random=254    |

  @attributes_metadata_name_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes metadata name without metadata type but with not plain ascii
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_name |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | metadata_name |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_name_with_type
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata name with metadata type
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_metadata_name_with_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | metadatas_type   | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_name |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
       # In metadata name always is added a suffix in code. Ex: _0
      | random=254    |

  @attributes_metadata_name_with_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes metadata name with metadata type and not plain ascii
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_metadata_name_with_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter        | value           |
      | entities_type    | house           |
      | entities_id      | room            |
      | attributes_name  | temperature     |
      | attributes_value | 56              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | metadatas_type   | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    Examples:
      | metadata_name |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_name_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes metadata name that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | celcius     |
      | metadatas_name   | random=255  |
      | metadatas_value  | random=5    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    # the metadata name is incremented in 2 chars because it is added with a suffix. Ex: "_0"
    And verify several error responses
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata name length: 257, max length supported: 256 |

  @attributes_metadata_name_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata name without metadata type
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_metadata_name_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_name |
      | room_1      | house<flat>   |
      | room_2      | house=flat    |
      | room_3      | house"flat"   |
      | room_4      | house'flat'   |
      | room_5      | house;flat    |
      | room_6      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_name_with_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata name with metadata type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_metadata_name_without_type_error |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | random=5        |
      | attributes_type  | random=6        |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata name |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_name |
      | room_1      | house<flat>   |
      | room_2      | house=flat    |
      | room_3      | house"flat"   |
      | room_4      | house'flat'   |
      | room_5      | house;flat    |
      | room_6      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_name_no_string_error
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes metadata name without metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | "my_default"    |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_name   |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

  @attributes_metadata_name_with_type_no_string_error
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong attributes metadata name with metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | <metadata_name> |
      | metadatas_value  | "my_default"    |
      | attributes_type  | "nothing"       |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_name   |
      | "room1"   | rewrewr         |
      | "room2"   | SDFSDFSDF       |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

   # ---------- attribute metadata value --------------------------------

  @attributes_metadata_value
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata value without metadata type
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_metadata_value |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | room2            |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_value |
      | dgdgdfgd       |
      | 34             |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | habitación     |
      | españa         |
      | barça          |
      | random=10      |
      | random=100     |
      | random=1000    |
      | random=10000   |
      | random=50000   |
      | random=100000  |

  @attributes_metadata_value_with_type
  Scenario Outline:  create entities using NGSI v2 with several attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | room2            |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | random=6         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_value |
      | 34             |
      | dgdgdfgd       |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | habitación     |
      | españa         |
      | barça          |
      | random=10      |
      | random=100     |
      | random=1000    |
      | random=10000   |
      | random=50000   |
      | random=100000  |

  @attributes_metadata_value_without @BUG_1200
  Scenario:  try to create entities using NGSI v2 without attributes metadata value nor metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   | random=5    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attributes_metadata_value_without_and_with_type @BUG_1200
  Scenario:  try to create entities using NGSI v2 without attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_metadata_value_with_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   | random=5    |
      | metadatas_type   | random=6    |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                  |
      | error       | BadRequest             |
      | description | missing metadata value |

  @attributes_metadata_value_special.row<row.id>
  @attributes_metadata_value_special @BUG_1106 @BUG_1713
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata special values without metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value             |
      | "room1"   | true                       |
      | "room2"   | false                      |
      | "room3"   | 34                         |
      | "room4"   | -34                        |
      | "room5"   | 5.00002                    |
      | "room6"   | -5.00002                   |
      | "room7"   | "41.3763726, 2.1864475,14" |
      | "room8"   | "2017-06-17T07:21:24.238Z" |
      | "room9"   | null                       |

  @attributes_metadata_value_special_3 @ISSUE_1712 @skip
  # The json values still are not allowed.
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values without metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value                                                                |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_special_4 @BUG_1106 @BUG_1713
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata special values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value             |
      | "room1"   | true                       |
      | "room2"   | false                      |
      | "room3"   | 34                         |
      | "room4"   | -34                        |
      | "room5"   | 5.00002                    |
      | "room6"   | -5.00002                   |
      | "room15"  | "41.3763726, 2.1864475,14" |
      | "room16"  | "2017-06-17T07:21:24.238Z" |

  @attributes_metadata_value_special_5 @BUG_1106 @BUG_1713 @skip
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata special values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value |
      | "room17"  | null           |

  @attributes_metadata_value_special_6 @ISSUE_1712 @skip
   # The json values still are not allowed.
  Scenario Outline:  create an entity using NGSI v2 with several attributes metadata json values with metadata type (null, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_metadata_value_special |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | metadata_value                                                                |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_error @BUG_1093 @BUG_1200
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata value without metadata type
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_metadata_value_error |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | <entities_id>    |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_value |
      | room_2      | house<flat>    |
      | room_3      | house=flat     |
      | room_4      | house"flat"    |
      | room_5      | house'flat'    |
      | room_6      | house;flat     |
      | room_8      | house(flat)    |

  @attributes_metadata_value_error_with_type @BUG_1093 @BUG_1200
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata value with metadata type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_metadata_value_with_type_error |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    And properties to entities
      | parameter        | value            |
      | entities_type    | room             |
      | entities_id      | <entities_id>    |
      | attributes_name  | temperature      |
      | attributes_value | 34               |
      | metadatas_name   | random=5         |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | random=6         |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid characters in metadata value |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_value |
      | room_2      | house<flat>    |
      | room_3      | house=flat     |
      | room_4      | house"flat"    |
      | room_5      | house'flat'    |
      | room_6      | house;flat     |
      | room_8      | house(flat)    |

  @attributes_metadata_value_special_wrong @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong special attributes metadata values without metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | "room_1"  | rwerwer        |
      | "room_2"  | True           |
      | "room_3"  | TRUE           |
      | "room_4"  | False          |
      | "room_5"  | FALSE          |
      | "room_6"  | 34r            |
      | "room_7"  | 5_34           |
      | "room_8"  | ["a", "b"      |
      | "room_9"  | ["a" "b"]      |
      | "room_10" | "a", "b"]      |
      | "room_11" | ["a" "b"}      |
      | "room_12" | {"a": "b"      |
      | "room_13" | {"a" "b"}      |
      | "room_14" | "a": "b"}      |

  @attributes_metadata_value_special_not_allowed @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several compound special attributes metadata values without metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | invalid JSON type for attribute metadata value |
    Examples:
      | entity_id | metadata_value                                                                |
      | "room15"  | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room16"  | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room17"  | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room18"  | {"x": "x1","x2": "b"}                                                         |
      | "room19"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room20"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room21"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room22"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attributes_metadata_value_special_wrong @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several wrong special attributes metadata values with metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_value |
      | "room_1"  | rwerwer        |
      | "room_2"  | True           |
      | "room_3"  | TRUE           |
      | "room_4"  | False          |
      | "room_5"  | FALSE          |
      | "room_6"  | 34r            |
      | "room_7"  | 5_34           |
      | "room_8"  | ["a", "b"      |
      | "room_9"  | ["a" "b"]      |
      | "room_10" | "a", "b"]      |
      | "room_11" | ["a" "b"}      |
      | "room_12" | {"a": "b"      |
      | "room_13" | {"a" "b"}      |
      | "room_14" | "a": "b"}      |

  @attributes_metadata_value_special_not_allowed @BUG_1110
  Scenario Outline:  try to create an entity using NGSI v2 with several compound special attributes metadata values with metadata type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_metadata_value_special_error |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter        | value            |
      | entities_type    | "room"           |
      | entities_id      | <entity_id>      |
      | attributes_name  | "temperature"    |
      | attributes_value | "34"             |
      | metadatas_name   | "alarm"          |
      | metadatas_value  | <metadata_value> |
      | metadatas_type   | "nothing"        |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | invalid JSON type for attribute metadata value |
    Examples:
      | entity_id | metadata_value                                                                |
      | "room15"  | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room16"  | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room17"  | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room18"  | {"x": "x1","x2": "b"}                                                         |
      | "room19"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room20"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room21"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room22"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  # ---------- attribute metadata type --------------------------------

  @attributes_metadata_type
  Scenario Outline:  create entities using NGSI v2 with several attributes with metadata types
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | room2           |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | metadata_type |
      | temperature   |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | house_flat    |
      | house.flat    |
      | house-flat    |
      | house@flat    |
      | random=10     |
      | random=100    |
      | random=150    |
      | random=256    |

  @attributes_metadata_type_not_allowed
  Scenario Outline:  try to create entities using NGSI v2 with several attributes with metadata types and not plain ascii
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_metadata_type |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | room2           |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
   Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    Examples:
      | metadata_type |
      | habitación    |
      | españa        |
      | barça         |

  @attributes_metadata_type_max_length @ISSUE_1601
  Scenario:  try to create entities using NGSI v2 with an attributes metadata type that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_attributes_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
      | attributes_type  | celcius     |
      | metadatas_name   | temperature |
      | metadatas_value  | random=5    |
      | metadatas_type   | random=257  |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                                |
      | error       | BadRequest                                           |
      | description | metadata type length: 257, max length supported: 256 |

  @attributes_metadata_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728 @skip
  Scenario Outline:  try to create entities using NGSI v2 with several wrong attributes metadata type
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_metadata_type_error |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And properties to entities
      | parameter        | value           |
      | entities_type    | room            |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | metadatas_name   | random=5        |
      | metadatas_value  | random=6        |
      | metadatas_type   | <metadata_type> |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | Invalid characters in metadata type |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id | metadata_type |
      | room_2      | house<flat>   |
      | room_3      | house=flat    |
      | room_4      | house"flat"   |
      | room_5      | house'flat'   |
      | room_6      | house;flat    |
      | room_8      | house(flat)   |
      | room_7      | house_?       |
      | room_8      | house_&       |
      | room_9      | house_/       |
      | room_10     | house_#       |
      | room_11     | my house      |

  @attributes_metadata_type_no_string_error @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several invalid attributes metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | "default"       |
      | metadatas_type   | <metadata_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | entity_id | metadata_type |
      | "room1"   | rewrewr       |
      | "room2"   | SDFSDFSDF     |

  @attributes_metadata_type_no_string_error @BUG_1109
  Scenario Outline:  try to create an entity using NGSI v2 with several not allowed attributes metadata type (integer, boolean, no-string, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_attributes_name_error_ |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value           |
      | entities_type    | "room"          |
      | entities_id      | <entity_id>     |
      | attributes_name  | "temperature"   |
      | attributes_value | true            |
      | metadatas_name   | "alarm"         |
      | metadatas_value  | "default"       |
      | metadatas_type   | <metadata_type> |
    When create an entity in raw and "normalized" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | invalid JSON type for attribute metadata type |
    Examples:
      | entity_id | metadata_type   |
      | "room3"   | false           |
      | "room4"   | true            |
      | "room5"   | 34              |
      | "room6"   | {"a":34}        |
      | "room7"   | ["34", "a", 45] |

  # ---------- Queries Parameters - options=keyValues --------------------------------

  @qp_key_values
  Scenario Outline:  create entities using NGSI v2 with keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter         | value                   |
      | entities_type     | room                    |
      | entities_id       | room2                   |
      | attributes_number | 3                       |
      | attributes_name   | timestamp               |
      | attributes_value  | 017-06-17T07:21:24.238Z |
      | attributes_type   | date                    |
      | metadatas_number  | 3                       |
      | metadatas_name    | very_hot                |
      | metadatas_type    | alarm                   |
      | metadatas_value   | hot                     |
     # queries parameters
      | qp_options        | keyValues               |
    When create entity group with "3" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_key_values_wrong
  Scenario Outline:  try to create several entities using NGSI v2 with wrong keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value     |
      | entities_type    | room      |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | 54        |
     # queries parameters
      | qp_options       | <options> |
    When create entity group with "1" entities in "<format>" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |
    Examples:
      | options  | format     |
      | sdfdsfsd | normalized |
      | sdfdsfsd | keyValues  |
      | house_?  | normalized |
      | house_?  | keyValues  |
      | house_&  | normalized |
      | house_&  | keyValues  |
      | house_/  | normalized |
      | house_/  | keyValues  |
      | house_#  | normalized |
      | house_#  | keyValues  |
      | my house | normalized |
      | my house | keyValues  |

  @qp_key_values_invalid
  Scenario Outline:  try to create several entities using NGSI v2 with wrong keyValues mode activated
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value     |
      | entities_type    | room      |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | 54        |
     # queries parameters
      | qp_options       | <options> |
    When create entity group with "1" entities in "<format>" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | options     | format     |
      | house<flat> | normalized |
      | house<flat> | keyValues  |
      | house=flat  | normalized |
      | house=flat  | keyValues  |
      | house"flat" | normalized |
      | house"flat" | keyValues  |
      | house'flat' | normalized |
      | house'flat' | keyValues  |
      | house;flat  | normalized |
      | house;flat  | keyValues  |
      | house(flat) | normalized |
      | house(flat) | keyValues  |

  @qp_key_values_off_only_value.row<row.id>
  @qp_key_values_off_only_value @BUG_1716
  Scenario Outline:  try to create an entity using NGSI v2 without keyValues mode activated, but in  only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | not a JSON object |
    Examples:
      | entity_id | attribute_value                                                               |
      | "room0"   | "34"                                                                          |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room11"  | null                                                                          |
      | "room12"  | "sdfsdf.sdfsdf"                                                               |
      | "room13"  | "41.3763726, 2.1864475,14"                                                    |

  @qp_key_values_off_only_value @BUG_1716
  Scenario Outline:  try to create an entity using NGSI v2 without keyValues mode activated, but in  only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |
    Examples:
      | entity_id | attribute_value                                |

      | "room14"  | {"x": {"x1": "a","x2": "b"}}                   |
      | "room15"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}      |
      | "room16"  | {"x": ["a", 45, "rt"],"x2": "b"}               |
      | "room17"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"} |
      | "room18"  | {"x": "x1","x2": "b"}                          |


  @qp_key_values_on_only_value
  Scenario Outline:  create an entity using NGSI v2 with keyValues mode activated, but in only values format
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_key_value_mode |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | <entity_id>       |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
      # queries parameters
      | qp_options       | keyValues         |
    When create an entity in raw and "keyValues" modes
    Then verify that receive an "Created" http code
    Examples:
      | entity_id | attribute_value                                                               |
      | "room0"   | "34"                                                                          |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    |
      | "room17"  | null                                                                          |
      | "room18"  | "sdfsdf.sdfsdf"                                                               |


