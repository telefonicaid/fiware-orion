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

# Missing Tests:
#   - verification of headers response
#   - verification of Special Attribute Types
#


Feature: create entities requests (POST) with DateTime type using NGSI v2. "POST" - /v2/entities/ plus payload and queries parameters
  As a context broker user
  I would like to  create entities requests using NGSI v2
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

  # ---------- attribute value using DateTime type --------------------------------

  @attributes_value_datetime_datetime  @attributes_value_datetime_datetime.row<row.id>
  Scenario Outline:  create entities using NGSI v2 using DateTime type with several attributes values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_attributes_value |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And properties to entities
      | parameter        | value              |
      | entities_type    | house              |
      | entities_id      | room               |
      | attributes_name  | timestamp          |
      | attributes_value | <attributes_value> |
      | attributes_type  | DateTime           |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Created" http code
    And verify that entities are stored in mongo
    Examples:
      | attributes_value            |
      | 2016-04-05T14:00:00.00Z     |
      | 2016-04-05T14:00:00.00+2:00 |
      | 2016-04-05T14:00:00.00-2:00 |
      | 2016-04-05                  |
      | 2016-04-05T12               |


  @attributes_value_datetime_error  @attributes_value_datetime_error.row<row.id> @BUG_2718
  Scenario Outline:  try to create entities using NGSI v2 using DateTime type with wrong attributes values
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_attributes_value |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And properties to entities
      | parameter        | value              |
      | entities_type    | house              |
      | entities_id      | room               |
      | attributes_name  | timestamp          |
      | attributes_value | <attributes_value> |
      | attributes_type  | DateTime           |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                   |
      | error       | BadRequest              |
      | description | date has invalid format |
    And verify that entities are not stored in mongo
    Examples:
      | attributes_value             |
      | 2r16-04-05T14:00:00.00Z      |
      | 2016-04-05T14:00:00.00J      |
      | 04-05T14:00:00.00Z           |
      | 2016_04-05T14:00:00.00Z      |
      | 2016--05T14:00:00.00Z        |
      | 2016-r4-05T14:00:00.00Z      |
      | 2016-04-r5T14:00:00.00Z      |
      | 2016-04-T14:00:00.00Z        |
      | 2016-04-05K14:00:00.00Z      |
      | 2016-04-05Tr4:00:00.00Z      |
      | 2016-04-05T:00:00.00Z        |
      | 2016-04-05T14;00:00.00Z      |
      | 2016-04-05T14:r0:00.00Z      |
      | 2016-04-05T14::00.00Z        |
      | 2016-04-05T14:10:x0.00Z      |
      | 2016-04-05T14:10:00,00Z      |
      | 2016-04-05T14:10:00.h00Z     |
      | 2016-04-05T14:10:00.0h0Z     |
      | 2016-04-05T14:10:00.,00L     |
      | 2016-04-05T14:00:00.00>15:00 |
      | 2016-04-05T14:00:00.00+1:00Z |
    Examples: # @BUG_2718
      | attributes_value             |
      | 2016-64-05T14:00:00.00Z      |
      | 20166405T140000Z             |
      | 2016-04-55T14:00:00.00Z      |
      | 2016-04-05T54:00:00.00Z      |
      | 2016-04-05T14:84:00.00Z      |
      | 2016-04-05T14:10-00.00Z      |
      | 2016-04-05T14:10:0x.00Z      |
      | 2016-04-05T14:10:.00Z        |
      | 26-04-05T14:00:00.00Z        |
      | 2016-04-05T14:00:00.00+45:00 |
      | 2016-04-05T14:00:00.00+15;00 |
      | 2016-04-05T14:00:00.00+1r:00 |
      | 2016-04-05T14:00:00.00+12:89 |
      | 2016-04-05T14:00:00.00+12:y7 |
      | 2016-04-05T14:00:00.00+1:00Z |

  @attributes_value_datetime_without_with_type
  Scenario:  try to create entities using NGSI v2 using DateTime type without attributes values but with attribute type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_attributes_value_without |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter       | value     |
      | entities_type   | house     |
      | entities_id     | room      |
      | attributes_name | timestamp |
      | attributes_type | DateTime  |
    When create entity group with "1" entities in "normalized" mode
    Then verify that receive several "Bad Request" http code
    And verify several error responses
      | parameter   | value                   |
      | error       | BadRequest              |
      | description | date has invalid format |

  @attributes_value_datetime_special
  Scenario Outline:  try to create an entity using NGSI v2 using DateTime type with several attributes special values without type (compound, vector, boolean, etc)
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attributes_special |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | <entity_id>        |
      | attributes_name  | "timestamp"        |
      | attributes_value | <attributes_value> |
      | attributes_type  | "DateTime"         |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                   |
      | error       | BadRequest              |
      | description | date has invalid format |
    Examples:
      | entity_id | attributes_value                                                              |
      | "room1"   | true                                                                          |
      | "room2"   | false                                                                         |
      | "room3"   | 34                                                                            |
      | "room4"   | -34                                                                           |
      | "room5"   | 5.00002                                                                       |
      | "room6"   | -5.00002                                                                      |
      | "room7"   | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | "room8"   | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | "room9"   | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | "room10"  | {"x": "x1","x2": "b"}                                                         |
      | "room11"  | {"x": {"x1": "a","x2": "b"}}                                                  |
      | "room12"  | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | "room13"  | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | "room14"  | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | "room15"  | "41.3763726, 2.1864475,14"                                                    |
      | "room16"  | null                                                                          |
