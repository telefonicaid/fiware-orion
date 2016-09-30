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


Feature: verify notifications from subscriptions with different subject entities values using NGSIv2
  As a context broker user
  I would like to verify notifications from subscriptions with different subject entities values using NGSIv2
  So that I can manage and use them in my scripts

  Actions Before the Feature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file
  Setup: start ContextBroker
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully
  Setup: start the subscription listener as a daemon using the port "1045"
  Check: verify subscription listener is started successfully

  Actions After each Scenario:
  Setup: delete database in mongo

  Actions After the Feature:
  Setup: stop ContextBroker

  @happy_path
  Scenario:  send a notification using NGSI v2
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
      | notification_attrs    | temperature_0                   |
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

  @without_service
  Scenario:  send a notification using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
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

  @without_service_path
  Scenario:  send a notification using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value               |
      | Fiware-Service | test_notif_entities |
      | Content-Type   | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
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

  @without_service_and_service_path
  Scenario:  send a notification using NGSI v2 without service and service path headers
    Given  a definition of headers
      | parameter    | value            |
      | Content-Type | application/json |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
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

  # ----------------------- id and idPattern  ---------------------------
  @id
  Scenario Outline:  send a notification using NGSI v2 with subject-id field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_id            | <id>                            |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | <id>        |
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
    Examples:
      | id         |
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

  @id_multiples
  Scenario:  send a notification using NGSI v2 with subject-id field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_id            | room3_2                         |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
      | expires               | 2017-04-05T14:00:00.00Z         |
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
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format

  @id_entities_multiples
  Scenario:  send a notification using NGSI v2 with multiples subject-entities field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                           |
      | subject_entities_number | 3                               |
      | subject_entities_prefix | id                              |
      | subject_id              | room4                           |
      | condition_attrs         | temperature_1                   |
      | notification_http_url   | http://replace_host:1045/notify |
      | notification_attrs      | temperature_0                   |
      | expires                 | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | random=3    |
      | entities_id       | room4       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format

  @id_pattern
  Scenario Outline:  send a notification using NGSI v2 with subject-idPattern field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | <id_pattern>                    |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
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
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | id_pattern    |
      | .*            |
      | ^r.?          |
      | .*oom.*       |
      | .*_2          |
      | ^ro+.*        |
      | [A-Za-z0-9_]* |
      | room2(_3)     |
      | room2(.*)     |

  # ----------------------- type and typePattern ---------------------------
  @type
  Scenario Outline:  send a notification using NGSI v2 with subject-type field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_type          | <type>                          |
      | subject_id            | my_id                           |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | <type>      |
      | entities_id       | my_id       |
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
    Examples:
      | type       |
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

  @type_multiples
  Scenario:  send a notification using NGSI v2 with subject-type field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_type          | room5_2                         |
      | subject_id            | my_id                           |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | room5       |
      | entities_id       | my_id       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format

  @type_entities_multiples
  Scenario:  send a notification using NGSI v2 with multiples subject-entities field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter               | value                           |
      | subject_entities_number | 3                               |
      | subject_entities_prefix | type                            |
      | subject_type            | room6                           |
      | subject_id              | my_id                           |
      | condition_attrs         | temperature_1                   |
      | notification_http_url   | http://replace_host:1045/notify |
      | notification_attrs      | temperature_0                   |
      | expires                 | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | room6       |
      | entities_id       | my_id       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format

  @type_pattern @ISSUE_1853
  Scenario Outline:  send a notification using NGSI v2 with subject-typePattern field
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_entities |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_typePattern   | <type_pattern>                  |
      | subject_id            | my_id                           |
      | condition_attrs       | temperature_1                   |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_attrs    | temperature_0                   |
      | expires               | 2017-04-05T14:00:00.00Z         |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter         | value       |
      | entities_type     | room2       |
      | entities_id       | my_id       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=5    |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    When create entity group with "4" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | type_pattern  |
      | .*            |
      | ^r.?          |
      | .*oom.*       |
      | .*_2          |
      | ^ro+.*        |
      | [A-Za-z0-9_]* |
      | room2(_3)     |
      | room2(.*)     |
