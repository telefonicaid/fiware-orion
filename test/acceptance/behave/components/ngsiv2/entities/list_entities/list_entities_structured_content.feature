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

  # -------------- structured content -----------------------
  @structured_content_q
  Scenario Outline: get entities with filters on structured content in attributes, "q" query parameter using NGSIv2
    Given a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_list_with_structured_content |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                 |
      | entities_type    | "random=4"            |
      | entities_id      | "room1"               |
      | attributes_name  | "temperature"         |
      | attributes_value | <attr_value_compound> |
      | attributes_type  | "celsius"             |
      | metadatas_name   | "very_cold"           |
      | metadatas_type   | "alarm"               |
      | metadatas_value  | "cold"                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value         |
      | entities_type    | "random=4"    |
      | entities_id      | "room2"       |
      | attributes_name  | "temperature" |
      | attributes_value | 50            |
      | attributes_type  | "celsius"     |
      | metadatas_name   | "very_cold"   |
      | metadatas_type   | "alarm"       |
      | metadatas_value  | "hot"         |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value      |
      | entities_type    | "random=4" |
      | entities_id      | "room3"    |
      | attributes_name  | "pressure" |
      | attributes_value | 850        |
      | attributes_type  | "bar"      |
      | metadatas_name   | "high"     |
      | metadatas_type   | "alarm"    |
      | metadatas_value  | 800        |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And modify headers and keep previous values "false"
      | parameter          | value                             |
      | Fiware-Service     | test_list_with_structured_content |
      | Fiware-ServicePath | /test                             |
    And get all entities
      | parameter | value          |
      | q         | <q_expression> |
      | options   | count          |
    And verify that receive a "OK" http code
    And verify that "<entities_returned>" entities are returned
    Examples:
      | attr_value_compound     | q_expression               | entities_returned |
      | {"B":{"C":45}}          | !attr                      | 3                 |
      | {"B":{"C":45}}          | temperature                | 2                 |
      | {"B":{"C":45}}          | temperature.B.C==45        | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D>=45      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=33      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D>54       | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D<=55      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D<56       | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D==54..55  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D==55..56  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=33..54  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=56..94  | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D==hot     | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D=='hot'   | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D!='cold'  | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D=='col'   | 0                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D=='coldd' | 0                 |
      | [34,56,78,90]           | temperature==34            | 1                 |
      | [34,56,78,90]           | temperature!=64            | 2                 |
      | [34,56,78,90]           | temperature==30..99        | 2                 |
      | ["one","two","three"]   | temperature=='one'         | 1                 |
      | ["one","two","three"]   | temperature==one           | 1                 |
      | ["one","two","three"]   | temperature!='five'        | 2                 |

  @structured_content_mq  @BUG_2446
  Scenario Outline: get entities with filters on structured content in attributes metadata, "mq" query parameter using NGSIv2
    Given a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_list_with_structured_content |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                 |
      | entities_type    | "random=4"            |
      | entities_id      | "room1"               |
      | attributes_name  | "temperature"         |
      | attributes_value | 45                    |
      | attributes_type  | "celsius"             |
      | metadatas_name   | "very_cold"           |
      | metadatas_type   | "alarm"               |
      | metadatas_value  | <meta_value_compound> |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value         |
      | entities_type    | "random=4"    |
      | entities_id      | "room2"       |
      | attributes_name  | "temperature" |
      | attributes_value | 50            |
      | attributes_type  | "celsius"     |
      | metadatas_name   | "very_cold"   |
      | metadatas_type   | "alarm"       |
      | metadatas_value  | "hot"         |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value      |
      | entities_type    | "random=4" |
      | entities_id      | "room3"    |
      | attributes_name  | "pressure" |
      | attributes_value | 850        |
      | attributes_type  | "bar"      |
      | metadatas_name   | "high"     |
      | metadatas_type   | "alarm"    |
      | metadatas_value  | 800        |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And modify headers and keep previous values "false"
      | parameter          | value                             |
      | Fiware-Service     | test_list_with_structured_content |
      | Fiware-ServicePath | /test                             |
    And get all entities
      | parameter | value           |
      | mq        | <mq_expression> |
      | options   | count           |
    And verify that receive a "OK" http code
    And verify that "<entities_returned>" entities are returned
    Examples:
      | meta_value_compound     | mq_expression                        | entities_returned |
      | {"B":{"C":45}}          | !attr.meta                           | 3                 |
      | {"B":{"C":45}}          | temperature.very_cold                | 2                 |
      | {"B":{"C":45}}          | temperature.very_cold.B.C==45        | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D>=45      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=33      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D>54       | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D<=55      | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D<56       | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D==54..55  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D==55..56  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=33..54  | 1                 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=56..94  | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D==hot     | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D=='hot'   | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D!='cold'  | 1                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D=='col'   | 0                 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D=='coldd' | 0                 |
      | [34,56,78,90]           | temperature.very_cold==34            | 1                 |
      | [34,56,78,90]           | temperature.very_cold!=64            | 2                 |
      | [34,56,78,90]           | temperature.very_cold==30..99        | 1                 |
      | ["one","two","three"]   | temperature.very_cold=='one'         | 1                 |
      | ["one","two","three"]   | temperature.very_cold==one           | 1                 |
      | ["one","two","three"]   | temperature.very_cold!='five'        | 2                 |
