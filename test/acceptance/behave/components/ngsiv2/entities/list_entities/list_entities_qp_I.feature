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
     pending: georel, geometry, coords and option=values
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
