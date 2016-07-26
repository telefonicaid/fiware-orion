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


Feature: update an attribute value by entity ID and attribute name if it exists using NGSI v2 API. "PUT" - /v2/entities/<entity_id>/attrs/<attr_name>/value plus payload
  Queries parameters:
  tested:type
  As a context broker user
  I would like to update an attribute value by entity ID and attribute name if it exists using NGSI v2 API
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

  # --------------------- attribute value  ------------------------------------

  @attribute_value_text_plain
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with text/plain in Content-Type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_value_text |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value           |
      | true                       |
      | false                      |
      | True                       |
      | TRUE                       |
      | False                      |
      | FALSE                      |
      | 34                         |
      | -34                        |
      | 34.4E-34                   |
      | 5.00002                    |
      | -5.00002                   |
      | "41.3763726, 2.1864475,14" |
      | "2017-06-17T07:21:24.238Z" |
      | null                       |
      | "dfgdfgdf"                 |
      | "56.56"                    |

  @attribute_value_text_plain
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with text/plain in Content-Type
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_update_attribute_value_text |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | house flat       |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=100000    |
      | random=1000000   |

  @attribute_value_text_plain_2.row<row.id>
  @attribute_value_text_plain_2 @BUG_1902 @skip
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with text/plain in Content-Type
    Given  a definition of headers
      | parameter          | value                         |
      | Fiware-Service     | test_update_attribute_value_2 |
      | Fiware-ServicePath | /test                         |
      | Content-Type       | application/json              |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value                                                 |
      | "[ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]" |
      | "{\"x\": \"x1\",\"x2\": \"b\"}"                                  |
      | "{"x": "x1","x2": "b"}"                                          |
      | "123"45"                                                         |
      | "123'45"                                                         |
      | "{x: x1,x2: b}"                                                  |

  @attribute_value_text_plain_invalid.row<row.id>
  @attribute_value_text_plain_invalid
  Scenario Outline:  try to update attributes value by entity ID and atrribute name using NGSI v2 with text/plain in Content-Type and invalid values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | attribute value type not recognized |
    Examples:
      | attributes_value                                                              |
      | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                         |
      | {"x": {"x1": "a","x2": "b"}}                                                  |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | {"rt.ty": "5678"}                                                             |
      | '{"rt.ty": "5678"}'                                                           |
      | cvbcbcb                                                                       |

  @attribute_value_text_plain_forbidden
  Scenario Outline:  try to update attributes value by entity ID and atrribute name using NGSI v2 with text/plain in Content-Type and forbidden values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | attribute value type not recognized |
    Examples:
      | attributes_value                                                               |
      | [ "json", "<vector>", "of", 6, "strings", "and", 2, "integers" ]               |
      | [ "json", ["a=3", 34, "c", ["r", 4, "t"]], "of", 6]                            |
      | [ "json", ["a;", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                          |
      | {"x": {"x1": "a(7)","x2": "b"}}                                                |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34?}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b&"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45/6, "rt"],"x2": "b"}                               |
      | {"rt.ty": "5678#"}                                                             |

  @attribute_value_application_json
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with application/json in Content-Type and json object values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value                                                              |
      | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                         |
      | {"x": {"x1": "a","x2": "b"}}                                                  |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | {"rt.ty": "5678"}                                                             |

  @attribute_value_application_json_invalid
  Scenario Outline:  try to update attributes value by entity ID and atrribute name using NGSI v2 with application/json in Content-Type and invalid values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | BadRequest                                              |
      | description | Neither JSON Object nor JSON Array for attribute::value |
    Examples:
      | attributes_value                |
      | true                            |
      | false                           |
      | 34                              |
      | -34                             |
      | 5.00002                         |
      | -5.00002                        |
      | "41.3763726, 2.1864475,14"      |
      | "2017-06-17T07:21:24.238Z"      |
      | null                            |
      | "dfgdfgdf"                      |
      | "56.56"                         |
      | "{\"x\": \"x1\",\"x2\": \"b\"}" |
      | "123'45"                        |

  @attribute_value_application_json_invalid
  Scenario Outline:  try to update attributes value by entity ID and atrribute name using NGSI v2 with application/json in Content-Type and invalid values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_value                                                 |
      | "[ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]" |
      | "{"x": "x1","x2": "b"}"                                          |
      | "123"45"                                                         |
      | '{"rt.ty": "5678"}'                                              |
      | cvbcbcb                                                          |

  @attribute_value_application_json_error
  Scenario Outline:  try to update attributes value by entity ID and atrribute name using NGSI v2 with application/json in Content-Type and error values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
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
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room_1" and attribute name "temperature_0" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_value                                                 |
      | "[ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]" |
      | "{"x": "x1","x2": "b"}"                                          |
      | "123"45"                                                         |
      | '{"rt.ty": "5678"}'                                              |
      | cvbcbcb                                                          |
      | {"x": [{"a":78, "b":"r"}, 45/6, "rt"],"x2": "b"}                 |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34?}}}}}}                       |
      | {"x":  "b\"}                                                     |

  @attribute_value_application_json_forbidden @BUG_1905
  Scenario Outline:  update attributes value by entity ID and atrribute name using NGSI v2 with application/json in Content-Type and forbidden values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_update_attribute_value |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
     # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | <entity_id> |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
     # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid characters in attribute value |
    Examples:
      | entity_id | attributes_value |
      | room1     | {"x": "<a>"}     |
      | room2     | {"x": "a=5"}     |
      | room3     | {"x": "a;"}      |
      | room4     | {"x": "a(7)"}    |

  @attribute_value_update_without_attribute_type
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                                    |
      | Fiware-Service     | test_update_attr_value_without_attr_type |
      | Fiware-ServicePath | /test                                    |
      | Content-Type       | application/json                         |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | fdgdfgfd         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=100000    |
      | random=1000000   |

  @attribute_value_update_with_attribute_type
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and with attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_update_attr_value_with_attr_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | fdgdfgfd         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=100000    |
      | random=1000000   |

  @attribute_value_update_with_metadatas
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with several attribute values and with metadatas
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_update_attr_value_with_metadata |
      | Fiware-ServicePath | /test                                |
      | Content-Type       | application/json                     |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value |
      | fdgdfgfd         |
      | 34               |
      | 34.4E-34         |
      | temp.34          |
      | temp_34          |
      | temp-34          |
      | TEMP34           |
      | house_flat       |
      | house.flat       |
      | house-flat       |
      | house@flat       |
      | random=10        |
      | random=100       |
      | random=1000      |
      | random=10000     |
      | random=100000    |
      | random=1000000   |

  @attribute_value_update_without_attribute_type_special @BUG_1106
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and without attribute type nor metadatas
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value                                                              |
      | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                         |
      | {"x": {"x1": "a","x2": "b"}}                                                  |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attribute_value_update_with_attribute_type_special @BUG_1106
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with attribute type
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value                                                              |
      | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                         |
      | {"x": {"x1": "a","x2": "b"}}                                                  |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attribute_value_update_with_metadata_special @BUG_1106
  Scenario Outline:  update an attribute value by entity ID and attribute name using NGSI v2 with special attribute values (compound, vector, boolean, etc) and with metadatas
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
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
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo
    Examples:
      | attributes_value                                                              |
      | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | {"x": "x1","x2": "b"}                                                         |
      | {"x": {"x1": "a","x2": "b"}}                                                  |
      | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |

  @attribute_value_invalid @BUG_1424
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 without invalid attribute values in update request
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_update_attr_value_error |
      | Fiware-ServicePath | /test                        |
      | Content-Type       | application/json             |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                 |
      | error       | BadRequest                            |
      | description | Invalid characters in attribute value |
    Examples:
      | attributes_value |
      | house<flat>      |
      | house=flat       |
      | house"flat"      |
      | house'flat'      |
      | house;flat       |
      | house(flat)      |

  @attribute_value_not_recognized
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with not recognized attribute values in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
    # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                               |
      | error       | BadRequest                          |
      | description | attribute value type not recognized |
    Examples:
      | attributes_value |
      | rwerwer          |
      | rwerwer"         |
      | 34r              |
      | 5_34             |
      | ["a", "b"        |
      | ["a" "b"]        |
      | ["a" "b"}        |
      | {"a": "b"        |
      | {"a" "b"}        |
      | TrUe             |
      | FaLsE            |

  @attribute_value_missing_citation_mark
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 and missing citation-mark in attribute values in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
     # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value    |
      | attributes_value | "rwerwer |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | Missing citation-mark at end of string |

  @attribute_value_error
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with wrong attribute values in update request
    Given  a definition of headers
      | parameter          | value                               |
      | Fiware-Service     | test_update_attribute_value_special |
      | Fiware-ServicePath | /test                               |
      | Content-Type       | application/json                    |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_number | 2           |
      | metadatas_name   | very_hot    |
      | metadatas_type   | alarm       |
      | metadatas_value  | hot         |
    And create entity group with "1" entities in "normalized" mode
    And verify that receive several "Created" http code
   # These properties below are used in update request
    And properties to entities
      | parameter        | value              |
      | attributes_value | <attributes_value> |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | attributes_value |
      | rwerwer          |
      | 34r              |
      | 5_34             |
      | ["a", "b"        |
      | ["a" "b"]        |
      | "a", "b"]        |
      | ["a" "b"}        |
      | {"a": "b"        |
      | {"a" "b"}        |
      | "a": "b"}        |
      | TrUe             |
      | FaLsE            |

   #   -------------- queries parameters ------------------------------------------
   #   ---  type query parameter ---

  @more_entities
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with more than one entity with the same id
    Given  a definition of headers
      | parameter          | value                   |
      | Fiware-Service     | test_attr_more_entities |
      | Fiware-ServicePath | /test                   |
      | Content-Type       | application/json        |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
      # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @qp_type
  Scenario:  update an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
      # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 80      |
      # query parameter
      | qp_type          | house_1 |
    When update an attribute value by ID "room" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "No Content" http code
    And verify that an entity is updated in mongo

  @qp_type_empty
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with empty value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
     # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value |
      | attributes_value | 80    |
      # query parameter
      | qp_type          |       |
    When update an attribute value by ID "room_1" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |

  @qp_invalid
  Scenario Outline:  try to update an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with invalid value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
     # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 80      |
      # query parameter
      | qp_<parameter>   | house_1 |
    When update an attribute value by ID "room_1" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | invalid character in URI parameter |
    Examples:
      | parameter           |
      | ========            |
      | p<flat>             |
      | p=flat              |
      | p'flat'             |
      | p\'flat\'           |
      | p;flat              |
      | p(flat)             |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |

  @qp_type_unknown @BUG_1909
  Scenario:  try to update an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with unknown type value
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_attr_type_qp |
      | Fiware-ServicePath | /test             |
      | Content-Type       | application/json  |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room        |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
     # These headers below are appended to previous headers in update request
    And modify headers and keep previous values "true"
      | parameter    | value      |
      | Content-Type | text/plain |
     # These properties below are used in update request
    And properties to entities
      | parameter        | value   |
      | attributes_value | 80      |
      # query parameter
      | qp_type          | gdfgdfg |
    When update an attribute value by ID "room_1" and attribute name "temperature" if it exists in raw mode
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |