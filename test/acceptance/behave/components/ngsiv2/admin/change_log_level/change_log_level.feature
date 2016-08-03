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

Feature: change the log level in Context Broker
  As a context broker user
  I would like to change the log level in Context Broker
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


  @change_log_level @BUG_2419 @skip
  Scenario Outline: change the log level in Context Broker
    When change the log level
      | parameter | value       |
      | level     | <log_level> |
    Then verify that receive an "OK" http code
    And retrieve the log level
    And verify that receive an "OK" http code
    And verify if the log level "<log_level>" is the expected
    Examples:
      | log_level |
      | FATAL     |
      | ERROR     |
      | WARN      |
      | INFO      |
      | DEBUG     |
      | NONE      |

  @change_without_log_level
  Scenario: try change the log level in Context Broker without level query parameter
    When change the log level
    Then verify that receive an "Bad Request" http code
    And verify admin error "log level missing"

  @change_log_level_unknown @ISSUE_2420 @skip
  Scenario Outline: try to change the log level in Context Broker with unknown level
    When change the log level
      | parameter | value       |
      | level     | <log_level> |
    Then verify that receive an "Bad Request" http code
    And verify admin error "invalid log level"
    Examples:
      | log_level |
      |           |
      | vcbvcbvc  |

  @change_log_level_traces  @BUG_2419 @skip
  Scenario Outline: change the log level in Context Broker and verify log traces
    When change the log level
      | parameter | value       |
      | level     | <log_level> |
    Then verify that receive an "OK" http code
    And retrieve the log level
    And verify that receive an "OK" http code
    And verify if the log level "<log_level>" is the expected
    And check in log, label "DEBUG" and message "--------------------- Serving request PUT /admin/log -----------------"
      | trace  | value           |
      | time   | ignored         |
      | corr   | N/A             |
      | trans  | N/A             |
      | srv    | N/A             |
      | subsrv | N/A             |
      | from   | N/A             |
      | op     | connectionTreat |
      | comp   | Orion           |
    And check in log, label "DEBUG" and message "msg=URI parameter:   level: <log_level>"
      | trace  | value          |
      | time   | ignored        |
      | corr   | ignored        |
      | trans  | ignored        |
      | srv    | pending        |
      | subsrv | pending        |
      | from   | ignored        |
      | op     | uriArgumentGet |
      | comp   | Orion          |
    And check in log, label "DEBUG" and message "Treating service PUT /admin/log"
      | trace  | value       |
      | time   | ignored     |
      | corr   | ignored     |
      | trans  | ignored     |
      | srv    | pending     |
      | subsrv | <default>   |
      | from   | ignored     |
      | op     | restService |
      | comp   | Orion       |
    And check in log, label "INFO" and message "Transaction ended"
      | trace  | value            |
      | time   | ignored          |
      | corr   | ignored          |
      | trans  | ignored          |
      | srv    | <default>        |
      | subsrv | <default>        |
      | from   | ignored          |
      | op     | lmTransactionEnd |
      | comp   | Orion            |
    Examples:
      | log_level |
      | FATAL     |
      | ERROR     |
      | WARN      |
      | INFO      |
      | DEBUG     |
      | NONE      |
