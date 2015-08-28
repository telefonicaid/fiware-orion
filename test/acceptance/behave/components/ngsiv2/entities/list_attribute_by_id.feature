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


Feature: get an attribute by entity ID in NGSI v2
  As a context broker user
  I would like to get an attribute by entity ID in NGSI v2
  So that I can manage and use them in my scripts

  BackgroundFeature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file and restart service
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  @happy_path
  Scenario:  get an attribute by entity ID in NGSI v2
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
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @more_entities
  Scenario:  try get an attribute by entity ID in NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attr_more_entities |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                          |
      | error       | TooManyResults                                                 |
      | description | There is more than one entity with that id. Refine your query. |

   # ------------------------ Service ----------------------------------------------
  @service
  Scenario Outline:  get an attribute by entity ID in NGSI v2 with several services
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
    When get an attribute "temperature_0" by ID "room_0"
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
  Scenario:  get an attribute by entity ID in NGSI v2 without service
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
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @service_error
  Scenario Outline:  try to get an attribute by entity ID in NGSI v2 with wrong several services
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

  @service_path
  Scenario Outline:  get an attribute by entity ID in NGSI v2 with several service paths
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
    When get an attribute "temperature_0" by ID "room_0"
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
  Scenario:  get an attribute by entity ID in NGSI v2 without service paths
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
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @service_path_error
  Scenario Outline:  try to get an attribute by entity ID in NGSI v2 with several invalid service paths
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
  Scenario Outline:  try to get an attribute by entity ID in NGSI v2 with several invalid service paths
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
  Scenario Outline:  try to get an attribute by entity ID in NGSI v2 with several invalid service paths
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
  Scenario:  try to get an attribute by entity ID in NGSI v2 with several invalid service paths
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
  Scenario Outline:  get an attribute by entity ID in NGSI v2 with several entity id values
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
      | room_2    |

  @entity_id_unknown
  Scenario:  try to get an attribute by entity ID in NGSI v2 with an unknown entity id
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
  Scenario Outline:  try to get an attribute by entity ID in NGSI v2 with invalid entity id
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
  Scenario Outline:  get an attribute by entity ID in NGSI v2 with several attribute name values
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
    When get an attribute "<attribute_name>" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned
    Examples:
      | attribute_name |
      | temperature_0  |
      | temperature_1  |
      | temperature_3  |
      | temperature_2  |

  @attribute_name_without_type
  Scenario:  get an attribute by entity ID in NGSI v2 without attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attr_attribute_name_without_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @attribute_name_with_metadatas
  Scenario:  get an attribute by entity ID in NGSI v2 with attribute metadatas
    Given  a definition of headers
      | parameter          | value                                   |
      | Fiware-Service     | test_attr_attribute_name_with_metadatas |
      | Fiware-ServicePath | /test                                   |
      | Content-Type       | application/json                        |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 3           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @attribute_name_without_metadatas_type
  Scenario:  get an attribute by entity ID in NGSI v2 without attribute metadata type
    Given  a definition of headers
      | parameter          | value                                           |
      | Fiware-Service     | test_attr_attribute_name_without_metadatas_type |
      | Fiware-ServicePath | /test                                           |
      | Content-Type       | application/json                                |
    When create "1" entities with "3" attributes
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 3           |
      | metadatas_name   | very_hot    |
      | metadatas_value  | hot         |
    And verify that receive several "Created" http code
    When get an attribute "temperature_0" by ID "room_0"
    Then verify that receive an "OK" http code
    And verify that the attribute by ID is returned

  @attribute_name_unknown
  Scenario:  try to get an attribute by entity ID in NGSI v2 with unknown attribute name
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
    When get an attribute "fgdfgdfgdf" by ID "room_0"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | NotFound                                   |
      | description | The entity does not have such an attribute |