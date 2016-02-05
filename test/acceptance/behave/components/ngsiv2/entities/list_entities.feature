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
#  Note: the "skip" tag is to skip the scenarios that still are not developed or failed:
#        -t=-skip
#        the "too_slow" tag is used to mark scenarios that running are too slow, if would you like to skip these scenarios:
#        -t=-too_slow


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
  Scenario:  list all entities using NGSI v2 (3 entity groups with 11 entities in total)
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_happy_path  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter         | value                   |
      | entities_type     | house                   |
      | entities_id       | room2                   |
      | attributes_number | 2                       |
      | attributes_name   | timestamp               |
      | attributes_value  | 017-06-17T07:21:24.238Z |
      | attributes_type   | date                    |
      | metadatas_number  | 2                       |
      | metadatas_name    | very_hot                |
      | metadatas_type    | alarm                   |
      | metadatas_value   | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value      |
      | entities_type    | "car"      |
      | entities_id      | "vehicle"  |
      | attributes_name  | "speed"    |
      | attributes_value | 89         |
      | attributes_type  | "km_h"     |
      | metadatas_name   | "very_hot" |
      | metadatas_type   | "alarm"    |
      | metadatas_value  | "hot"      |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | limit     | 9        |
      | offset    | 0        |
      | type      | car      |
      | id        | vehicle  |
      | q         | speed>78 |
      | options   | count    |
    Then verify that receive an "OK" http code
    And verify that "1" entities are returned
    And verify headers in response
      | parameter     | value |
      | x-total-count | 1     |

  @id_multiples @BUG_1720
  Scenario:  try to list all entities using NGSI v2 (3 entity groups with 15 entities in total) but with id query parameter used as regexp
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_id_multiples |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter         | value                   |
      | entities_type     | house                   |
      | entities_id       | room2                   |
      | attributes_number | 2                       |
      | attributes_name   | timestamp               |
      | attributes_value  | 017-06-17T07:21:24.238Z |
      | attributes_type   | date                    |
      | metadatas_number  | 2                       |
      | metadatas_name    | very_hot                |
      | metadatas_type    | alarm                   |
      | metadatas_value   | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter         | value    |
      | entities_type     | car      |
      | entities_id       | vehicle  |
      | attributes_number | 2        |
      | attributes_name   | speed    |
      | attributes_value  | 89       |
      | attributes_type   | kmh      |
      | metadatas_number  | 2        |
      | metadatas_name    | very_hot |
      | metadatas_type    | alarm    |
      | metadatas_value   | hot      |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | limit     | 9       |
      | offset    | 0       |
      | type      | car     |
      | id        | vehicle |
      | options   | count   |
    Then verify that receive an "OK" http code
    And verify that "0" entities are returned

   # ------------------------ Service ----------------------------------------------
  @service.row<row.id>
  @service
  Scenario Outline:  list all entities using NGSI v2 with several services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

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
  @service_path.row<row.id>
  @service_path @BUG_1423
  Scenario Outline:  list all entities using NGSI v2 with several service paths headers
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath | <service_path>    |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

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

  # -------------- Attribute value -----------------------

  @attribute_value_without_attribute_type.row<row.id>
  @attribute_value_without_attribute_type
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and without attribute type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value             |
      | entities_type    | home              |
      | entities_id      | room1             |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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
      | random=500000           |
      | random=1000000          |

  @attribute_value_compound_without_attribute_type.row<row.id>
  @attribute_value_compound_without_attribute_type @BUG_1106
  Scenario Outline:  list an entity using NGSI v2 with special attribute values and without attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value              |
      | entities_type    | "home"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And create an entity in raw and "normalized" modes
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
      | "room15"  | "41.3763726, 2.1864475, 14"                                                   | str      |
      | "room16"  | "2017-06-17T07:21:24.238Z"                                                    | str      |
      | "room17"  | null                                                                          | NoneType |

  @attribute_value_with_attribute_type
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and attribute type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_list_with_attribute_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value             |
      | entities_type     | home              |
      | entities_id       | room1             |
      | attributes_number | 2                 |
      | attributes_name   | temperature       |
      | attributes_value  | <attribute value> |
      | attributes_type   | celcius           |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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

  @attribute_value_compound_with_attribute_type @BUG_1106
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value              |
      | entities_type    | "home"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_type  | "celcius"          |
    And create an entity in raw and "normalized" modes
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

  @attribute_value_with_metadatas.row<row.id>
  @attribute_value_with_metadatas
  Scenario Outline:  list all entities using NGSI v2 with several attribute values and with metadatas
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_list_attr_w_meta |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value             |
      | entities_type     | home              |
      | entities_id       | room1             |
      | attributes_number | 2                 |
      | attributes_name   | temperature       |
      | attributes_value  | <attribute_value> |
      | metadatas_number  | 2                 |
      | metadatas_name    | very_hot          |
      | metadatas_type    | alarm             |
      | metadatas_value   | random=10         |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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
      | random=500000           |

  @attribute_value_compound_with_metadata @BUG_1106
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and metadatas (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value              |
      | entities_type    | "home"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    And create an entity in raw and "normalized" modes
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

  @attribute_value_without_metadata_type
  Scenario Outline:  list all entities using NGSI v2 without metadata type
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_list_without_metadata_type |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value             |
      | entities_type     | home              |
      | entities_id       | room1             |
      | attributes_number | 2                 |
      | attributes_name   | temperature       |
      | attributes_value  | <attribute_value> |
      | metadatas_number  | 2                 |
      | metadatas_name    | very_hot          |
      | metadatas_value   | random=10         |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
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
      | random=500000           |

  @compound_with_metadata_without_meta_type @BUG_1106
  Scenario Outline:  list all entities using NGSI v2 with special attribute values and metatadas but without metadata type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value              |
      | entities_type    | "home"             |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 2                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_value  | "hot"              |
    And create an entity in raw and "normalized" modes
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
  @only_limit.row<row.id>
  @only_limit
  Scenario Outline:  list all entities using NGSI v2 with only limit query parameter
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "<value>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | limit     | <value> |
    Then verify that receive an "OK" http code
    And verify that "<value>" entities are returned
    Examples:
      | value |
      | 1     |
      | 10    |
      | 50    |

  @only_limit @too_slow
  Scenario Outline:  list all entities using NGSI v2 with only limit query parameter
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "<value>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | limit     | <value> |
    Then verify that receive an "OK" http code
    And verify that "<value>" entities are returned
    Examples:
      | value |
      | 100   |
      | 500   |
      | 1000  |

  @pag_max_without_limit
  Scenario:  list all entities using NGSI v2 without limit query parameter and the pagination by defauult (20)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "21" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
    Then verify that receive an "OK" http code
    And verify that "20" entities are returned

  @only_limit_max_exceed @too_slow
  Scenario:  list all entities using NGSI v2 with only limit query parameter but with pagination max exceed (1000 entities)
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "1001" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | limit     | 1001  |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | Bad pagination limit: /1001/ [max: 1000] |

  @only_limit_error
  Scenario Outline:  try to list all entities using NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_list_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | offset | returned |
      | 0      | 10       |
      | 1      | 9        |
      | 20     | 0        |

  @only_offset_error
  Scenario Outline:  try to list all entities using NGSI v2 with only wrong offset query parameter
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_only_offset_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | limit | offset | returned |
      | 1     | 1      | 1        |
      | 20    | 1      | 9        |
      | 1     | 20     | 0        |
      | 5     | 3      | 5        |
      | 10    | 0      | 10       |

  # --- id, id pattern and type ---
  @only_id.row<row.id>
  @only_id @BUG_1720
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | bedroom     |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | bathroom                |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value             |
      | entities_type    | "<entities_type>" |
      | entities_id      | "<entities_id>"   |
      | attributes_name  | "speed"           |
      | attributes_value | 89                |
      | attributes_type  | "km_h"            |
      | metadatas_name   | "very_hot"        |
      | metadatas_type   | "alarm"           |
      | metadatas_value  | "hot"             |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | id        | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that "1" entities are returned
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
      | room_17       | random=10   |
      | room_18       | random=100  |
      | room_19       | random=256  |

  @only_id_list.row<row.id>
  @only_id_list
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter with a list of ids
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | 2           |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | 2                       |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | 2         |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | id        | <id>  |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | id                               | returned |
      | room1_0                          | 1        |
      | room1_0,room2_0                  | 2        |
      | room1_1,room2_1,car_1            | 3        |
      | room1_0,room2_3,car_2,sdfsdfsd_3 | 3        |

  @only_id_same.row<row.id>
  @only_id_same
  Scenario Outline:  list all entities using NGSI v2 with only id query parameter and the same entity id in several entities
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value           |
      | entities_type    | "home"          |
      | entities_id      | "<entities_id>" |
      | attributes_name  | "temperature"   |
      | attributes_value | 34              |
      | attributes_type  | "celcius"       |
      | metadatas_name   | "very_hot"      |
      | metadatas_type   | "alarm"         |
      | metadatas_value  | "hot"           |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | "house"                                |
      | entities_id      | the same value of the previous request |
      | attributes_name  | "timestamp"                            |
      | attributes_value | "017-06-17T07:21:24.238Z"              |
      | attributes_type  | "date"                                 |
      | metadatas_name   | "very_hot"                             |
      | metadatas_type   | "alarm"                                |
      | metadatas_value  | "hot"                                  |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | "vehicle"                              |
      | entities_id      | the same value of the previous request |
      | attributes_name  | "speed"                                |
      | attributes_value | 80                                     |
      | attributes_type  | "km_h"                                 |
      | metadatas_name   | "very_hot"                             |
      | metadatas_type   | "alarm"                                |
      | metadatas_value  | "hot"                                  |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | id        | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
    Examples:
      | entities_id |
      | vehicle     |
      | 34          |
      | false       |
      | true        |
      | 34.4E-34    |
      | temp.34     |
      | temp_34     |
      | temp-34     |
      | house_flat  |
      | house.flat  |
      | house-flat  |
      | house@flat  |
      | random=10   |
      | random=100  |
      | random=256  |

  @only_id_unknown
  Scenario:  list any entity using NGSI v2 with only id query parameter but unknown value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | bedroom     |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | id        | fdgfdgfdgfd |
    Then verify that receive an "OK" http code
    And verify that "0" entities are returned

  @only_id_invalid
  Scenario Outline:  try to list all entities using NGSI v2 with only id query parameter but invalid value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_only_id |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | bedroom     |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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
    And properties to entities
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | bedroom         |
      | attributes_name  | temperature     |
      | attributes_value | 34              |
      | attributes_type  | celsius         |
      | metadatas_number | 2               |
      | metadatas_name   | very_hot        |
      | metadatas_type   | alarm           |
      | metadatas_value  | random=10       |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | the same value of the previous request |
      | entities_id      | bathroom                               |
      | attributes_name  | temperature                            |
      | attributes_value | 27                                     |
      | attributes_type  | celsius                                |
      | metadatas_number | 2                                      |
      | metadatas_name   | very_hot                               |
      | metadatas_type   | alarm                                  |
      | metadatas_value  | random=10                              |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | the same value of the previous request |
      | entities_id      | hall                                   |
      | attributes_name  | temperature                            |
      | attributes_value | 31                                     |
      | attributes_type  | celsius                                |
      | metadatas_number | 2                                      |
      | metadatas_name   | very_hot                               |
      | metadatas_type   | alarm                                  |
      | metadatas_value  | random=10                              |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | type      | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that "15" entities are returned
    Examples:
      | entities_type |
      | vehicle       |
      | 34            |
      | false         |
      | true          |
      | 34.4E-34      |
      | temp.34       |
      | temp_34       |
      | temp-34       |
      | TEMP34        |
      | temp_flat     |
      | temp.flat     |
      | temp-flat     |
      | temp@flat     |
      | random=10     |
      | random=100    |
      | random=256    |

  @only_type_list.row<row.id>
  @only_type_list @BUG_1749 @skip
  Scenario Outline:  list all entities using NGSI v2 with only type query parameter with a list of types
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room1       |
      | attributes_name  | 2           |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room2                   |
      | attributes_name  | 2                       |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | 2         |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value  |
      | type      | <type> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | type                        | returned |
      | home                        | 5        |
      | house,home                  | 10       |
      | home,house,vehicle          | 15       |
      | house,home,vehicle,sdfsdfsd | 15       |

  @only_type_same.row<row.id>
  @only_type_same
  Scenario Outline:  list all entities using NGSI v2 with only type query parameter and the same entity type in several entities
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_same_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value             |
      | entities_type    | "<entities_type>" |
      | entities_id      | "bedroom"         |
      | attributes_name  | "temperature"     |
      | attributes_value | 34                |
      | attributes_type  | "celcius"         |
      | metadatas_name   | "very_hot"        |
      | metadatas_type   | "alarm"           |
      | metadatas_value  | "hot"             |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | the same value of the previous request |
      | entities_id      | "bathroom"                             |
      | attributes_name  | "timestamp"                            |
      | attributes_value | "017-06-17T07:21:24.238Z"              |
      | attributes_type  | "date"                                 |
      | metadatas_name   | "very_hot"                             |
      | metadatas_type   | "alarm"                                |
      | metadatas_value  | "hot"                                  |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                  |
      | entities_type    | the same value of the previous request |
      | entities_id      | "car"                                  |
      | attributes_name  | "speed"                                |
      | attributes_value | 80                                     |
      | attributes_type  | "km_h"                                 |
      | metadatas_name   | "very_hot"                             |
      | metadatas_type   | "alarm"                                |
      | metadatas_value  | "hot"                                  |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value                                  |
      | type      | the same value of the previous request |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
    Examples:
      | entities_type |
      | vehicle       |
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
      | random=256    |

  @only_type_unknown
  Scenario:  list any entity using NGSI v2 with only type query parameter but unknown value
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | bedroom     |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value       |
      | type      | fdgfdgfdgfd |
    Then verify that receive an "OK" http code
    And verify that "0" entities are returned

  @only_type_invalid
  Scenario Outline:  try to list all entities using NGSI v2 with only type query parameter but invalid value
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_list_only_type |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | bedroom     |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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

  @only_idPattern.row<row.id>
  @only_idPattern
  Scenario Outline:  list all entities using NGSI v2 with only idPattern query parameter
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value         |
      | entities_type    | nothing       |
      | entities_id      | <entities_id> |
      | attributes_name  | speed         |
      | attributes_value | 89            |
      | attributes_type  | km_h          |
      | metadatas_number | 2             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | random=10     |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value        |
      | idPattern | <id_pattern> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | entities_id | id_pattern   | returned |
      | vehicle     | veh.*        | 5        |
      | 34          | ^3.?         | 5        |
      | false       | .*als.*      | 5        |
      | true        | .*ru.*       | 5        |
      | 34.4E-34    | .*34         | 5        |
      | tete.34     | ^te+.*       | 5        |
      | aBc56       | [A-Za-z0-9]* | 15       |
      | defG        | \a*          | 15       |
      | def45       | \w*          | 15       |
      | 23445       | \d*          | 15       |
      | afaffasf    | \D*          | 15       |
      | houseflat   | house(flat)  | 5        |
      | houseflat   | house(f*)    | 5        |
      | ewrwer      | .*           | 15       |

  @only_idPattern_unknown
  Scenario:  list zero entity using NGSI v2 with only idPattern query parameter but the pattern is unknown
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value   |
      | idPattern | dfgdg.* |
    Then verify that receive an "OK" http code
    And verify that "0" entities are returned

  @id_and_id_pattern
  Scenario:  try to list all entities using NGSI v2 with id and idPattern queries parameters (incompatibles)
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_list_only_id_pattern |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
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
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | <type>    |
      | entities_id      | <id>      |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                |
      | id        | <id>_0,<id>_1,<id>_4 |
      | type      | <type>               |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned
    Examples:
      | id        | type    |
      | door      | house   |
      | car       | vehicle |
      | light     | park    |
      | semaphore | street  |

  @idPattern_type.row<row.id>
  @idPattern_type
  Scenario Outline:  list all entities using NGSI v2 with idPattern and type queries parameters
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_id_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | home      |
      | entities_id      | hall      |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value        |
      | idPattern | <id_pattern> |
      | type      | <type>       |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | id_pattern | type  | returned |
      | room1.*    | home  | 0        |
      | room2.*    | home  | 5        |
      | room2.*    | house | 0        |
      | ^hall      | home  | 5        |
      | \w*        | home  | 10       |

  @id_type_limit_offset.row<row.id>
  @id_type_limit_offset
  Scenario Outline:  list all entities using NGSI v2 with id, type, limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_list_id_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 78        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "10" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
      | id        | <id>     |
      | type      | <type>   |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | limit | offset | id                                      | type    | returned |
      | 1     | 0      | car_0                                   | vehicle | 1        |
      | 20    | 1      | car_1,car_2,car_3,car_4,car_5           | vehicle | 4        |
      | 1     | 20     | room2_0                                 | house   | 0        |
      | 5     | 3      | room2_7,room2_5,room2_8,room2_9,room2_4 | house   | 2        |

  # --- q = <expression> ---
  @only_q_type @BUG_1587 @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=type query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_id      | room3       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression | returned |
      | type         | 13       |
      | !type        | 6        |

  @only_q_attribute @BUG_1589 @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with only q=attribute query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value       |
      | entities_id      | room3       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression      | returned |
      | myAttr            | 0        |
      | !myAttr           | 19       |
      | roses             | 1        |
      | !speed;!timestamp | 11       |

  @only_q_parse_error.row<row.id>
  @only_q_parse_error @BUG_1592 @BUG_1754 @ISSUE_1751 @skip
  Scenario Outline:  try to list entities using NGSI v2 with only q query parameter but with parse errors
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid query expression |
    Examples:
      | q_expression |
      | speed==      |
      | speed!=      |
      | speed>=      |
      | speed<=      |
      | speed>       |
      | speed<       |
      | ==speed      |
      | !=speed      |
      | >=speed      |
      | <=speed      |
      | >speed       |
      | <speed       |
      | speed+       |
      | +speed       |
      | speed!       |
      | speed*       |
      | speed-       |
      | speed+       |
      | *speed       |
      | +type        |
      | type!        |

  @only_q_value_boolean @BUG_1594
  Scenario Outline:  list entities using NGSI v2 with only q=attr==true query parameter (boolean values)
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value             |
      | entities_type    | "vehicle"         |
      | entities_id      | "car"             |
      | attributes_name  | "temperature"     |
      | attributes_value | <attribute_value> |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that "1" entities are returned
    Examples:
      | q_expression       | attribute_value |
      | temperature==true  | true            |
      | temperature==false | false           |

  @only_q_string_numbers.row<row.id>
  @only_q_string_numbers @BUG_1595
  Scenario Outline:  list entities using NGSI v2 with only q=attr=="89" query parameter (number in string)
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And properties to entities
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89        |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression | returned |
      | speed=='89'  | 5        |
      | speed==89    | 1        |

  @only_q_operators_errors @BUG_1607
  Scenario Outline:  try to list entities using NGSI v2 with only q query parameter, with range, but wrong operators
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | car       |
      | attributes_name  | speed     |
      | attributes_value | 89        |
      | attributes_type  | km_h      |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89        |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    And get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid query expression |
    Examples:
      | q_expression  |
      | speed>=69..90 |
      | speed>69..90  |
      | speed<=69..79 |
      | speed<99..190 |

  @only_q.row<row.id>
  @only_q @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with only q query parameter with several statements
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | vehicle   |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value  |
      | entities_id      | garden |
      | attributes_name  | roses  |
      | attributes_value | red    |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "car"   |
      | attributes_name  | "speed" |
      | attributes_value | 79.2    |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | simple  |
      | attributes_name  | comma   |
      | attributes_value | one,two |
      | attributes_type  | celsius |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression                      | returned |
      | !type                             | 8        |
      | type                              | 23       |
      | !myAttr                           | 31       |
      | !type;!myAttr                     | 8        |
      | type;!myAttr                      | 23       |
      | speed==89.6                       | 1        |
      | speed==89.6;!myAttr               | 1        |
      | speed==89.6;!type                 | 1        |
      | speed==89.6;type                  | 0        |
      | speed!=99.6                       | 2        |
      | speed>=89.6                       | 1        |
      | speed<=89.6                       | 1        |
      | speed<89.6                        | 1        |
      | speed>79.6                        | 1        |
      | speed!=99.6;!myAttr               | 2        |
      | speed>=89.6;!type                 | 1        |
      | speed>=89.6;type                  | 0        |
      | speed<=89.6;!myAttr;!type         | 2        |
      | speed<=89.6;!myAttr;type          | 2        |
      | speed<89.6;!type                  | 1        |
      | speed<89.6;type                   | 0        |
      | speed>79.6                        | 1        |
      | speed!=23..56                     | 2        |
      | speed!=90..100                    | 0        |
      | speed==79..90                     | 2        |
      | speed==79..85                     | 1        |
      | speed!=23..56;!type;!myAttr       | 2        |
      | speed!=23..56;type;!myAttr        | 0        |
      | speed!=90..100;!myAttr            | 2        |
      | speed==79..90;!type               | 2        |
      | speed==79..90;type                | 0        |
      | speed==79..85;!myAttr;!type       | 2        |
      | speed==79..85;!myAttr;type        | 0        |
      | humidity==high,low,medium         | 8        |
      | humidity!=high,low                | 3        |
      | humidity==high                    | 5        |
      | humidity!=high                    | 3        |
      | humidity==high,low,medium;!myAttr | 8        |
      | humidity!=high,low;!myAttr        | 3        |
      | humidity==high;!myAttr            | 5        |
      | humidity!=high;-myAttr            | 3        |
      | humidity!=high;!myAttr;!type      | 0        |
      | humidity!=high;!myAttr;type       | 3        |
      | comma=='one,two',high             | 10       |
      | comma!='three,four',high          | 8        |
      | comma=='one,two',high;!myAttr     | 10       |
      | comma=='one,two';!myAttr;!type    | 5        |
      | comma=='one,two';!myAttr;type     | 0        |

  @qp_q_and_limit.row<row.id>
  @qp_q_and_limit @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q and limit queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "6" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | limit | returned |
      | !type               | 3     | 3        |
      | type                | 8     | 7        |
      | !my_attr            | 3     | 3        |
      | seats               | 5     | 5        |
      | speed==89.6         | 3     | 1        |
      | speed!=79.6         | 8     | 1        |
      | speed<23            | 3     | 0        |
      | speed>99.6          | 8     | 0        |
      | speed==89..90       | 3     | 1        |
      | speed!=79..80       | 8     | 1        |
      | temperature_0==high | 2     | 1        |
      | temperature_1!=low  | 9     | 1        |

  @qp_q_and_offset.row<row.id>
  @qp_q_and_offset @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q and offset queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | offset    | <offset>       |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | returned |
      | !type               | 3      | 3        |
      | type                | 8      | 2        |
      | !my_attr            | 8      | 8        |
      | seats               | 2      | 3        |
      | speed==89.6         | 3      | 0        |
      | speed!=79.6         | 0      | 1        |
      | speed<23            | 3      | 0        |
      | speed>99.6          | 8      | 0        |
      | speed==89..90       | 1      | 0        |
      | speed!=79..80       | 0      | 1        |
      | temperature_0==high | 2      | 0        |
      | temperature_1!=low  | 0      | 1        |

  @qp_q_limit_and_offset.row<row.id>
  @qp_q_limit_and_offset @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q, limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
      | offset    | <offset>       |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | limit | returned |
      | !type               | 3      | 3     | 3        |
      | type                | 8      | 8     | 2        |
      | !my_attr            | 3      | 6     | 6        |
      | seats               | 2      | 8     | 3        |
      | speed==89.6         | 3      | 3     | 0        |
      | speed!=79.6         | 0      | 8     | 1        |
      | speed<23            | 3      | 3     | 0        |
      | speed>99.6          | 0      | 1     | 0        |
      | speed==89..90       | 0      | 3     | 1        |
      | speed!=79..80       | 8      | 8     | 0        |
      | temperature_0==high | 2      | 2     | 0        |
      | temperature_1!=low  | 0      | 9     | 1        |

  @qp_q_limit_offset_and_id.row<row.id>
  @qp_q_limit_offset_and_id @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q, limit offset and id queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
      | offset    | <offset>       |
      | id        | <id>           |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | limit | id    | returned |
      | !type               | 3      | 3     | bus   | 2        |
      | type                | 4      | 8     | room2 | 6        |
      | !my_attr            | 3      | 3     | bus   | 13       |
      | seats               | 1      | 8     | bus   | 4        |
      | speed==89.6         | 0      | 3     | moto  | 1        |
      | speed!=79.6         | 8      | 8     | car   | 0        |
      | speed<23            | 3      | 3     | moto  | 0        |
      | speed>99.6          | 8      | 8     | moto  | 0        |
      | speed==89..90       | 1      | 3     | moto  | 0        |
      | speed!=79..80       | 8      | 8     | moto  | 0        |
      | temperature_0==high | 0      | 2     | room1 | 1        |
      | temperature_1!=low  | 0      | 9     | room3 | 0        |

  @qp_q_limit_offset_and_type.row<row.id>
  @qp_q_limit_offset_and_type @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q, limit offset and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
      | offset    | <offset>       |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | limit | type    | returned |
      | type                | 2      | 8     | house   | 3        |
      | !my_attr            | 3      | 3     | house   | 2        |
      | seats               | 0      | 1     | house   | 0        |
      | speed==89.6         | 0      | 3     | vehicle | 1        |
      | speed!=79.6         | 8      | 8     | vehicle | 0        |
      | speed<23            | 3      | 3     | vehicle | 0        |
      | speed>99.6          | 8      | 8     | vehicle | 0        |
      | speed==89..90       | 0      | 3     | vehicle | 1        |
      | speed!=79..80       | 8      | 8     | vehicle | 0        |
      | temperature_0==high | 0      | 2     | home    | 1        |
      | temperature_1!=low  | 9      | 9     | home    | 0        |

  @qp_q_limit_offset_id_and_type.row<row.id>
  @qp_q_limit_offset_id_and_type @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q, limit offset, id and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | bus       |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
      | offset    | <offset>       |
      | id        | <id>           |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | limit | type    | id    | returned |
      | type                | 2      | 8     | house   | room2 | 3        |
      | !my_attr            | 3      | 3     | house   | bus   | 3        |
      | seats               | 0      | 1     | house   | bus   | 0        |
      | speed==89.6         | 0      | 3     | vehicle | moto  | 1        |
      | speed!=79.6         | 2      | 8     | vehicle | moto  | 0        |
      | speed<23            | 3      | 3     | vehicle | moto  | 0        |
      | speed>99.6          | 8      | 8     | vehicle | moto  | 0        |
      | speed==89..90       | 3      | 3     | vehicle | car   | 0        |
      | speed!=79..80       | 0      | 8     | vehicle | moto  | 1        |
      | temperature_0==high | 0      | 1     | home    | room1 | 1        |
      | temperature_1!=low  | 9      | 9     | home    | room1 | 0        |

  @qp_q_limit_offset_idpattern_and_type.row<row.id>
  @qp_q_limit_offset_idpattern_and_type @ISSUE_1751 @skip
  Scenario Outline:  list entities using NGSI v2 with q, limit offset, idPattern and type queries parameters
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_list_only_q |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                   |
      | entities_type    | house                   |
      | entities_id      | room3                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_id      | room4     |
      | attributes_name  | seats     |
      | attributes_value | 37        |
      | metadatas_number | 2         |
      | metadatas_name   | very_hot  |
      | metadatas_type   | alarm     |
      | metadatas_value  | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value   |
      | entities_id      | "moto"  |
      | attributes_name  | "speed" |
      | attributes_value | 89.6    |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value     |
      | entities_type    | "vehicle" |
      | entities_id      | "moto"    |
      | attributes_name  | "speed"   |
      | attributes_value | 89.6      |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    When get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | limit     | <limit>        |
      | offset    | <offset>       |
      | idPattern | <idPattern>    |
      | type      | <type>         |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression        | offset | limit | type    | idPattern    | returned |
      | type                | 8      | 8     | house   | roo.*        | 2        |
      | !my_attr            | 3      | 3     | house   | ^r.*         | 3        |
      | seats               | 0      | 1     | house   | .*us.*       | 0        |
      | speed==89.6         | 0      | 3     | vehicle | .*to$        | 1        |
      | speed!=79.6         | 8      | 8     | vehicle | ^mo+.*       | 0        |
      | speed<23            | 3      | 3     | vehicle | [A-Za-z0-9]* | 0        |
      | speed>99.6          | 8      | 8     | vehicle | \w*          | 0        |
      | speed==89..90       | 0      | 3     | vehicle | mo(to)       | 1        |
      | speed!=79..80       | 0      | 8     | vehicle | mo(\w)       | 1        |
      | temperature_0==high | 0      | 2     | home    | .*           | 1        |
      | temperature_1!=low  | 9      | 9     | home    | \a*          | 0        |

  # --- options = count ---
  @qp_options_count.row<row.id>
  @qp_options_count
  Scenario Outline:  list entities using NGSI v2 with options=count query parameter only
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "<entities>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    And verify headers in response
      | parameter     | value      |
      | x-total-count | <entities> |
    Examples:
      | entities | returned |
      | 1        | 1        |
      | 5        | 5        |
      | 21       | 20       |

  @qp_options_count_and_limit
  Scenario Outline:  list entities using NGSI v2 with options=count and limit queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "<entities>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value      |
      | options   | count      |
      | limit     | <entities> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    And verify headers in response
      | parameter     | value      |
      | x-total-count | <entities> |
    Examples:
      | entities | returned |
      | 1        | 1        |
      | 5        | 5        |
      | 30       | 30       |

  @qp_options_count_offset_and_limit.row<row.id>
  @qp_options_count_offset_and_limit
  Scenario Outline:  list entities using NGSI v2 with options=count, offset and limit queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
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
    And create entity group with "<entities>" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value    |
      | options   | count    |
      | limit     | <limit>  |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    And verify headers in response
      | parameter     | value      |
      | x-total-count | <entities> |
    Examples:
      | entities | offset | limit | returned |
      | 1        | 1      | 1     | 0        |
      | 5        | 2      | 5     | 3        |
      | 30       | 10     | 10    | 10       |


