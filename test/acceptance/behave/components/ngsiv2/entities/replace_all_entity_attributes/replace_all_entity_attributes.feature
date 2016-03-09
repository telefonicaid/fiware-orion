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


Feature: replace attributes by entity ID using NGSI v2. "PUT" - /v2/entities/<entity_id> plus payload
  As a context broker user
  I would like to replace attributes by entity ID using NGSI v2
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
  Scenario:  replace attributes by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_replace_happy_path |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
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
      | parameter         | value      |
      | attributes_number | 5          |
      | attributes_name   | humidity   |
      | attributes_value  | 80         |
      | attributes_type   | Fahrenheit |
      | metadatas_number  | 3          |
      | metadatas_name    | very_cold  |
      | metadatas_type    | alarm      |
      | metadatas_value   | cold       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @more_entities_replace @BUG_1320
  Scenario:  try to replace attributes by entity ID using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_replace_more_entities |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    # These properties below are used in update request
    And properties to entities
      | parameter         | value      |
      | attributes_number | 5          |
      | attributes_name   | humidity   |
      | attributes_value  | 80         |
      | attributes_type   | Fahrenheit |
      | metadatas_number  | 3          |
      | metadatas_name    | very_cold  |
      | metadatas_type    | alarm      |
      | metadatas_value   | cold       |
    When replace attributes by ID "room" if it exists and with "normalized" mode
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @length_required
  Scenario:  try to replace attributes by entity ID using NGSI v2 without payload
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_replace_length_required |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    And properties to entities
      | parameter          |
      | without_properties |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | LengthRequired                                   |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  @maximum_size
    # 4708 attributes is a way of generating a request longer than 1MB (in fact, 1048774 bytes)
  Scenario:  try to replace attributes using NGSI v2 with maximum size in payload (4708 attributes = 1048774 bytes)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_maximum_size |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
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
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048774, max size supported: 1048576 |

  @multiple_attributes
  Scenario Outline:  replace multiples attributes by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_replace_multi_attrs |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
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
      | parameter         | value               |
      | attributes_number | <attributes_number> |
      | attributes_name   | humidity            |
      | attributes_value  | 80                  |
      | attributes_type   | Fahrenheit          |
      | metadatas_number  | 1                   |
      | metadatas_name    | very_cold           |
      | metadatas_type    | alarm               |
      | metadatas_value   | cold                |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_number |
      | 2                 |
      | 10                |
      | 50                |
      | 100               |
      | 500               |

  @multiple_attributes_metadatas
  Scenario Outline:  replace multiples attributes metadatas by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_replace_multi_attrs_meta |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
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
      | attributes_name  | humidity          |
      | attributes_value | 80                |
      | attributes_type  | Fahrenheit        |
      | metadatas_number | <metadata_number> |
      | metadatas_name   | very_cold         |
      | metadatas_type   | alarm             |
      | metadatas_value  | cold              |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | metadata_number |
      | 2               |
      | 10              |
      | 50              |
      | 100             |
      | 500             |

   # ------------------------ Service ----------------------------------------------
  @service_replace
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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

  @service_replace_without
  Scenario:  replace attributes by entity ID using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_replace_error @BUG_1873
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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

  @service_replace_bad_length
  Scenario:  try to replace attributes by entity ID using NGSI v2 with bad length service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ------------------------ Service path ----------------------------------------------

  @service_path_replace @BUG_1423
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_replace_service_path |
      | Fiware-ServicePath | <service_path>            |
      | Content-Type       | application/json          |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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

  @service_path_replace_without
  Scenario:  replace attributes by entity ID using NGSI v2 without service header
    Given  a definition of headers
      | parameter      | value                     |
      | Fiware-Service | test_replace_service_path |
      | Content-Type   | application/json          |
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
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @service_path_replace_error
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_replace_service_path_error |
      | Fiware-ServicePath | <service_path>                  |
      | Content-Type       | application/json                |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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

  @service_path_replace_error
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_replace_service_path_error |
      | Fiware-ServicePath | <service_path>                  |
      | Content-Type       | application/json                |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_replace_error
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_replace_service_path_error |
      | Fiware-ServicePath | <service_path>                  |
      | Content-Type       | application/json                |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_replace_error
  Scenario:  try to replace attributes by entity ID using NGSI v2 with wrong service header values
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_service_path_error      |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_name  | humidity |
      | attributes_value | 80       |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  # --------------------- attribute value  ------------------------------------

  @attribute_value_replace_without_attribute_type
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute values and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                                     |
      | Fiware-Service     | test_replace_attr_value_without_attr_type |
      | Fiware-ServicePath | /test                                     |
      | Content-Type       | application/json                          |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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
      | random=100000    |
      | random=1000000   |

  @attribute_value_replace_with_attribute_type
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute values and with attribute type
    Given  a definition of headers
      | parameter          | value                                  |
      | Fiware-Service     | test_replace_attr_value_with_attr_type |
      | Fiware-ServicePath | /test                                  |
      | Content-Type       | application/json                       |
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
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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
      | random=100000    |
      | random=1000000   |

  @attribute_value_replace_with_metadatas
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute values and with metadatas
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_replace_attr_value_with_metadata |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
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
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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
      | random=100000    |
      | random=1000000   |

  @attribute_value_replace_without_attribute_type_special @BUG_1106
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special attribute values (compound, vector, boolean, etc) and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
      | room_18   | {"rt.ty": "5678"}                                                             |

  @attribute_value_replace_with_attribute_type_special @BUG_1106
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with attribute type
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 67          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
      | room_18   | {"rt.ty": "5678"}                                                             |

  @attribute_value_replace_with_metadata_special @BUG_1106
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with metadatas
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
      | room_18   | {"rt.ty": "5678"}                                                             |

  @attribute_value_replace_with_attribute_type_in_replace
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute values and without attribute type nor metadatas, but with attribute type in replace request
    Given  a definition of headers
      | parameter          | value                                             |
      | Fiware-Service     | test_replace_attr_value_with_attr_type_in_replace |
      | Fiware-ServicePath | /test                                             |
      | Content-Type       | application/json                                  |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
      | attributes_type  | celsius            |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
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
      | random=100000    |
      | random=1000000   |

  @attribute_value_replace_special_with_attr_type @BUG_1106
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special attribute values (compound, vector, boolean, etc) without attribute type not metadata but with attribute type in replace request
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celsius"          |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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

  @attribute_value_replace_with_metadata
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with several attribute values and without attribute type nor metadatas, but with metadatas in replace request
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_replace_attr_value_with_meta |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | very_hot           |
      | metadatas_type   | alarm              |
      | metadatas_value  | hot                |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | dsfsfsd          |
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
      | random=1000000   |

  @attribute_value_replace_special_with_metadata @BUG_1106
  Scenario Outline:  replace attributes by entity ID using NGSI v2 with special attribute values (compound, vector, boolean, etc) without attribute type not metadata but with metadatas in replace
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
      | room_18   | {"rt.ty": "5678"}                                                             |

  @attribute_value_error_without
  Scenario:  try to replace attributes by entity ID using NGSI v2 without attribute values
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_replace_attr_value_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter       | value    |
      | attributes_name | pressure |
    When replace attributes by ID "room_1" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                             |
      | error       | BadRequest                                        |
      | description | no 'value' for ContextAttribute without keyValues |

  @attribute_value_invalid
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 without invalid attribute values in replace request
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_replace_attr_value_error |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | pressure           |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "<entity_id>" if it exists and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid characters in attribute value |
    Examples:
      | entity_id | attributes_value |
      | room_1    | house<flat>      |
      | room_2    | house=flat       |
      | room_3    | house"flat"      |
      | room_4    | house'flat'      |
      | room_5    | house;flat       |
      | room_6    | house(flat)      |

  @attribute_value_error_special
  Scenario Outline:  try to replace attributes by entity ID using NGSI v2 with wrong attribute values in replace request (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_replace_attribute_value_special |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_name  | "pressure"         |
      | attributes_value | <attributes_value> |
    When replace attributes by ID "<entity_id>" if it exists in raw and "normalized" modes
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
