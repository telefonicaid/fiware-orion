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

Feature: general operations in NGSI v1 and v2 requests
  As a context broker user
  I would like to verify general operations in NGSI v1 and v2 requests
  So that I can manage and use them in my scripts

  BackgroundFeature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file and restart service
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  @version
  Scenario: verify NGSI version request
    When send a version request
    Then verify that receive an "OK" http code
    And verify if version is the expected

  @version_fields
  Scenario Outline: verify fields in NGSI version request
    When send a version request
    Then verify that receive an "OK" http code
    And verify version "<field>" field does exists
  Examples:
    | field        |
    | version      |
    | uptime       |
    | git_hash     |
    | compile_time |
    | compiled_by  |
    | compiled_in  |

  @statistics
  Scenario Outline: verify fields in NGSI statistics request
    When send a statistics request
    Then verify that receive an "OK" http code
    And verify statistics "<field>" field does exists
  Examples:
    | field                      |
    | xmlRequests                |
    | versionRequests            |
    | statisticsRequests         |
    | uptime_in_secs             |
    | measuring_interval_in_secs |

  @api_entry_point
  Scenario Outline: verify NGSI v2 API entry point request
    When send a API entry point request
    Then verify that receive an "OK" http code
    And verify "<url>" url with "<value>" value in response
  Examples:
    | url               | value             |
    | entities_url      | /v2/entities      |
    | types_url         | /v2/types         |
    | subscriptions_url | /v2/subscriptions |
    | registrations_url | /v2/registrations |




