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


Feature: get an attribute by entity ID using NGSI v2. "GET" - /v2/entities/<entity_id>/attrs/<attr_name>
  As a context broker user
  I would like to get an attribute by entity ID using NGSI v2
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
  Scenario:  get an attribute by entity ID using NGSI v2
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_id_happy_path |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    When create "1" entities with "3" attributes
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
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @more_entities
  Scenario:  try get an attribute by entity ID using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attr_more_entities |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
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
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                          |
      | error       | TooManyResults                                                 |
      | description | There is more than one entity with that id. Refine your query. |

   # ------------------------ Service ----------------------------------------------
  @service
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with several services
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  get an attribute by entity ID using NGSI v2 without service
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @service_error
  Scenario Outline:  try to get an attribute by entity ID using NGSI v2 with wrong several services
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "Bad Request" http code
    And verify an error response
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

  @service_path @BUG_1423 @skip
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with several service paths
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_attr_service_path |
      | Fiware-ServicePath | <service_path>         |
      | Content-Type       | application/json       |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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
  Scenario:  get an attribute by entity ID using NGSI v2 without service paths
    Given  a definition of headers
      | parameter      | value                          |
      | Fiware-Service | test_attr_service_path_without |
      | Content-Type   | application/json               |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @service_path_error
  Scenario Outline:  try to get an attribute by entity ID using NGSI v2 with several invalid service paths
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
    When get an attribute "temperature_0" by ID "room_0"
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
  Scenario Outline:  try to get an attribute by entity ID using NGSI v2 with several invalid service paths
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
    When get an attribute "temperature_0" by ID "room_0"
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
  Scenario Outline:  try to get an attribute by entity ID using NGSI v2 with several invalid service paths
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
    When get an attribute "temperature_0" by ID "room_0"
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
  Scenario:  try to get an attribute by entity ID using NGSI v2 with several invalid service paths
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_attr_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #    -------------- entity Id ------------------------------------------
  @entity_id
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with several entity id values between 5 entities
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_attr_entity_id |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    When create "5" entities with "3" attributes
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
    When get an attribute "temperature_0" by ID "<entity_id>"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
    Examples:
      | entity_id |
      | room_0    |
      | room_1    |
      | room_4    |
      | room_2    |

  @entity_id_one_entity
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with several entity id values with one entity
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_attr_entity_id |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    When create "1" entities with "3" attributes
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
    When get an attribute "temperature_0" by ID "the same value of the previous request"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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
      | room_19     | random=900 |


  @entity_id_unknown
  Scenario:  try to get an attribute by entity ID using NGSI v2 with an unknown entity id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_attr_entity_unkwnown |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    When create "5" entities with "3" attributes
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
    When get an attribute "temperature_0" by ID "retretert"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @entity_id_invalid
  Scenario Outline:  try to get an attribute by entity ID using NGSI v2 with invalid entity id
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    When create "5" entities with "3" attributes
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
    When get an attribute "temperature_0" by ID "<entity_id>"
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

  #    -------------- attribute name ------------------------------------------

  @attribute_name
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with several attribute name values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_id_attr_attribute_name |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    When create "1" entities with "10" attributes
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
    When get an attribute "<attribute_name>" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
    Examples:
      | attribute_name |
      | temperature_0  |
      | temperature_1  |
      | temperature_3  |
      | temperature_2  |

  @attribute_name_without_type
  Scenario Outline:  get an attribute by entity ID using NGSI v2 without attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attr_attribute_name_without_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    When create "3" entities with "3" attributes
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_1"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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

  @compound_attribute_name_without_attribute_type @BUG_1106 @skip
  Scenario Outline: get an attribute by entity ID using NGSI v2 with special attribute values and without attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attr_without_attribute_type |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And verify that receive an "Created" http code
    When get an attribute "temperature" by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an attribute by ID in raw mode with type "<type>" in attribute value from http response
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

  @attribute_name_with_type
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with attribute type
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_attr_attribute_name_with_type |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
    When create "3" entities with "3" attributes
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | attributes_type  | celsius           |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_1"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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

  @compound_attribute_name_with_attribute_type @BUG_1106 @skip
  Scenario Outline: get an attribute by entity ID using NGSI v2 with special attribute values and with attribute type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_attr_with_attribute_type |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | attributes_vtype | "celcius"          |
    And verify that receive an "Created" http code
    When get an attribute "temperature" by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an attribute by ID in raw mode with type "<type>" in attribute value from http response
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

  @attribute_name_with_metadatas
  Scenario Outline:  get an attribute by entity ID using NGSI v2 with attribute metadatas
    Given  a definition of headers
      | parameter          | value                                   |
      | Fiware-Service     | test_attr_attribute_name_with_metadatas |
      | Fiware-ServicePath | /test                                   |
      | Content-Type       | application/json                        |
    When create "3" entities with "3" attributes
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 3                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | hot               |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_1"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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

  @compound_attribute_name_with_metadatas @BUG_1106 @skip
  Scenario Outline: get an attribute by entity ID using NGSI v2 with special attribute values and with metadatas (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_attr_with_metadatas |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 3                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_type   | "alarm"            |
      | metadatas_value  | "hot"              |
    And verify that receive an "Created" http code
    When get an attribute "temperature" by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an attribute by ID in raw mode with type "<type>" in attribute value from http response
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

  @attribute_name_without_metadatas_type
  Scenario Outline:  get an attribute by entity ID using NGSI v2 without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                           |
      | Fiware-Service     | test_attr_attribute_name_without_metadatas_type |
      | Fiware-ServicePath | /test                                           |
      | Content-Type       | application/json                                |
    When create "1" entities with "3" attributes
      | parameter        | value             |
      | entities_type    | house             |
      | entities_id      | room              |
      | attributes_name  | temperature       |
      | attributes_value | <attribute_value> |
      | metadatas_number | 3                 |
      | metadatas_name   | very_hot          |
      | metadatas_value  | hot               |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
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

  @compound_attribute_name_with_metadatas_without_meta_type @BUG_1106 @skip
  Scenario Outline: get an attribute by entity ID using NGSI v2 with special attribute values and with metadatas, but without metadata type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_attr_with_metadatas |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    And  create an entity and attribute with special values in raw
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
      | metadatas_number | 3                  |
      | metadatas_name   | "very_hot"         |
      | metadatas_value  | "hot"              |
    And verify that receive an "Created" http code
    When get an attribute "temperature" by ID <entity_id>
    Then verify that receive an "OK" http code
    And verify an attribute by ID in raw mode with type "<type>" in attribute value from http response
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

  @attribute_name_unknown
  Scenario:  try to get an attribute by entity ID using NGSI v2 with unknown attribute name
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attr_attribute_name_unknown |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "fgdfgdfgdf" by ID "room"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | NotFound                                   |
      | description | The entity does not have such an attribute |
