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


Feature: list all entities with get request and queries parameters using NGSI v2. "GET" - /v2/entities/
  Queries parameters tested: limit, offset, id, idPattern, type and q
  As a context broker user
  I would like to list all entities with get request and queries parameter using NGSI v2
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
  Scenario:  list all entities using NGSI v2
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_happy_path |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | car       |
      | entities_id      | vehicle   |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value      |
      | limit     | 9          |
      | offset    | 3          |
      | id        | vehicle    |
      | type      | car        |
      | q         | speed_0<78 |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  # ------------------------ Service ----------------------------------------------
  @service
  Scenario Outline:  list all entities using NGSI v2 with several services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  list all entities using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @service_error
  Scenario Outline:  try to list all entities using NGSI v2 with several wrong services headers
    Given  a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When get all entities
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
  Scenario:  try to list all entities using NGSI v2 with bad length services headers
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
    When get all entities
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

 # ------------------------ Service path ----------------------------------------------
  @service_path @BUG_1423 @skip
  Scenario Outline:  list all entities using NGSI v2 with several service paths headers
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath | <service_path>    |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
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
  Scenario:  list all entities using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                     |
      | Fiware-Service | test_service_path_without |
      | Content-Type   | application/json          |
    And initialize entity groups recorder
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @service_path_error
  Scenario Outline:  try to list all entities using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario Outline:  try to list all entities using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario Outline: try to list all entities using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario: try to list all entities using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_list_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  # -------------- with/without attribute type or metadatas -----------------------

  @without_attribute_type
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and without attribute type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value             |
      | entities_type    | room              |
      | entities_id      | room2             |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
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

  @compound_without_attribute_type @BUG_1106 @skip
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and without attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And verify that receive an "Created" http code
    And record entity group
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

  @with_attribute_type
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and attribute type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_list_with_attribute_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value             |
      | entities_type    | room              |
      | entities_id      | room2             |
      | attributes_name  | temperature       |
      | attributes_value | <attribute value> |
      | attributes_type  | celcius           |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | attribute value         |
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

  @compound_with_attribute_type @BUG_1106 @skip
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celcius"          |
    And verify that receive an "Created" http code
    And record entity group
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

  @with_metadatas
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and with metadatas
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_happy_path |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value             |
      | entities_type    | room              |
      | entities_id      | room2             |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | random=10         |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
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

  @compound_with_metadata @BUG_1106 @skip
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and metadatas (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    And verify that receive an "Created" http code
    And record entity group
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

  @without_metadata_type
  Scenario Outline:  list all entities using NGSI v2 without metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_list_without_metadata_type |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value             |
      | entities_type    | room              |
      | entities_id      | room2             |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 2                 |
      | metadatas_name   | very_hot          |
      | metadatas_value  | random=10         |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
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

  @compound_with_metadata_without_meta_type @BUG_1106 @skip
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and metatadas but without metadata type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "room"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_value  | "hot"              |
    And verify that receive an "Created" http code
    And record entity group
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

  # ------------------ queries parameters -------------------------------
  # --- limit and offset ---
  @only_limit
  Scenario Outline:  list all entities using NGSI v2 with only limit query parameter
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | limit     | <limit> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | limit |
      | 1     |
      | 20    |

  @only_limit_error
  Scenario Outline:  try to list all entities using NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_list_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value   |
      | limit     | <limit> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | BadRequest                                                 |
      | description | Bad pagination limit: /<limit>/ [must be a decimal number] |
    Examples:
      | limit |
      | -3    |
      | 1.5   |
      | ewrw  |
      | <2    |
      | %@#   |

  @only_limit_error
  Scenario:  try to list all entities using NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_list_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 0     |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | Bad pagination limit: /0/ [a value of ZERO is unacceptable] |

  @only_offset
  Scenario Outline:  list all entities using NGSI v2 with only offset query parameter
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_list_only_offset |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And initialize entity groups recorder
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | offset |
      | 0      |
      | 1      |
      | 20     |

  @only_offset_error
  Scenario Outline:  try to list all entities using NGSI v2 with only wrong offset query parameter
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_only_offset_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                        |
      | error       | BadRequest                                                   |
      | description | Bad pagination offset: /<offset>/ [must be a decimal number] |
    Examples:
      | offset |
      | -3     |
      | 1.5    |
      | ewrw   |
      | <2     |
      | %@#    |

  @limit_offset
  Scenario Outline:  list all entities using NGSI v2 with limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_limit_offset |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | limit | offset |
      | 1     | 1      |
      | 20    | 1      |
      | 1     | 20     |
      | 5     | 3      |

  # --- id, id pattern and type ---
  @only_id
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | speed           |
      | attributes_value | 89              |
      | attributes_type  | km/h            |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | id        | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | entities_type | entities_id |
      | room_1        | vehicle     |
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
      | room_14       | habitación  |
      | room_15       | españa      |
      | room_16       | barça       |
      | room_17       | random=10   |
      | room_18       | random=100  |
      | room_19       | random=900  |

  @only_id_list
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter with a list of ids
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | id        | <id>  |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | id                       |
      | room1                    |
      | room1,room2              |
      | room1,room2,car          |
      | room1,room2,car,sdfsdfsd |

  @only_id_same
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter and the same entity id in several entities
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value         |
      | entities_type    | home          |
      | entities_id      | <entities_id> |
      | attributes_name  | temperature   |
      | attributes_value | 34            |
      | attributes_type  | celsius       |
      | metadatas_number | 2             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | random=10     |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | <entities_id>           |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | speed           |
      | attributes_value | 89              |
      | attributes_type  | km/h            |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | id        | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | entities_type | entities_id |
      | room_1        | vehicle     |
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
      | room_14       | habitación  |
      | room_15       | españa      |
      | room_16       | barça       |
      | room_17       | random=10   |
      | room_18       | random=100  |
      | room_19       | random=900  |

  @only_id_unknown
  Scenario:  list any entity using NGSI v2 with only id query parameter but unknown value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | id        | fdgfdgfdgfd |
    Then verify that receive an "OK" http code
    And verify that any entities are returned

  @only_id_invalid
  Scenario Outline:  try to list all entities using NGSI v2 with only id query parameter but invalid value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | id        | <entity_id> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
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

  @only_type
  Scenario Outline:  list all entities using NGSI v2 with only type query parameter
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | speed           |
      | attributes_value | 89              |
      | attributes_type  | km/h            |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | type      | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | entities_id | entities_type |
      | room_1      | vehicle       |
      | room_2      | 34            |
      | room_3      | false         |
      | room_4      | true          |
      | room_5      | 34.4E-34      |
      | room_6      | temp.34       |
      | room_7      | temp_34       |
      | room_8      | temp-34       |
      | room_9      | TEMP34        |
      | room_10     | temp_flat     |
      | room_11     | temp.flat     |
      | room_12     | temp-flat     |
      | room_13     | temp@flat     |
      | room_14     | habitación    |
      | room_15     | españa        |
      | room_16     | barça         |
      | room_17     | random=10     |
      | room_18     | random=100    |
      | room_19     | random=900    |

  @only_type_list
  Scenario Outline:  list all entities using NGSI v2 with only type query parameter with a list of types
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value  |
      | type      | <type> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | type                     |
      | room1                    |
      | room1,room2              |
      | room1,room2,car          |
      | room1,room2,car,sdfsdfsd |

  @only_type_same
  Scenario Outline:  list all entities using NGSI v2 with only type query parameter and the same entity type in several entities
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | room1           |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | attributes_type  | celsius         |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | <entities_type>         |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | speed           |
      | attributes_value | 89              |
      | attributes_type  | km/h            |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | type      | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | entities_id | entities_type |
      | room_1      | vehicle       |
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
      | room_14     | habitación    |
      | room_15     | españa        |
      | room_16     | barça         |
      | room_17     | random=10     |
      | room_18     | random=100    |
      | room_19     | random=900    |

  @only_type_unknown
  Scenario:  list any entity using NGSI v2 with only type query parameter but unknown value
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | type      | fdgfdgfdgfd |
    Then verify that receive an "OK" http code
    And verify that any entities are returned

  @only_type_invalid
  Scenario Outline:  try to list all entities using NGSI v2 with only type query parameter but invalid value
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | type      | <entity_id> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
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

  @only_idPattern
  Scenario Outline:  list all entities using NGSI v2 with only idPattern query parameter
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "2" attributes
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | speed           |
      | attributes_value | 89              |
      | attributes_type  | km/h            |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value        |
      | idPattern | <id_pattern> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | entities_type | entities_id | id_pattern   |
      | room_1        | vehicle     | veh.*        |
      | room_2        | 34          | ^3.?         |
      | room_3        | false       | .*als.*      |
      | room_4        | true        | .*ru.*       |
      | room_5        | 34.4E-34    | .*34$        |
      | room_6        | tete.34     | ^te+.*       |
      | room_7        | aBc56       | [A-Za-z0-9]+ |
      | room_8        | defG        | \a+          |
      | room_9        | def45       | \w+          |
      | room_10       | 23445       | \d+          |
      | room_11       | house flat  | \s+          |
      | room_12       | afaffasf    | \D+          |
      | room_13       | hOUSEFLAT   | \u+          |
      | room_14       | houseflat   | house(flat)  |
      | room_15       | houseflat   | house(f*)    |
      | room_16       | ewrwer      | .*           |

  @only_idPattern_wrong
  Scenario:  list any entity using NGSI v2 with only idPattern query parameter but the pattern is wrong
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicles  |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | idPattern | dfgdg.* |
    Then verify that receive an "OK" http code
    And verify that any entities are returned

  @id_and_id_pattern
  Scenario:  try to list all entities using NGSI v2 with id and idPattern queries parameters (incompatibles)
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value  |
      | id        | room1  |
      | idPattern | room.* |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | Incompatible parameters: id, IdPattern |

  @id_type
  Scenario Outline:  list all entities using NGSI v2 with id and type queries parameters
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_id_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "10" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | <type>    |
      | entities_id      | <id>      |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value  |
      | id        | <id>   |
      | type      | <type> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | id        | type    |
      | door      | house   |
      | car       | vehicle |
      | light     | park    |
      | semaphore | street  |

  @idPattern_type
  Scenario Outline:  list all entities using NGSI v2 with idPattern and type queries parameters
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_id_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "10" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value        |
      | idPattern | <id_pattern> |
      | type      | <type>       |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | id_pattern | type    |
      | room2.*    | house   |
      | ^car       | vehicle |
      | \w+        | home    |

  @id_type_limit_offset
  Scenario Outline:  list all entities using NGSI v2 with id, type, limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_id_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "10" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | <type>    |
      | entities_id      | <id>      |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
      | id        | <id>     |
      | type      | <type>   |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | limit | offset | id        | type    |
      | 1     | 1      | door      | house   |
      | 20    | 1      | car       | vehicle |
      | 1     | 20     | light     | park    |
      | 5     | 3      | semaphore | street  |

  # --- q = <expression> ---
  @only_q_type @BUG_1587 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=+type query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_id      | room3       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "3" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | home                    |
      | entities_id      | room4l2                 |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "1" attributes
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression |
      | +type        |
      | -type        |

  @only_q_attribute @BUG_1589 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=+dfgdfg query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "1" attributes
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression |
      | +myAttr      |

  @only_q_parse_error @BUG_1592 @skip
  Scenario Outline:  try to list entities using NGSI v2 with only q query parameter but with parse errors
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "1" attributes
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value           |
      | error       | ParseError      |
      | description | Not defined yet |
    Examples:
      | q_expression |
      | speed==      |
      | speed!=      |
      | speed>=      |
      | speed<=      |
      | speed>       |
      | speed<       |
      | 89==speed    |
      | 89!=speed    |
      | 89>=speed    |
      | 89<=speed    |
      | 89>speed     |
      | 89<speed     |
      | ==speed      |
      | !=speed      |
      | >=speed      |
      | <=speed      |
      | >speed       |
      | <speed       |
      | speed        |
      | speed-       |
      | speed+       |
      | *speed       |
      | type         |

  @only_q_value_boolean @BUG_1594 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=attr==true query parameter (boolean values)
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "3" attributes
      | parameter        | value     |
      | entities_type    | house     |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | true      |
      | attributes_type  | date      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    When create an entity and attribute with special values in raw
      | parameter        | value             |
      | entities_type    | "vehicle"         |
      | entities_id      | "car"             |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression       | attribute_value |
      | temperature==true  | true            |
      | temperature==false | false           |

  @only_q_string_numbers @BUG_1595 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=attr=="89" query parameter (number in string)
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "12" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "3" attributes
      | parameter        | value     |
      | entities_type    | house     |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | true      |
      | attributes_type  | date      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    When create an entity and attribute with special values in raw
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "car"     |
      | attributes_name  | "speed"   |
      | attributes_value | 89        |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression |
      | speed=='89'  |
      | speed==89    |

  @only_q_operators_errors @BUG_1607 @skip
  Scenario Outline:  try to list entities using NGSI v2 with only q query parameter, with range, but wrong operators
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "12" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km/h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "3" attributes
      | parameter        | value     |
      | entities_type    | house     |
      | entities_id      | room2     |
      | attributes_name  | timestamp |
      | attributes_value | true      |
      | attributes_type  | date      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And create an entity and attribute with special values in raw
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "car"     |
      | attributes_name  | "speed"   |
      | attributes_value | 89        |
    And verify that receive several "Created" http code
    And record entity group
    And get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                     |
      | error       | BadRequest                                                                |
      | description | <, <=, > and >= operators are not allowed with a range in query parameter |
    Examples:
      | q_expression  |
      | speed>=69..90 |
      | speed>69..90  |
      | speed<=69..79 |
      | speed<99..190 |

  @only_q
  Scenario Outline:  list entities using NGSI v2 with only q query parameter with several statements
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "1" entities with "1" attributes
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "car"   |
      | attributes_name  | "speed" |
      | attributes_value | 79.2    |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_type    | home      |
      | entities_id      | bathroom  |
      | attributes_name  | humidity  |
      | attributes_value | high      |
      | attributes_type  | celsius   |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_type    | home      |
      | entities_id      | kitchen   |
      | attributes_name  | humidity  |
      | attributes_value | medium    |
      | attributes_type  | celsius   |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value   |
      | entities_id      | simple  |
      | attributes_name  | comma   |
      | attributes_value | one,two |
      | attributes_type  | celsius |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression                      |
      | -type                             |
      | +type                             |
      | -myAttr                           |
      | -type;-myAttr                     |
      | +type;-myAttr                     |
      | speed==89.6                       |
      | speed==89.6;-myAttr               |
      | speed==89.6;-type                 |
      | speed==89.6;+type                 |
      | speed!=99.6                       |
      | speed>=89.6                       |
      | speed<=89.6                       |
      | speed<89.6                        |
      | speed>79.6                        |
      | speed!=99.6;-myAttr               |
      | speed>=89.6;-type                 |
      | speed>=89.6;+type                 |
      | speed<=89.6;-myAttr;-type         |
      | speed<=89.6;-myAttr;+type         |
      | speed<89.6;-type                  |
      | speed<89.6;+type                  |
      | speed>79.6                        |
      | speed!=23..56                     |
      | speed!=90..100                    |
      | speed==79..90                     |
      | speed==79..85                     |
      | speed!=23..56;-type;-myAttr       |
      | speed!=23..56;+type;-myAttr       |
      | speed!=90..100;-myAttr            |
      | speed==79..90;-type               |
      | speed==79..90;+type               |
      | speed==79..85;-myAttr;-type       |
      | speed==79..85;-myAttr;+type       |
      | humidity==high,low,medium         |
      | humidity!=high,low                |
      | humidity==high                    |
      | humidity!=high                    |
      | humidity==high,low,medium;-myAttr |
      | humidity!=high,low;-myAttr        |
      | humidity==high;-myAttr            |
      | humidity!=high;-myAttr            |
      | humidity!=high;-myAttr;-type      |
      | humidity!=high;-myAttr;+type      |
      | comma=='one,two',high             |
      | comma!='three,four',high          |
      | comma=='one,two',high;-myAttr     |
      | comma=='one,two';-myAttr;-type    |
      | comma=='one,two';-myAttr;+type    |

  @qp_q_and_limit.row<row.id>
  @qp_q_and_limit
  Scenario Outline:  list entities using NGSI v2 with q and limit queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | limit |
      | -type               | 3     |
      | +type               | 8     |
      | -my_attr            | 3     |
      | +seats              | 8     |
      | speed==89.6         | 3     |
      | speed!=79.6         | 8     |
      | speed<23            | 3     |
      | speed>99.6          | 8     |
      | speed==89..90       | 3     |
      | speed!=79..80       | 8     |
      | temperature_0==high | 2     |
      | temperature_1!=low  | 9     |

  @qp_q_and_offset.row<row.id>
  @qp_q_and_offset
  Scenario Outline:  list entities using NGSI v2 with q and offset queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | offset    | <offset>       |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset |
      | -type               | 3      |
      | +type               | 8      |
      | -my_attr            | 3      |
      | +seats              | 8      |
      | speed==89.6         | 3      |
      | speed!=79.6         | 8      |
      | speed<23            | 3      |
      | speed>99.6          | 8      |
      | speed==89..90       | 3      |
      | speed!=79..80       | 8      |
      | temperature_0==high | 2      |
      | temperature_1!=low  | 9      |


  @qp_q_limit_and_offset.row<row.id>
  @qp_q_limit_and_offset
  Scenario Outline:  list entities using NGSI v2 with q, limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limitt    | <limit>        |
      | offset    | <offset>       |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset | limit |
      | -type               | 3      | 3     |
      | +type               | 8      | 8     |
      | -my_attr            | 3      | 3     |
      | +seats              | 8      | 8     |
      | speed==89.6         | 3      | 3     |
      | speed!=79.6         | 8      | 8     |
      | speed<23            | 3      | 3     |
      | speed>99.6          | 8      | 8     |
      | speed==89..90       | 3      | 3     |
      | speed!=79..80       | 8      | 8     |
      | temperature_0==high | 2      | 2     |
      | temperature_1!=low  | 9      | 9     |

  @qp_q_limit_offset_and_id.row<row.id>
  @qp_q_limit_offset_and_id
  Scenario Outline:  list entities using NGSI v2 with q, limit offset and id queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limitt    | <limit>        |
      | offset    | <offset>       |
      | id        | <id>           |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset | limit | id    |
      | -type               | 3      | 3     | bus   |
      | +type               | 8      | 8     | room2 |
      | -my_attr            | 3      | 3     | bus   |
      | +seats              | 8      | 8     | bus   |
      | speed==89.6         | 3      | 3     | moto  |
      | speed!=79.6         | 8      | 8     | moto  |
      | speed<23            | 3      | 3     | moto  |
      | speed>99.6          | 8      | 8     | moto  |
      | speed==89..90       | 3      | 3     | moto  |
      | speed!=79..80       | 8      | 8     | moto  |
      | temperature_0==high | 2      | 2     | room1 |
      | temperature_1!=low  | 9      | 9     | room1 |

  @qp_q_limit_offset_and_type.row<row.id>
  @qp_q_limit_offset_and_type
  Scenario Outline:  list entities using NGSI v2 with q, limit offset and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limitt    | <limit>        |
      | offset    | <offset>       |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset | limit | type    |
      | +type               | 8      | 8     | house   |
      | -my_attr            | 3      | 3     | house   |
      | +seats              | 0      | 0     | house   |
      | speed==89.6         | 3      | 3     | vehicle |
      | speed!=79.6         | 8      | 8     | vehicle |
      | speed<23            | 3      | 3     | vehicle |
      | speed>99.6          | 8      | 8     | vehicle |
      | speed==89..90       | 3      | 3     | vehicle |
      | speed!=79..80       | 8      | 8     | vehicle |
      | temperature_0==high | 2      | 2     | home    |
      | temperature_1!=low  | 9      | 9     | home    |

  @qp_q_limit_offset_id_and_type.row<row.id>
  @qp_q_limit_offset_id_and_type
  Scenario Outline:  list entities using NGSI v2 with q, limit offset, id and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limitt    | <limit>        |
      | offset    | <offset>       |
      | id        | <id>           |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset | limit | type    | id    |
      | +type               | 8      | 8     | house   | room2 |
      | -my_attr            | 3      | 3     | house   | bus   |
      | +seats              | 0      | 0     | house   | bus   |
      | speed==89.6         | 3      | 3     | vehicle | moto  |
      | speed!=79.6         | 8      | 8     | vehicle | moto  |
      | speed<23            | 3      | 3     | vehicle | moto  |
      | speed>99.6          | 8      | 8     | vehicle | moto  |
      | speed==89..90       | 3      | 3     | vehicle | moto  |
      | speed!=79..80       | 8      | 8     | vehicle | moto  |
      | temperature_0==high | 2      | 2     | home    | room1 |
      | temperature_1!=low  | 9      | 9     | home    | room1 |


  @qp_q_limit_offset_idpattern_and_type.row<row.id>
  @qp_q_limit_offset_idpattern_and_type
  Scenario Outline:  list entities using NGSI v2 with q, limit offset, idPattern and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And create "5" entities with "2" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | high        |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    And record entity group
    And create "5" entities with "1" attributes
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And verify that receive several "Created" http code
    And record entity group
    And create an entity and attribute with special values in raw
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limitt    | <limit>        |
      | offset    | <offset>       |
      | idPattern | <idPattern>    |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | q_expression        | offset | limit | type    | idPattern    |
      | +type               | 8      | 8     | house   | roo.*        |
      | -my_attr            | 3      | 3     | house   | ^b.?         |
      | +seats              | 0      | 0     | house   | .*us.*       |
      | speed==89.6         | 3      | 3     | vehicle | .*to$        |
      | speed!=79.6         | 8      | 8     | vehicle | ^mo+.*       |
      | speed<23            | 3      | 3     | vehicle | [A-Za-z0-9]+ |
      | speed>99.6          | 8      | 8     | vehicle | \w+          |
      | speed==89..90       | 3      | 3     | vehicle | mo(to)       |
      | speed!=79..80       | 8      | 8     | vehicle | mo(\w)       |
      | temperature_0==high | 2      | 2     | home    | .*           |
      | temperature_1!=low  | 9      | 9     | home    | \a+          |

