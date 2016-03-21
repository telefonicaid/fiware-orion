# -*- coding: utf-8 -*-

# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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


Feature: update or append an attribute by entity ID using NGSI v2. "POST" - /v2/entities/<entity_id> plus payload
  queries parameters:
  tested: options=append, options=keyValues
  pending: type
  s a context broker user
  I would like to update or append an attribute by entity ID
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

  # ----------------------------- queries parameters -----------------------------
  # ------ options=append ----------
  @qp_options_append
  Scenario:  append an attribute by entity ID using NGSI v2 with "options=append" query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_4 |
      | attributes_value | 80            |
      | attributes_type  | Fahrenheit    |
      | metadatas_number | 3             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | cold          |
      # query parameter
      | qp_options       | append        |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_options_append_update
  Scenario:  try to update an attribute by entity ID using NGSI v2 with "op" query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_0 |
      | attributes_value | 80            |
      | attributes_type  | Fahrenheit    |
      | metadatas_number | 3             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | cold          |
      # query parameter
      | qp_options       | append        |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                         |
      | error       | BadRequest                                                                    |
      | description | one or more of the attributes in the request already exist: [ temperature_0 ] |

  @qp_options_append_in_blank
  Scenario:  try to append an attribute by entity ID using NGSI v2 with options query parameter with empty right-hand-side
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_4 |
      | attributes_value | 80            |
      | attributes_type  | Fahrenheit    |
      | metadatas_number | 3             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | cold          |
      # query parameter
      | qp_options       |               |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                         |
      | error       | BadRequest                                    |
      | description | Empty right-hand-side for URI param /options/ |

  @qp_options_unknown_value.row<row.id>
  @qp_options_unknown_value
  Scenario Outline:  try to append an attribute by entity ID using NGSI v2 with unkwnown "op" query parameter value
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | temperature       |
      | attributes_value | 80                |
      | attributes_type  | Fahrenheit        |
      | metadatas_number | 3                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | cold              |
      # query parameter
      | qp_options       | <query_parameter> |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |
    Examples:
      | query_parameter |
      | 34              |
      | false           |
      | true            |
      | 34.4E-34        |
      | temp.34         |
      | temp_34         |
      | temp-34         |
      | TEMP34          |
      | house_flat      |
      | house.flat      |
      | house-flat      |
      | house@flat      |
      | habitación      |
      | españa          |
      | barça           |
      | random=10       |
      | random=100      |
      | random=1000     |
      | random=10000    |
      | house_?         |
      | house_&         |
      | house_/         |
      | house_#         |
      | my house        |

  @qp_unknown
  Scenario Outline:  append an attribute by entity ID using NGSI v2 with unkwnown query parameter name
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      | attributes_type   | Fahrenheit  |
      | metadatas_number  | 3           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | cold        |
      # query parameter
      | <query_parameter> | append      |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | query_parameter |
      | 34              |
      | false           |
      | true            |
      | 34.4E-34        |
      | temp.34         |
      | temp_34         |
      | temp-34         |
      | TEMP34          |
      | house_flat      |
      | house.flat      |
      | house-flat      |
      | house@flat      |
      | habitación      |
      | españa          |
      | barça           |
      | random=10       |
      | random=100      |
      | random=1000     |
      | random=10000    |
      | random=100000   |

  @qp_options_invalid
  Scenario Outline:  try to append an attribute by entity ID using NGSI v2 with invalid "op" query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
   # These properties below are used in update request
    And properties to entities
      | parameter        | value             |
      | attributes_name  | temperature       |
      | attributes_value | 80                |
      | attributes_type  | Fahrenheit        |
      | metadatas_number | 3                 |
      | metadatas_name   | very_hot          |
      | metadatas_type   | alarm             |
      | metadatas_value  | cold              |
      # query parameter
      | qp_options       | <query_parameter> |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | query_parameter |
      | house<flat>     |
      | house=flat      |
      | house"flat"     |
      | house'flat'     |
      | house;flat      |
      | house(flat)     |
      | {"a":34}        |
      | ["34", "a", 45] |

  @qp_invalid
  Scenario Outline:  try to append an attribute by entity ID using NGSI v2 with invalid query parameter
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_op   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
   # These properties below are used in update request
    And properties to entities
      | parameter            | value       |
      | attributes_name      | temperature |
      | attributes_value     | 80          |
      | attributes_type      | Fahrenheit  |
      | metadatas_number     | 3           |
      | metadatas_name       | very_hot    |
      | metadatas_type       | alarm       |
      | metadatas_value      | cold        |
      # query parameter
      | qp_<query_parameter> | append      |
    When update or append attributes by ID "room_1" and with "normalized" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | query_parameter |
      | house<flat>     |
      | house=flat      |
      | house"flat"     |
      | house'flat'     |
      | house;flat      |
      | house(flat)     |
      | {"a":34}        |
      | ["34", "a", 45] |

  # ---- options=keyValues ---------

  @qp_options_key_values
  Scenario:  append attributes by entity ID if it exists using NGSI v2 with options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value         |
      | attributes_name  | temperature_4 |
      | attributes_value | 80            |
      | attributes_type  | Fahrenheit    |
      | metadatas_number | 3             |
      | metadatas_name   | very_hot      |
      | metadatas_type   | alarm         |
      | metadatas_value  | cold          |
      # query parameter
      | qp_options       | keyValues     |
    When update or append attributes by ID "room_1" and with "keyValues" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_options_key_values_error
  Scenario:  try to append attributes by entity ID if it exists using NGSI v2 without options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
    When update or append attributes by ID "room_1" and with "keyValues" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | attribute must be a JSON object, unless keyValues option is used |

  @qp_options_key_duplicated @BUG_1433
  Scenario:  append attributes by entity ID if it exists using NGSI v2 with options=keyvalues query parameter and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter         | value       |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 80          |
      # query parameter
      | qp_options        | keyValues   |
    When update or append attributes by ID "room_1" and with "keyValues" mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_options_key_value_and_append
  Scenario:  try to update attributes by entity ID if it exists using NGSI v2 with options=keyvalues and options=append queries parameterz and keyValues format
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_update_qp   |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 2           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
  # These properties below are used in update request
    And properties to entities
      | parameter         | value            |
      | attributes_number | 2                |
      | attributes_name   | temperature      |
      | attributes_value  | 80               |
      # query parameter
      | qp_options        | keyValues,append |
    When update or append attributes by ID "room_1" and with "keyValues" mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                        |
      | error       | BadRequest                                                                                   |
      | description | one or more of the attributes in the request already exist: [ temperature_0, temperature_1 ] |