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
  pending: georel, geometry, coords and option=values,distinct
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room4l2     |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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

  @only_q_attribute.row<row.id>
  @only_q_attribute @BUG_1589 @ISSUE_1751
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room4l2     |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | options   | count          |
    Then verify that receive an "OK" http code
    And verify that "<returned>" entities are returned
    Examples:
      | q_expression      | returned |
      | myAttr            | 0        |
      | !myAttr           | 19       |
      | roses             | 1        |
      | !speed;!timestamp | 14       |
      | temperature       | 13       |

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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | random=10   |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room3       |
      | attributes_name  | temperature |
      | attributes_value | 78          |
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
