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


Feature: NGSI v2 base requests
  As a context broker user
  I would like to verify NGSI v2 base request
  So that I can manage and use them in my scripts

  BackgroundFeature:
    Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
    Setup: update contextBroker config file and restart service
    Check: verify contextBroker is installed successfully
    Check: verify mongo is installed successfully

  @version
  Scenario: verify NGSI v2 version request
    When send a version request
    Then verify that receive an "OK" http code
    And verify if version is the expected

  @statistics
  Scenario: verify NGSI v2 statistics request
    When send a statistics request
    Then verify that receive an "OK" http code
    And verify statistics

  @base
  Scenario: verify NGSI v2 base request
    When send a base request
    Then verify that receive an "OK" http code
    And verify main paths





