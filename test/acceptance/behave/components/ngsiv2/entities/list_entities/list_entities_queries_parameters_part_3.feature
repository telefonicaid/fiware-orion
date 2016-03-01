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

  # ------------------ queries parameters -------------------------------
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

 # --- options = keyValues ---
  @qp_options_key_values
  Scenario:  list entities using NGSI v2 with options=keyValues query parameter only with normalized format
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
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  @qp_options_key_values
  Scenario:  list entities using NGSI v2 with options=keyValues query parameter only with keyValues format
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | room        |
      | entities_id       | room2       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 78          |
      | attributes_type   | celsius     |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
     # queries parameters
      | qp_options        | keyValues   |
    When create entity group with "3" entities in "keyValues" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  # --- attrs ---
  @qp_attrs_only
  Scenario:  list entities using NGSI v2 with attrs query parameter only
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 5           |
      | attributes_name   | temperature |
      | attributes_value  | high        |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                       |
      | attrs     | temperature_3,temperature_4 |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  @qp_attrs_and_id
  Scenario:  list entities using NGSI v2 with attrs and id queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_only_options |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And initialize entity groups recorder
    And properties to entities
      | parameter         | value       |
      | entities_type     | home        |
      | entities_id       | room1       |
      | attributes_number | 5           |
      | attributes_name   | temperature |
      | attributes_value  | high        |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | random=10   |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And record entity group
    When get all entities
      | parameter | value                       |
      | attrs     | temperature_3,temperature_4 |
      | id        | room1_2                     |
    Then verify that receive an "OK" http code
    And verify that "1" entities are returned
