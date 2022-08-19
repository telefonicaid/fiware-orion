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


Feature: get an entity by ID using NGSI v2. "GET" - /v2/entities/<entity_id>
  As a context broker user
  I would like to get an entity by ID using NGSI v2
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
  Scenario:  get an entity by ID using NGSI v2
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
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
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
    When get an entity by ID "room_0"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned

  # ------------------------ Content-Type header ----------------------------------------------
  @with_content_type @BUG_2128
  Scenario Outline:  get an entity by ID using NGSI v2 with Content-Type header
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
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
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       | <content_type>     |
    When get an entity by ID "room_0"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
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
  Scenario:  get an entity by ID using NGSI v2 with Content-Type header and empty value
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
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
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       |                    |
    When get an entity by ID "room_0"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                        |
      | error       | BadRequest                                                                                   |
      | description | Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden |

  # ------------------------ Service header ----------------------------------------------
  @service
  Scenario Outline:  get an entity by ID using NGSI v2 with several service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | the same value of the previous request |
      | Fiware-ServicePath | /test                                  |
    When get an entity by ID "room"
      | parameter | value       |
      | attrs     | temperature |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  get an entity by ID using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value |
      | Fiware-ServicePath | /test |
    When get an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned

  @service_error @BUG_1873
  Scenario Outline:  try to get an entity by ID using NGSI v2 with several wrong service header
    Given  a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
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

  @service_bad_length
  Scenario:  try to get an entity by ID using NGSI v2 with bad length service header
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |


   # ------------------------ Service path header ----------------------------------------------

  @service_path @BUG_1423
  Scenario Outline:  get an entity by ID using NGSI v2 with several service path header
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_id_service_path |
      | Fiware-ServicePath | <service_path>       |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | test_id_service_path                   |
      | Fiware-ServicePath | the same value of the previous request |
    When get an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
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
  Scenario:  get an entity by ID using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                        |
      | Fiware-Service | test_id_service_path_without |
      | Content-Type   | application/json             |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter      | value                        |
      | Fiware-Service | test_id_service_path_without |
    When get an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned

  @service_path_error
  Scenario Outline:  try to get an entity by ID using NGSI v2 with several wrong service path header
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_id_service_path_error |
      | Fiware-ServicePath | <service_path>             |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
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

  @service_path_error
  Scenario Outline:  try to get an entity by ID using NGSI v2 with several wrong service path header
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_id_service_path_error |
      | Fiware-ServicePath | <service_path>             |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_error
  Scenario Outline:  try to get an entity by ID using NGSI v2 with several wrong service path header
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_id_service_pat:error |
      | Fiware-ServicePath | <service_path>            |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_error
  Scenario:  try to get an entity by ID using NGSI v2 with several wrong service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_id_service_path_error           |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When get an entity by ID "room_0"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  # -------------- with/without attribute type or metadatas -----------------------

  @without_attribute_type
  Scenario Outline:  get an entity by ID using NGSI v2 without attribute type
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_without_attribute_type |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_without_attribute_type |
      | Fiware-ServicePath | /test                          |
    When get an entity by ID "room_1"
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | attribute_value         |
      | 017-06-17T07:21:24.238Z |
      | 34                      |
      | 34.4E-34                |
      | temp.34                 |
      | temp_34                 |
      | temp-34                 |
      | TEMP34                  |
      | house_flat              |
      | house.flat              |
      | house-flat              |
      | house@flat              |
      | habitación              |
      | españa                  |
      | barça                   |
      | random=10               |
      | random=100              |
      | random=1000             |
      | random=10000            |
      | random=100000           |
      | random=1000000          |

  @compound_without_attribute_type @BUG_1106
  Scenario Outline: get an entity by ID using NGSI v2 with special attribute values and without attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_without_attribute_type |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And create an entity in raw and "normalized" modes
    And verify that receive an "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_without_attribute_type |
      | Fiware-ServicePath | /test                          |
    When get an entity by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an entity in raw mode with type "<type>" in attribute value from http response
    Examples:
      | entity_id | attributes_value                                                              | type     |
      | "room1"   | true                                                                          | bool     |
      | "room2"   | false                                                                         | bool     |
      | "room3"   | 34                                                                            | int      |
      | "room4"   | -34                                                                           | int      |
      | "room5"   | 5.00002                                                                       | float    |
      | "room6"   | -5.00002                                                                      | float    |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                | list     |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             | list     |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] | list     |
      | "room10"  | {"x": "x1","x2": "b"}                                                         | dict     |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  | dict     |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": "ert"}}}}}}                                  | dict     |
      | "room13"  | {"x": ["a", 45.56, "rt"],"x2": "b"}                                           | dict     |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                | dict     |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    | str      |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    | str      |
      | "room17"  | null                                                                          | NoneType |

  @with_attribute_type
  Scenario Outline:  get an entity by ID using NGSI v2 with attribute type
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_id_with_attribute_type |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | attributes_type  | celsius           |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_id_with_attribute_type |
      | Fiware-ServicePath | /test                       |
    When get an entity by ID "room_1"
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | attribute_value         |
      | 017-06-17T07:21:24.238Z |
      | 34                      |
      | 34.4E-34                |
      | temp.34                 |
      | temp_34                 |
      | temp-34                 |
      | TEMP34                  |
      | house_flat              |
      | house.flat              |
      | house-flat              |
      | house@flat              |
      | habitación              |
      | españa                  |
      | barça                   |
      | random=10               |
      | random=100              |
      | random=1000             |
      | random=10000            |
      | random=100000           |
      | random=1000000          |

  @compound_with_attribute_type @BUG_1106
  Scenario Outline: get an entity by ID using NGSI v2 with special attribute values and with attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_id_with_attribute_type |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celsius"          |
    And create an entity in raw and "normalized" modes
    And verify that receive an "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_id_with_attribute_type |
      | Fiware-ServicePath | /test                       |
    When get all entities
    Then verify that receive an "OK" http code
    And verify an entity in raw mode with type "<type>" in attribute value from http response
    Examples:
      | entity_id | attributes_value                                                              | type     |
      | "room1"   | true                                                                          | bool     |
      | "room2"   | false                                                                         | bool     |
      | "room3"   | 34                                                                            | int      |
      | "room4"   | -34                                                                           | int      |
      | "room5"   | 5.00002                                                                       | float    |
      | "room6"   | -5.00002                                                                      | float    |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                | list     |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             | list     |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] | list     |
      | "room10"  | {"x": "x1","x2": "b"}                                                         | dict     |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  | dict     |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": "ert"}}}}}}                                  | dict     |
      | "room13"  | {"x": ["a", 45.56, "rt"],"x2": "b"}                                           | dict     |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                | dict     |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    | str      |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    | str      |
      | "room17"  | null                                                                          | NoneType |

  @with_metadata
  Scenario Outline:  get an entity by ID using NGSI v2 with metadata
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_id_metadata |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | hot               |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value            |
      | Fiware-Service     | test_id_metadata |
      | Fiware-ServicePath | /test            |
    When get an entity by ID "room_0"
      | parameter | value       |
      | attrs     | temperature |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | attribute_value         |
      | 017-06-17T07:21:24.238Z |
      | 34                      |
      | 34.4E-34                |
      | temp.34                 |
      | temp_34                 |
      | temp-34                 |
      | TEMP34                  |
      | house_flat              |
      | house.flat              |
      | house-flat              |
      | house@flat              |
      | habitación              |
      | españa                  |
      | barça                   |
      | random=10               |
      | random=100              |
      | random=1000             |
      | random=10000            |
      | random=100000           |
      | random=1000000          |

  @compound_with_metadata @BUG_1106
  Scenario Outline: get an entity by ID using NGSI v2 with special attribute values and with metadatas (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_id_with_metadata |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celsius"          |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    And create an entity in raw and "normalized" modes
    And verify that receive an "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                 |
      | Fiware-Service     | test_id_with_metadata |
      | Fiware-ServicePath | /test                 |
    When get an entity by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an entity in raw mode with type "<type>" in attribute value from http response
    Examples:
      | entity_id | attributes_value                                                              | type     |
      | "room1"   | true                                                                          | bool     |
      | "room2"   | false                                                                         | bool     |
      | "room3"   | 34                                                                            | int      |
      | "room4"   | -34                                                                           | int      |
      | "room5"   | 5.00002                                                                       | float    |
      | "room6"   | -5.00002                                                                      | float    |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                | list     |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             | list     |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] | list     |
      | "room10"  | {"x": "x1","x2": "b"}                                                         | dict     |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  | dict     |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": "ert"}}}}}}                                  | dict     |
      | "room13"  | {"x": ["a", 45.56, "rt"],"x2": "b"}                                           | dict     |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                | dict     |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    | str      |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    | str      |
      | "room17"  | null                                                                          | NoneType |

  @without_metadata_type
  Scenario Outline:  get an entity by ID using NGSI v2 without metadata type
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_id_metadata |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | hot               |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value            |
      | Fiware-Service     | test_id_metadata |
      | Fiware-ServicePath | /test            |
    When get an entity by ID "room_2"
      | parameter | value       |
      | attrs     | temperature |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | attribute_value         |
      | 017-06-17T07:21:24.238Z |
      | 34                      |
      | 34.4E-34                |
      | temp.34                 |
      | temp_34                 |
      | temp-34                 |
      | TEMP34                  |
      | house_flat              |
      | house.flat              |
      | house-flat              |
      | house@flat              |
      | habitación              |
      | españa                  |
      | barça                   |
      | random=10               |
      | random=100              |
      | random=1000             |
      | random=10000            |
      | random=100000           |
      | random=1000000          |

  @compound_with_metadata_without_meta_type @BUG_1106
  Scenario Outline: get an entity by ID using NGSI v2 with special attribute values and with metadatas but without
  metadata type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_id_without_meta_type |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celsius"          |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_value  | "hot"              |
    And create an entity in raw and "normalized" modes
    And verify that receive an "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                     |
      | Fiware-Service     | test_id_without_meta_type |
      | Fiware-ServicePath | /test                     |
    When get an entity by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an entity in raw mode with type "<type>" in attribute value from http response
    Examples:
      | entity_id | attributes_value                                                              | type     |
      | "room1"   | true                                                                          | bool     |
      | "room2"   | false                                                                         | bool     |
      | "room3"   | 34                                                                            | int      |
      | "room4"   | -34                                                                           | int      |
      | "room5"   | 5.00002                                                                       | float    |
      | "room6"   | -5.00002                                                                      | float    |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                | list     |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             | list     |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] | list     |
      | "room10"  | {"x": "x1","x2": "b"}                                                         | dict     |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  | dict     |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": "ert"}}}}}}                                  | dict     |
      | "room13"  | {"x": ["a", 45.56, "rt"],"x2": "b"}                                           | dict     |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                | dict     |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    | str      |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    | str      |
      | "room17"  | null                                                                          | NoneType |

 # --------------------- entity ID ----------------------------------------------

  @entity_id
  Scenario Outline:  get an entity by ID using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_id_entity_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
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
      | parameter          | value             |
      | Fiware-Service     | test_id_entity_id |
      | Fiware-ServicePath | /test             |
    When get an entity by ID "<entity_id>"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
    Then verify that receive an "OK" http code
    And verify that the entity by ID is returned
    Examples:
      | entity_id |
      | room_0    |
      | room_1    |
      | room_2    |

  @entity_id_unknown
  Scenario:  try to get an entity by ID using NGSI v2 with unknown entity id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_id_entity_id_unknown |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
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
      | parameter          | value                     |
      | Fiware-Service     | test_id_entity_id_unknown |
      | Fiware-ServicePath | /test                     |
    When get an entity by ID "room_34"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @entity_id_invalid
  Scenario Outline:  try to get an entity by ID using NGSI v2 with invalid entity id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_id_entity_id_invalid |
      | Fiware-ServicePath | /test                     |
    When get an entity by ID "<entity_id>"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
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

  @more_entities
  Scenario:  try get an entity by ID using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_id_more_entities |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
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
      | parameter          | value                 |
      | Fiware-Service     | test_id_more_entities |
      | Fiware-ServicePath | /test                 |
    When get an entity by ID "room"
      | parameter | value       |
      | attrs     | temperature |
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |
