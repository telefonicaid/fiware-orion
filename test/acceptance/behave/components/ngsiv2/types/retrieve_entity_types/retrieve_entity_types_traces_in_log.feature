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


Feature: verify fields in log traces with retrieve entity types request using NGSI v2.
  As a context broker user
  I would like to verify fields in log traces with retrieve entity types request using NGSI v2
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

  @traces_in_log
  Scenario:  verify log traces using NGSI v2 with retrieve entity types request
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_log_traces  |
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
      | attributes_type   | DateTime                 |
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
    And modify headers and keep previous values "false"
      | parameter          | value           |
      | Fiware-Service     | test_log_traces |
      | Fiware-ServicePath | /test           |
    When get entity types
      | parameter | value |
      | options   | count |
      | limit     | 2     |
      | offset    | 1     |
    Then verify that receive an "OK" http code
    And verify that entity types returned in response are: "house,home"
    And verify that attributes types are returned in response based on the info in the recorder
    And check in log, label "INFO" and message "Starting transaction from"
      | trace    | value              |
      | time     | ignored            |
      | trans    | ignored            |
      | srv      | pending            |
      | subsrv   | pending            |
      | from     | pending            |
      | function | lmTransactionStart |
      | comp     | Orion              |
    And check in log, label "DEBUG" and message "--------------------- Serving request GET /v2/types -----------------"
      | trace    | value           |
      | time     | ignored         |
      | corr     | N/A             |
      | trans    | N/A             |
      | srv      | N/A             |
      | subsrv   | N/A             |
      | from     | N/A             |
      | function | connectionTreat |
      | comp     | Orion           |
    And check in log, label "INFO" and message "Transaction ended"
      | trace    | value            |
      | time     | ignored          |
      | trans    | ignored          |
      | srv      | test_log_traces  |
      | subsrv   | /test            |
      | from     | ignored          |
      | function | lmTransactionEnd |
      | comp     | Orion            |
