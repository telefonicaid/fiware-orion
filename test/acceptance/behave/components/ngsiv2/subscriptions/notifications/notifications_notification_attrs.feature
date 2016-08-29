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


Feature: verify notifications from subscriptions with diferent formats using NGSIv2
  As a context broker user
  I would like toverify notifications from subscriptions with diferent formats using NGSIv2
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

   # ----------------------- attributes name  ---------------------------
  @notification_attrs_without
  Scenario:  send a notification using NGSI v2 without notification attributes field
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room2       |
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

  @notification_attrs_only_one
  Scenario Outline:  send a notification using NGSI v2 with only one notification attribute
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | <attributes>_1                  |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | <attributes>_0                  |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value        |
      | entities_type     | random=3     |
      | entities_id       | room2        |
      | attributes_number | 3            |
      | attributes_name   | <attributes> |
      | attributes_value  | random=5     |
      | attributes_type   | celsius      |
      | metadatas_number  | 2            |
      | metadatas_name    | very_hot     |
      | metadatas_type    | alarm        |
      | metadatas_value   | hot          |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | attributes |
      | room       |
      | 34         |
      | false      |
      | true       |
      | 34.4E-34   |
      | temp.34    |
      | temp_34    |
      | temp-34    |
      | TEMP34     |
      | house_flat |
      | house.flat |
      | house-flat |
      | house@flat |

  @notification_attrs_with_several @BUG_2463
  Scenario:  send a notification using NGSI v2 with several notification attributes
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                           |
      | subject_idPattern         | .*                              |
      | condition_attrs           | temperature_1                   |
      | notification_http_url     | http://replace_host:1045/notify |
      | notification_attrs_number | 2                               |
      | notification_attrs        | temperature                     |
      | expires                   | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room2       |
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

  @notification_attrs_unknown
  Scenario:  not send a notification using NGSI v2 with unknown notification attribute
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expired |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | sfsfsfsdf                       |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room2       |
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
    And verify that no notification is received

  @notification_attrs_except_only_one
  Scenario:  send a notification using NGSI v2 with only one notification except attribute
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                           |
      | subject_idPattern         | .*                              |
      | condition_attrs           | temperature_1                   |
      | notification_http_url     | http://replace_host:1045/notify |
      | notification_except_attrs | temperature_0                   |
      | expires                   | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
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

  @notification_attrs_except_several
  Scenario:  send a notification using NGSI v2 with several notification except attributes
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                           |
      | subject_idPattern         | .*                              |
      | condition_attrs           | temperature_1                   |
      | notification_http_url     | http://replace_host:1045/notify |
      | notification_attrs_number | 2                               |
      | notification_except_attrs | temperature                     |
      | expires                   | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room2       |
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

  @notification_except_attrs_unknown
  Scenario:  send a notification using NGSI v2 with unknown notification except attribute
    Given  a definition of headers
      | parameter          | value              |
      | Fiware-Service     | test_notif_expires |
      | Fiware-ServicePath | /test              |
      | Content-Type       | application/json   |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                 | value                           |
      | subject_idPattern         | .*                              |
      | condition_attrs           | temperature_1                   |
      | notification_http_url     | http://replace_host:1045/notify |
      | notification_except_attrs | sfsfsfsdf                       |
      | expires                   | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room3       |
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
