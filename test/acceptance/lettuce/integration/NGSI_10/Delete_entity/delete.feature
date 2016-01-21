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
@delete_entity
Feature: Subscribe context tests

  Background:
    Given the Context Broker started with multitenancy

  @delete_entity
  Scenario: Delete an entity does not exist with convenience and type extended
    # Append
    Given a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | temperature    | centigrade     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | RoomOne     |
      | Room2     | RoomTwo     |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    And a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    #Fixme: these steps are comment per changes in main library cb_utils.py
    #When a convenience delete context is asked with the following data
    #  | entity_id | entity_type |
    #  | Room2     | RoomOne     |
    #Then check the response has the key "code" with the value "404"