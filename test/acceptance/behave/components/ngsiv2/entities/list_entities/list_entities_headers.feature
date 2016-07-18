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
  Queries parameters
  tested : limit, offset, id, idPattern, type, q and option=count,keyValues
  pending: georel, geometry, coords and option=values,unique
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
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room2       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
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
    And modify headers and keep previous values "false"
      | parameter          | value           |
      | Fiware-Service     | test_happy_path |
      | Fiware-ServicePath | /test           |
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
      | parameter          | value |
      | fiware-total-count | 1     |

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
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room2       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 56          |
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_id_multiples |
      | Fiware-ServicePath | /test             |
    When get all entities
      | parameter | value   |
      | limit     | 9       |
      | offset    | 0       |
      | type      | car     |
      | id        | vehicle |
      | options   | count   |
    Then verify that receive an "OK" http code
    And verify that "0" entities are returned

  # ------------------------ Content-Type header ----------------------------------------------
  @with_content_type @BUG_2128
  Scenario Outline:  list all entities using NGSI v2 and Content-Type header
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
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
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room2       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       | <content_type>    |
    When get all entities
      | parameter | value    |
      | limit     | 9        |
      | offset    | 0        |
      | type      | car      |
      | id        | vehicle  |
      | q         | speed>78 |
      | options   | count    |
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
  Scenario:  list all entities using NGSI v2 and Content-Type header and empty value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
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
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room2       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
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
    And modify headers and keep previous values "false"
      | parameter          | value             |
      | Fiware-Service     | test_content_type |
      | Fiware-ServicePath | /test             |
      | Content-Type       |                   |
    When get all entities
      | parameter | value    |
      | limit     | 9        |
      | offset    | 0        |
      | type      | car      |
      | id        | vehicle  |
      | q         | speed>78 |
      | options   | count    |
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                        |
      | error       | BadRequest                                                                                   |
      | description | Orion accepts no payload for GET/DELETE requests. HTTP header Content-Type is thus forbidden |

  # ------------------------ Service header ----------------------------------------------
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
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | the same value of the previous request |
      | Fiware-ServicePath | /test                                  |
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
    And modify headers and keep previous values "false"
      | parameter          | value |
      | Fiware-ServicePath | /test |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  @service_error @BUG_1873
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

 # ------------------------ Service path header ----------------------------------------------
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
    And modify headers and keep previous values "false"
      | parameter          | value                                  |
      | Fiware-Service     | test_service_path                      |
      | Fiware-ServicePath | the same value of the previous request |
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
    And modify headers and keep previous values "false"
      | parameter      | value                     |
      | Fiware-Service | test_service_path_without |
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
