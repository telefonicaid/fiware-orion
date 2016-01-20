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

@subscribe_context
Feature: Subscribe context tests

  Background:
    Given the Context Broker started with multitenancy

  @reset_CB_with_ontime_sub
  Scenario: Reset the context broker with ontime subscriptions active
    # Set mock
    Given a started mock
    And set the response of the mock in the path "/subscription" as "ok"
    # Append
    And a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | temperature    | centigrade     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | RoomOne     |
      | Room2     | RoomOne     |
      | Room3     | RoomTwo     |
      | Room4     | RoomTwo     |
    And build the standard entity creation payload with the previous data
    When a standard context entity creation is asked with the before information
    # Subscribe context
    Given a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | RoomOne     |
      | Room2     | RoomOne     |
      | Room3     | RoomTwo     |
      | Room4     | RoomTwo     |
    And a notify conditions with the following data
      | type   | time  | attributes  |
      | ontime | PT10S | temperature |
    And build the standard context subscription ontime payload with the previous data and the following
      | attributes  | reference     | duration |
      | temperature | /subscription | PT1M     |
    And a standard context subscription is asked with the before information
    # Wait 12 seconds for the notification
    And wait "12" seconds
    # Check the mock gets the notify
    And retrieve information from the mock
    And there is "2" requests sent to the mock
    # Update the attribute value
    And a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | temperature    | centigrade     | 26              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | RoomOne     |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Wait 10 seconds for the notification
    And wait "10" seconds
    # Check the mock gets the notify
    And retrieve information from the mock
    And there is "3" requests sent to the mock
    And the "3" requests of the mock has the key "value" with the value "26"
    # Restart Context Broker
    And request a restart of cb
    And check cb is running
    # Update the attribute again
    And a new "NGSI10" api request with the service "service" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | temperature    | centigrade     | 27              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room2     | RoomOne     |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Wait 5 seconds for the notification
    And wait "5" seconds
    # Check the mock gets the notify
    And retrieve information from the mock
    And there is "4" requests sent to the mock
    And the "4" requests of the mock has the key "value" with the value "27"
    And print the information stored in the mock