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


Feature: get entity types using NGSI v2 API. "GET" - /v2/types
  Queries parameters
  tested: limit, offset, options=count
  pending: options=values
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

  @happy_path @BUG_1636 @ISSUE_1833 @BUG_2046
  Scenario:  get entity types using NGSI v2 API
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
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
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
    When get entity types
      | parameter | value |
      | options   | count |
      | limit     | 2     |
      | offset    | 1     |
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | x-total-count     | 4          |
      | fiware-correlator | [a-f0-9-]* |
    And verify that entity types returned in response are: "house,home"
    And verify that attributes types are returned in response based on the info in the recorder

  # ------------------------ Service header ----------------------------------------------
  @service
  Scenario Outline:  get entity types using NGSI v2 API with several services headers
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
    When get entity types
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "none,home"
    And verify that attributes types are returned in response based on the info in the recorder
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  list all entities using NGSI v2 API without service header
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
    When get entity types
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "none,home"
    And verify that attributes types are returned in response based on the info in the recorder

  @service_error
  Scenario Outline:  try to get entities types using NGSI v2 with several wrong services headers
    Given  a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When get entity types
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
  Scenario:  try to get entities types using NGSI v2 with bad length services headers
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
    When get entity types
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

 # ------------------------ Service path header --------------------------------------
  @service_path.row<row.id>
  @service_path @BUG_1423
  Scenario Outline:  get entities type using NGSI v2 with several service paths headers
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
    When get entity types
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "none,home"
    And verify that attributes types are returned in response based on the info in the recorder
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
  Scenario:  get entities type using NGSI v2 without service path header
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
    When get entity types
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "none,home"
    And verify that attributes types are returned in response based on the info in the recorder

  @service_path_error
  Scenario Outline:  try to get entities type using NGSI v2 with forbidden service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get entity types
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
  Scenario Outline:  try to get entities type using NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get entity types
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
  Scenario Outline: try to get entities type using NGSI v2 with too long in ServicePath header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_type_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get entity types
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
  Scenario: try to get entities type using NGSI v2 with too many components in ServicePath header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_type_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When get entity types
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  # --- types ---
  @types_multiples.row<row.id>
  @types_multiples @BUG_2046
  Scenario Outline:  get entities type using NGSI v2 with multiples type
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_type_multiples |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room34      |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "<number>" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get entity types
      | parameter | value |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter     | value    |
      | x-total-count | <number> |
    And verify that attributes types are returned in response based on the info in the recorder
    Examples:
      | number |
      | 2      |
      | 10     |
      | 100    |

  @attributes_types_multiples
  Scenario: get entities type using NGSI v2 with the same type but with different attribute types
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_type_multiples |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
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
      | attributes_type   | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
       # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room3       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | random=10   |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get entity types
      | parameter | value |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter     | value |
      | x-total-count | 1     |
    And verify that entity types returned in response are: "home"
    And verify that attributes types are returned in response based on the info in the recorder

  @types_empty_list
  Scenario: get entities type using NGSI v2 with any entity types (empty list)
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_type_multiples |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    And initialize entity groups recorder
    When get entity types
      | parameter | value |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | x-total-count     | 0          |
      | fiware-correlator | [a-f0-9-]* |
    And verify that entity types returned in response are: "home"

  # ------------------ queries parameters -------------------------------
  # --- limit and offset ---
  @only_limit.row<row.id>
  @only_limit
  Scenario Outline:  get entities type using NGSI v2 with only limit query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type_limit  |
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
    When get entity types
      | parameter | value   |
      | limit     | <limit> |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "<types>"
    And verify that attributes types are returned in response based on the info in the recorder
    Examples:
      | limit | types         |
      | 1     | car           |
      | 2     | home,car      |
      | 10    | none,home,car |

  @only_limit_wrong.row<row.id>
  @only_limit_wrong
  Scenario Outline:  try to get entities type using NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_type_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    When get entity types
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

  @only_limit_zero
  Scenario:  try to get entities type using NGSI v2 with only limit query parameter with zero value
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_type_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    When get entity types
      | parameter | value |
      | limit     | 0     |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | Bad pagination limit: /0/ [a value of ZERO is unacceptable] |

  @only_limit_empty
  Scenario:  try to get entities type using NGSI v2 with only limit query parameter with empty value
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_type_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
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
    When get entity types
      | parameter | value |
      | limit     |       |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                       |
      | error       | BadRequest                                  |
      | description | Empty right-hand-side for URI param /limit/ |

  @only_offset
  Scenario Outline:  get entities  type using NGSI v2 with only offset query parameter
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_type_only_offset |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
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
      | parameter         | value    |
      | entities_type     | house    |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | pressure |
      | attributes_value  | low      |
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
    When get entity types
      | parameter | value    |
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "<types>"
    And verify that attributes types are returned in response based on the info in the recorder
    Examples:
      | offset | types               |
      | 0      | house,home,car,none |
      | 1      | house,home,none     |
      | 20     |                     |

  @only_offset_not_decimal
  Scenario Outline:  try to get entities type using NGSI v2 with only offset query parameter, but the value is not a decimal number
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_type_only_offset_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
    When get entity types
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

  @limit_offsets.row<row.id>
  @limit_offsets
  Scenario Outline:  get entities type using NGSI v2 with limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_limit_offset |
      | Fiware-ServicePath | /test                  |
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
      | parameter         | value    |
      | entities_type     | house    |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | pressure |
      | attributes_value  | low      |
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
    When get entity types
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "<types>"
    And verify that attributes types are returned in response based on the info in the recorder
    Examples:
      | limit | offset | types           |
      | 1     | 0      | none,home,car   |
      | 20    | 1      | home,house,none |
      | 1     | 20     | home            |
      | 5     | 2      | none,house      |

 # --- options=count ---
  @only_count_unknown
  Scenario:  try to get entities type using NGSI v2 with only options=count query parameter but the value is unknown
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type_count  |
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
    When get entity types
      | parameter | value   |
      | options   | zxczxcz |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |

  @only_count_empty
  Scenario:  try to get entities type using NGSI v2 with only options=count query parameter but the value is empty
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type_count  |
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
    When get entity types
      | parameter | value |
      | options   |       |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | Empty right-hand-side for URI param /options/ |

  @only_count
  Scenario:  get entities type using NGSI v2 with only options=count query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type_count  |
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
      | parameter         | value    |
      | entities_type     | house    |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | pressure |
      | attributes_value  | low      |
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
    When get entity types
      | parameter | value |
      | options   | count |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "house,home,none,car"
    And verify that attributes types are returned in response based on the info in the recorder
    And verify headers in response
      | parameter     | value |
      | x-total-count | 4     |

  @limit_offset_count.row<row.id>
  @limit_offset_count @BUG_2046
  Scenario Outline:  get entities type using NGSI v2 with limit, offset and options=count queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_limit_offset |
      | Fiware-ServicePath | /test                  |
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
      | parameter         | value    |
      | entities_type     | house    |
      | entities_id       | room2    |
      | attributes_number | 2        |
      | attributes_name   | pressure |
      | attributes_value  | low      |
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
    When get entity types
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
      | options   | count    |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "<types>"
    And verify that attributes types are returned in response based on the info in the recorder
    And verify headers in response
      | parameter     | value |
      | x-total-count | 4     |
    Examples:
      | limit | offset | types               |
      | 1     | 0      | car                 |
      | 20    | 1      | home,car,house,none |
      | 1     | 20     |                     |
      | 5     | 2      | house,none          |

 # --- options=value ---
  @only_count_value
  Scenario:  get entities type using NGSI v2 with only options=values query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_type_values |
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
    When get entity types
      | parameter | value |
      | options   | value |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |
