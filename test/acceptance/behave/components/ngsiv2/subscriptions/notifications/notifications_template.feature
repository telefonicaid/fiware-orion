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


Feature: verify notifications from subscriptions with different templates using NGSIv2
  As a context broker user
  I would like to verify notifications from subscriptions with different templates using NGSIv2
  So that I can manage and use them in my scripts

  Actions Before the Feature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file
  Setup: start ContextBroker
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully
  Setup: start the subscription listener as a daemon using the port "1045"

  Actions before each Scenario:
  Check: verify subscription listener is started successfully

  Actions After each Scenario:
  Setup: delete database in mongo

  Actions After the Feature:
  Setup: stop ContextBroker


  # ----------------------- templates  ---------------------------
  @templates
  Scenario Outline:  send a notification using NGSI v2 with custom templates
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_template |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                        | value                                                       |
      | subject_idPattern                | ".*"                                                        |
      | condition_attrs                  | "temperature"                                               |
      | notification_http_custom_url     | "http://replace_host:1045/notify/${id}"                     |
      | notification_http_custom_headers | {"Content-Type": "<content_type>", "My-Header": "my_value"} |
      | notification_http_custom_method  | "POST"                                                      |
      | notification_http_custom_qs      | {"type": "${type}"}                                         |
      | notification_http_custom_payload | "<payload>"                                                 |
      | notification_attrs               | "pressure","temperature"                                    |
    And create a new subscription in raw mode
    And verify that receive a "Created" http code
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value                               |
      | entities_type    | "random=4"                          |
      | entities_id      | "room_2"                            |
      | attributes_name  | "temperature"&"pressure"&"humidity" |
      | attributes_value | "34"&"high"&88.9                    |
      | attributes_type  | "celsius"&&"porcent"                |
      | metadatas_name   | "very_hot"                          |
      | metadatas_type   | "alarm"                             |
      | metadatas_value  | "default"                           |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the custom notification
    Examples:
      | payload                                                                             | content_type     |
      | The pressure is ${pressure}                                                         | text/plain       |
      | The temperature is ${temperature} degrees celsius                                   | text/plain       |
      | {%22pressure%22: {%22value%22: %22${pressure}%22, %22type%22: %22bar%22}}           | application/json |
      | {%22temperature%22: {%22value%22: %22${temperature}%22, %22type%22: %22celsius%22}} | application/json |

  @templates_methods
  Scenario Outline:  send a notification using NGSI v2 with custom templates and several http methods
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_template |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                        | value                                                   |
      | subject_idPattern                | ".*"                                                    |
      | condition_attrs                  | "temperature"                                           |
      | notification_http_custom_url     | "http://replace_host:1045/notify/${id}"                 |
      | notification_http_custom_headers | {"Content-Type": "text/plain", "My-Header": "my_value"} |
      | notification_http_custom_method  | "<method>"                                              |
      | notification_http_custom_qs      | {"type": "${type}"}                                     |
      | notification_http_custom_payload | "The pressure is ${pressure}"                           |
      | notification_attrs               | "pressure","temperature"                                |
    And create a new subscription in raw mode
    And verify that receive a "Created" http code
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value                               |
      | entities_type    | "random=4"                          |
      | entities_id      | "room_2"                            |
      | attributes_name  | "temperature"&"pressure"&"humidity" |
      | attributes_value | "34"&"high"&88.9                    |
      | attributes_type  | "celsius"&&"porcent"                |
      | metadatas_name   | "very_hot"                          |
      | metadatas_type   | "alarm"                             |
      | metadatas_value  | "default"                           |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the custom notification
    Examples:
      | method |
      | POST   |
      | PUT    |
      | DELETE |
      | PATCH  |

  @templates_decimal_number @ISSUE_2207 @skip
  Scenario Outline:  send a notification using NGSI v2 with custom templates and decimal number as value
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_template |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                        | value                                                   |
      | subject_idPattern                | ".*"                                                    |
      | condition_attrs                  | "temperature"                                           |
      | notification_http_custom_url     | "http://replace_host:1045/notify/${id}"                 |
      | notification_http_custom_headers | {"Content-Type": "text/plain", "My-Header": "my_value"} |
      | notification_http_custom_method  | "POST"                                                  |
      | notification_http_custom_qs      | {"type": "${type}"}                                     |
      | notification_http_custom_payload | "The temperature is ${temperature} degrees celsius"     |
      | notification_attrs               | "pressure","temperature"                                |
    And create a new subscription in raw mode
    And verify that receive a "Created" http code
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value                               |
      | entities_type    | "random=4"                          |
      | entities_id      | "room_2"                            |
      | attributes_name  | "temperature"&"pressure"&"humidity" |
      | attributes_value | <value>&"high"&88.9                 |
      | attributes_type  | "celsius"&&"porcent"                |
      | metadatas_name   | "very_hot"                          |
      | metadatas_type   | "alarm"                             |
      | metadatas_value  | "default"                           |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the custom notification
    Examples:
      | value |
      | 34    |
      | 33.3  |
    Examples: # @ISSUE_2207
      | value                |
      | 34.54654654654654646 |
      | 3434534000000        |
