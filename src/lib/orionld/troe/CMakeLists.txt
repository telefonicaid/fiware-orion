# Copyright 2020 FIWARE Foundation e.V.
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
    troeInit.cpp
    troeDeleteAttribute.cpp
    troeDeleteEntity.cpp
    troePatchAttribute.cpp
    troePatchEntity.cpp
    troePatchEntity2.cpp
    troePutEntity.cpp
    troePostBatchCreate.cpp
    troePostBatchDelete.cpp
    troePostBatchUpdate.cpp
    troePostBatchUpsert.cpp
    troePostEntities.cpp
    troePostEntity.cpp
    troePatchAttribute.cpp
    troePutAttribute.cpp
    pgInit.cpp
    pgConnectionGet.cpp
    pgConnectionRelease.cpp
    pgDatabaseCreate.cpp
    pgDatabasePrepare.cpp
    pgDatabaseTableCreateAll.cpp
    pgTransactionBegin.cpp
    pgTransactionCommit.cpp
    pgTransactionRollback.cpp
    kjGeoPointExtract.cpp
    kjGeoMultiPointExtract.cpp
    kjGeoPolygonExtract.cpp
    kjGeoMultiPolygonExtract.cpp
    kjGeoLineStringExtract.cpp
    kjGeoMultiLineStringExtract.cpp
    pgObservedAtExtract.cpp
    dbCreationCommand.cpp
    pgAppendInit.cpp
    pgAppend.cpp
    pgEntityBuild.cpp
    pgAttributeBuild.cpp
    pgAttributeAppend.cpp
    pgAttributesBuild.cpp
    pgSubAttributeBuild.cpp
    pgSubAttributeAppend.cpp
    pgQuotedString.cpp
    pgCommands.cpp
    pgEntityAppend.cpp
    pgConnect.cpp
    pgConnectionPools.cpp
    pgConnectionPoolsPresent.cpp
    pgConnectionPoolsFree.cpp
    pgConnectionPoolFree.cpp
    pgConnectionPoolGet.cpp
    pgConnectionPoolCreate.cpp
    pgConnectionPoolInsert.cpp
    pgConnectionPoolInit.cpp
    pgVersionGet.cpp
)

SET (HEADERS
    troeInit.h
    troeDeleteAttribute.h
    troeDeleteEntity.h
    troePatchAttribute.h
    troePatchEntity.h
    troePostBatchCreate.h
    troePostBatchDelete.h
    troePostBatchUpdate.h
    troePostBatchUpsert.h
    troePostEntities.h
    troePostEntity.h
    troePatchAttribute.h
    pgInit.h
    pgConnectionGet.h
    pgConnectionRelease.h
    pgDatabaseCreate.h
    pgDatabasePrepare.h
    pgDatabaseTableCreateAll.h
    pgTransactionBegin.h
    pgTransactionCommit.h
    pgTransactionRollback.h
    kjGeoPointExtract.h
    kjGeoMultiPointExtract.h
    kjGeoPolygonExtract.h
    kjGeoMultiPolygonExtract.h
    kjGeoLineStringExtract.h
    kjGeoMultiLineStringExtract.h
    pgObservedAtExtract.h
    pgAppendInit.h
    pgAppend.h
    pgEntityBuild.h
    pgAttributeBuild.h
    pgSubAttributeBuild.h
    pgQuotedString.h
    pgCommands.h
    pgEntityAppend.h
    pgAttributeAppend.h
    pgConnect.h
    pgConnectionPools.h
    pgConnectionPoolsPresent.h
    pgConnectionPoolsFree.h
    pgConnectionPoolFree.h
    pgConnectionPoolGet.h
    pgConnectionPoolCreate.h
    pgConnectionPoolInsert.h
    pgConnectionPoolInit.h
)


# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/lib")


# Library declaration
# -----------------------------------------------------------------
ADD_LIBRARY(orionld_troe STATIC ${SOURCES} ${HEADERS})
