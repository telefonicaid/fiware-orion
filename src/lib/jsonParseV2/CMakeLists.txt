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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    jsonRequestTreat.cpp
    parseEntity.cpp
    parseContextAttribute.cpp
    parseContextAttributeCompoundValue.cpp
    parseMetadataVector.cpp
    parseMetadata.cpp
    parseAttributeValue.cpp
    jsonParseTypeNames.cpp
    parseSubscription.cpp
    parseBatchQuery.cpp
    parseEntityVector.cpp
    parseEntitiesVector.cpp
    parseEntityObject.cpp
    parseStringList.cpp
    parseScope.cpp
    parseBatchUpdate.cpp
    utilsParse.cpp
    parseMetadataCompoundValue.cpp
    parseRegistration.cpp
    badInput.cpp
    parseStringVector.cpp
    parseNotification.cpp
    parseExpression.cpp
    parseEntitiesResponse.cpp
)

SET (HEADERS
    jsonRequestTreat.h
    parseEntity.h
    parseContextAttribute.h
    parseContextAttributeCompoundValue.cpp
    parseMetadataVector.h
    parseMetadata.h
    parseAttributeValue.h
    jsonParseTypeNames.h
    parseSubscription.h
    parseBatchQuery.h
    parseEntityVector.h
    parseEntitiesVector.h
    parseEntityObject.h
    parseScope.h
    parseStringList.h
    parseBatchUpdate.h
    utilsParse.h
    parseMetadataCompoundValue.h
    parseRegistration.h
    parseStringVector.h
    badInput.h
    parseNotification.h
    parseExpression.h
    parseEntitiesResponse.h
)



# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(jsonParseV2 STATIC ${SOURCES} ${HEADERS})
