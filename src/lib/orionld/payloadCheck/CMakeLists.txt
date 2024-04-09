# Copyright 2019 FIWARE Foundation e.V.
#
# This file is part of Orion-LD Context Broker.
#
# Orion-LD Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion-LD Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# orionld at fiware dot org

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

SET (SOURCES
     fieldPaths.cpp
     pCheckAttribute.cpp
     pCheckAttributeType.cpp
     pCheckEntity.cpp
     pCheckEntityId.cpp
     pCheckEntityType.cpp
     pCheckGeoLineStringCoordinates.cpp
     pCheckGeoMultiLineStringCoordinates.cpp
     pCheckGeoMultiPointCoordinates.cpp
     pCheckGeoMultiPolygonCoordinates.cpp
     pCheckGeoPointCoordinates.cpp
     pCheckGeoPolygonCoordinates.cpp
     pCheckGeoProperty.cpp
     pCheckGeoPropertyType.cpp
     pCheckGeoPropertyValue.cpp
     pCheckNotification.cpp
     pCheckSubscription.cpp
     pCheckUri.cpp
     pCheckStringArray.cpp
     pcheckAttribute.cpp
     pcheckAttrs.cpp
     pcheckEndpoint.cpp
     pcheckEntity.cpp
     pcheckEntityInfo.cpp
     pcheckEntityInfoArray.cpp
     pcheckGeoPropertyValue.cpp
     pCheckGeo.cpp
     pcheckGeoQ.cpp
     pCheckGeometry.cpp
     pCheckGeoCoordinates.cpp
     pCheckGeorel.cpp
     pCheckGeorelString.cpp
     pcheckInformation.cpp
     pcheckInformationItem.cpp
     pcheckLanguagePropertyValue.cpp
     pcheckName.cpp
     pcheckNotifierInfo.cpp
     pcheckQ.cpp
     pcheckQuery.cpp
     pcheckReceiverInfo.cpp
     pcheckRegistration.cpp
     pcheckTimeInterval.cpp
     pCheckQuery.cpp
     pCheckRegistrationMode.cpp
     pCheckRegistrationOperations.cpp
     pCheckKeyValueArray.cpp
     pCheckRegistrationManagement.cpp
     pCheckTenant.cpp
     pCheckScope.cpp
     pCheckQueryParams.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_payloadCheck STATIC ${SOURCES})
