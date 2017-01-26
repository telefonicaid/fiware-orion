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

  # subscriptions
  @subscription_create
  Scenario: get common metrics using create a new subscription (POST) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
      | expires                   | 2019-04-05T14:00:00.00Z                                                             |
      | status                    | active                                                                              |
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    And create a new subscription
    And verify that receive a "Created" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3,4" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "5" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @subscription_update
  Scenario: get common metrics using update a subscription by id (PATCH) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to subscriptions
      | parameter             | value                              |
      | subject_idPattern     | "room.*"                           |
      | subject_typePattern   | "hou.*"                            |
      | condition_attrs       | "temperature"                      |
      | notification_http_url | "http://replace_host:10032/notify" |
      | notification_attrs    | "temperature"                      |
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to subscriptions
      | parameter             | value                              |
      | subject_idPattern     | "room.*"                           |
      | subject_typePattern   | "hou.*"                            |
      | condition_attrs       | "temperature"                      |
      | notification_http_url | "http://replace_host:10032/notify" |
      | notification_attrs    | "temperature"                      |
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to subscriptions
      | parameter             | value                              |
      | subject_idPattern     | "room.*"                           |
      | subject_typePattern   | "hou.*"                            |
      | condition_attrs       | "temperature"                      |
      | notification_http_url | "http://replace_host:10032/notify" |
      | notification_attrs    | "temperature"                      |
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    And update the subscription "previous subsc" in raw mode
    And verify that receive a "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "6,7" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "8" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @subscription_get_all
  Scenario: get common metrics using get all subscriptions (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get all subcriptions
    And verify that receive a "OK" http code
    And get all subcriptions
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get all subcriptions
    And verify that receive a "OK" http code
    And get all subcriptions
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get all subcriptions
    And verify that receive a "OK" http code
    And get all subcriptions
    And verify that receive a "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "9,10" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "11" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @subscription_get_by_id
  Scenario: get common metrics using get a subscription by id (GET) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    And get subcription by Id "previous subsc"
    And verify that receive a "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "12,13" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "14" by "test_incomingTransactions_2" service and "root-subserv" subservice

  @subscription_delete_by_id
  Scenario: get common metrics using delete a subscription by id (DELETE) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
    And delete a subscription by Id "previous subsc"
    And verify that receive a "No Content" http code
    And delete a subscription by Id "previous subsc"
    And verify that receive a "Not Found" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
    And delete a subscription by Id "previous subsc"
    And verify that receive a "No Content" http code
    And delete a subscription by Id "previous subsc"
    And verify that receive a "Not Found" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                 |
      | description           | my first subscription |
      | subject_type          | room                  |
      | subject_idPattern     | .*                    |
      | condition_attrs       | temperature           |
      | notification_http_url | http://localhost:1234 |
      | notification_attrs    | temperature           |
    And create a new subscription
    And verify that receive a "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
    And delete a subscription by Id "previous subsc"
    And verify that receive a "No Content" http code
    And delete a subscription by Id "previous subsc"
    And verify that receive a "Not Found" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "15,16" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "17" by "test_incomingTransactions_2" service and "root-subserv" subservice

  # batch operations
  @update_batch_op
  Scenario Outline: get common metrics using update batch operation (POST) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "16,17" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "18" by "test_incomingTransactions_2" service and "root-subserv" subservice
    Examples:
      | operation |
      | APPEND    |

  @query_batch_op
  Scenario Outline: get common metrics using query batch operation (POST) request and several services and subservices in Context Broker
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    # return entities with query batch operation
    And define properties to query in a single batch operation
      | parameter  | value                    |
      | entities   | type>>>house,id>>>room_1 |
      | entities   | type>>>home,id>>>room_2  |
      | attributes | temperature              |
    And query entities in a single batch operation
      | parameter | value           |
      | options   | keyValues,count |
    And verify that receive a "OK" http code
    And query entities in a single batch operation
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_1 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    # return entities with query batch operation
    And define properties to query in a single batch operation
      | parameter  | value                    |
      | entities   | type>>>house,id>>>room_1 |
      | entities   | type>>>home,id>>>room_2  |
      | attributes | temperature              |
    And query entities in a single batch operation
    And verify that receive a "OK" http code
    And query entities in a single batch operation
    And verify that receive a "OK" http code
    And modify headers and keep previous values "false"
      | parameter          | value                       |
      | Fiware-Service     | test_incomingtransactions_2 |
      | Fiware-ServicePath | /                           |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    And update entities in a single batch operation "<operation>"
    And verify that receive a "No Content" http code
    # return entities with query batch operation
    And define properties to query in a single batch operation
      | parameter  | value                    |
      | entities   | type>>>house,id>>>room_1 |
      | entities   | type>>>home,id>>>room_2  |
      | attributes | temperature              |
    And query entities in a single batch operation
    And verify that receive a "OK" http code
    And query entities in a single batch operation
    And verify that receive a "OK" http code
    When get common metrics
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "19,20" by "test_incomingTransactions_1" service and "test,root-subserv" subservice
    And verify that incoming Transactions are "21" by "test_incomingTransactions_2" service and "root-subserv" subservice
    Examples:
      | operation |
      | APPEND    |

  # reset metrics
  # log level
  @log_level
  Scenario: get common metrics with reset parameter using the log level (POST and GET) requests and several services and subservices in Context Broker
    Given change the log level
      | parameter | value |
      | level     | INFO  |
    And verify that receive an "OK" http code
    And retrieve the log level
    And verify that receive an "OK" http code
    When get common metrics
      | parameter | value |
      | reset     | true  |
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "10" by "default-service" service and "root-subserv" subservice

  # generals
  @version @BUG_2846 @skip
  Scenario: get common metrics with reset parameter using the version (GET) requests and several services and subservices in Context Broker
    Given send a version request
    And verify that receive an "OK" http code
    And send a version request
    And verify that receive an "OK" http code
    When get common metrics
      | parameter | value |
      | reset     | true  |
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3" by "default-service" service and "root-subserv" subservice

  @statistics @BUG_2846 @skip
  Scenario: get common metrics with reset parameter using the statistics (GET) requests and several services and subservices in Context Broker
    Given send a statistics request
    And verify that receive an "OK" http code
    And send a statistics request
    And verify that receive an "OK" http code
    When get common metrics
      | parameter | value |
      | reset     | true  |
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3" by "default-service" service and "root-subserv" subservice

  @cache_statistics @BUG_2846 @skip
  Scenario: get common metrics with reset parameter using the cache statistics (GET) requests and several services and subservices in Context Broker
    Given send a cache statistics request
    And verify that receive an "OK" http code
    And send a cache statistics request
    And verify that receive an "OK" http code
    When get common metrics
      | parameter | value |
      | reset     | true  |
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3" by "default-service" service and "root-subserv" subservice

  @entry_point @BUG_2846 @skip
  Scenario: get common metrics with reset parameter using the API entry point (GET) requests and several services and subservices in Context Broker
    Given send a API entry point request
    And verify that receive an "OK" http code
    And send a API entry point request
    And verify that receive an "OK" http code
    When get common metrics
      | parameter | value |
      | reset     | true  |
    And verify that receive an "OK" http code
    Then verify that incoming Transactions are "3" by "default-service" service and "root-subserv" subservice
