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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    mongoCppLegacyInit.cpp
    mongoCppLegacyEntityLookup.cpp
    mongoCppLegacyEntityAttributeLookup.cpp
    mongoCppLegacyEntityUpdate.cpp
    mongoCppLegacyEntityDelete.cpp
    mongoCppLegacyEntityBatchDelete.cpp
    mongoCppLegacyKjTreeFromBsonObj.cpp
    mongoCppLegacyKjTreeToBsonObj.cpp
    mongoCppLegacySubscriptionMatchEntityIdAndAttributes.cpp
    mongoCppLegacyEntityListLookupWithIdTypeCreDate.cpp
    mongoCppLegacyRegistrationLookup.cpp
    mongoCppLegacyRegistrationExists.cpp
    mongoCppLegacyRegistrationDelete.cpp
    mongoCppLegacyEntityAttributesDelete.cpp
    mongoCppLegacySubscriptionGet.cpp
    mongoCppLegacySubscriptionReplace.cpp
    mongoCppLegacyRegistrationGet.cpp
    mongoCppLegacyRegistrationReplace.cpp
    mongoCppLegacyEntityTypesFromRegistrationsGet.cpp
    mongoCppLegacyEntitiesGet.cpp
    mongoCppLegacyTenantsGet.cpp
    mongoCppLegacyGeoIndexInit.cpp
    mongoCppLegacyGeoIndexCreate.cpp
    mongoCppLegacyDbFieldGet.cpp
    mongoCppLegacyDbStringFieldGet.cpp
    mongoCppLegacyDbArrayFieldGet.cpp
    mongoCppLegacyDbNumberFieldGet.cpp
    mongoCppLegacyDbObjectFieldGet.cpp
    mongoCppLegacyEntityRetrieve.cpp
    mongoCppLegacyEntitiesQuery.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_mongoCppLegacy STATIC ${SOURCES})
