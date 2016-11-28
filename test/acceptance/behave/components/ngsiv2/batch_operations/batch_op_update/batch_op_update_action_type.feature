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


Feature: actionType in update batch operation using NGSI v2. "POST" - /v2/op/update plus payload and queries parameters
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

  # --------------------- actionType -------------------------

  @action_type_append_normalized
  Scenario Outline:  append several entities with batch operations using NGSI v2 with APPEND and APPEND_STRICT in normalized format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "<operation>"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | operation     |
      | APPEND        |
      | APPEND_STRICT |

  @action_type_append_key_values
  Scenario Outline:  append several entities with batch operations using NGSI v2 with APPEND and APPEND_STRICT in normalized format in keyValues format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "<operation>"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | operation     |
      | APPEND        |
      | APPEND_STRICT |

  @action_type_update_normalized
  Scenario:  update several entities with batch operations using NGSIv2 in normalized format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 45          |
      | attributes_type    | kelvin      |
    When update entities in a single batch operation "UPDATE"
    Then verify that receive a "No Content" http code

  @action_type_update_key_values
  Scenario:  update several entities with batch operations using NGSIv2 in keyvalues format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 45          |
      | attributes_type    | kelvin      |
    When update entities in a single batch operation "UPDATE"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code

  @action_type_delete
  Scenario:  delete several entities with batch operations using NGSIv2 in normalized format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value |
      | entities_number    | 5     |
      | entities_prefix_id | true  |
      | entities_type      | home  |
      | entities_id        | room2 |
    When update entities in a single batch operation "DELETE"
    Then verify that receive a "No Content" http code

  @action_type_replace
  Scenario:  delete several entities with batch operations using NGSIv2 in normalized format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value |
      | entities_number    | 5     |
      | entities_prefix_id | true  |
      | entities_type      | home  |
      | entities_id        | room2 |
    When update entities in a single batch operation "REPLACE"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code

  @action_type_replace_key_values
  Scenario:  delete several entities with batch operations using NGSIv2 in keyvalues format
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value |
      | entities_number    | 5     |
      | entities_prefix_id | true  |
      | entities_type      | home  |
      | entities_id        | room2 |
    When update entities in a single batch operation "REPLACE"
    Then verify that receive a "No Content" http code

  @action_type_empty @BUG_2653
  Scenario:  try to append entities with batch operations using NGSI v2 with empty action
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation ""
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | empty update action type |

  @action_type_unknown @BUG_2653
  Scenario:  try to append entities with batch operations using NGSI v2 with unknown action
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "dfgdf"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                      |
      | error       | BadRequest                                                                                 |
      | description | invalid update action type: right ones are: APPEND, APPEND_STRICT, DELETE, REPLACE, UPDATE |

  @action_type_without
  Scenario:  try to append entities with batch operations using NGSI v2 without actionType field
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "without actionType field"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                        |
      | error       | BadRequest                                                   |
      | description | Invalid JSON payload, mandatory field /actionType/ not found |

  @action_type_not_plain_ascii @BUG_2653
  Scenario Outline:  try to append entities with batch operations using NGSI v2 with not plain ascii in actionType field
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "<action_type>"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                      |
      | error       | BadRequest                                                                                 |
      | description | invalid update action type: right ones are: APPEND, APPEND_STRICT, DELETE, REPLACE, UPDATE |
    Examples: # BUG_2653
      | action_type |
      | habitación  |
      | españa      |
      | barça       |

  @action_type_forbidden @BUG_2653
  Scenario Outline:  try to append entities with batch operations using NGSI v2 with forbidden chars in actionType field
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
    When update entities in a single batch operation "<action_type>"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                      |
      | error       | BadRequest                                                                                 |
      | description | invalid update action type: right ones are: APPEND, APPEND_STRICT, DELETE, REPLACE, UPDATE |
    Examples: # @BUG_2653
      | action_type   |
      | house<flat>   |
      | house=flat    |
      | house\'flat\' |
      | house'flat'   |
      | house;flat    |
      | house(flat)   |
      | house_?       |
      | house_&       |
      | house_/       |
      | house_#       |

  @action_type_malformed
  Scenario:  try to append an entity with batch operations using NGSI v2 but the actionType is malformed, without quotes
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_op_update_action_type_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value         |
      | entities_type    | "house"       |
      | entities_id      | "room1"       |
      | attributes_name  | "temperature" |
      | attributes_type  | "celsius"     |
      | attributes_value | 34            |
      | metadatas_name   | "alarm"       |
      | metadatas_type   | "hot"         |
      | metadatas_value  | "warning"     |
    When update entities in a single batch operation "APPEND" in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |

  # ----------------------- APPEND_STRICT actionType -------------------------------------
  @action_type_append_strict_existent
  Scenario:  try update several entities with batch operations using NGSIv2 in normalized format with APPEND_STRICT actionType but the attribute already exist
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 45          |
      | attributes_type    | kelvin      |
    When update entities in a single batch operation "APPEND_STRICT"
    Then verify that receive an "Unprocessable Entity" http code
    And verify an error response
      | parameter   | value                                                                       |
      | error       | Unprocessable                                                               |
      | description | one or more of the attributes in the request already exist: [ temperature ] |

  @action_type_append_strict_no_existent
  Scenario:  update several entities with batch operations using NGSIv2 in normalized format with APPEND_STRICT actionType but the attribute does not exist
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And initialize entity groups recorder
    And define a entity properties to update in a single batch operation
      | parameter          | value    |
      | entities_number    | 5        |
      | entities_prefix_id | true     |
      | entities_type      | home     |
      | entities_id        | room2    |
      | attributes_name    | pressure |
      | attributes_value   | 456      |
      | attributes_type    | bar      |
    When update entities in a single batch operation "APPEND_STRICT"
    Then verify that receive an "No Content" http code
    And verify that entities are stored in mongo

  # ----------------------- UPDATE actionType -------------------------------------
  @action_type_update_id_unknown
  Scenario:  try update several entities with batch operations using NGSIv2 in normalized format with id unknown with UPDATE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room3       |
      | attributes_name    | temperature |
      | attributes_value   | 45          |
      | attributes_type    | kelvin      |
    When update entities in a single batch operation "UPDATE"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @action_type_update_attr_unknown
  Scenario:  try to update several entities with batch operations using NGSIv2 in normalized format and the attribute is unknown with UPDATE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
  # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value    |
      | entities_number    | 5        |
      | entities_prefix_id | true     |
      | entities_type      | home     |
      | entities_id        | room2    |
      | attributes_name    | pressure |
      | attributes_value   | 456      |
    When update entities in a single batch operation "UPDATE"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | NotFound                                   |
      | description | The entity does not have such an attribute |

  # ----------------------- REPLACE actionType -------------------------------------
  @action_type_replace_id_unknown
  Scenario:  try update several entities with batch operations using NGSIv2 in normalized format with id unknown with REPLACE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room3       |
      | attributes_name    | temperature |
      | attributes_value   | 45          |
      | attributes_type    | kelvin      |
    When update entities in a single batch operation "REPLACE"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @action_type_replace_attr_unknown
  Scenario:  update several entities with batch operations using NGSIv2 in normalized format and the attribute is unknown with REPLACE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
  # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value    |
      | entities_number    | 5        |
      | entities_prefix_id | true     |
      | entities_type      | home     |
      | entities_id        | room2    |
      | attributes_name    | pressure |
      | attributes_value   | 456      |
    When update entities in a single batch operation "REPLACE"
    Then verify that receive an "No Content" http code
    And verify that entities are stored in mongo

  # ----------------------- DELETE actionType -------------------------------------
  @action_type_delete_id_unknown
  Scenario:  try update several entities with batch operations using NGSIv2 in normalized format with id unknown with DELETE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value |
      | entities_number    | 5     |
      | entities_prefix_id | true  |
      | entities_type      | home  |
      | entities_id        | room3 |
    When update entities in a single batch operation "DELETE"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @action_type_delete_attr_unknown
  Scenario:  update several entities with batch operations using NGSIv2 in normalized format and the attribute is unknown with DELETE actionType
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
  # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 5           |
      | entities_prefix_id | true        |
      | entities_type      | home        |
      | entities_id        | room2       |
      | attributes_name    | temperature |
      | attributes_value   | 44          |
      | attributes_type    | celsius     |
    When update entities in a single batch operation "APPEND"
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter          | value    |
      | entities_number    | 5        |
      | entities_prefix_id | true     |
      | entities_type      | home     |
      | entities_id        | room2    |
      | attributes_name    | pressure |
    When update entities in a single batch operation "DELETE"
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | NotFound                                   |
      | description | The entity does not have such an attribute |

  @action_type_append_with_entities_empty @BUG_2720 @skip
  Scenario:  try to update entities with batch operations using NGSIv2 with entities empty
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    When update entities in a single batch operation "APPEND"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value      |
      | error       | BadRequest |
      | description | TBD        |

  @action_type_append_without_entities
  Scenario:  try to update entities with batch operations using NGSIv2 without entities field
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_op_update_action_type |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    When update entities in a single batch operation "APPEND without entities"
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | BadRequest                                                 |
      | description | Invalid JSON payload, mandatory field /entities/ not found |
