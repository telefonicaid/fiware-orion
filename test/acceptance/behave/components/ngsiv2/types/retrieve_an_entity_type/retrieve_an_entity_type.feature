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


Feature: get an entity type using NGSI v2 API. "GET" - /v2/types/<entity_type>
  As a context broker user
  I would like to get entity types using NGSI v2 API
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
  Scenario:  get an entity type by type using NGSI v2 API
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_type_happy_path |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And initialize entity groups recorder
    # These properties below are used in create request
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
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room2       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    # These properties below are used in create request
    And properties to entities
      | parameter         | value    |
      | entities_type     | house    |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | pressure |
      | attributes_value  | low      |
      | attributes_type   | bar      |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                    |
      | entities_id       | room3                    |
      | attributes_number | 2                        |
      | attributes_name   | timestamp                |
      | attributes_value  | 2017-06-17T07:21:24.238Z |
      | attributes_type   | date                     |
      | metadatas_number  | 2                        |
      | metadatas_name    | very_hot                 |
      | metadatas_type    | alarm                    |
      | metadatas_value   | random=10                |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    # These properties below are used in create request
    And properties to entities
      | parameter         | value     |
      | entities_type     | car       |
      | entities_id       | vehicle   |
      | attributes_number | 2         |
      | attributes_name   | brake     |
      | attributes_value  | 25        |
      | attributes_type   | seconds   |
      | metadatas_number  | 2         |
      | metadatas_name    | very_hot  |
      | metadatas_type    | alarm     |
      | metadatas_value   | random=10 |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    # These properties below are used in create request
    And properties to entities
      | parameter         | value |
      | entities_type     | car   |
      | entities_id       | moto  |
      | attributes_number | 2     |
      | attributes_name   | speed |
      | attributes_value  | 45    |
      | attributes_type   | km_h  |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "home"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder

  # ------------------------ Service header ----------------------------------------------
  @service
  Scenario Outline:  get an entity type by type using NGSI v2 API with several services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
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
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                    |
      | entities_id       | room2                    |
      | attributes_number | 2                        |
      | attributes_name   | timestamp                |
      | attributes_value  | 2017-06-17T07:21:24.238Z |
      | attributes_type   | date                     |
      | metadatas_number  | 2                        |
      | metadatas_name    | very_hot                 |
      | metadatas_type    | alarm                    |
      | metadatas_value   | random=10                |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "home"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  get an entity type by type using NGSI v2 API without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
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
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                    |
      | entities_id       | room2                    |
      | attributes_number | 2                        |
      | attributes_name   | timestamp                |
      | attributes_value  | 2017-06-17T07:21:24.238Z |
      | attributes_type   | date                     |
      | metadatas_number  | 2                        |
      | metadatas_name    | very_hot                 |
      | metadatas_type    | alarm                    |
      | metadatas_value   | random=10                |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "home"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder

  @service_error
  Scenario Outline:  try to get an entity type by type using NGSI v2 with several wrong services headers
    Given  a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When get an entity type by type "home"
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
  Scenario:  try to get an entity type by type using NGSI v2 with bad length services headers
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
    When get an entity type by type "home"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

 # ------------------------ Service path header --------------------------------------
  @service_path
  Scenario Outline: get an entity type by type using NGSI v2 with several service paths headers
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_type_service_path |
      | Fiware-ServicePath | <service_path>         |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    # These properties below are used in create request
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
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                    |
      | entities_id       | room2                    |
      | attributes_number | 2                        |
      | attributes_name   | timestamp                |
      | attributes_value  | 2017-06-17T07:21:24.238Z |
      | attributes_type   | date                     |
      | metadatas_number  | 2                        |
      | metadatas_name    | very_hot                 |
      | metadatas_type    | alarm                    |
      | metadatas_value   | random=10                |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "home"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder
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
  Scenario:  get an entity type by type using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                  |
      | Fiware-Service | test_type_service_path |
      | Content-Type   | application/json       |
    And initialize entity groups recorder
    # These properties below are used in create request
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
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                    |
      | entities_id       | room2                    |
      | attributes_number | 2                        |
      | attributes_name   | timestamp                |
      | attributes_value  | 2017-06-17T07:21:24.238Z |
      | attributes_type   | date                     |
      | metadatas_number  | 2                        |
      | metadatas_name    | very_hot                 |
      | metadatas_type    | alarm                    |
      | metadatas_value   | random=10                |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "home"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder

  @service_path_error
  Scenario Outline:  try to get an entity type by type using NGSI v2 with forbidden service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get an entity type by type "home"
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
  Scenario Outline:  try to get an entity type by type using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get an entity type by type "home"
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
  Scenario Outline: try to get an entity type by type using NGSI v2 with too long in ServicePath header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get an entity type by type "home"
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
  Scenario: try to get an entity type by type using NGSI v2 with too many components in ServicePath header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_type_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When get an entity type by type "home"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  # --- type ---
  @type
  Scenario Outline:  get an entity type by type using NGSI v2 with several type values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | <types>     |
      | entities_id       | room34      |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "the same value of the previous request"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types by entity type are returned in response based on the info in the recorder
    Examples:
      | types      |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |
      | random=10  |
      | random=100 |
      | random=256 |

  @type_unknown @ISSUE_2056 @skip
  Scenario:  get an entity type by type using NGSI v2 with unknown type value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room3       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "fdgdfgdfgdfgfd"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                    |
      | error       | NotFound                 |
      | description | No context element found |

  @type_empty
  Scenario:  get an entity type by type using NGSI v2 with empty type value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room3       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type ""
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes types are returned in response based on the info in the recorder

  @type_forbidden
  Scenario Outline: try to get an entity type by type using NGSI v2 with forbidden type value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room3       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get an entity type by type "<types>"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | types               |
      | house<flat>         |
      | house=flat          |
      | house'flat'         |
      | house\'flat\'       |
      | house;flat          |
      | house(flat)         |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |


