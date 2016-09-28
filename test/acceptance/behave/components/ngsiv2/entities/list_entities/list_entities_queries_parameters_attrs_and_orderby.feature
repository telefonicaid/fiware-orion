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
  tested : attrs and orderBy
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
  # --- attrs ---
  @qp_attrs_only
  Scenario:  list entities using NGSI v2 with attrs query parameter only
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
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
    And modify headers and keep previous values "false"
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
    When get all entities
      | parameter | value                       |
      | attrs     | temperature_3,temperature_4 |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  @qp_attrs_unknown @BUG_2245
  Scenario:  list entities using NGSI v2 with unknown value in attrs query parameter
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
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
    And modify headers and keep previous values "false"
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
    When get all entities
      | parameter | value    |
      | attrs     | fdgdfgdf |
    Then verify that receive an "OK" http code
    And verify that "3" entities are returned

  @qp_attrs_and_id
  Scenario:  list entities using NGSI v2 with attrs and id queries parameters
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
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
    And modify headers and keep previous values "false"
      | parameter          | value                    |
      | Fiware-Service     | test_list_entities_attrs |
      | Fiware-ServicePath | /test                    |
    When get all entities
      | parameter | value                       |
      | attrs     | temperature_3,temperature_4 |
      | id        | room1_2                     |
    Then verify that receive an "OK" http code
    And verify that "1" entities are returned

  # --- orderBy ---
  @qp_order_by_only
  Scenario Outline:  list entities using NGSI v2 with orderBy query parameter only
    Given a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_1"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 34&"high"                |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "default"                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                           |
      | entities_type    | "random=4"                      |
      | entities_id      | "room_2"                        |
      | attributes_name  | "temperature"&"pressure"        |
      | attributes_value | 23&"medium"                     |
      | attributes_type  | "celsius"&&"porcent"&"DateTime" |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_3"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 456&"low"                |
      | attributes_type  | "celsius"&&"porcent"     |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_4"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 5&"critical"             |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_5"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 34&"critical"            |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
   # the "X-Auth-Token" header is stored and passed internally from "I request a token..." step
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
    Then get all entities
      | parameter | value      |
      | orderBy   | <order_by> |
      | options   | count      |
    And verify that receive a "OK" http code
    And verify that "5" entities are returned
    And verify that entities are sorted by some attributes
    Examples:
      | order_by              |
      | temperature           |
      | !temperature          |
      | temperature,pressure  |
      | temperature,!pressure |

  @qp_order_by_with_different_types
  Scenario:  list entities using NGSI v2 with orderBy query parameter only and with different types
    Given a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_1"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 34&"high"                |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "default"                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                           |
      | entities_type    | "random=4"                      |
      | entities_id      | "room_2"                        |
      | attributes_name  | "temperature"&"pressure"        |
      | attributes_value | "trtrt"&"medium"                |
      | attributes_type  | "celsius"&&"porcent"&"DateTime" |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_3"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | 0.456&"low"              |
      | attributes_type  | "celsius"&&"porcent"     |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_4"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | true&"critical"          |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_5"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | false&"critical"         |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_6"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | {"a":"b"}&"critical"     |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room_7"                 |
      | attributes_name  | "temperature"&"pressure" |
      | attributes_value | [1,2,3,4]&"critical"     |
      | attributes_type  | "celsius"&&"porcent"     |
      | metadatas_name   | "very_hot"               |
      | metadatas_type   | "alarm"                  |
      | metadatas_value  | "hot"                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
   # the "X-Auth-Token" header is stored and passed internally from "I request a token..." step
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
    Then get all entities
      | parameter | value       |
      | orderBy   | temperature |
      | options   | count       |
    And verify that receive a "OK" http code
    And verify that "7" entities are returned

  @qp_order_by_dates
  Scenario Outline:  list entities using NGSI v2 with orderBy query parameter using dates
    Given a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_1"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 23&"medium"&"low"&"2015-06-15T07:21:24.00Z"&"null"&false                 |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_3"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 456&"low"&"high"&"2025-06-15T07:21:24.00Z"&"nothing"&false               |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_4"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 5&"critical"&"medium"&"2000-06-15T07:21:24.00Z"&"free"&true              |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_5"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"critical"&"medium"&"1900-06-15T07:21:24.00Z"&"free"&true             |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
   # the "X-Auth-Token" header is stored and passed internally from "I request a token..." step
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
    Then get all entities
      | parameter | value       |
      | attrs     | temperature |
      | orderBy   | <order_by>  |
      | options   | count       |
    And verify that receive a "OK" http code
    And verify that "5" entities are returned
    And verify that entities are sorted by some attributes
    Examples:
      | order_by                |
      | timestamp               |
      | !timestamp              |
      | timestamp,temperature   |
      | !timestamp,!temperature |

  @qp_order_by_and_attrs
  Scenario Outline:  list entities using NGSI v2 with orderBy and attrs queries parameters
    Given a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_1"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 23&"medium"&"low"&"2015-06-15T07:21:24.00Z"&"null"&false                 |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_3"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 456&"low"&"high"&"2025-06-15T07:21:24.00Z"&"nothing"&false               |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_4"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 5&"critical"&"medium"&"2000-06-15T07:21:24.00Z"&"free"&true              |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_5"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"critical"&"medium"&"2000-06-15T07:21:24.00Z"&"free"&true             |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
   # the "X-Auth-Token" header is stored and passed internally from "I request a token..." step
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
    Then get all entities
      | parameter | value       |
      | attrs     | temperature |
      | orderBy   | <order_by>  |
      | options   | count       |
    And verify that receive a "OK" http code
    And verify that "5" entities are returned
    And verify that entities are sorted by some attributes
    Examples:
      | order_by              |
      | temperature           |
      | !temperature          |
      | temperature,pressure  |
      | temperature,!pressure |

  @qp_order_by_with_unknown
  Scenario: list entities using NGSI v2 with orderBy query parameter but with unknown attribute
    Given a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And initialize entity groups recorder
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_1"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 23&"medium"&"low"&"2015-06-15T07:21:24.00Z"&"null"&false                 |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_3"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 456&"low"&"high"&"2025-06-15T07:21:24.00Z"&"nothing"&false               |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_4"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 5&"critical"&"medium"&"2000-06-15T07:21:24.00Z"&"free"&true              |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_5"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"critical"&"medium"&"2000-06-15T07:21:24.00Z"&"free"&true             |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "hot"                                                                    |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    And record entity group
   # the "X-Auth-Token" header is stored and passed internally from "I request a token..." step
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_list_entities_order_by |
      | Fiware-ServicePath | /test                       |
    Then get all entities
      | parameter | value       |
      | attrs     | temperature |
      | orderBy   | gdgdgfg     |
    And verify that receive a "OK" http code
    And verify that "5" entities are returned
    And verify that entities are not sorted by attributes
