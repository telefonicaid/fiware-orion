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

Feature: get common metrics in Context Broker
  As a context broker user
  I would like get common metrics in Context Broker
  So that I can manage and use them in my scripts

  Actions Before the Feature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file
  Setup: start ContextBroker
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  Actions After each Scenario:
  Setup: delete database "orion-test_incomingtransactions_1" in mongo
  Setup: delete database "orion-test_incomingtransactions_2" in mongo

  Actions After the Feature:
  Setup: stop ContextBroker

############## Metrics to implement:
#
#    incomingTransactions: number of requests consumed by the component.
#    incomingTransactionRequestSize: total size (bytes) in requests associated to incoming transactions (“in” from the point of view of the component).
#    incomingTransactionResponseSize: total size (bytes) in responses associated to incoming transactions (“out” from the point of view of the component).
#    incomingTransactionErrors: number of incoming transactions resulting in error.
#    serviceTime: average time to serve a transaction.
#    outgoingTransactions: number of requests sent by the component.
#    outgoingTransactionRequestSize: total size (bytes) in requests associated to outgoing transactions (“out” from the point of view of the component).
#    outgoingTransactionResponseSize: total size (bytes) in responses associated to outgoing transactions (“in” from the point of view of the component).
#    outgoingTransactionErrors: number of outgoing transactions resulting in error.
#
##############  WARNING:
#   This tests suite must be executed complete, because the counters are accumulative in the same CB session. If you execute a scenario alone will fail.
#   To execute individually a scenario, it is necessary to reset the metrics, so append these lines below of the "get common metrics" step:
#         | parameter | value |
#         | reset     | true  |
#   and modify the number of incoming Transactions expected in each verification step.
#####################################################################

  # entities requests
  @create_entities
  Scenario: get common metrics using create entities (POST) request and several services and subservices in Context Broker
    # GET /admin/metrics
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3,4" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "5" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @update_entities_post
  Scenario: get common metrics using update entities (POST) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 456         |
      | attributes_type  | kelvin      |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 456         |
      | attributes_type  | kelvin      |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
        # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 456         |
      | attributes_type  | kelvin      |
    And update or append attributes by ID "the same value of the previous request" and with "normalized" mode
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "6,7" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "8" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @update_entities_put
  Scenario: get common metrics using update entities (PUT) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value    |
      | attributes_number | 5        |
      | attributes_name   | humidity |
      | attributes_value  | 80       |
    And replace attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "9,10" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "11" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @update_entities_patch
  Scenario: get common metrics using update entities (PATCH) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 4535        |
      | attributes_type  | kelvin      |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 4535        |
      | attributes_type  | kelvin      |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 80          |
      | attributes_type  | Fahrenheit  |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value       |
      | attributes_name  | temperature |
      | attributes_value | 4535        |
      | attributes_type  | kelvin      |
    When update attributes by ID "the same value of the previous request" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "12,13" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "14" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @get_entities_all
  Scenario: get common metrics using get all entities (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get all entities
    And verify that receive an "OK" http code
    And get all entities
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get all entities
    And verify that receive an "OK" http code
    And get all entities
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get all entities
    And verify that receive an "OK" http code
    And get all entities
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "17,18" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "19" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @get_entity_id
  Scenario: get common metrics using get an entity by id (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "20,21" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "22" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @get_entity_attr_id
  Scenario: get common metrics using get attributes in an entity by id (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get attributes in an entity by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "23,24" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "25" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @delete_entity_id
  Scenario: get common metrics using remove an entity by id (DELETE) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "2" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And delete an entity with id "room_0"
    And verify that receive an "No Content" http code
    And delete an entity with id "room_1"
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "2" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And delete an entity with id "room_0"
    And verify that receive an "No Content" http code
    And delete an entity with id "room_1"
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "2" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And delete an entity with id "room_0"
    And verify that receive an "No Content" http code
    And delete an entity with id "room_1"
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "27,28" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "29" by "test_incomingTransactions_2" service and "root-subserv" subservice

  # attributes data requests
  @get_attributes_data
  Scenario: get common metrics using get attributes data (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    And get an attribute "temperature" by ID "the same value of the previous request"
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "30,31" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "32" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @update_attributes_data
  Scenario: get common metrics using update attribute data (PUT) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value  |
      | attributes_value | 324    |
      | attributes_type  | kelvin |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value  |
      | attributes_value | 324    |
      | attributes_type  | kelvin |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | 80         |
      | attributes_type  | Fahrenheit |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value  |
      | attributes_value | 324    |
      | attributes_type  | kelvin |
    And update an attribute by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "33,34" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "35" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @remove_a_single_attribute
  Scenario: get common metrics using remove a single attribute (DELETE) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter         | value       |
      | entities_id       | random=3    |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And delete an attribute "temperature_0" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    And delete an attribute "temperature_1" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter         | value       |
      | entities_id       | random=3    |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And delete an attribute "temperature_0" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    And delete an attribute "temperature_1" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter         | value       |
      | entities_id       | random=3    |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And delete an attribute "temperature_0" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    And delete an attribute "temperature_1" in the entity with id "the same value of the previous request"
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "36,37" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "38" by "test_incomingTransactions_2" service and "root-subserv" subservice

  # attributes value requests
  @get_attributes_value
  Scenario: get common metrics using get attributes value (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Accept             | text/plain                  |
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Accept             | text/plain                  |
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Accept             | text/plain                  |
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    And get an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "39,40" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "41" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @update_attributes_value
  Scenario: get common metrics using update attributes value (PUT) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 90    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 90    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 90    |
    When update an attribute value by ID "the same value of the previous request" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "42,43" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "44" by "test_incomingTransactions_2" service and "root-subserv" subservice

  # types request
  @entity_type
  Scenario: get common metrics using retrieve an entity type (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    And get an entity type by type "home"
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "45,46" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "47" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @entity_types
  Scenario: get common metrics using retrieve entity types (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get entity types
    And verify that receive an "OK" http code
    And get entity types
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get entity types
    And verify that receive an "OK" http code
    And get entity types
    And verify that receive an "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And properties to entities
      | parameter        | value       |
      | entities_id      | random=3    |
      | entities_type    | home        |
      | attributes_name  | temperature |
      | attributes_value | 45          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get entity types
    And verify that receive an "OK" http code
    And get entity types
    And verify that receive an "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "48,49" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "50" by "test_incomingTransactions_2" service and "root-subserv" subservice

  # remove metrics
  @version @BUG_2846 @skip
  Scenario: get common metrics after delete metrics (DELETE) requests
    Given delete common metrics
    And verify that receive an "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "1" by "default-service" service and "root-subserv" subservice
