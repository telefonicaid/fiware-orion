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
# __author__ = 'Jon Calderin Goñi (jon dot caldering at gmail dot com)'

Feature: Subscribe context tests

  Scenario: Reset the context broker with ontime subscriptions active
    # Set mock
    Given a started mock
    And set the response of the context provider mock in path "/subscription" as "ok"
    # Subscribe context
    Given a new NGSI version "10" petition with the service "service" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | entity1   | type1       |
    And a notify conditions with the following data
      | type   | time | attributes  |
      | ontime | P1M  | temperature |
    And build the standard context subscription ontime payload with the previous data and the following
      | attributes(list, separated by comma) | reference     | duration |
      | temperature                          | /subscription | P1M      |
    And a standard context subscription is asked with the before information