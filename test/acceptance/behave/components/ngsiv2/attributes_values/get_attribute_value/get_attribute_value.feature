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


Feature: get an attribute value by entity ID and attribute name if it exists using NGSI v2 API. "GET" - /v2/entities/<entity_id>/attrs/<attr_name>/value
  As a context broker user
  I would like to get an attribute value by entity ID and attribute name if it exists using NGSI v2 API
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

  @happy_path
  Scenario:  get an attribute value by entity ID and attribute name if it exists using NGSI v2
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned

  @date_type @BUG_1885 @ISSUE_1604
  # this scenario is only to automate the check for a bug.
  # later, it is necessary more scenarios and examples to testing of the date type, See ISO8601 format.
  Scenario:  get an attribute value by entity ID and attribute name if it exists using NGSI v2 with date in attribute type
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value                   |
      | entities_type     | house                   |
      | entities_id       | room                    |
      | attributes_number | 3                       |
      | attributes_name   | timestamp               |
      | attributes_value  | 2035-06-15T07:21:24.00Z |
      | attributes_type   | date                    |
      | metadatas_number  | 2                       |
      | metadatas_name    | very_hot                |
      | metadatas_type    | alarm                   |
      | metadatas_value   | hot                     |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "timestamp_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned

   # ------------------------ Accept header ----------------------------------------------

  @accept_application_json
  Scenario Outline:  try to get an attribute value by entity ID and attribute name if it exists using NGSI v2 with text plain but with accept equals to application/json
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
   # These properties below are used in create request
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value            |
      | Accept    | application/json |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "Not Acceptable" http code
    And verify an error response
      | parameter   | value                           |
      | error       | NotAcceptable                   |
      | description | accepted MIME types: text/plain |
    Examples:
      | entity_id | attributes_value           |
      | room1     | true                       |
      | room2     | false                      |
      | room3     | 34                         |
      | room4     | -34                        |
      | room5     | 5.00002                    |
      | room6     | -5.00002                   |
      | room7     | "41.3763726, 2.1864475,14" |
      | room8     | "2017-06-17T07:21:24.238Z" |
      | room9     | null                       |

  @without_accept
  Scenario:  try to get an attribute value by entity ID and attribute name if it exists using NGSI v2 without Accept header
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Acceptable" http code
    And verify an error response
      | parameter   | value                           |
      | error       | NotAcceptable                   |
      | description | accepted MIME types: text/plain |

  @wrong_accept.row<row.id>
  @wrong_accept @BUG_1886 @skip
  Scenario Outline:  try to get an attribute value by entity ID and attribute name if it exists using NGSI v2 with wrong Accept header
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value    |
      | Accept    | <accept> |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Acceptable" http code
    And verify an error response
      | parameter   | value                           |
      | error       | NotAcceptable                   |
      | description | accepted MIME types: text/plain |
    Examples:
      | accept                            |
      | application/x-www-form-urlencoded |
      | application/xml                   |
      | multipart/form-data               |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  @json_with_accept_application_json
  Scenario Outline:  get an attribute value by entity ID and attribute name if it exists using NGSI v2 with application/json in Accept and json object in value
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value            |
      | Accept    | application/json |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | entity_id | attributes_value                                                              |
      | room1     | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | room2     | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | room3     | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | room4     | {"x": "x1","x2": "b"}                                                         |
      | room5     | {"x": {"x1": "a","x2": "b"}}                                                  |
      | room6     | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | room7     | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | room8     | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |


  @json_with_accept_text_plain
  Scenario Outline:  get an attribute value by entity ID and attribute name if it exists using NGSI v2 with text/plain in Accept and json object in value
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_update_happy_path |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
     # These properties below are used in create request
    And properties to entities
      | parameter        | value              |
      | entities_type    | "house"            |
      | entities_id      | "<entity_id>"      |
      | attributes_name  | "temperature"      |
      | attributes_value | <attributes_value> |
    And create an entity in raw and "normalized" modes
    And verify that receive a "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | entity_id | attributes_value                                                              |
      | room1     | true                                                                          |
      | room2     | false                                                                         |
      | room3     | 34                                                                            |
      | room4     | -34                                                                           |
      | room5     | 5.00002                                                                       |
      | room6     | -5.00002                                                                      |
      | room7     | [ "json", "vector", "of", 6, "strings", "and", 2, "integers" ]                |
      | room8     | [ "json", ["a", 34, "c", ["r", 4, "t"]], "of", 6]                             |
      | room9     | [ "json", ["a", 34, "c", {"r": 4, "t":"4", "h":{"s":"3", "g":"v"}}], "of", 6] |
      | room10    | {"x": "x1","x2": "b"}                                                         |
      | room11    | {"x": {"x1": "a","x2": "b"}}                                                  |
      | room12    | {"a":{"b":{"c":{"d": {"e": {"f": 34}}}}}}                                     |
      | room13    | {"x": ["a", 45, "rt"],"x2": "b"}                                              |
      | room14    | {"x": [{"a":78, "b":"r"}, 45, "rt"],"x2": "b"}                                |
      | room15    | "41.3763726, 2.1864475,14"                                                    |
      | room16    | "2017-06-17T07:21:24.238Z"                                                    |
      | room17    | null                                                                          |

   # ------------------------ Service header ----------------------------------------------
  @service
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with several services header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  get an attribute value by entity ID and attribute name using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned

  @service_error
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with wrong several services
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                  |
      | error       | BadRequest                                                                             |
      | description | bad character in tenant name - only underscore and alphanumeric characters are allowed |
    Examples:
      | service     |
      | service.sr  |
      | Service-sr  |
      | Service(sr) |
      | Service=sr  |
      | Service<sr> |
      | Service,sr  |
      | service#sr  |
      | service%sr  |
      | service&sr  |

  @service_bad_length
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with bad length several services
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

    # ------------------------ Service path header ----------------------------------------------

  @service_path
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with several service path headers
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_attr_service_path |
      | Fiware-ServicePath | <service_path>         |
      | Content-Type       | application/json       |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | service_path                                                  |
      |                                                               |
      | /                                                             |
      | /service_path                                                 |
      | /service_path_12                                              |
      | /Service_path                                                 |
      | /SERVICE                                                      |
      | /serv1/serv2/serv3/serv4/serv5/serv6/serv7/serv8/serv9/serv10 |
      | max length allowed                                            |
      | max length allowed and ten levels                             |

  @service_path_without
  Scenario:  get an attribute value by entity ID and attribute name using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                          |
      | Fiware-Service | test_attr_service_path_without |
      | Content-Type   | application/json               |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34.67       |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned

  @service_path_error
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with an illegal character service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
      | Accept             | text/plain                   |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | a component of ServicePath contains an illegal character |
    Examples:
      | service_path |
      | /service.sr  |
      | /service;sr  |
      | /service=sr  |
      | /Service-sr  |
      | /serv<45>    |
      | /serv(45)    |

  @service_path_error
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with only absolute value allowed in service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
      | Accept             | text/plain                   |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_error
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with component-name too long in service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_attr_service_path_error |
      | Fiware-ServicePath | <service_path>               |
      | Content-Type       | application/json             |
      | Accept             | text/plain                   |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_error
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with too many components in service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_attr_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
      | Accept             | text/plain                           |
    When get an attribute value by ID "room_1" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  #    -------------- entity Id ------------------------------------------

  @entity_id
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with several entity id values
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_attr_entity_id |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
    # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 4           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
      | attributes_type   | celsius     |
      | metadatas_number  | 2           |
      | metadatas_name    | very_hot    |
      | metadatas_type    | alarm       |
      | metadatas_value   | hot         |
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | entity_id |
      | room_0    |
      | room_1    |
      | room_4    |
      | room_2    |

  @entity_id_one_entity_not_ascii_plain
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with not ascii plain entity id values with one entity
    Given  a definition of headers
      | parameter          | value               |
      | Fiware-Service     | test_attr_entity_id |
      | Fiware-ServicePath | /test               |
      | Content-Type       | application/json    |
      | Accept             | text/plain          |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | entity_id  |
      | habitación |
      | españa     |
      | barça      |

  @entity_id_unknown
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with an unknown entity id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_attr_entity_unkwnown |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
     # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "fdgfdgfdgdfg" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  @entity_id_multiples
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with multiples entity id
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_attr_entity_unkwnown |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
  # These properties below are used in create request
    And properties to entities
      | parameter         | value       |
      | entities_type     | house       |
      | entities_id       | room        |
      | attributes_number | 3           |
      | attributes_name   | temperature |
      | attributes_value  | 34          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | type   | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room" and attribute name "temperature_0" if it exists
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @entity_id_forbidden
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with forbidden entity id
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Accept             | text/plain             |
    When get an attribute value by ID "<entity_id>" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | entity_id           |
      | house<flat>         |
      | house=flat          |
      | house'flat'         |
      | house\'flat\'       |
      | house;flat          |
      | house(flat)         |
      | {\'a\':34}          |
      | [\'34\', \'a\', 45] |
      | house_&             |
      | my house            |

  @entity_id_wrong
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with wrong entity id
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Accept             | text/plain             |
    When get an attribute value by ID "house_?" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                            |
      | error       | BadRequest                                                       |
      | description | Empty right-hand-side for URI param //attrs/temperature_0/value/ |

  @entity_id_wrong @BUG_1887 @skip
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with wrong entity id
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Accept             | text/plain             |
    When get an attribute value by ID "house_/" and attribute name "temperature_0" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value             |
      | error       | BadRequest        |
      | description | service not found |

  @entity_id_wrong
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with wrong entity id
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Accept             | text/plain             |
    When get an attribute value by ID "house_#" and attribute name "temperature_0" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |

  #   -------------- attribute name ------------------------------------------

  @attribute_name
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with several attribute name values
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_id_attr_attribute_name |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in create request
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
    And create entity group with "5" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "<attribute_name>" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | attribute_name |
      | temperature_0  |
      | temperature_1  |
      | temperature_3  |
      | temperature_2  |
      | temperature_4  |

  @attribute_name_without_type
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 without attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attr_attribute_name_without_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value            |
      | entities_type    | house            |
      | entities_id      | room             |
      | attributes_name  | <attribute_name> |
      | attributes_value | 34               |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "the same value of the previous request" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | attribute_name |
      | dfdsfdsf       |
      | sdfds:dsfds    |
      | 34             |
      | 34.4E-34       |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | random=10      |
      | random=100     |
      | random=256     |

  @attribute_name_not_ascii_plain
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with not ascii plain attribute type
    Given  a definition of headers
      | parameter          | value                                 |
      | Fiware-Service     | test_attr_attribute_name_without_type |
      | Fiware-ServicePath | /test                                 |
      | Content-Type       | application/json                      |
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "<attribute_name>" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
    Examples:
      | attribute_name |
      | habitación     |
      | españa         |
      | barça          |

  @attribute_name_with_type
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with attribute type
    Given  a definition of headers
      | parameter          | value                              |
      | Fiware-Service     | test_attr_attribute_name_with_type |
      | Fiware-ServicePath | /test                              |
      | Content-Type       | application/json                   |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value            |
      | entities_type    | house            |
      | entities_id      | room             |
      | attributes_name  | <attribute_name> |
      | attributes_value | 34               |
      | attributes_type  | celsius          |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "the same value of the previous request" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | attribute_name |
      | dfdsfdsf       |
      | sdfds:dsfds    |
      | 34             |
      | 34.4E-34       |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | random=10      |
      | random=100     |
      | random=256     |

  @attribute_name_with_metadatas
  Scenario Outline:  get an attribute value by entity ID and attribute name using NGSI v2 with attribute metadatas
    Given  a definition of headers
      | parameter          | value                                   |
      | Fiware-Service     | test_attr_attribute_name_with_metadatas |
      | Fiware-ServicePath | /test                                   |
      | Content-Type       | application/json                        |
    # These properties below are used in create request
    And properties to entities
      | parameter        | value            |
      | entities_type    | house            |
      | entities_id      | room             |
      | attributes_name  | <attribute_name> |
      | attributes_value | 34               |
      | metadatas_number | 3                |
      | metadatas_name   | very_hot         |
      | metadatas_type   | alarm            |
      | metadatas_value  | hot              |
    And create entity group with "3" entities in "normalized" mode
      | entity | prefix |
      | id     | true   |
    And verify that receive several "Created" http code
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "the same value of the previous request" if it exists
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned
    Examples:
      | attribute_name |
      | dfdsfdsf       |
      | sdfds:dsfds    |
      | 34             |
      | 34.4E-34       |
      | temp.34        |
      | temp_34        |
      | temp-34        |
      | TEMP34         |
      | house_flat     |
      | house.flat     |
      | house-flat     |
      | house@flat     |
      | random=10      |
      | random=100     |
      | random=256     |

  @attribute_name_unknown @BUG_1890 @skip
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with unknown attribute name
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_attr_attribute_name_unknown |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "dfdsfsdfds" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | NotFound                                   |
      | description | The entity does not have such an attribute |

  @attribute_name_forbidden
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 v2 with forbidden attribute_name
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Accept             | text/plain            |
    When get an attribute value by ID "room_1" and attribute name "<attribute_name>" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                    |
      | error       | BadRequest               |
      | description | invalid character in URI |
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
      | house_&             |
      | my house            |

  @attribute_name_invalid @BUG_1887 @skip
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with invalid attribute_name
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_update_entity_id |
      | Fiware-ServicePath | /test                 |
      | Accept             | text/plain            |
    When get an attribute value by ID "room_1" and attribute name "<attribute_name>" if it exists
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |
    Examples:
      | attribute_name |
      | house_/        |
      | house_#        |

  @attribute_name_wrong
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with wrong attribute_name
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_id_entity_invalid |
      | Fiware-ServicePath | /test                  |
      | Accept             | text/plain             |
    When get an attribute value by ID "room_1" and attribute name "house_?" if it exists
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                        |
      | error       | BadRequest                                   |
      | description | Empty right-hand-side for URI param //value/ |

   #   -------------- queries parameters ------------------------------------------
   #   ---  type query parameter ---

  @more_entities
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with more than one entity with the same id
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room" and attribute name "temperature_0" if it exists
    Then verify that receive an "Conflict" http code
    And verify an error response
      | parameter   | value                                                   |
      | error       | TooManyResults                                          |
      | description | More than one matching entity. Please refine your query |

  @qp_type
  Scenario:  get an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room" and attribute name "temperature" if it exists
      | parameter | value   |
      | type      | house_1 |
    Then verify that receive an "OK" http code
    And verify that the attribute value by ID is returned

  @qp_type_empty
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with empty value
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature" if it exists
      | parameter | value |
      | type      |       |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |

  @qp_unknown @BUG_1831 @skip
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with empty value
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature" if it exists
      | parameter   | value   |
      | <parameter> | house_1 |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | Empty right-hand-side for URI param /type/ |
    Examples:
      | parameter |
      |           |
      | dfgdfgfgg |
      | house_&   |
      | my house  |
      | p_/       |
      | p_#       |

  @qp_invalid
  Scenario Outline:  try to get an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with invalid value
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature" if it exists
      | parameter   | value   |
      | <parameter> | house_1 |
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

  @qp_type_unknown
  Scenario:  try to get an attribute value by entity ID and attribute name using NGSI v2 with "type" query parameter with unknown type value
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
    # These headers below are appended to previous headers in get request
    And modify headers and keep previous values "true"
      | parameter | value      |
      | Accept    | text/plain |
    When get an attribute value by ID "room_1" and attribute name "temperature" if it exists
      | parameter | value   |
      | type      | gdfgdfg |
    Then verify that receive an "Not Found" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | NotFound                                                   |
      | description | The requested entity has not been found. Check type and id |