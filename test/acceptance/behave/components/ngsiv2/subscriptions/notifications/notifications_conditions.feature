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


Feature: verify notifications from subscriptions with different conditions values using NGSIv2
  As a context broker user
  I would like to verify notifications from subscriptions with different conditions values using NGSIv2
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

   # ----------------------- condition - attrs  ---------------------------
  @condition_attrs_without
  Scenario:  send a notification using NGSI v2 without attributes condition field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | array is empty                  |
      | notification_http_url | http://replace_host:1045/notify |
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

  @condition_attrs_only_one
  Scenario:  send a notification using NGSI v2 with attributes condition field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature_0                   |
      | notification_http_url | http://replace_host:1045/notify |
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

  @condition_attrs_only_one
  Scenario Outline:  send a notification using NGSI v2 with only one condition attribute
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | <attributes>_1                  |
      | notification_http_url | http://replace_host:1045/notify |
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

  @condition_attrs_with_several
  Scenario:  send a notification using NGSI v2 with several codition attributes
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter              | value                           |
      | subject_idPattern      | .*                              |
      | condition_attrs_number | 2                               |
      | condition_attrs        | temperature                     |
      | notification_http_url  | http://replace_host:1045/notify |
      | expires                | 2017-04-05T14:00:00.00Z         |
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

  @condition_attrs_unknown
  Scenario:  not send a notification using NGSI v2 with unknown condition attribute
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | sfsfsfsdf                       |
      | notification_http_url | http://replace_host:1045/notify |
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
    When create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    Then get notification sent to listener
    And verify that no notification is received

   # ----------------------- condition - q  ---------------------------
  @condition_q
  Scenario Outline:  send a notification using NGSI v2 with "q" condition field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | q>>><q_expression>              |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | q_expression                                                |
      | temperature==34                                             |
      | temperature!=44                                             |
      | temperature>25                                              |
      | temperature>=34                                             |
      | temperature<55                                              |
      | temperature<=34                                             |
      | temperature==3..74                                          |
      | temperature!=35..74                                         |
      | temperature!=3..33                                          |
      | pressure==high                                              |
      | pressure:high                                               |
      | pressure==nothing,high,red                                  |
      | pressure!=green,blue,red                                    |
      | pressure!='green,blue',red                                  |
      | humidity=='air,density',low                                 |
      | pressure~=hi                                                |
      | pressure                                                    |
      | !speed                                                      |
      | temperature<55;pressure~=hi                                 |
      | temperature<55;pressure~=hi;humidity!=nothing               |
      | timestamp==2017-06-15T07:21:24.00Z                          |
      | timestamp>=2017-06-15T07:21:24.00Z                          |
      | timestamp>=2016-06-15T07:21:24.00Z                          |
      | timestamp>2016-06-15T07:21:24.00Z                           |
      | timestamp<=2017-06-15T07:21:24.00Z                          |
      | timestamp<=2026-06-15T07:21:24.00Z                          |
      | timestamp<2026-06-15T07:21:24.00Z                           |
      | timestamp==2016-06-15T07:21:24.00Z..2026-06-15T07:21:24.00Z |
    Examples:
      | q_expression      |
      | value_str=='true' |
      | value_bool==true  |
      | value_bool!=false |

  @condition_q_eval_not_match
  Scenario:  not send a notification using NGSI v2 with "q" condition field, but eval does not match
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | q>>>temperature==56             |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify that no notification is received

  @condition_q_attribute_not_exist
  Scenario:  not send a notification using NGSI v2 with "q" condition field, but attribute does not exist
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | q>>>gdfgdfgdf==56               |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                    |
      | entities_type    | "random=4"                                                               |
      | entities_id      | "room_2"                                                                 |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool" |
      | attributes_value | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true            |
      | attributes_type  | "celsius"&&"porcent"&"DateTime"                                          |
      | metadatas_name   | "very_hot"                                                               |
      | metadatas_type   | "alarm"                                                                  |
      | metadatas_value  | "default"                                                                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify that no notification is received

  @condition_q_structured_content @BUG_2442 @skip
  Scenario Outline:  send a notification using NGSI v2 with "q" condition field and structured content in attributes
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | q>>><q_expression>              |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                 |
      | entities_type    | "random=4"            |
      | entities_id      | "room1"               |
      | attributes_name  | "temperature"         |
      | attributes_value | <attr_value_compound> |
      | attributes_type  | "celsius"             |
      | metadatas_name   | "very_cold"           |
      | metadatas_type   | "alarm"               |
      | metadatas_value  | "cold"                |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | attr_value_compound     | q_expression              |
      | {"B":{"C":45}}          | temperature               |
      | {"B":{"C":45}}          | temperature.B.C==45       |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D>=54     |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D>=55     |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=33     |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D>54      |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D<=55     |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D<=56     |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D<56      |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D==54..55 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D==55..56 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=33..54 |
      | {"B":{"C":{"D":55}}}    | temperature.B.C.D!=56..94 |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D==hot    |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D=='hot'  |
      | {"B":{"C":{"D":"hot"}}} | temperature.B.C.D!='cold' |
      | [34,56,78,90]           | temperature!=64           |
      | ["one","two","three"]   | temperature!='five'       |
    Examples:  # @BUG_2442
      | attr_value_compound   | q_expression        |
      | [34,56,78,90]         | temperature==34     |
      | [34,56,78,90]         | temperature==30..99 |
      | ["one","two","three"] | temperature=='one'  |
      | ["one","two","three"] | temperature==one    |

   # ----------------------- condition - mq  ---------------------------
  @condition_mq @BUG_2445 @skip
  Scenario Outline:  send a notification using NGSI v2 with "mq" condition field
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_attrs       | temperature                     |
      | condition_expression  | mq>>><mq_expression>            |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                                                  |
      | entities_type    | "house"                                                                                                |
      | entities_id      | "room4"                                                                                                |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool"                               |
      | attributes_value | 34&35&36&37&38&39                                                                                      |
      | attributes_type  | "celsius"&&"porcent"&"nothing"                                                                         |
      | metadatas_name   | "meta_temperature"&"meta_pressure"&"meta_humidity"&"meta_timestamp"&"meta_value_str"&"meta_value_bool" |
      | metadatas_type   | "alarm"&&&"DateTime"&&"alarm"                                                                          |
      | metadatas_value  | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true                                          |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | mq_expression                                                                              |
      | temperature.meta_temperature                                                               |
      | temperature.meta_temperature==34                                                           |
      | temperature.meta_temperature!=44                                                           |
      | temperature.meta_temperature>33                                                            |
      | temperature.meta_temperature>=34                                                           |
      | temperature.meta_temperature<35                                                            |
      | temperature.meta_temperature<=34                                                           |
      | temperature.meta_temperature==3..74                                                        |
      | temperature.meta_temperature==34..35                                                       |
      | temperature.meta_temperature!=35..74                                                       |
      | temperature.meta_temperature!=3..33                                                        |
      | pressure.meta_pressure==high                                                               |
      | pressure.meta_pressure:high                                                                |
      | pressure.meta_pressure==nothing,high,red                                                   |
      | pressure.meta_pressure!=green,blue,red                                                     |
      | pressure.meta_pressure!='green,blue',red                                                   |
      | humidity.meta_humidity=='air,density',low                                                  |
      | pressure.meta_pressure~=hi                                                                 |
      | pressure.meta_pressure                                                                     |
      | temperature.meta_temperature<55;pressure.meta_pressure~=hi                                 |
      | temperature.meta_temperature<55;pressure.meta_pressure~=hi;humidity.meta_humidity!=nothing |
      | value_str.meta_value_str=='true'                                                           |
      | value_bool.meta_value_bool==true                                                           |
      | value_bool.meta_value_bool!=false                                                          |
      | timestamp.meta_timestamp!=2018-06-15T07:21:24.00Z                                          |
      | timestamp.meta_timestamp>=2017-06-15T07:21:24.00Z                                          |
      | timestamp.meta_timestamp>=2016-06-15T07:21:24.00Z                                          |
      | timestamp.meta_timestamp<=2017-06-15T07:21:24.00Z                                          |
      | timestamp.meta_timestamp<=2026-06-15T07:21:24.00Z                                          |
      | timestamp.meta_timestamp!=2006-06-15T07:21:24.00Z..2015-06-15T07:21:24.00Z                 |
      | timestamp.meta_timestamp!=2017-06-15T07:21:24.00Z..2026-06-15T07:21:24.00Z                 |
    Examples: # @BUG_2445
      | mq_expression                                                              |
      | timestamp.meta_timestamp==2017-06-15T07:21:24.00Z                          |
      | timestamp.meta_timestamp>2016-06-15T07:21:24.00Z                           |
      | timestamp.meta_timestamp<2026-06-15T07:21:24.00Z                           |
      | timestamp.meta_timestamp==2016-06-15T07:21:24.00Z..2026-06-15T07:21:24.00Z |

  @condition_mq_eval_not_match
  Scenario:  not send a notification using NGSI v2 with "mq" condition field, but eval does not match
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                 |
      | subject_idPattern     | .*                                    |
      | condition_expression  | mq>>>temperature.meta_temperature==56 |
      | notification_http_url | http://replace_host:1045/notify       |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                                                  |
      | entities_type    | "house"                                                                                                |
      | entities_id      | "room4"                                                                                                |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool"                               |
      | attributes_value | 34&35&36&37&38&39                                                                                      |
      | attributes_type  | "celsius"&&"porcent"&"nothing"                                                                         |
      | metadatas_name   | "meta_temperature"&"meta_pressure"&"meta_humidity"&"meta_timestamp"&"meta_value_str"&"meta_value_bool" |
      | metadatas_type   | "alarm"&&&"DateTime"&&"alarm"                                                                          |
      | metadatas_value  | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true                                          |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify that no notification is received

  @condition_mq_attribute_not_exist @BUG_2496 @skip
  Scenario:  not send a notification using NGSI v2 with "mq" condition field, but attribute does not exist
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | mq>>>temperature.fghfghfgh==56  |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                                                                  |
      | entities_type    | "house"                                                                                                |
      | entities_id      | "room4"                                                                                                |
      | attributes_name  | "temperature"&"pressure"&"humidity"&"timestamp"&"value_str"&"value_bool"                               |
      | attributes_value | 34&35&36&37&38&39                                                                                      |
      | attributes_type  | "celsius"&&"porcent"&"nothing"                                                                         |
      | metadatas_name   | "meta_temperature"&"meta_pressure"&"meta_humidity"&"meta_timestamp"&"meta_value_str"&"meta_value_bool" |
      | metadatas_type   | "alarm"&&&"DateTime"&&"alarm"                                                                          |
      | metadatas_value  | 34&"high"&"air,density"&"2017-06-15T07:21:24.00Z"&"true"&true                                          |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify that no notification is received

  @condition_mq_structured_content  @BUG_2442 @skip
  Scenario Outline:  send a notification using NGSI v2 with "mq" condition field and structured content in attributes metadata
    Given  a definition of headers
      | parameter          | value                    |
      | Fiware-Service     | test_notif_condition_not |
      | Fiware-ServicePath | /test                    |
      | Content-Type       | application/json         |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                           |
      | subject_idPattern     | .*                              |
      | condition_expression  | mq>>><mq_expression>            |
      | notification_http_url | http://replace_host:1045/notify |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                 |
      | entities_type    | "random=4"            |
      | entities_id      | "room1"               |
      | attributes_name  | "temperature"         |
      | attributes_value | 56                    |
      | attributes_type  | "celsius"             |
      | metadatas_name   | "very_cold"           |
      | metadatas_type   | "alarm"               |
      | metadatas_value  | <meta_value_compound> |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | meta_value_compound     | mq_expression                       |
      | {"B":{"C":45}}          | temperature.very_cold               |
      | {"B":{"C":45}}          | temperature.very_cold.B.C==45       |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D>=45     |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=33     |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D>54      |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D<=55     |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D<56      |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D==54..55 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D==55..56 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=33..54 |
      | {"B":{"C":{"D":55}}}    | temperature.very_cold.B.C.D!=56..94 |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D==hot    |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D=='hot'  |
      | {"B":{"C":{"D":"hot"}}} | temperature.very_cold.B.C.D!='cold' |
      | [34,56,78,90]           | temperature.very_cold!=64           |
      | ["one","two","three"]   | temperature.very_cold!='five'       |
    Examples:  # @BUG_2442
      | meta_value_compound   | mq_expression                 |
      | [34,56,78,90]         | temperature.very_cold==34     |
      | [34,56,78,90]         | temperature.very_cold==30..99 |
      | ["one","two","three"] | temperature.very_cold=='one'  |
      | ["one","two","three"] | temperature.very_cold==one    |

   # ----------------------- condition - geo-location ---------------------------
  @condition_geo_location_geo_json  @condition_geo_location_geo_json.row<row.id>  @BUG_2499
  Scenario Outline:  send a notification using NGSI v2 with geo-location condition fields and GeoJSON attribute
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                     |
      | subject_idPattern     | room.*                                                    |
      | condition_expression  | georel>>><georel>&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://replace_host:1045/notify                           |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                                                           |
      | entities_type    | "random=4"                                                      |
      | entities_id      | "room1"                                                         |
      | attributes_name  | "location"&"temperature"                                        |
      | attributes_value | {"type": "<geojson_type>","coordinates": [<geojson_coords>]}&45 |
      | attributes_type  | "geo:json"                                                      |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | geojson_type | geojson_coords                                                                        | georel                | geometry | coords                                                                    |
      | Point        | 25.774,-80.190                                                                        | near;maxDistance:1000 | point    | -80.190,25.774                                                            |
      | Point        | 25.774,-80.190                                                                        | near;minDistance:0    | point    | -80.190,25.774                                                            |
      | Point        | 75.774,40.190                                                                         | intersects            | point    | 40.190,75.774                                                             |
      | Point        | 125.774,80.190                                                                        | equals                | point    | 80.190,125.774                                                            |
      | Point        | 15.774,50.190                                                                         | disjoint              | point    | 25.774,-80.190                                                            |
      | LineString   | [25.774,-80.190],[18.466,-66.118]                                                     | intersects            | line     | -80.190,25.774;-66.118,18.466                                             |
      | LineString   | [25.774,-80.190],[18.466,-66.118]                                                     | equals                | line     | -80.190,25.774;-66.118,18.466                                             |
      | LineString   | [125.774,-80.190],[118.466,-66.118]                                                   | disjoint              | line     | 35.774,-80.190;78.466,-66.118                                             |
      | LineString   | [25.774,-80.190],[18.466,-66.118]                                                     | coveredBy             | box      | -80.190,25.774;-66.118,18.466                                             |
      | LineString   | [25.774,11.190],[48.466,66.118]                                                       | intersects            | box      | 11.190,25.774;66.118,48.466                                               |
      | Polygon      | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]] | disjoint              | box      | 25.774,-80.190;18.466,-66.118                                             |
      | Polygon      | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]] | coveredBy             | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
      | Polygon      | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]] | intersects            | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
      | Polygon      | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]] | equals                | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
    Examples: # @BUG_2499
      | geojson_type | geojson_coords                                                                        | georel   | geometry | coords                                                                              |
      | Polygon      | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]] | disjoint | polygon  | 115.742,-165.690;135.742,-125.59;135.585,-175.541;135.485,-175.941;115.742,-165.690 |

  @condition_geo_location_simple_location_format @BUG_2499
  Scenario Outline:  send a notification using NGSI v2 with geo-location condition fields and Simple Location Format in attributes
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_notif_condition |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                                                     |
      | subject_idPattern     | room.*                                                    |
      | condition_expression  | georel>>><georel>&geometry>>><geometry>&coords>>><coords> |
      | notification_http_url | http://replace_host:1045/notify                           |
    And create a new subscription
    And verify that receive a "Created" http code
    And properties to entities
      | parameter        | value                    |
      | entities_type    | "random=4"               |
      | entities_id      | "room1"                  |
      | attributes_name  | "location"&"temperature" |
      | attributes_value | <geojson_coords>&45      |
      | attributes_type  | "<geojson_type>"         |
    When create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    Then get notification sent to listener
    And verify the notification in "normalized" format
    Examples:
      | geojson_type | geojson_coords                                                                        | georel                | geometry | coords                                                                    |
      | geo:point    | "-80.190,25.774"                                                                      | near;maxDistance:1000 | point    | -80.190,25.774                                                            |
      | geo:point    | "-80.190,25.774"                                                                      | near;minDistance:0    | point    | -80.190,25.774                                                            |
      | geo:point    | "40.190,75.774"                                                                       | intersects            | point    | 40.190,75.774                                                             |
      | geo:point    | "80.190, 125.774"                                                                     | equals                | point    | 80.190,125.774                                                            |
      | geo:point    | "50.190,15.774"                                                                       | disjoint              | point    | 25.774,-80.190                                                            |
      | geo:line     | ["-80.190,25.774","-66.118,18.466"]                                                   | intersects            | line     | -80.190,25.774;-66.118,18.466                                             |
      | geo:line     | ["-80.190,25.774","-66.118,18.466"]                                                   | equals                | line     | -80.190,25.774;-66.118,18.466                                             |
      | geo:line     | ["-80.190,125.774","-66.118,118.466"]                                                 | disjoint              | line     | 35.774,-80.190;78.466,-66.118                                             |
      | geo:line     | ["-80.190,25.774","-66.118,18.466"]                                                   | coveredBy             | box      | -80.190,25.774;-66.118,18.466                                             |
      | geo:line     | ["11.190,25.774","66.118,48.466"]                                                     | intersects            | box      | 11.190,25.774;66.118,48.466                                               |
      | geo:polygon  | ["35.742,-75.690","35.742,-75.59","35.585,-75.541","35.485,-75.941","35.742,-75.690"] | disjoint              | box      | 25.774,-80.190;18.466,-66.118                                             |
      | geo:polygon  | ["35.742,-75.690","35.742,-75.59","35.585,-75.541","35.485,-75.941","35.742,-75.690"] | coveredBy             | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
      | geo:polygon  | ["35.742,-75.690","35.742,-75.59","35.585,-75.541","35.485,-75.941","35.742,-75.690"] | intersects            | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
      | geo:polygon  | ["35.742,-75.690","35.742,-75.59","35.585,-75.541","35.485,-75.941","35.742,-75.690"] | equals                | polygon  | 35.742,-75.690;35.742,-75.59;35.585,-75.541;35.485,-75.941;35.742,-75.690 |
    Examples: # @BUG_2499
      | geojson_type | geojson_coords                                                                        | georel   | geometry | coords                                                                              |
      | geo:polygon  | ["35.742,-75.690","35.742,-75.59","35.585,-75.541","35.485,-75.941","35.742,-75.690"] | disjoint | polygon  | 115.742,-165.690;135.742,-125.59;135.585,-175.541;135.485,-175.941;115.742,-165.690 |

