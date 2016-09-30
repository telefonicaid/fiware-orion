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


Feature: verify notifications from subscriptions with different formats using NGSIv2
  As a context broker user
  I would like to verify notifications from subscriptions with different formats using NGSIv2
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


  # ----------------------- formats  ---------------------------
  @formats_without
  Scenario:  send a notification using NGSI v2 without format defined (normalized by default)
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_notif_format |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in entity request
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room1       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format

  @formats
  Scenario Outline:  send a notification using NGSI v2 with different format with attrsFormat field
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_notif_format |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                | value                           |
      | subject_idPattern        | .*                              |
      | condition_attrs          | without condition field         |
      | notification_http_url    | http://replace_host:1045/notify |
      | notification_attrsFormat | <format>                        |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in entity request
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room1       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "<format>" format
    Examples:
      | format     |
      | normalized |
      | keyValues  |
      | values     |
      | legacy     |
