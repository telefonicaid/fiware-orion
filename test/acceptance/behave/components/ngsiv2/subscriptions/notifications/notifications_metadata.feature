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


Feature: verify notifications with specials metadata from subscriptions using NGSIv2
  As a context broker user
  I would like to verify notifications with specials metadata from subscriptions using NGSIv2
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


  # ----------------------- metadata  ---------------------------
  @metadata_without
  Scenario:  send a notification using NGSI v2 without special metadata
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_notif_metadata |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
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
    And verify metadata in notification with "*"

  @metadata_create_entity  @metadata_create_entity.row<row.id>
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an entity is created
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_notif_metadata_created |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in entity request
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
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_update_entity_post @ISSUE_2549
  Scenario Outline: send a notification using NGSI v2 with special metadata when an entity attribute is updated or appended
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_notif_metadata_update_post |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in entity request
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
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=8    |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update or append attributes by ID "room2" and with "normalized" mode
    And verify that receive an "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_update_entity_patch
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an existent entity attribute is updated
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_metadata_update_patch |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in entity request
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
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | random=8    |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When update attributes by ID "room2" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_update_entity_put
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an entity attribute is replaced
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_notif_metadata_update_put |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    # These properties below are used in entity request
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
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | random=8    |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 2           |
      | metadatas_name    | very_cold   |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
    When replace attributes by ID "room2" if it exists and with "normalized" mode
    And verify that receive an "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_update_attribute_value_put  @BUG_2553 @skip
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an entity attribute value is updated
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_notif_metadata_update_attr_value |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value       |
      | entities_type    | random=3    |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | random=5    |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_value | random=8 |
    When update an attribute value by ID "room2" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_update_attribute_data_put
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an entity attribute data is updated
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_notif_metadata_update_attr_data |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value       |
      | entities_type    | random=3    |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | random=5    |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value      |
      | attributes_value | random=8   |
      | attributes_type  | Fahrenheit |
      | metadatas_number | 2          |
      | metadatas_name   | very_hot   |
      | metadatas_type   | alarm      |
      | metadatas_value  | cold       |
    When update an attribute by ID "room2" and attribute name "temperature" if it exists
    And verify that receive an "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  #The "delete" case will be supported upon completion of this issue #1494.
  @metadata_entity_delete  @ISSUE_1494 @skip
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an entity is deleted
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_notif_metadata_delete_entity |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value       |
      | entities_type    | random=3    |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | random=5    |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "false"
      | parameter          | value                             |
      | Fiware-Service     | test_notif_metadata_delete_entity |
      | Fiware-ServicePath | /test                             |
    When delete an entity with id "room2"
    Then verify that receive a "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |

  @metadata_attribute_delete @ISSUE_1494 @skip
  Scenario Outline:  send a notification using NGSI v2 with special metadata when an attribute is deleted
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_notif_metadata_delete_attr |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in entity request
    And properties to entities
      | parameter        | value       |
      | entities_type    | random=3    |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | random=5    |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | without condition field         |
      | notification_http_url | http://replace_host:1045/notify |
      | notification_metadata | <metadata>                      |
    And create a new subscription
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "false"
      | parameter          | value                           |
      | Fiware-Service     | test_notif_metadata_delete_attr |
      | Fiware-ServicePath | /test                           |
    When delete an attribute "temperature" in the entity with id "room2"
    Then verify that receive a "No Content" http code
    Then get notification sent to listener
    And verify metadata in notification with "<metadata>"
    Examples:
      | metadata                   |
      | previousValue              |
      | previousValue,*            |
      | actionType                 |
      | actionType,*               |
      | actionType,previousValue   |
      | actionType,previousValue,* |
