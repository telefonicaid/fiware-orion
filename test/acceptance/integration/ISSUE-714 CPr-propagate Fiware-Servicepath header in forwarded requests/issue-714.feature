# -*- coding: latin-1 -*-
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

  @issue-714
Feature: When the ContextBroker forwards a requests to a Context Provider, the header Fiware-Servicepath, is fowareded too
  # Enter feature description here
#TODO: Check if there is more operations forwarded

  Background:
    Given the Context Broker started with multitenancy

  Scenario: Fiware-Servicepath header is forwarded to a Context Provider query operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service1/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_714" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application      |
      | /context_provider/service1 |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_714" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data
    When a standard query context is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And the path in the last mock request contains "service1"
    And there is "1" requests sent to the mock
    And headers of the last mock request contains the head "Fiware-Servicepath" with the value "/subservice"
    And  clean the mongo database of the service "issue_714"

  Scenario: Fiware-Servicepath header is not forwarded to a Context Provider query operation if its not send
    Given a started mock
    And set the response of the mock in the path "/context_provider/service1/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_714" and the subservice "empty"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application      |
      | /context_provider/service1 |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_714" and the subservice "empty"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data
    When a standard query context is asked with the before information
    #Mock information
    Then retrieve information from the mock
    And the path in the last mock request contains "service1"
    And there is "1" requests sent to the mock
    And headers of the last mock request not contains the head "Fiware-Servicepath"
    And headers of the last mock request contains the head "Fiware-Service" with the value "issue_714"
    And  clean the mongo database of the service "issue_714"

  Scenario: Fiware-Servicepath header is forwarded to a Context Provider update operation
    Given a started mock
    And set the response of the mock in the path "/context_provider/service1/updateContext" as "update_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_714" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application      |
      | /context_provider/service1 |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_714" and the subservice "/subservice"
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
    And the path in the last mock request contains "service1"
    And there is "1" requests sent to the mock
    And headers of the last mock request contains the head "Fiware-Servicepath" with the value "/subservice"
    And  clean the mongo database of the service "issue_714"

  Scenario: Fiware-Servicepath header is not forwarded to a Context Provider update operation if it is not send
    Given a started mock
    And set the response of the mock in the path "/context_provider/service1/updateContext" as "update_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_714" and the subservice "empty"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And the following attributes to consult
      | attribute_name | attribute_type |
      | att1           | att_type_1     |
    And a context registrations with the before entities and attributes and the following providing applications
      | providing_application      |
      | /context_provider/service1 |
    And build the standard context registration payload with the previous data and duration "P1M"
    And a standard context registration is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_714" and the subservice "empty"
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
    And the path in the last mock request contains "service1"
    And there is "1" requests sent to the mock
    And headers of the last mock request not contains the head "Fiware-Servicepath"
    And clean the mongo database of the service "issue_714"
