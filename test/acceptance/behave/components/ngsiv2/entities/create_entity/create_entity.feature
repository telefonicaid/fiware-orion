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
#   - verification of headers response
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
      | parameter         | value    |
      | entities_type     | room     |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | random=5 |
      | attributes_value  | 45       |
      | attributes_type   | celsius  |
      | metadatas_number  | 2        |
      | metadatas_name    | very_hot |
      | metadatas_type    | alarm    |
      | metadatas_value   | hot      |
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
    # 8137 is a way of generating a request longer than 1MB (in fact, 1048594 bytes)
  Scenario:  try to create a new entity NGSI v2 with maximum size in payload (8137 attributes = 1048594 bytes)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And properties to entities
      | parameter         | value      |
      | entities_type     | room       |
      | entities_id       | room2      |
      | attributes_number | 8137       |
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
      | description | payload size: 1048594, max size supported: 1048576 |
    And verify that entities are not stored in mongo

  @length_required @BUG_1199 @BUG_1203
  Scenario:  try to create several entities using NGSI v2 wihout payload
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_length_required |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Content Length Required" http code
    And verify several error responses
      | parameter   | value                                            |
      | error       | ContentLengthRequired                            |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

   # ---------- Content-Type header --------------------------------

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

  # ---------- Services header --------------------------------

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

  @service_error @BUG_1087 @BUG_1873
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
    Examples:
      | service       |
      | servicedot.sr |
      | Service-sr    |
      | Service(sr)   |
      | Service=sr    |
      | Service<sr>   |
      | Service,sr    |
      | service#sr    |
      | service%sr    |
      | service&sr    |

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

  # ---------- Services path header --------------------------------

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

  @metadatas_number @too_slow
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
  @entities_type_error @BUG_1093 @BUG_1200 @BUG_1351 @BUG_1728
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

  @entities_id_wrong @BUG_1093 @BUG_1200 @BUG_1351  @BUG_1728
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
