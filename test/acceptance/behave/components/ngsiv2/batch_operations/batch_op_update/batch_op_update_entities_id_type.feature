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


Feature: entities id and entities type in update batch operation using NGSI v2. "POST" - /v2/op/update plus payload and queries parameters
  As a context broker user
  I would like to  create entities requests using NGSI v2
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

  # --------------------- entities id -------------------------
  @entities_id
  Scenario Outline:  append entities with batch operations using NGSI v2 with several entities id values
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_entities_id |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_type  | celsius         |
      | attributes_value | 4               |
      | metadatas_name   | alarm           |
      | metadatas_type   | hot             |
      | metadatas_value  | warning         |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | entities_type | entities_id |
      | room_1        | room        |
      | room_2        | 34          |
      | room_3        | false       |
      | room_4        | true        |
      | room_5        | 34.4E-34    |
      | room_6        | temp.34     |
      | room_7        | temp_34     |
      | room_8        | temp-34     |
      | room_9        | TEMP34      |
      | room_10       | house_flat  |
      | room_11       | house.flat  |
      | room_12       | house-flat  |
      | room_13       | house@flat  |
      | room_17       | random=10   |
      | room_18       | random=100  |
      | room_19       | random=256  |

  @entities_id_multiples
  Scenario Outline:  append entities with batch operations using NGSI v2 with multiples entities id
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_entities_id |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
  # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | <quantity>  |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room        |
      | attributes_name    | temperature |
      | attributes_type    | celsius     |
      | attributes_value   | 4           |
      | metadatas_name     | alarm       |
      | metadatas_type     | hot         |
      | metadatas_value    | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | quantity |
      | 2        |
      | 10       |
      | 100      |
      | 1000     |
      | 5000     |

  @entities_id_duplicated @BUG_2685 @skip
  Scenario:  append entities with batch operations using NGSI v2 with duplicated entities id
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_entities_id |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
   # These properties below are used in batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_type  | celsius     |
      | attributes_value | 4           |
    And define a entity properties to update in a single batch operation
      | parameter        | value    |
      | entities_id      | room     |
      | attributes_name  | humidity |
      | attributes_type  | absolute |
      | attributes_value | 100      |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_type  | kelvin      |
      | attributes_value | 345         |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo

  @entities_id_not_plain_ascii @BUG_2658
  Scenario Outline: try to append entities with batch operations using NGSI v2 with not plain ascii in entities type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | house         |
      | entities_id      | <entities_id> |
      | attributes_name  | temperature   |
      | attributes_type  | celsius       |
      | attributes_value | 4             |
      | metadatas_name   | alarm         |
      | metadatas_type   | hot           |
      | metadatas_value  | warning       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |
    Examples:
      | entities_id |
      | habitación  |
      | españa      |
      | barça       |
      | my house    |

  @entities_id_length_exceed @BUG_2659
  Scenario: try to append entities with batch operations using NGSI v2 with entity id length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | random=257  |
      | attributes_name  | temperature |
      | attributes_type  | celsius     |
      | attributes_value | 4           |
      | metadatas_name   | alarm       |
      | metadatas_type   | hot         |
      | metadatas_value  | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | BadRequest                                       |
      | description | entity id length: 257, max length supported: 256 |

  @entities_id_length_zero @BUG_2659
  Scenario:  try to update entities with update batch operations using NGSI v2 with length zero in the entity id
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | ""            |
      | attributes_name  | "temperature" |
      | attributes_value | 34            |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | entity id length: 0, min length supported: 1 |

  @entities_id_without
  Scenario: try to append entities with batch operations using NGSI v2 without entities id value
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | attributes_name  | temperature |
      | attributes_type  | celsius     |
      | attributes_value | 4           |
      | metadatas_name   | alarm       |
      | metadatas_type   | hot         |
      | metadatas_value  | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | nor /id/ nor /idPattern/ present |
    And verify that entities are not stored in mongo

  @entities_id_wrong @BUG_2661
  Scenario Outline: try to append entities with batch operations using NGSI v2 with wrong entities id value
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | house         |
      | entities_id      | <entities_id> |
      | attributes_name  | temperature   |
      | attributes_type  | celsius       |
      | attributes_value | 4             |
      | metadatas_name   | alarm         |
      | metadatas_type   | hot           |
      | metadatas_value  | warning       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | Invalid characters in entity id |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |
      | house_?     |
      | house_&     |
      | house_/     |
      | house_#     |

  @entities_id_invalid_quotes
  Scenario Outline: try to append entities with batch operations using NGSI v2 without quotes in the value of the entities.type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | <entities_id> |
      | attributes_name  | "temperature" |
      | attributes_type  | "celsius"     |
      | attributes_value | 4             |
      | metadatas_name   | "alarm"       |
      | metadatas_type   | "hot"         |
      | metadatas_value  | "warning"     |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    And verify that entities are not stored in mongo
    Examples:
      | entities_id |
      | rewrewr     |
      | SDFSDFSDF   |

  @entities_id_invalid_raw
  Scenario Outline: try to append entities with batch operations using NGSI v2 with several not allowed entities id without attribute type (integer, boolean, no-string, etc
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_entities_id_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | <entities_id> |
      | attributes_name  | "temperature" |
      | attributes_type  | "celsius"     |
      | attributes_value | 4             |
      | metadatas_name   | "alarm"       |
      | metadatas_type   | "hot"         |
      | metadatas_value  | "warning"     |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                           |
      | error       | BadRequest                      |
      | description | invalid JSON type for entity id |
    Examples:
      | entities_id     |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |

  # --------------------- entities type -------------------------
  @entities_type_without @BUG_2685 @skip
  Scenario:  append entities with batch operations using NGSI v2 without entities type
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_op_update_entities_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_id      | room_3      |
      | attributes_name  | temperature |
      | attributes_type  | celsius     |
      | attributes_value | 4           |
      | metadatas_name   | alarm       |
      | metadatas_type   | hot         |
      | metadatas_value  | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo

  @entities_type
  Scenario Outline:  append entities with batch operations using NGSI v2 with several entities type values
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_op_update_entities_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | <entities_id>   |
      | attributes_name  | temperature     |
      | attributes_type  | celsius         |
      | attributes_value | 4               |
      | metadatas_name   | alarm           |
      | metadatas_type   | hot             |
      | metadatas_value  | warning         |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | entities_id | entities_type |
      | room_1      | room          |
      | room_2      | 34            |
      | room_3      | false         |
      | room_4      | true          |
      | room_5      | 34.4E-34      |
      | room_6      | temp.34       |
      | room_7      | temp_34       |
      | room_8      | temp-34       |
      | room_9      | TEMP34        |
      | room_10     | house_flat    |
      | room_11     | house.flat    |
      | room_12     | house-flat    |
      | room_13     | house@flat    |
      | room_17     | random=10     |
      | room_18     | random=100    |
      | room_19     | random=256    |

  @entities_type_multiples
  Scenario Outline:  append entities with batch operations using NGSI v2 with multiples entities type
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_op_update_entities_type |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
  # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter            | value       |
      | entities_number      | <quantity>  |
      | entities_prefix_type | true        |
      | entities_type        | house       |
      | entities_id          | room        |
      | attributes_name      | temperature |
      | attributes_type      | celsius     |
      | attributes_value     | 4           |
      | metadatas_name       | alarm       |
      | metadatas_type       | hot         |
      | metadatas_value      | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | quantity |
      | 2        |
      | 10       |
      | 100      |
      | 1000     |
      | 5000     |

  @entities_type_not_plain_ascii @BUG_2664
  Scenario Outline: try to append entities with batch operations using NGSI v2 with not plain ascii in entities type
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | room_2          |
      | attributes_name  | temperature     |
      | attributes_type  | celsius         |
      | attributes_value | 4               |
      | metadatas_name   | alarm           |
      | metadatas_type   | hot             |
      | metadatas_value  | warning         |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | Invalid characters in entity type |
    Examples:
      | entities_type |
      | habitación    |
      | españa        |
      | barça         |
      | my house      |

  @entities_type_length_exceed @BUG_2665
  Scenario: try to append entities with batch operations using NGSI v2 with entity type length that exceeds the maximum allowed (256)
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | random=257  |
      | entities_id      | room_3      |
      | attributes_name  | temperature |
      | attributes_type  | celsius     |
      | attributes_value | 4           |
      | metadatas_name   | alarm       |
      | metadatas_type   | hot         |
      | metadatas_value  | warning     |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | BadRequest                                         |
      | description | entity type length: 257, max length supported: 256 |

  @entities_type_length_zero @BUG_2659
  Scenario: try to  update entities with update batch operations using NGSI v2 with length zero in the entity type
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in update beh batch operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | ""            |
      | entities_id      | "room1"       |
      | attributes_name  | "temperature" |
      | attributes_value | 34            |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                          |
      | error       | BadRequest                                     |
      | description | entity type length: 0, min length supported: 1 |

  @entities_type_wrong @BUG_2666
  Scenario Outline: try to append entities with batch operations using NGSI v2 with wrong entities type value
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | room_5          |
      | attributes_name  | temperature     |
      | attributes_type  | celsius         |
      | attributes_value | 4               |
      | metadatas_name   | alarm           |
      | metadatas_type   | hot             |
      | metadatas_value  | warning         |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | Invalid characters in entity type |
    And verify that entities are not stored in mongo
    Examples:
      | entities_type |
      | house<flat>   |
      | house=flat    |
      | house"flat"   |
      | house'flat'   |
      | house;flat    |
      | house(flat)   |
      | house_?       |
      | house_&       |
      | house_/       |
      | house_#       |

  @entities_type_invalid_quotes
  Scenario Outline: try to append entities with batch operations using NGSI v2 without quotes in the value of the entities.type
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | "room_6"        |
      | attributes_name  | "temperature"   |
      | attributes_type  | "celsius"       |
      | attributes_value | 4               |
      | metadatas_name   | "alarm"         |
      | metadatas_type   | "hot"           |
      | metadatas_value  | "warning"       |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    And verify that entities are not stored in mongo
    Examples:
      | entities_type |
      | rewrewr       |
      | SDFSDFSDF     |

  @entities_type_invalid_raw
  Scenario Outline: try to append entities with batch operations using NGSI v2 with several not allowed entities type without attribute type (integer, boolean, no-string, etc
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_op_update_entities_type_error |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
     # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value           |
      | entities_type    | <entities_type> |
      | entities_id      | "room_8"        |
      | attributes_name  | "temperature"   |
      | attributes_type  | "celsius"       |
      | attributes_value | 4               |
      | metadatas_name   | "alarm"         |
      | metadatas_type   | "hot"           |
      | metadatas_value  | "warning"       |
    When update entities in a single batch operation "'APPEND'" in raw mode
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | invalid JSON type for entity type |
    Examples:
      | entities_type   |
      | false           |
      | true            |
      | 34              |
      | {"a":34}        |
      | ["34", "a", 45] |
      | null            |
