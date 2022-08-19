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

@issue-715
Feature: CPr: propagate X-Auth-Token header in forwarded requests

  Background:
    Given the Context Broker started with multitenancy

  Scenario: Forward the X-Auth-Token for the context provider in a query operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_715" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application     |
      | /context_provider/service |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_715" and the subservice "/subservice"
    And add the following headers to the request
      | header       | value |
      | X-Auth-Token | aaaa  |
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data
    When a standard query context is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And headers of the last mock request contains the head "X-Auth-Token" with the value "aaaa"
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_715"

  Scenario: Forward the X-Auth-Token for the context provider in an update operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/updateContext" as "update_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_715" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application     |
      | /context_provider/service |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_715" and the subservice "/subservice"
    And add the following headers to the request
      | header       | value |
      | X-Auth-Token | aaaa  |
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att1           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And headers of the last mock request contains the head "X-Auth-Token" with the value "aaaa"
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_715"

  Scenario: Do not forward the X-Auth-Token for the context provider in a query operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_715" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application     |
      | /context_provider/service |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_715" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data
    When a standard query context is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And headers of the last mock request not contains the head "X-Auth-Token"
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_715"

  Scenario: Do not forward the X-Auth-Token for the context provider in a update operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/updateContext" as "update_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_715" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application     |
      | /context_provider/service |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_715" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att1           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And headers of the last mock request not contains the head "X-Auth-Token"
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_715"

#Fixme: Repeat all tests with convenience operation, when developed. This is pending on the "big refactoring" in CPr funciontality being done in 1Q2015"