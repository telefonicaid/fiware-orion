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
  Scenario:  verify log traces using NGSI v2 with create a new subscription request
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_log_traces  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                                                                               |
      | description               | my first subscription                                                               |
      | subject_type              | room                                                                                |
      | subject_idPattern         | .*                                                                                  |
      | subject_entities_number   | 2                                                                                   |
      | subject_entities_prefix   | type                                                                                |
      | condition_attrs           | temperature                                                                         |
      | condition_attrs_number    | 3                                                                                   |
      | condition_expression      | q>>>temperature>40&georel>>>near;minDistance:1000&geometry>>>point&coords>>>40,6391 |
      | notification_http_url     | http://localhost:1234                                                               |
      | notification_attrs        | temperature                                                                         |
      | notification_attrs_number | 3                                                                                   |
      | throttling                | 5                                                                                   |
      | expires                   | 2016-04-05T14:00:00.00Z                                                             |
      | status                    | active                                                                              |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | location       | /v2/subscriptions/.* |
      | content-length | 0                    |
    And verify that the subscription is stored in mongo
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
    And check in log, label "DEBUG" and message "--------------------- Serving request POST /v2/subscriptions -----------------"
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
