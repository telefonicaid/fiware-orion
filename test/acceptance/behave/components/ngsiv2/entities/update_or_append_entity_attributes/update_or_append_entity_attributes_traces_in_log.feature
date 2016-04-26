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


Feature: verify fields in log traces with update or append entity attributes request using NGSI v2.
  As a context broker user
  I would like to verify fields in log traces with update or append entity attributes request using NGSI v2
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
  Scenario:  verify log traces using NGSI v2 with update or append entity attributes request
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_log_traces  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    And check in log, label "INFO" and message "Starting transaction from"
      | trace    | value              |
      | time     | ignored            |
      | corr     | ignored            |
      | trans    | ignored            |
      | srv      | pending            |
      | subsrv   | pending            |
      | from     | pending            |
      | function | lmTransactionStart |
      | comp     | Orion              |
    And check in log, label "DEBUG" and message "--------------------- Serving request POST /v2/entities/room_1/attrs -----------------"
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
      | corr     | ignored          |
      | trans    | ignored          |
      | srv      | test_log_traces  |
      | subsrv   | /test            |
      | from     | ignored          |
      | function | lmTransactionEnd |
      | comp     | Orion            |
