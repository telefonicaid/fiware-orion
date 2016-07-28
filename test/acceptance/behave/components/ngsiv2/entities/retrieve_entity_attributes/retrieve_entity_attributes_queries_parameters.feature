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


Feature: get an entity by ID using NGSI v2. "GET" - /v2/entities/<entity_id>/attrs
  queries parameters:
  - tested: attrs, type and options=keyValues,values
  - pending: options=unique
  As a context broker user
  I would like to get an entity by ID using NGSI v2
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

  # --------------------- queries parameters -----------------------------
  @query_parameter_without
  Scenario:  get attributes in an entity by ID using NGSI v2 without queries parameters
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_id_q_param_without |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                   |
      | Fiware-Service     | test_id_q_param_without |
      | Fiware-ServicePath | /test                   |
    When get attributes in an entity by ID "room"
    Then verify that receive an "OK" http code
    And verify headers in response
      | parameter         | value      |
      | fiware-correlator | [a-f0-9-]* |
    And verify that attributes in an entity by ID are returned

  # --- attrs query parameter ---
  @query_parameter_without_attribute_type
  Scenario:  get attributes in an entity by ID using NGSI v2 with attrs query parameter without attribute type
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_id_q_param_without_attr_type |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                             |
      | Fiware-Service     | test_id_q_param_without_attr_type |
      | Fiware-ServicePath | /test                             |
    When get attributes in an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @query_parameter_with_metadatas
  Scenario:  get attributes in an entity by ID using NGSI v2 with attrs query parameter and metadata
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
    When get attributes in an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @query_parameter_without_metadata_type
  Scenario:  get attributes in an entity by ID using NGSI v2 with attrs query parameter and without metadata type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_value   | hot         |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
    When get attributes in an entity by ID "room"
      | parameter | value         |
      | attrs     | temperature_0 |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @query_parameter_multiples_attributes
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with attrs query parameter and multiples attributes
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | attributes_number | 10          |
      | entities_id       | room        |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                         |
      | Fiware-Service     | test_id_q_param_with_metadata |
      | Fiware-ServicePath | /test                         |
    When get attributes in an entity by ID "room"
      | parameter | value            |
      | attrs     | <attribute_name> |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
    Examples:
      | attribute_name                            |
      | temperature_0                             |
      | temperature_0,temperature_1               |
      | temperature_0,temperature_5,temperature_8 |

  @qp_attrs_unknown @BUG_1769 @skip
  Scenario:  try to get attributes in an entity by ID using NGSI v2 with attrs query parameter but the attribute unknown
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_id_attr_qp  |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 5           |
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
    And modify headers and keep previous values "false"
      | parameter          | value           |
      | Fiware-Service     | test_id_attr_qp |
      | Fiware-ServicePath | /test           |
    When get attributes in an entity by ID "room_0"
      | parameter | value    |
      | attrs     | humidity |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                     |
      | error       | NotFound                                                  |
      | description | The requested entity has not been found. Check attributes |

  @query_parameter_invalid
  Scenario Outline:  try to get attributes in an entity by ID using NGSI v2 with invalid attr query parameter
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_id_q_param_invalid |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 10          |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                   |
      | Fiware-Service     | test_id_q_param_invalid |
      | Fiware-ServicePath | /test                   |
    When get attributes in an entity by ID "room_0"
      | parameter | value            |
      | attrs     | <attribute_name> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | attribute_name      |
      | house<flat>         |
      | house=flat          |
      | house'flat'         |
      | house\'flat\'       |
      | house;flat          |
      | house(flat)         |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |

    # --- type query parameter ---
  @query_parameter_type
  Scenario:  get attributes in an entity by ID using NGSI v2 with type query parameter
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
    When get attributes in an entity by ID "room"
      | parameter | value |
      | type      | house |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @query_parameter_type_unknown
  Scenario:  get attributes in an entity by ID using NGSI v2 with type query parameter amd unknown type value
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
    When get attributes in an entity by ID "room"
      | parameter | value    |
      | type      | sdfsdfsd |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @query_parameter_type_invalid
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with type query parameter amd invalid type value
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
    When get attributes in an entity by ID "room"
      | parameter | value  |
      | type      | <type> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | type          |
      | house<flat>   |
      | house=flat    |
      | house'flat'   |
      | house\'flat\' |
      | house;flat    |
      | house(flat)   |

  @query_parameter_type_and_attrs
  Scenario:  get attributes in an entity by ID using NGSI v2 with type and attrs queries parameters
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
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
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                |
      | Fiware-Service     | test_id_q_param_type |
      | Fiware-ServicePath | /test                |
    When get attributes in an entity by ID "room"
      | parameter | value                       |
      | type      | house                       |
      | attrs     | temperature_0,temperature_2 |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

    # --- options_keyValues query parameter ---
  @qp_options_unknown
  Scenario:  get attributes in an entity by ID using NGSI v2 with options=unknown query parameter
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_id_options_q_param |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                   |
      | Fiware-Service     | test_id_options_q_param |
      | Fiware-ServicePath | /test                   |
    When get attributes in an entity by ID "room"
      | parameter | value   |
      | options   | unknown |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid value for URI param /options/ |

  @qp_options_keyvalues
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with options=keyValues query parameter with keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_id_q_param_with_key_values |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      # queries parameters
      | qp_options        | keyValues   |
    And create entity group with "1" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                           |
      | Fiware-Service     | test_id_q_param_with_key_values |
      | Fiware-ServicePath | /test                           |
    When get attributes in an entity by ID "room"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_options_keyvalues
  Scenario:  get attributes in an entity by ID using NGSI v2 with options=keyValues query parameter without keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_id_q_param_without_key_values |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                              |
      | Fiware-Service     | test_id_q_param_without_key_values |
      | Fiware-ServicePath | /test                              |
    When get attributes in an entity by ID "room"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @qp_attrs_and_options_keyvalues
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with attrs and options=keyValues query parameters with keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | test_id_q_param_with_key_values |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      # queries parameters
      | qp_options        | keyValues   |
    And create entity group with "1" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                           |
      | Fiware-Service     | test_id_q_param_with_key_values |
      | Fiware-ServicePath | /test                           |
    When get attributes in an entity by ID "room"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
      | options   | keyValues                   |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_attrs_and_options_keyvalues
  Scenario:  get attributes in an entity by ID using NGSI v2 with attrs, type and options=keyValues query parameter without keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_id_q_param_without_key_values |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                              |
      | Fiware-Service     | test_id_q_param_without_key_values |
      | Fiware-ServicePath | /test                              |
    When get attributes in an entity by ID "room"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
      | type      | house                       |
      | options   | keyValues                   |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

    # --- options_values query parameter ---
  @qp_options_values
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with options=values query parameter with keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      # queries parameters
      | qp_options        | keyValues   |
    And create entity group with "1" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
    When get attributes in an entity by ID "room"
      | parameter | value  |
      | options   | values |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_options_values
  Scenario:  get attributes in an entity by ID using NGSI v2 with options=values query parameter without keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
    When get attributes in an entity by ID "room"
      | parameter | value  |
      | options   | values |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned

  @qp_attrs_and_options_values
  Scenario Outline:  get attributes in an entity by ID using NGSI v2 with attrs and options=values query parameters with keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      # queries parameters
      | qp_options        | keyValues   |
    And create entity group with "1" entities in "<format>" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
    When get attributes in an entity by ID "room"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
      | options   | values                      |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
    Examples:
      | format     |
      | normalized |
      | keyValues  |

  @qp_attrs_and_options_values
  Scenario:  get attributes in an entity by ID using NGSI v2 with attrs, type and options=values query parameter without keyValues query parameter in create request
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "1" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    And modify headers and keep previous values "false"
      | parameter          | value                          |
      | Fiware-Service     | test_id_q_param_options_values |
      | Fiware-ServicePath | /test                          |
    When get attributes in an entity by ID "room"
      | parameter | value                       |
      | attrs     | temperature_0,temperature_1 |
      | type      | house                       |
      | options   | values                      |
    Then verify that receive an "OK" http code
    And verify that attributes in an entity by ID are returned
