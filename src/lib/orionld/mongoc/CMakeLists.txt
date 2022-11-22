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
    mongocAttributeDelete.cpp
    mongocAttributesAdd.cpp
    mongocConnectionGet.cpp
    mongocConnectionRelease.cpp
    mongocContextCacheDelete.cpp
    mongocContextCacheGet.cpp
    mongocContextCachePersist.cpp
    mongocEntitiesDelete.cpp
    mongocEntitiesExist.cpp
    mongocEntitiesQuery.cpp
    mongocEntitiesQuery2.cpp
    mongocEntitiesUpsert.cpp
    mongocEntityDelete.cpp
    mongocEntityGet.cpp
    mongocEntityInsert.cpp
    mongocEntityLookup.cpp
    mongocEntityReplace.cpp
    mongocEntityRetrieve.cpp
    mongocEntityUpdate.cpp
    mongocGeoIndexCreate.cpp
    mongocGeoIndexInit.cpp
    mongocIdIndexCreate.cpp
    mongocInit.cpp
    mongocKjTreeFromBson.cpp
    mongocKjTreeToBson.cpp
    mongocRegistrationLookup.cpp
    mongocServerVersionGet.cpp
    mongocSubCachePopulateByTenant.cpp
    mongocSubCountersUpdate.cpp
    mongocSubscriptionDelete.cpp
    mongocSubscriptionExists.cpp
    mongocSubscriptionInsert.cpp
    mongocSubscriptionLookup.cpp
    mongocSubscriptionReplace.cpp
    mongocTenantExists.cpp
    mongocTenantsGet.cpp
    mongocEntitiesGet.cpp
    mongocEntityTypesFromRegistrationsGet.cpp
    mongocEntityTypeGet.cpp
    mongocSubscriptionsGet.cpp
    mongocRegistrationExists.cpp
    mongocRegistrationInsert.cpp
    mongocRegistrationGet.cpp
    mongocRegistrationsGet.cpp
    mongocRegistrationDelete.cpp
    mongocRegistrationReplace.cpp
    mongocRegistrationsIter.cpp
    mongocAuxAttributesFilter.cpp
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_mongoc STATIC ${SOURCES})
