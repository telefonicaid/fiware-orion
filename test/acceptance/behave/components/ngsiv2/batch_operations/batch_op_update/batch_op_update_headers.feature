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


Feature: Batch operation - update using NGSI v2. "POST" - /v2/op/update plus payload and queries parameters
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

  @happy_path
  Scenario Outline:  update several entities with batch operations using NGSI v2
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_op_update_happy_path |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | metadatas_name   | warning     |
      | metadatas_value  | hot         |
      | metadatas_type   | alarm       |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
      | attributes_type  | celsius     |
      | metadatas_number | 2           |
      | metadatas_name   | warning     |
      | metadatas_value  | cold        |
      | metadatas_type   | alarm       |
    When update entities in a single batch operation "<operation>"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | operation     |
      | APPEND        |
      | APPEND_STRICT |

  @happy_path
  Scenario Outline:  update several entities with batch operations using NGSI v2 with keyValues format
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_op_update_happy_path |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    When update entities in a single batch operation "<operation>"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | operation     |
      | APPEND        |
      | APPEND_STRICT |

  @happy_path
  Scenario:  update several entities  with batch operations) using NGSIv2
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_op_update_happy_path |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 44          |
      | attributes_type  | celsius     |
    When update entities in a single batch operation "APPEND"
      | parameter | value     |
      | options   | keyValues |
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 56          |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 45          |
      | attributes_type  | kelvin      |
    When update entities in a single batch operation "UPDATE"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code

  @happy_path
  Scenario:  delete several entities  with batch operations) using NGSIv2
    Given  a definition of headers
      | parameter          | value                     |
      | Fiware-Service     | test_op_update_happy_path |
      | Fiware-ServicePath | /test                     |
      | Content-Type       | application/json          |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 44          |
      | attributes_type  | celsius     |
    When update entities in a single batch operation "APPEND"
      | parameter | value     |
      | options   | keyValues |
    And verify that receive a "No Content" http code
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | house       |
      | entities_id      | room1       |
    And define a entity properties to update in a single batch operation
      | parameter        | value       |
      | entities_type    | home        |
      | entities_id      | room2       |
    When update entities in a single batch operation "DELETE"
      | parameter | value     |
      | options   | keyValues |
    Then verify that receive a "No Content" http code

  @maximum_size @too_slow
   # 7239 entities is a way of generating a request longer than 1MB (in fact, 1048583 bytes)
  Scenario:  try to update several entities with batch operations using NGSI v2 with maximum size in payload
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_op_update_maximum_size |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_number    | 7239        |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Request Entity Too Large" http code
    And verify an error response
      | parameter   | value                                              |
      | error       | RequestEntityTooLarge                              |
      | description | payload size: 1048583, max size supported: 1048576 |

  @length_required
  Scenario:  try to update several entities with batch operations using NGSI v2 wihout payload
    Given  a definition of headers
      | parameter          | value                          |
      | Fiware-Service     | test_op_update_length_required |
      | Fiware-ServicePath | /test                          |
      | Content-Type       | application/json               |
    When update entities in a single batch operation "APPEND"
      | parameter | value |
      | payload   | empty |
    Then verify that receive a "Content Length Required" http code
    And verify an error response
      | parameter   | value                                            |
      | error       | ContentLengthRequired                            |
      | description | Zero/No Content-Length in PUT/POST/PATCH request |

  # ---------- Content-Type header --------------------------------

  @content_type_without
  Scenario:  try to update several entities with batch operations using NGSI v2 without Content-Type header
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_op_update_content_type |
      | Fiware-ServicePath | /test                       |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                                                           |
      | error       | UnsupportedMediaType                                                            |
      | description | Content-Type header not used, default application/octet-stream is not supported |

  @content_type_error
  Scenario Outline:  try to update several entities with batch operations using NGSI v2 with wrong Content-Type header
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_op_update_content_type |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | <content_type>              |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive an "Unsupported Media Type" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | UnsupportedMediaType                       |
      | description | not supported content type: <content_type> |
    Examples:
      | content_type                      |
      | application/x-www-form-urlencoded |
      | application/xml                   |
      | multipart/form-data               |
      | text/plain                        |
      | text/html                         |
      | dsfsdfsdf                         |
      | <sdsd>                            |
      | (eeqweqwe)                        |

  # ---------- Service header --------------------------------

  @service_without
  Scenario:  update several entities with batch operations using NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo

  @service
  Scenario Outline: update several entities with batch operations using NGSI v2 with several service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_error
  Scenario Outline:  try to update several entities with batch operations using NGSI v2 with several wrong service header values
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                  |
      | error       | BadRequest                                                                             |
      | description | bad character in tenant name - only underscore and alphanumeric characters are allowed |
    Examples:
      | service       |
      | servicedot.sr |
      | Service-sr    |
      | Service(sr)   |
      | Service=sr    |
      | Service<sr>   |
      | Service,sr    |
      | service#sr    |
      | service%sr    |
      | service&sr    |

  @service_bad_length
  Scenario:  try to update several entities with batch operations using NGSI v2 with bad length in service header
    Given  a definition of headers
      | parameter          | value                           |
      | Fiware-Service     | greater than max length allowed |
      | Fiware-ServicePath | /test                           |
      | Content-Type       | application/json                |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                    |
      | error       | BadRequest                                               |
      | description | bad length - a tenant name can be max 50 characters long |

  # ---------- Services path header --------------------------------

  @service_path
  Scenario Outline:  update entities with batch operations using NGSI v2 with several service path header value
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_op_update_service_path |
      | Fiware-ServicePath | <service_path>              |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo
    Examples:
      | service_path                                                  |
      | /                                                             |
      | /service_path                                                 |
      | /service_path_12                                              |
      | /Service_path                                                 |
      | /SERVICE                                                      |
      | /serv1/serv2/serv3/serv4/serv5/serv6/serv7/serv8/serv9/serv10 |
      | max length allowed                                            |
      | max length allowed and ten levels                             |

  @service_path_empty
  Scenario:  update entities with batch operations using NGSI v2 with empty value in service path header
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_op_update_service_path |
      | Fiware-ServicePath |                             |
      | Content-Type       | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo

  @service_path_without
  Scenario:  update entities with batch operations using NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                       |
      | Fiware-Service | test_op_update_service_path |
      | Content-Type   | application/json            |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "No Content" http code
    And verify that entities are stored in mongo

  @service_path_error
  Scenario Outline:  try to update entities with batch operations using NGSI v2 with wrong service path header values
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_op_update_service_path_error |
      | Fiware-ServicePath | <service_path>                    |
      | Content-Type       | application/json                  |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
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
  Scenario Outline:  try to update entities with batch operations using NGSI v2 with wrong service paths header vslues
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_op_update_service_path_error |
      | Fiware-ServicePath | <service_path>                    |
      | Content-Type       | application/json                  |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                    |
      | error       | BadRequest                                                               |
      | description | Only /absolute/ Service Paths allowed [a service path must begin with /] |
    Examples:
      | service_path |
      | sdffsfs      |
      | /service,sr  |

  @service_path_error
  Scenario Outline:  try to update entities with batch operations using NGSI v2 with component-name too long in service path header values
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_op_update_service_path_error |
      | Fiware-ServicePath | <service_path>                    |
      | Content-Type       | application/json                  |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | component-name too long in ServicePath |
    And verify that entities are not stored in mongo
    Examples:
      | service_path                                   |
      | greater than max length allowed                |
      | greater than max length allowed and ten levels |

  @service_path_error
  Scenario:  try to update entities with batch operations using NGSI v2 with too many components in service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_op_update_service_path_error    |
      | Fiware-ServicePath | max length allowed and eleven levels |
      | Content-Type       | application/json                     |
    # These properties below are used in bach operation request
    And define a entity properties to update in a single batch operation
      | parameter          | value       |
      | entities_type      | house       |
      | entities_id        | room1       |
      | entities_prefix_id | true        |
      | attributes_number  | 2           |
      | attributes_name    | temperature |
      | attributes_value   | 34          |
      | attributes_type    | celsius     |
      | metadata_number    | 2           |
      | metadata_name      | warning     |
      | metadata_value     | hot         |
      | metadata_type      | alarm       |
    When update entities in a single batch operation "APPEND"
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |
