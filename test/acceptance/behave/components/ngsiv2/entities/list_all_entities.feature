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


Feature: list all entities with get requests in NGSI v2
  As a context broker user
  I would like to list all entities with get requests in NGSI v2
  So that I can manage and use them in my scripts

  BackgroundFeature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file and restart service
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  @happy_path
  Scenario:  list all entities in NGSI v2
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_happy_path |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  # ------------------------ Service ----------------------------------------------
  @service
  Scenario Outline:  list all entities in NGSI v2 with several services headers
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | <service>        |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | service            |
      |                    |
      | service            |
      | service_12         |
      | service_sr         |
      | SERVICE            |
      | max length allowed |

  @service_without
  Scenario:  list all entities in NGSI v2 without service header
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @service_error
  Scenario Outline:  list all entities in NGSI v2 with several wrong services headers
    Given  a definition of headers
      | parameter          | value     |
      | Fiware-Service     | <service> |
      | Fiware-ServicePath | /test     |
    When get all entities
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                                                                                         |
      | error       | BadRequest                                                                                                                                    |
      | description | tenant name not accepted - a tenant string must not be longer than 50 characters and may only contain underscores and alphanumeric characters |
    Examples:
      | service                         |
      | service.sr                      |
      | Service-sr                      |
      | Service(sr)                     |
      | Service=sr                      |
      | Service<sr>                     |
      | Service,sr                      |
      | greater than max length allowed |

 # ------------------------ Service path ----------------------------------------------
  @service_path
  Scenario Outline:  list all entities in NGSI v2 with several service paths headers
    Given  a definition of headers
      | parameter          | value             |
      | Fiware-Service     | test_service_path |
      | Fiware-ServicePath | <service_path>    |
      | Content-Type       | application/json  |
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
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
  Scenario:  list all entities in NGSI v2 without service path header
    Given  a definition of headers
      | parameter      | value                     |
      | Fiware-Service | test_service_path_without |
      | Content-Type   | application/json          |
    And create "5" entities with "1" attributes
      | parameter        | value       |
      | entities_type    | room        |
      | entities_id      | room2       |
      | attributes_name  | temperature |
      | attributes_value | 34          |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @service_path_error
  Scenario Outline:  list all entities in NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario Outline:  list all entities in NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario Outline:  list all entities in NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                        |
      | Fiware-Service     | test_list_service_path_error |
      | Fiware-ServicePath | <service_path>               |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
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
  Scenario:  list all entities in NGSI v2 with wrong service path header
    Given  a definition of headers
      | parameter          | value                                |
      | Fiware-Service     | test_list_service_path_error         |
      | Fiware-ServicePath | max length allowed and eleven levels |
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                              |
      | error       | BadRequest                         |
      | description | too many components in ServicePath |

  @without_attribute_type
  Scenario:  list all entities in NGSI v2 but without attribute type
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_without_attribute_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @with_metadatas
  Scenario:  list all entities in NGSI v2 with metadatas
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_happy_path |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  @without_metadata_type
  Scenario:  list all entities in NGSI v2 without metadata type
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_without_metadata_type |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "5" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 3     |
      | offset    | 2     |
    Then verify that receive an "OK" http code
    And verify that all entities are returned

  # ------------------ queries parameters -------------------------------

  @only_limit
  Scenario Outline:  list all entities in NGSI v2 with only limit query parameter
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_list_only_limit |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value   |
      | limit     | <limit> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | limit |
      | 1     |
      | 20    |

  @only_limit_error
  Scenario Outline:  list all entities in NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_list_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value   |
      | limit     | <limit> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                      |
      | error       | BadRequest                                                 |
      | description | Bad pagination limit: /<limit>/ [must be a decimal number] |
    Examples:
      | limit |
      | -3    |
      | 1.5   |
      | ewrw  |
      | <2    |
      | %@#   |

  @only_limit_error
  Scenario:  list all entities in NGSI v2 with only wrong limit query parameter
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_list_only_limit_error |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value |
      | limit     | 0     |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                       |
      | error       | BadRequest                                                  |
      | description | Bad pagination limit: /0/ [a value of ZERO is unacceptable] |

  @only_offset
  Scenario Outline:  list all entities in NGSI v2 with only offset query parameter
    Given  a definition of headers
      | parameter          | value                 |
      | Fiware-Service     | test_list_only_offset |
      | Fiware-ServicePath | /test                 |
      | Content-Type       | application/json      |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | offset |
      | 0      |
      | 1      |
      | 20     |

  @only_offset_error
  Scenario Outline:  list all entities in NGSI v2 with only wrong offset query parameter
    Given  a definition of headers
      | parameter          | value                       |
      | Fiware-Service     | test_list_only_offset_error |
      | Fiware-ServicePath | /test                       |
      | Content-Type       | application/json            |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value    |
      | offset    | <offset> |
    Then verify that receive an "Bad Request" http code
    And verify an error response
      | parameter   | value                                                        |
      | error       | BadRequest                                                   |
      | description | Bad pagination offset: /<offset>/ [must be a decimal number] |
    Examples:
      | offset |
      | -3     |
      | 1.5    |
      | ewrw   |
      | <2     |
      | %@#    |

  @limit_offset
  Scenario Outline:  list all entities in NGSI v2 with limit and offset queries parameters
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_list_limit_offset |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    And create "10" entities with "2" attributes
      | parameter        | value                   |
      | entities_type    | room                    |
      | entities_id      | room2                   |
      | attributes_name  | timestamp               |
      | attributes_value | 017-06-17T07:21:24.238Z |
      | attributes_type  | date                    |
      | metadatas_number | 2                       |
      | metadatas_name   | very_hot                |
      | metadatas_type   | alarm                   |
      | metadatas_value  | random=10               |
    And verify that receive several "Created" http code
    When get all entities
      | parameter | value    |
      | limit     | <limit>  |
      | offset    | <offset> |
    Then verify that receive an "OK" http code
    And verify that all entities are returned
    Examples:
      | limit | offset |
      | 1     | 1      |
      | 20    | 1      |
      | 1     | 20     |
      | 5     | 3      |

