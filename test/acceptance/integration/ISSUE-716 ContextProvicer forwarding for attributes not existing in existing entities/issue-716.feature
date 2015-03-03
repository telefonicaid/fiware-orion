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

@issue-716
Feature: Context Provider forwarding for attributes not existing in existing entities

  Background:
    Given the Context Broker started with multitenancy

  Scenario: Query an entity in the CB and in the CP with the attributes in the CP
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data and the following attributes
      | attributes |
      | att1       |
    When a standard query context is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_716"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CB
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data and the following attributes
      | attributes |
      | att2       |
    When a standard query context is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CP but in other subservice (Not found is ok)
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice2"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice2"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data and the following attributes
      | attributes |
      | att1       |
    When a standard query context is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"
    And check the response has the key "errorCode"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CP but in other service (Not found is ok)
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716_2" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Query consult
    And a new "NGSI10" api request with the service "issue_716_2" and the subservice "/subservice"
    And the following entities to consult
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard query context payload with the previous data and the following attributes
      | attributes |
      | att1       |
    When a standard query context is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"
    And clean the mongo database of the service "issue_716_2"
    And check the response has the key "errorCode"

    # Update

  Scenario: Update an entity in the CB and in the CP with the attributes in the CP
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/updateContext" as "update_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att1           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And the path in the last mock request contains "service"
    And there is "1" requests sent to the mock
    And clean the mongo database of the service "issue_716"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CB
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CP but in other subservice (Not found is ok)
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice2"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_716" and the subservice "/subservice2"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att1           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"
    And check the response has the key "code" with the value "472"

  Scenario: Query an entity in the CB and in the CP with the attributes in the CP but in other service (Not found is ok)
    Given a started mock
    And set the response of the mock in the path "/context_provider/service/queryContext" as "query_context_response_from_context_provider_xml"
    # First registration
    And a new "NGSI9" api request with the service "issue_716" and the subservice "/subservice"
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
    # Append operation
    And a new "NGSI10" api request with the service "issue_716_2" and the subservice "/subservice"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att2           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity creation payload with the previous data
    And a standard context entity creation is asked with the before information
    # Update operation
    And a new "NGSI10" api request with the service "issue_716_2" and the subservice "/subservice2"
    And the following attributes to create
      | attribute_name | attribute_type | attribute_value |
      | att1           | att_type_1     | 25              |
    And a context elements with the before attrs and the following entities
      | entity_id | entity_type |
      | Room1     | Room        |
    And build the standard entity update payload with the previous data
    When a standard context entity update is asked with the before information
    # Mock information
    Then retrieve information from the mock
    And there is "0" requests sent to the mock
    And clean the mongo database of the service "issue_716"
    And clean the mongo database of the service "issue_716_2"
    And check the response has the key "code" with the value "404"
